#include "app/bookmarks/OutlineSafeSave.h"

#include "app/bookmarks/BookmarkService.h"
#include "app/document/DocumentCapabilityCheck.h"
#include "app/document/QpdfOutlineWriter.h"
#include "app/reader/DocumentTextWorker.h"
#include "core/document/CapabilityPolicy.h"
#include "core/bookmarks/LocationEnvelope.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPdfDocument>
#include <QRandomGenerator>
#include <QStorageInfo>

#include <algorithm>
#include <unordered_map>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace atlas::app {
namespace {

using namespace atlas::bookmarks;

QString utf8(const std::string& text) { return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size())); }

QByteArray sha256(const QString& path) {
    QFile file{path};
    if (!file.open(QIODevice::ReadOnly)) return {};
    QCryptographicHash hash{QCryptographicHash::Sha256};
    if (!hash.addData(&file)) return {};
    return hash.result();
}

QString qpath(const std::filesystem::path& path) {
    const auto text = path.u16string();
    return QString::fromUtf16(text.data(), static_cast<qsizetype>(text.size()));
}

// Keys the plan's items get in the written PDF ("0", "0/1", …), depth first.
std::vector<std::string> plannedKeys(const SavePlan& plan) {
    std::vector<std::string> keys;
    std::vector<int> childCount(plan.items.size(), 0);
    int topCount = 0;
    for (const auto& item : plan.items) {
        const int position = item.parent >= 0 ? childCount[static_cast<std::size_t>(item.parent)]++ : topCount++;
        keys.push_back(item.parent >= 0 ? keys[static_cast<std::size_t>(item.parent)] + "/" + std::to_string(position)
                                        : std::to_string(position));
    }
    return keys;
}

// The outline Qt PDF reads from `path` (independent of the qpdf writer).
std::optional<QVariantList> reopen(const QString& path, const QString& password) {
    QPdfDocument document;
    if (!password.isEmpty()) document.setPassword(password);
    document.load(path);
    if (document.status() != QPdfDocument::Status::Ready) return std::nullopt;
    return readOutline(document);
}

bool sameAsPlan(const QVariantList& outline, const SavePlan& plan, const std::vector<std::string>& keys) {
    if (outline.size() != static_cast<qsizetype>(plan.items.size())) return false;
    for (qsizetype i = 0; i < outline.size(); ++i) {
        const QVariantMap entry = outline.at(i).toMap();
        const auto& item = plan.items[static_cast<std::size_t>(i)];
        if (entry.value(QStringLiteral("key")).toString() != utf8(keys[static_cast<std::size_t>(i)]) ||
            entry.value(QStringLiteral("title")).toString() != utf8(item.title))
            return false;
        if (!item.location.empty()) {
            const auto decoded = decodeLocation(item.location);
            if (decoded.status == LocationDecodeStatus::ok &&
                entry.value(QStringLiteral("page")).toInt() != static_cast<int>(decoded.envelope->pdf.pageIndex))
                return false;
        }
    }
    return true;
}

// Replaces `target` with `replacement` in one step; on Windows keeps the
// original's attributes and security (ReplaceFileW), elsewhere its permissions.
bool replaceFile(const QString& target, const QString& replacement) {
#ifdef Q_OS_WIN
    const std::wstring to = QDir::toNativeSeparators(target).toStdWString();
    const std::wstring from = QDir::toNativeSeparators(replacement).toStdWString();
    return ReplaceFileW(to.c_str(), from.c_str(), nullptr, REPLACEFILE_IGNORE_MERGE_ERRORS, nullptr, nullptr) != 0;
#else
    QFile::setPermissions(replacement, QFileInfo{target}.permissions());
    std::error_code error;
    std::filesystem::rename(std::filesystem::path{replacement.toStdU16String()},
                            std::filesystem::path{target.toStdU16String()}, error);
    return !error;
#endif
}

// A recovery copy of the original in the profile; the newest three are kept.
QString backup(const QString& source, const std::filesystem::path& root, const std::string& documentId,
               const QByteArray& expectedHash) {
    const QString folder = qpath(root / documentId);
    if (!QDir{}.mkpath(folder)) return {};
    const QString name = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) +
                         QStringLiteral(".pdf");
    const QString copy = QDir{folder}.filePath(name);
    if (!QFile::copy(source, copy) || sha256(copy) != expectedHash) {
        QFile::remove(copy);
        return {};
    }
    QDir dir{folder};
    const QStringList kept = dir.entryList({QStringLiteral("*.pdf")}, QDir::Files, QDir::Name | QDir::Reversed);
    for (qsizetype i = 3; i < kept.size(); ++i) dir.remove(kept.at(i));
    return copy;
}

struct TempFile final {
    QString path;
    ~TempFile() {
        if (!path.isEmpty()) QFile::remove(path);
    }
};

} // namespace

std::string fileRevision(const QString& path) {
    const QFileInfo info{path};
    if (!info.exists()) return {};
    return "size=" + std::to_string(info.size()) + ";mtime=" + std::to_string(info.lastModified().toMSecsSinceEpoch());
}

SafeSaveOutcome saveOutlineCopy(const SafeSaveInput& input, const QString& target, const SafeSaveFault& fault) {
    SafeSaveOutcome out;
    const auto fail = [&out](const char* reason) {
        out.ok = false;
        out.reason = QString::fromLatin1(reason);
        return out;
    };
    const auto step = [&fault](std::string_view name) { return !fault || fault(name); };
    std::vector<const BookmarkNode*> inOrder;
    for (const auto& node : input.nodes) inOrder.push_back(&node);
    const BaseSnapshot base = input.base.value_or(BaseSnapshot{fileRevision(input.path), {}});
    out.plan = planOutlineSave(inOrder, base, input.options);
    if (!out.plan.refusal.empty()) return fail(out.plan.refusal.c_str());
    const QFileInfo targetInfo{target};
    if (target.isEmpty() || targetInfo.absoluteFilePath() == QFileInfo{input.path}.absoluteFilePath())
        return fail("sameFile");
    if (input.base && input.base->sourceRevision != fileRevision(input.path)) return fail("changedOutside");

    // The PDF's own security decides; the file being read-only, in use, signed
    // or certified does not (the copy is a new file).
    using R = document::CapabilityReason;
    const auto capabilities = checkDocumentCapabilities(input.path, input.password, {});
    for (const auto reason : capabilities.assessment.reasons) {
        switch (reason) {
        case R::fileReadOnly:
        case R::folderReadOnly:
        case R::sharingLocked:
        case R::signatures:
        case R::certified:
        case R::annotationsNotPermitted:
            break;
        default:
            return fail("notPermitted");
        }
    }
    const QString qpdf = input.qpdf.isEmpty() ? QpdfInspector::locate() : input.qpdf;
    if (qpdf.isEmpty()) return fail("qpdfUnavailable");
    const QStorageInfo volume{targetInfo.absolutePath()};
    if (volume.bytesAvailable() >= 0 && volume.bytesAvailable() < 2 * QFileInfo{input.path}.size() + (4 << 20))
        return fail("diskFull");

    TempFile temp;
    temp.path = QDir{targetInfo.absolutePath()}.filePath(
        QStringLiteral(".%1.atlas-copy-%2.pdf").arg(targetInfo.completeBaseName())
            .arg(QRandomGenerator::global()->generate(), 8, 16, QLatin1Char('0')));
    {
        QFile probe{temp.path};
        if (!probe.open(QIODevice::WriteOnly)) {
            temp.path.clear();
            return fail("folderReadOnly");
        }
    }
    const PdfSourceOutline before = QpdfOutlineWriter::read(qpdf, input.path, input.password);
    if (!before.ok) return fail("outlineMismatch");
    QString reason;
    const auto update = QpdfOutlineWriter::buildUpdate(before, base, out.plan, &reason);
    if (!update) return fail(reason == QStringLiteral("unreadableLocation") ? "unreadableLocation" : "outlineMismatch");
    if (const QString written = QpdfOutlineWriter::write(qpdf, input.path, input.password, *update, temp.path);
        !written.isEmpty() || !step("write"))
        return fail(written == QStringLiteral("validation") ? "validation" : "writeFailed");
    const PdfSourceOutline after = QpdfOutlineWriter::read(qpdf, temp.path, input.password);
    const QString verdict = QpdfOutlineWriter::verify(before, after, out.plan);
    if (!verdict.isEmpty() || !step("verify")) return fail(verdict == QStringLiteral("preservation") ? "preservation" : "outlineDiffers");
    const auto reread = reopen(temp.path, input.password);
    if (!reread || !sameAsPlan(*reread, out.plan, plannedKeys(out.plan)) || !step("reopen")) return fail("reopenDiffers");
    // Into place under the chosen name (the file dialog already confirmed any overwrite).
    if (QFileInfo::exists(target) ? !replaceFile(target, temp.path)
                                  : !QFile::rename(temp.path, target))
        return fail("replaceFailed");
    temp.path.clear();
    out.ok = true;
    out.sourceRevision = fileRevision(target);
    return out;
}

SafeSaveOutcome saveOutlineSafely(SqliteBookmarkOverlay& store, const SafeSaveInput& input, const SafeSaveFault& fault) {
    SafeSaveOutcome out;
    const auto fail = [&out](const char* reason) {
        out.ok = false;
        out.reason = QString::fromLatin1(reason);
        return out;
    };
    const auto step = [&fault](std::string_view name) { return !fault || fault(name); };

    // 1. Plan.
    std::vector<const BookmarkNode*> inOrder;
    inOrder.reserve(input.nodes.size());
    for (const auto& node : input.nodes) inOrder.push_back(&node);
    const BaseSnapshot base = input.base.value_or(BaseSnapshot{fileRevision(input.path), {}});
    out.plan = planOutlineSave(inOrder, base, input.options);
    if (!out.plan.refusal.empty()) return fail(out.plan.refusal.c_str());
    if (!out.plan.hasChanges()) return fail("nothingToSave");

    // 2. Preflight.
    const QFileInfo info{input.path};
    if (!info.exists()) return fail("changedOutside");
    if (input.base && input.base->sourceRevision != fileRevision(input.path)) return fail("changedOutside");
    const auto capabilities = checkDocumentCapabilities(input.path, input.password, {});
    if (!capabilities.assessment.capabilities.canEmbedOutline) return fail("notPermitted");
    const QStorageInfo volume{info.absolutePath()};
    if (volume.bytesAvailable() >= 0 && volume.bytesAvailable() < 2 * info.size() + (4 << 20)) return fail("diskFull");
    const QString qpdf = input.qpdf.isEmpty() ? QpdfInspector::locate() : input.qpdf;
    if (qpdf.isEmpty()) return fail("qpdfUnavailable");
    const QByteArray originalHash = sha256(input.path);
    if (originalHash.isEmpty()) return fail("changedOutside");
    const std::string originalRevision = fileRevision(input.path);

    // 3. Write a temporary file beside the original (same volume, so the
    //    replace is one rename) and check it.
    TempFile temp;
    temp.path = QDir{info.absolutePath()}.filePath(
        QStringLiteral(".%1.atlas-save-%2.pdf")
            .arg(info.completeBaseName())
            .arg(QRandomGenerator::global()->generate(), 8, 16, QLatin1Char('0')));
    {
        QFile probe{temp.path};
        if (!probe.open(QIODevice::WriteOnly)) {
            temp.path.clear();
            return fail("folderReadOnly");
        }
    }
    const PdfSourceOutline before = QpdfOutlineWriter::read(qpdf, input.path, input.password);
    if (!before.ok) return fail("outlineMismatch");
    QString reason;
    const auto update = QpdfOutlineWriter::buildUpdate(before, base, out.plan, &reason);
    if (!update) return fail(reason == QStringLiteral("unreadableLocation") ? "unreadableLocation" : "outlineMismatch");
    if (const QString written = QpdfOutlineWriter::write(qpdf, input.path, input.password, *update, temp.path);
        !written.isEmpty() || !step("write"))
        return fail(written == QStringLiteral("validation") ? "validation" : "writeFailed");

    // 4. Verify by qpdf's reading, then reopen with Qt PDF (independent of the writer).
    const PdfSourceOutline after = QpdfOutlineWriter::read(qpdf, temp.path, input.password);
    const QString verdict = QpdfOutlineWriter::verify(before, after, out.plan);
    if (!verdict.isEmpty() || !step("verify"))
        return fail(verdict == QStringLiteral("preservation") ? "preservation"
                    : verdict.isEmpty()                         ? "validation"
                                                                : "outlineDiffers");
    const auto keys = plannedKeys(out.plan);
    const auto reread = reopen(temp.path, input.password);
    if (!reread || !sameAsPlan(*reread, out.plan, keys) || !step("reopen")) return fail("reopenDiffers");

    // 5. Recovery backup, then make sure nobody changed the original meanwhile.
    out.backupPath = backup(input.path, input.backupRoot, input.documentId, originalHash);
    if (out.backupPath.isEmpty() || !step("backup")) return fail("backupFailed");
    if (sha256(input.path) != originalHash || fileRevision(input.path) != originalRevision || !step("beforeReplace"))
        return fail("changedDuringSave");

    // 6. Replace in one step and prove the result is the verified file.
    const QByteArray verifiedHash = sha256(temp.path);
    if (!step("replace") || !replaceFile(input.path, temp.path)) return fail("replaceFailed");
    temp.path.clear(); // it is the PDF now
    out.pdfChanged = true;
    if (sha256(input.path) != verifiedHash || !step("afterReplace")) {
        // Put the original back from the verified backup.
        QFile::remove(input.path);
        const bool restored = QFile::copy(out.backupPath, input.path) && sha256(input.path) == originalHash;
        out.pdfChanged = !restored;
        return fail("replaceUnverified");
    }

    // 7. Re-read the saved PDF the way Atlas opens books, then commit the overlay.
    out.sourceRevision = fileRevision(input.path);
    const auto finalOutline = reopen(input.path, input.password);
    const auto newBase = finalOutline ? baseFromOutline(*finalOutline, out.sourceRevision) : std::nullopt;
    if (!finalOutline || !newBase || !sameAsPlan(*finalOutline, out.plan, keys)) return fail("reindexFailed");
    std::unordered_map<std::string, const BaseNode*> newByKey;
    for (const auto& node : newBase->nodes) newByKey.emplace(node.key, &node);

    SaveCommit commit;
    commit.base = *newBase;
    commit.proof = {true, true, true, true, out.sourceRevision};
    std::unordered_map<std::string, std::size_t> planIndex;
    for (std::size_t i = 0; i < out.plan.items.size(); ++i) planIndex.emplace(out.plan.items[i].nodeId, i);
    std::vector<std::uint32_t> nextPosition(out.plan.items.size() + 1, 0); // [parent + 1]
    for (const auto& node : input.nodes) {
        const auto found = planIndex.find(node.id);
        if (found == planIndex.end()) {
            commit.removed.push_back(node.id); // hidden, removed from the PDF
            continue;
        }
        const auto& item = out.plan.items[found->second];
        const BaseNode* written = newByKey.at(keys[found->second]);
        BookmarkNode next = node;
        next.parentId = item.parent >= 0 ? std::optional<std::string>{out.plan.items[static_cast<std::size_t>(item.parent)].nodeId}
                                         : std::nullopt;
        next.position = nextPosition[static_cast<std::size_t>(item.parent + 1)]++;
        next.baseKey = written->key;
        if (item.hidden) {
            next.state = StorageState::locallyHidden; // still hidden; its local fields stay as they were
        } else {
            next.title = written->title;              // exactly what the PDF now holds
            next.location = written->location;
            next.state = StorageState::embedded;
        }
        next.conflict.reset();
        commit.nodes.push_back(std::move(next));
    }
    try {
        out.overlayRevision = store.commitSave(input.documentId, input.overlayRevision, commit);
    } catch (const OverlayRevisionConflict&) {
        return fail("overlayChanged");
    } catch (const std::exception&) {
        return fail("overlayChanged");
    }
    out.ok = true;
    return out;
}

} // namespace atlas::app
