#include "app/library/LibraryController.h"

#include "app/index/LibraryMetadataPolicy.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFuture>
#include <QtConcurrentRun>
#include <QUuid>

#include <algorithm>
#include <stdexcept>

namespace atlas::app {
namespace {

[[nodiscard]] std::string toUtf8(const QString& value) {
    const auto bytes = value.toUtf8();
    return {bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

[[nodiscard]] std::filesystem::path toPath(const QString& value) {
#ifdef _WIN32
    return std::filesystem::path{value.toStdWString()};
#else
    const auto bytes = value.toUtf8();
    return std::filesystem::path{std::string{bytes.constData(), static_cast<std::size_t>(bytes.size())}};
#endif
}

} // namespace

LibraryController::LibraryController(std::filesystem::path databasePath, QObject* parent)
    : QAbstractListModel{parent},
      index_{std::move(databasePath)},
      enumerator_{std::make_shared<atlas::index::NativeLibraryEnumerator>()},
      scanner_{enumerator_},
      scanService_{scanner_, index_, fileIdentityProvider_},
      ingestionService_{scanService_, index_, documentInspector_, [] {
          return toUtf8(QUuid::createUuid().toString(QUuid::WithoutBraces));
      }} {
    workerContext_ = new QObject;
    workerContext_->moveToThread(&workerThread_);
    workerThread_.setObjectName(QStringLiteral("AtlasLibraryWorker"));
    workerThread_.start();
    connect(&scanWatcher_, &QFutureWatcherBase::finished, this, [this] {
        try {
            const auto summary = scanWatcher_.result();
            lastAddedCount_ = static_cast<int>(summary.documentsAdded);
            statusKey_ = summary.inspectionFailures == 0
                ? QStringLiteral("complete")
                : QStringLiteral("completeWithErrors");
        } catch (const std::exception& error) {
            statusKey_ = QStringLiteral("error");
            emit operationError(QString::fromUtf8(error.what()));
        }
        requestRefresh();
        emit scanningChanged();
        emit statusChanged();
    });
    requestRefresh();
}

LibraryController::~LibraryController() {
    cancelled_.store(true);
    scanWatcher_.waitForFinished();
    if (workerContext_ != nullptr && workerThread_.isRunning()) {
        auto* context = workerContext_;
        QMetaObject::invokeMethod(context, [context] { delete context; }, Qt::BlockingQueuedConnection);
        workerContext_ = nullptr;
    }
    workerThread_.quit();
    workerThread_.wait();
}

int LibraryController::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(records_.size());
}

QVariant LibraryController::data(const QModelIndex& modelIndex, int role) const {
    if (!modelIndex.isValid() || modelIndex.row() < 0
        || modelIndex.row() >= static_cast<int>(records_.size())) return {};
    const auto& record = records_.at(static_cast<std::size_t>(modelIndex.row()));
    switch (role) {
    case DocumentIdRole: return QString::fromUtf8(record.id);
    case TitleRole: return atlas::index::libraryDisplayTitle(
        QString::fromUtf8(record.title), pathToQString(record.source.stem()));
    case AuthorRole: return atlas::index::libraryDisplayAuthor(QString::fromUtf8(record.author));
    case SourceRole: return pathToQString(record.source);
    case FileNameRole: return pathToQString(record.source.filename());
    case AvailabilityRole: return availabilityName(record.availability);
    case FavoriteRole: return favoriteIds_.contains(record.id);
    default: return {};
    }
}

QHash<int, QByteArray> LibraryController::roleNames() const {
    return {
        {DocumentIdRole, "documentId"}, {TitleRole, "bookTitle"}, {AuthorRole, "bookAuthor"},
        {SourceRole, "sourcePath"}, {FileNameRole, "fileName"}, {AvailabilityRole, "availability"},
        {FavoriteRole, "favorite"},
    };
}

QString LibraryController::query() const { return query_; }

void LibraryController::setQuery(const QString& query) {
    const auto normalized = query.trimmed();
    if (query_ == normalized) return;
    query_ = normalized;
    emit queryChanged();
    requestRefresh();
}

int LibraryController::viewMode() const { return viewMode_; }

void LibraryController::setViewMode(int mode) {
    if (mode < AllBooks || mode > Recent || viewMode_ == mode) return;
    viewMode_ = mode;
    if (mode != AllBooks && !selectedRootId_.isEmpty()) {
        selectedRootId_.clear();
        selectedRootName_.clear();
        emit selectedRootChanged();
    }
    emit viewModeChanged();
    requestRefresh();
}

bool LibraryController::scanning() const { return scanPreparationPending_ || scanWatcher_.isRunning(); }
QString LibraryController::statusKey() const { return statusKey_; }
int LibraryController::lastAddedCount() const { return lastAddedCount_; }
int LibraryController::rootCount() const { return rootCount_; }
QVariantList LibraryController::roots() const { return roots_; }
QString LibraryController::selectedRootId() const { return selectedRootId_; }
QString LibraryController::selectedRootName() const { return selectedRootName_; }

void LibraryController::setSelectedRootId(const QString& rootId) {
    if (selectedRootId_ == rootId) return;
    selectedRootId_ = rootId;
    if (!rootId.isEmpty() && viewMode_ != AllBooks) {
        viewMode_ = AllBooks;
        emit viewModeChanged();
    }
    selectedRootName_.clear();
    for (const auto& value : roots_) {
        const auto root = value.toMap();
        if (root.value(QStringLiteral("id")).toString() == rootId) {
            selectedRootName_ = root.value(QStringLiteral("name")).toString();
            break;
        }
    }
    emit selectedRootChanged();
    requestRefresh();
}
int LibraryController::pendingCount() const { return pendingProposals_.size(); }
QVariantList LibraryController::pendingProposals() const { return pendingProposals_; }

void LibraryController::addRoot(const QUrl& folder) {
    if (scanning()) return;
    const auto localPath = QDir::cleanPath(folder.toLocalFile());
    if (localPath.isEmpty() || !QFileInfo{localPath}.isDir()) {
        emit operationError(QStringLiteral("Choose an existing folder."));
        return;
    }
    scanPreparationPending_ = true;
    statusKey_ = QStringLiteral("scanning");
    emit scanningChanged();
    emit statusChanged();
    const auto source = toPath(localPath);
    const auto rootId = toUtf8(QUuid::createUuid().toString(QUuid::WithoutBraces));
    runOnWorker([this, source, localPath, rootId] {
        try {
            const auto persistedRoots = index_.roots();
            const bool exists = std::any_of(persistedRoots.begin(), persistedRoots.end(), [&](const auto& root) {
                return pathToQString(root.source).compare(localPath, Qt::CaseInsensitive) == 0;
            });
            if (!exists) {
                index_.registerRoot({rootId, source, atlas::index::LibraryRootAvailability::available});
            }
            std::vector<atlas::index::LibraryScanRoot> roots;
            for (const auto& root : index_.roots()) roots.push_back({root.id, root.source});
            QMetaObject::invokeMethod(this, [this, roots = std::move(roots)]() mutable {
                beginScan(std::move(roots));
            }, Qt::QueuedConnection);
        } catch (const std::exception& error) {
            const auto message = QString::fromUtf8(error.what());
            QMetaObject::invokeMethod(this, [this, message] { setScanPreparationFailed(message); }, Qt::QueuedConnection);
        }
    });
}

void LibraryController::rescan() { startScan(); }

void LibraryController::setFavorite(const QString& documentId, bool favorite) {
    const auto id = toUtf8(documentId);
    runOnWorker([this, id, favorite] {
        try {
            index_.setFavorite(id, favorite);
            QMetaObject::invokeMethod(this, [this] { requestRefresh(); }, Qt::QueuedConnection);
        } catch (const std::exception& error) { reportWorkerError(QString::fromUtf8(error.what())); }
    });
}

void LibraryController::openExternally(const QString& documentId) {
    const auto found = std::find_if(records_.begin(), records_.end(), [&](const auto& record) {
        return QString::fromUtf8(record.id) == documentId;
    });
    if (found == records_.end() || found->source.empty()) return;
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(pathToQString(found->source)))) {
        emit operationError(QStringLiteral("Windows could not open this PDF."));
        return;
    }
    const auto id = found->id;
    const auto openedAt = QDateTime::currentMSecsSinceEpoch();
    runOnWorker([this, id, openedAt] {
        try {
            index_.recordOpened(id, openedAt);
            QMetaObject::invokeMethod(this, [this] { requestRefresh(); }, Qt::QueuedConnection);
        } catch (const std::exception& error) { reportWorkerError(QString::fromUtf8(error.what())); }
    });
}

void LibraryController::applyProposal(const QString& proposalId) {
    const auto id = toUtf8(proposalId);
    runOnWorker([this, id] {
        try {
            index_.applyReconciliation(id);
            QMetaObject::invokeMethod(this, [this] { requestRefresh(); }, Qt::QueuedConnection);
        } catch (const std::exception& error) { reportWorkerError(QString::fromUtf8(error.what())); }
    });
}

void LibraryController::dismissProposal(const QString& proposalId) {
    const auto id = toUtf8(proposalId);
    runOnWorker([this, id] {
        try {
            index_.dismissReconciliation(id);
            QMetaObject::invokeMethod(this, [this] { requestRefresh(); }, Qt::QueuedConnection);
        } catch (const std::exception& error) { reportWorkerError(QString::fromUtf8(error.what())); }
    });
}

void LibraryController::requestRefresh() {
    const auto revision = ++refreshRevision_;
    const auto query = query_;
    const auto selectedRootId = selectedRootId_;
    const auto viewMode = viewMode_;
    runOnWorker([this, revision, query, selectedRootId, viewMode] {
        struct Snapshot final {
            std::vector<atlas::index::LibraryRecord> records;
            std::set<std::string> favoriteIds;
            QVariantList roots;
            QVariantList proposals;
            QString selectedRootName;
        } snapshot;

        try {
            if (!query.isEmpty()) {
                snapshot.records = index_.search(toUtf8(query));
                if (!selectedRootId.isEmpty()) {
                    const auto rootRecords = index_.recordsForRoot(toUtf8(selectedRootId));
                    std::set<std::string> allowed;
                    for (const auto& record : rootRecords) allowed.insert(record.id);
                    std::erase_if(snapshot.records, [&](const auto& record) { return !allowed.contains(record.id); });
                }
            } else if (viewMode == Favorites) snapshot.records = index_.favorites();
            else if (viewMode == Recent) snapshot.records = index_.recentlyOpened();
            else if (!selectedRootId.isEmpty()) snapshot.records = index_.recordsForRoot(toUtf8(selectedRootId));
            else snapshot.records = index_.allRecords();

            for (const auto& favorite : index_.favorites()) snapshot.favoriteIds.insert(favorite.id);
            for (const auto& root : index_.roots()) {
                QVariantMap item;
                item.insert(QStringLiteral("id"), QString::fromUtf8(root.id));
                item.insert(QStringLiteral("path"), pathToQString(root.source));
                const auto name = pathToQString(root.source.filename());
                item.insert(QStringLiteral("name"), name.isEmpty() ? pathToQString(root.source) : name);
                item.insert(QStringLiteral("availability"),
                    root.availability == atlas::index::LibraryRootAvailability::available
                        ? QStringLiteral("available")
                        : root.availability == atlas::index::LibraryRootAvailability::offline
                            ? QStringLiteral("offline") : QStringLiteral("partial"));
                snapshot.roots.push_back(item);
                if (QString::fromUtf8(root.id) == selectedRootId) {
                    snapshot.selectedRootName = item.value(QStringLiteral("name")).toString();
                }
            }
            for (const auto& proposal : index_.reconciliationProposals()) {
                if (proposal.state != atlas::index::ReconciliationProposalState::pending) continue;
                QVariantMap item;
                item.insert(QStringLiteral("id"), QString::fromUtf8(proposal.id));
                item.insert(QStringLiteral("beforePath"), pathToQString(proposal.previousSource));
                item.insert(QStringLiteral("candidatePath"), pathToQString(proposal.candidateSource));
                item.insert(QStringLiteral("canApply"), proposal.mayRelinkAutomatically);
                item.insert(QStringLiteral("decision"), proposal.mayRelinkAutomatically
                    ? QStringLiteral("move") : QStringLiteral("ambiguous"));
                snapshot.proposals.push_back(item);
            }
        } catch (const std::exception& error) {
            reportWorkerError(QString::fromUtf8(error.what()));
            return;
        }

        QMetaObject::invokeMethod(this, [this, revision, snapshot = std::move(snapshot)]() mutable {
            if (revision != refreshRevision_) return;
            beginResetModel();
            records_ = std::move(snapshot.records);
            favoriteIds_ = std::move(snapshot.favoriteIds);
            endResetModel();
            roots_ = std::move(snapshot.roots);
            pendingProposals_ = std::move(snapshot.proposals);
            rootCount_ = roots_.size();
            if (selectedRootName_ != snapshot.selectedRootName) {
                selectedRootName_ = std::move(snapshot.selectedRootName);
                emit selectedRootChanged();
            }
            emit rootsChanged();
            emit rootCountChanged();
            emit pendingProposalsChanged();
        }, Qt::QueuedConnection);
    });
}

void LibraryController::runOnWorker(std::function<void()> task) {
    if (workerContext_ == nullptr) return;
    QMetaObject::invokeMethod(workerContext_, std::move(task), Qt::QueuedConnection);
}

void LibraryController::reportWorkerError(const QString& message) {
    QMetaObject::invokeMethod(this, [this, message] { emit operationError(message); }, Qt::QueuedConnection);
}

void LibraryController::startScan() {
    if (scanning()) return;
    scanPreparationPending_ = true;
    lastAddedCount_ = 0;
    statusKey_ = QStringLiteral("scanning");
    emit statusChanged();
    emit scanningChanged();
    runOnWorker([this] {
        try {
            std::vector<atlas::index::LibraryScanRoot> roots;
            for (const auto& root : index_.roots()) roots.push_back({root.id, root.source});
            QMetaObject::invokeMethod(this, [this, roots = std::move(roots)]() mutable {
                beginScan(std::move(roots));
            }, Qt::QueuedConnection);
        } catch (const std::exception& error) {
            const auto message = QString::fromUtf8(error.what());
            QMetaObject::invokeMethod(this, [this, message] { setScanPreparationFailed(message); }, Qt::QueuedConnection);
        }
    });
}

void LibraryController::beginScan(std::vector<atlas::index::LibraryScanRoot> roots) {
    scanPreparationPending_ = false;
    if (roots.empty()) {
        statusKey_ = QStringLiteral("noRoots");
        emit statusChanged();
        emit scanningChanged();
        return;
    }
    cancelled_.store(false);
    scanWatcher_.setFuture(QtConcurrent::run([this, roots = std::move(roots)] {
        return ingestionService_.ingest(roots, cancelled_, QDateTime::currentMSecsSinceEpoch());
    }));
    emit scanningChanged();
}

void LibraryController::setScanPreparationFailed(const QString& message) {
    scanPreparationPending_ = false;
    statusKey_ = QStringLiteral("error");
    emit statusChanged();
    emit scanningChanged();
    emit operationError(message);
}

QString LibraryController::availabilityName(atlas::document::Availability availability) {
    switch (availability) {
    case atlas::document::Availability::available: return QStringLiteral("available");
    case atlas::document::Availability::locked: return QStringLiteral("locked");
    case atlas::document::Availability::readOnly: return QStringLiteral("readOnly");
    case atlas::document::Availability::offline: return QStringLiteral("offline");
    case atlas::document::Availability::missing: return QStringLiteral("missing");
    case atlas::document::Availability::cloudPlaceholder: return QStringLiteral("cloud");
    case atlas::document::Availability::unreadable: return QStringLiteral("unreadable");
    case atlas::document::Availability::unsupported: return QStringLiteral("unsupported");
    }
    return QStringLiteral("unknown");
}

QString LibraryController::pathToQString(const std::filesystem::path& path) {
#ifdef _WIN32
    return QString::fromStdWString(path.wstring());
#else
    const auto bytes = path.generic_u8string();
    return QString::fromUtf8(reinterpret_cast<const char*>(bytes.data()), static_cast<qsizetype>(bytes.size()));
#endif
}

} // namespace atlas::app
