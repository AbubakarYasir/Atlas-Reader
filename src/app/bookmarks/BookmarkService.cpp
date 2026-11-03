#include "app/bookmarks/BookmarkService.h"

#include "app/bookmarks/OutlineSafeSave.h"
#include "core/bookmarks/Reconcile.h"

#include "app/index/SqliteLibraryIndex.h"
#include "core/bookmarks/LocationEnvelope.h"

#include <QFileInfo>
#include <QMetaObject>
#include <QMutex>
#include <QSemaphore>
#include <QVariantMap>

#include <algorithm>
#include <cmath>

namespace atlas::app {

namespace {

[[nodiscard]] std::string toUtf8(const QString& text) {
    const QByteArray bytes = text.toUtf8();
    return {bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

[[nodiscard]] std::filesystem::path toPath(const QString& path) {
    return std::filesystem::path{reinterpret_cast<const char16_t*>(path.utf16()),
                                 reinterpret_cast<const char16_t*>(path.utf16()) + path.size()};
}

} // namespace

std::optional<bookmarks::BaseSnapshot> baseFromOutline(const QVariantList& outline,
                                                       const std::string& sourceRevision) {
    bookmarks::BaseSnapshot base;
    base.sourceRevision = sourceRevision;
    base.nodes.reserve(static_cast<std::size_t>(outline.size()));
    for (const QVariant& item : outline) {
        const QVariantMap entry = item.toMap();
        bookmarks::BaseNode node;
        node.key = toUtf8(entry.value(QStringLiteral("key")).toString());
        const QString parentKey = entry.value(QStringLiteral("parentKey")).toString();
        if (!parentKey.isEmpty()) node.parentKey = toUtf8(parentKey);
        node.position = static_cast<std::uint32_t>(std::max(0, entry.value(QStringLiteral("position")).toInt()));
        node.title = toUtf8(entry.value(QStringLiteral("title")).toString());
        const int page = entry.value(QStringLiteral("page")).toInt();
        if (page >= 0) {
            bookmarks::LocationEnvelope envelope;
            envelope.pdf.pageIndex = static_cast<std::uint32_t>(page);
            if (entry.value(QStringLiteral("hasPoint")).toBool()) {
                envelope.pdf.kind = bookmarks::DestinationKind::xyz;
                envelope.pdf.left = std::clamp(entry.value(QStringLiteral("fx")).toDouble(), 0.0, 1.0);
                envelope.pdf.top = std::clamp(entry.value(QStringLiteral("fy")).toDouble(), 0.0, 1.0);
                const double zoom = entry.value(QStringLiteral("zoom")).toDouble();
                if (std::isfinite(zoom) && zoom > 0.0 && zoom <= 64.0) envelope.pdf.zoom = zoom;
            } else {
                envelope.pdf.kind = bookmarks::DestinationKind::fit;
            }
            node.location = bookmarks::encodeLocation(envelope);
        }
        base.nodes.push_back(std::move(node));
    }
    if (!bookmarks::validateBase(base).empty()) return std::nullopt;
    return base;
}

struct BookmarkService::Storage final {
    std::filesystem::path databasePath;
    std::unique_ptr<index::SqliteLibraryIndex> library;
    std::unique_ptr<bookmarks::SqliteBookmarkOverlay> overlay;

    // Opened on first use, on the worker thread. The library index opens first
    // so the profile is migrated before the overlay checks its schema.
    void ensureOpen() {
        if (!library) library = std::make_unique<index::SqliteLibraryIndex>(databasePath);
        if (!overlay) overlay = std::make_unique<bookmarks::SqliteBookmarkOverlay>(databasePath);
    }
};

BookmarkService::BookmarkService(std::filesystem::path databasePath, QObject* parent)
    : QObject(parent), storage_(std::make_shared<Storage>()) {
    storage_->databasePath = std::move(databasePath);
    thread_.setObjectName(QStringLiteral("atlas-bookmarks"));
    worker_ = new QObject;
    worker_->moveToThread(&thread_);
    connect(&thread_, &QThread::finished, worker_, &QObject::deleteLater);
    thread_.start();
}

BookmarkService::~BookmarkService() {
    // Let queued writes finish (research is never dropped on exit), then close
    // the database on the thread that opened it.
    post([](Storage& storage) {
        storage.overlay.reset();
        storage.library.reset();
    });
    waitForIdle();
    thread_.quit();
    thread_.wait();
}

void BookmarkService::post(std::function<void(Storage&)> job) {
    QMetaObject::invokeMethod(
        worker_, [shared = storage_, task = std::move(job)] { task(*shared); }, Qt::QueuedConnection);
}

void BookmarkService::exchangeWork(std::function<void()> work) {
    post([task = std::move(work)](Storage&) { task(); });
}

void BookmarkService::waitForIdle() {
    QSemaphore done;
    post([&done](Storage&) { done.release(); });
    done.acquire();
}

void BookmarkService::open(const QString& path, const QVariantList& outline, QObject* receiver,
                           std::function<void(BookmarkOpenResult)> done, bool followPdf) {
    QPointer<QObject> target{receiver};
    QPointer<BookmarkService> self{this};
    post([path, outline, target, self, followPdf, callback = std::move(done)](Storage& storage) mutable {
        BookmarkOpenResult result;
        result.base = baseFromOutline(outline, fileRevision(path));
        try {
            storage.ensureOpen();
            const auto documents = storage.library->documentsAtPath(toPath(path));
            if (documents.empty()) {
                result.status = BookmarkOpenResult::Status::notInLibrary;
            } else if (documents.size() > 1) {
                result.status = BookmarkOpenResult::Status::ambiguous;
            } else if (!outline.isEmpty() && !result.base) {
                result.status = BookmarkOpenResult::Status::outlineUnsupported;
            } else {
                result.documentId = documents.front();
                const auto recovery = recoverOutlineSave(*storage.overlay, path, result.documentId,
                                                         storage.databasePath.parent_path() / "backups");
                if (recovery == SaveRecovery::required || recovery == SaveRecovery::restored) {
                    result.snapshot = storage.overlay->load(result.documentId);
                    result.status = recovery == SaveRecovery::required ? BookmarkOpenResult::Status::recoveryRequired
                                                                       : BookmarkOpenResult::Status::recoveredReopen;
                } else {
                if (result.base && !result.base->sourceRevision.empty()) {
                    result.adoption = storage.overlay->adoptBase(result.documentId, *result.base);
                }
                result.snapshot = storage.overlay->load(result.documentId);
                if (result.adoption == bookmarks::BaseAdoption::needsReconcile && result.snapshot.base) {
                    auto merged = bookmarks::reconcile(result.documentId, result.snapshot.nodes,
                                                       *result.snapshot.base, *result.base);
                    std::size_t embedded = 0;
                    for (const auto& node : result.snapshot.nodes)
                        embedded += node.state == bookmarks::StorageState::embedded ? 1 : 0;
                    // Dropping most of the PDF's bookmarks may be a reading
                    // problem, not an edit: wait for the reader's go-ahead.
                    const bool large = merged.removedFromPdf > 0 && merged.removedFromPdf * 2 > embedded;
                    if (!merged.refusal.empty()) {
                        result.reconciled.held = "cannotMerge";
                    } else if (large && !followPdf) {
                        result.reconciled.held = "largeRemoval";
                        result.reconciled.removed = static_cast<int>(merged.removedFromPdf);
                    } else {
                        storage.overlay->commitReconcile(result.documentId, result.snapshot.revision,
                                                         {*result.base, std::move(merged.nodes), std::move(merged.removed)});
                        result.reconciled = {true, static_cast<int>(merged.added), static_cast<int>(merged.updated),
                                             static_cast<int>(merged.removedFromPdf), static_cast<int>(merged.conflicts), {}};
                        result.adoption = bookmarks::BaseAdoption::unchanged;
                        result.snapshot = storage.overlay->load(result.documentId);
                    }
                }
                result.status = BookmarkOpenResult::Status::editable;
                }
            }
        } catch (const std::exception&) {
            result.status = BookmarkOpenResult::Status::storageError;
        }
        QMetaObject::invokeMethod(
            self.data(),
            [target, reply = std::move(callback), answer = std::move(result)]() mutable {
                if (target) reply(std::move(answer));
            },
            Qt::QueuedConnection);
    });
}

void BookmarkService::save(SafeSaveInput input, QObject* receiver, std::function<void(SafeSaveOutcome)> done) {
    QPointer<QObject> target{receiver};
    QPointer<BookmarkService> self{this};
    if (passwordSource_) input.password = passwordSource_(input.path);
    post([request = std::move(input), target, self, callback = std::move(done)](Storage& storage) mutable {
        SafeSaveOutcome outcome;
        try {
            storage.ensureOpen();
            request.backupRoot = storage.databasePath.parent_path() / "backups";
            outcome = saveOutlineSafely(*storage.overlay, request);
        } catch (const std::exception&) {
            outcome.reason = QStringLiteral("storageError");
            const auto journal = request.backupRoot / request.documentId / "pending-save.json";
            if (QFileInfo::exists(QString::fromStdU16String(journal.u16string()))) {
                // An exception after replacement must not masquerade as an unchanged file.
                outcome.reason = QStringLiteral("recoveryRequired");
                outcome.disposition = SaveDisposition::recoveryRequired;
                outcome.pdfChanged = true; // refresh the reader; its cached bytes may be stale
            }
        }
        request.password.fill(u'\0'); // as far as Qt allows: the copy dies here
        QMetaObject::invokeMethod(
            self.data(),
            [target, reply = std::move(callback), answer = std::move(outcome)]() mutable {
                if (target) reply(std::move(answer));
            },
            Qt::QueuedConnection);
    });
}

void BookmarkService::saveCopy(SafeSaveInput input, QString target, QObject* receiver,
                               std::function<void(SafeSaveOutcome)> done) {
    QPointer<QObject> target_{receiver};
    QPointer<BookmarkService> self{this};
    if (passwordSource_) input.password = passwordSource_(input.path);
    post([request = std::move(input), target, target_, self, callback = std::move(done)](Storage&) mutable {
        SafeSaveOutcome outcome = saveOutlineCopy(request, target);
        request.password.fill(u'\0');
        QMetaObject::invokeMethod(
            self.data(),
            [target_, reply = std::move(callback), answer = std::move(outcome)]() mutable {
                if (target_) reply(std::move(answer));
            },
            Qt::QueuedConnection);
    });
}

void BookmarkService::write(const std::string& documentId, std::int64_t expectedRevision,
                            bookmarks::OverlayChangeSet change, QObject* receiver,
                            std::function<void(BookmarkWriteResult)> done) {
    QPointer<QObject> target{receiver};
    QPointer<BookmarkService> self{this};
    post([documentId, expectedRevision, changes = std::move(change), target, self,
          callback = std::move(done)](Storage& storage) mutable {
        BookmarkWriteResult result;
        try {
            storage.ensureOpen();
            result.revision = storage.overlay->apply(documentId, expectedRevision, changes);
            result.ok = true;
        } catch (const bookmarks::OverlayRuleViolation& violation) {
            result.reason = violation.reason();
        } catch (const bookmarks::OverlayRevisionConflict&) {
            result.reason = "revisionConflict";
        } catch (const std::exception&) {
            result.reason = "storageError";
        }
        QMetaObject::invokeMethod(
            self.data(),
            [target, reply = std::move(callback), answer = std::move(result)]() mutable {
                if (target) reply(std::move(answer));
            },
            Qt::QueuedConnection);
    });
}

void BookmarkService::reload(const std::string& documentId, QObject* receiver,
                             std::function<void(std::optional<bookmarks::OverlaySnapshot>)> done) {
    QPointer<QObject> target{receiver};
    QPointer<BookmarkService> self{this};
    post([documentId, target, self, callback = std::move(done)](Storage& storage) mutable {
        std::optional<bookmarks::OverlaySnapshot> snapshot;
        try {
            storage.ensureOpen();
            snapshot = storage.overlay->load(documentId);
        } catch (const std::exception&) {
        }
        QMetaObject::invokeMethod(
            self.data(),
            [target, reply = std::move(callback), answer = std::move(snapshot)]() mutable {
                if (target) reply(std::move(answer));
            },
            Qt::QueuedConnection);
    });
}

} // namespace atlas::app
