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
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QLockFile>
#include <QUuid>

#include <algorithm>
#include <limits>
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
    QFile source{path};
    if (!source.open(QIODevice::ReadOnly)) return std::nullopt;
    return readOutline(document, source.readAll(), password);
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
    // Pending recovery owns its backup. Retention runs only after success.
    return copy;
}

struct TempFile final {
    QString path;
    ~TempFile() {
        if (!path.isEmpty()) QFile::remove(path);
    }
};

} // namespace

SaveRecovery recoverOutlineSave(SqliteBookmarkOverlay& store, const QString& path,
                               const std::string& documentId, const std::filesystem::path& backupRoot) {
    const QString folder = qpath(backupRoot / documentId);
    const QString journal = QDir{folder}.filePath(QStringLiteral("pending-save.json"));
    if (!QFileInfo::exists(journal)) return SaveRecovery::none;
    QLockFile lock{QDir{folder}.filePath(QStringLiteral("save.lock"))};
    if (!lock.tryLock(0)) return SaveRecovery::required;
    QFile file{journal};
    if (!file.open(QIODevice::ReadOnly) || file.size() > 16384) return SaveRecovery::required;
    const auto record = QJsonDocument::fromJson(file.readAll()).object();
    file.close();
    if (record.value("schema").toInt() != 1 || record.value("document").toString() != utf8(documentId) ||
        record.value("path").toString() != QFileInfo{path}.absoluteFilePath()) return SaveRecovery::required;
    const QByteArray original = QByteArray::fromHex(record.value("original").toString().toLatin1());
    const QByteArray replacement = QByteArray::fromHex(record.value("replacement").toString().toLatin1());
    if (original.size() != 32 || replacement.size() != 32) return SaveRecovery::required;
    bool validRevision = false;
    const auto revision = record.value("revision").toString().toLongLong(&validRevision);
    if (!validRevision || revision < 0 || revision == std::numeric_limits<qint64>::max()) return SaveRecovery::required;
    const auto snapshot = store.load(documentId);
    const QByteArray current = sha256(path);
    if (current == replacement && snapshot.revision == revision + 1 && snapshot.base &&
        snapshot.base->sourceRevision == fileRevision(path)) {
        return QFile::remove(journal) ? SaveRecovery::completed : SaveRecovery::required;
    }
    if (snapshot.revision != revision) return SaveRecovery::required;
    if (current == original)
        return QFile::remove(journal) ? SaveRecovery::completed : SaveRecovery::required;
    if (current != replacement) return SaveRecovery::required; // preserve external edits
    const QString name = record.value("backup").toString();
    if (name.isEmpty() || QFileInfo{name}.fileName() != name) return SaveRecovery::required;
    const QString source = QDir{folder}.filePath(name);
    if (sha256(source) != original) return SaveRecovery::required;
    TempFile restored{path + QStringLiteral(".atlas-recover-") + QUuid::createUuid().toString(QUuid::Id128)};
    if (!QFile::copy(source, restored.path) || sha256(restored.path) != original ||
        sha256(path) != replacement || !replaceFile(path, restored.path) || sha256(path) != original)
        return SaveRecovery::required;
    restored.path.clear();
    return QFile::remove(journal) ? SaveRecovery::restored : SaveRecovery::required;
}

QString resolveSaveRecovery(SqliteBookmarkOverlay& store, const QString& path, const std::string& documentId,
                            const std::filesystem::path& backupRoot, RecoveryChoice choice, const QString& password) {
    const QString folder = qpath(backupRoot / documentId);
    const QString journal = QDir{folder}.filePath(QStringLiteral("pending-save.json"));
    if (!QFileInfo::exists(journal)) return QStringLiteral("noJournal");
    QLockFile lock{QDir{folder}.filePath(QStringLiteral("save.lock"))};
    if (!lock.tryLock(0)) return QStringLiteral("busy");
    const auto settle = [&journal, &folder](const char* outcome) {
        const QString resolved = QDir{folder}.filePath(
            QStringLiteral("resolved-") +
            QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) + QStringLiteral(".json"));
        return QFile::rename(journal, resolved) ? QString::fromLatin1(outcome) : QStringLiteral("replaceFailed");
    };
    if (choice == RecoveryChoice::keepCurrent) {
        lock.unlock();
        return keepCurrentAfterRecovery(path, password, documentId, backupRoot)
            ? QStringLiteral("kept") : QStringLiteral("copyFailed");
    }

    QFile file{journal};
    if (!file.open(QIODevice::ReadOnly) || file.size() > 16384) return QStringLiteral("backupMissing");
    const auto record = QJsonDocument::fromJson(file.readAll()).object();
    file.close();
    const QByteArray original = QByteArray::fromHex(record.value("original").toString().toLatin1());
    const QString name = record.value("backup").toString();
    bool validRevision = false;
    const auto revision = record.value("revision").toString().toLongLong(&validRevision);
    if (record.value("schema").toInt() != 1 ||
        record.value("path").toString() != QFileInfo{path}.absoluteFilePath() ||
        record.value("document").toString() != utf8(documentId) || original.size() != 32 || name.isEmpty() ||
        QFileInfo{name}.fileName() != name)
        return QStringLiteral("backupMissing");
    const QString source = QDir{folder}.filePath(name);
    if (sha256(source) != original || !reopen(source, password)) return QStringLiteral("backupMissing");
    if (!validRevision || store.load(documentId).revision != revision) return QStringLiteral("notSafe");
    const QByteArray current = sha256(path);
    if (current == original) return settle("restored"); // already the original
    // Keep what is there now before replacing it.
    const QDir archive{QDir{folder}.filePath(QStringLiteral("restored-") + QUuid::createUuid().toString(QUuid::Id128))};
    if (current.isEmpty() || !QDir{}.mkpath(archive.path()) ||
        !QFile::copy(path, archive.filePath(QStringLiteral("displaced-current.pdf"))) ||
        sha256(archive.filePath(QStringLiteral("displaced-current.pdf"))) != current ||
        !QFile::copy(source, archive.filePath(QStringLiteral("before-save.pdf"))) ||
        sha256(archive.filePath(QStringLiteral("before-save.pdf"))) != original ||
        !QFile::copy(journal, archive.filePath(QStringLiteral("resolution-source.json")))) return QStringLiteral("copyFailed");
    TempFile restored{path + QStringLiteral(".atlas-recover-") + QUuid::createUuid().toString(QUuid::Id128)};
    if (!QFile::copy(source, restored.path) || sha256(restored.path) != original) return QStringLiteral("copyFailed");
    if (sha256(path) != current || !replaceFile(path, restored.path) || sha256(path) != original)
        return QStringLiteral("replaceFailed");
    restored.path.clear();
    return settle("restored");
}

std::string fileRevision(const QString& path) {
    const QFileInfo info{path};
    if (!info.exists()) return {};
    return "size=" + std::to_string(info.size()) + ";mtime=" + std::to_string(info.lastModified().toMSecsSinceEpoch());
}

bool keepCurrentAfterRecovery(const QString& path, const QString& password,
                              const std::string& documentId, const std::filesystem::path& backupRoot,
                              const SafeSaveFault& fault) {
    const QDir folder{qpath(backupRoot / documentId)};
    QLockFile lock{folder.filePath(QStringLiteral("save.lock"))};
    if (!lock.tryLock(0)) return false;
    QFile journal{folder.filePath(QStringLiteral("pending-save.json"))};
    if (!journal.open(QIODevice::ReadOnly) || journal.size() > 16384) return false;
    const QByteArray bytes = journal.readAll();
    const auto record = QJsonDocument::fromJson(bytes).object();
    journal.close();
    const QString name = record.value("backup").toString();
    const QByteArray original = QByteArray::fromHex(record.value("original").toString().toLatin1());
    if (record.value("schema").toInt() != 1 || record.value("document").toString() != utf8(documentId) ||
        record.value("path").toString() != QFileInfo{path}.absoluteFilePath() || original.size() != 32 ||
        name.isEmpty() || QFileInfo{name}.fileName() != name || sha256(folder.filePath(name)) != original)
        return false;
    const QByteArray current = sha256(path);
    if (current.size() != 32 || !reopen(path, password)) return false;
    // Subdirectories are deliberately outside ordinary successful-save retention.
    // A failed attempt also retains its copies for inspection, never deletes them.
    const QDir archive{folder.filePath(QStringLiteral("resolved-") + QUuid::createUuid().toString(QUuid::Id128))};
    if (!QDir{}.mkpath(archive.path())) return false;
    const QString originalCopy = archive.filePath(QStringLiteral("before-save.pdf"));
    const QString currentCopy = archive.filePath(QStringLiteral("kept-current.pdf"));
    if (!QFile::copy(folder.filePath(name), originalCopy) || sha256(originalCopy) != original ||
        !QFile::copy(path, currentCopy) || sha256(currentCopy) != current) return false;
    QJsonObject resolution = record;
    resolution.insert(QStringLiteral("resolution"), QStringLiteral("owner-kept-current"));
    resolution.insert(QStringLiteral("keptHash"), QString::fromLatin1(current.toHex()));
    QSaveFile receipt{archive.filePath(QStringLiteral("resolution.json"))};
    const QByteArray receiptBytes = QJsonDocument{resolution}.toJson();
    if (!receipt.open(QIODevice::WriteOnly) || receipt.write(receiptBytes) != receiptBytes.size() || !receipt.commit())
        return false;
    if ((fault && !fault("beforeRecoveryResolution")) || sha256(path) != current) return false;
    // Recheck the pending record too: a failed/changed record never clears a hold.
    if (!journal.open(QIODevice::ReadOnly) || journal.readAll() != bytes) return false;
    journal.close();
    return journal.remove();
}

SafeSaveOutcome saveOutlineCopy(const SafeSaveInput& input, const QString& target, const SafeSaveFault& fault) {
    SafeSaveOutcome out;
    const auto fail = [&out](const char* reason) {
        out.ok = false;
        out.reason = QString::fromLatin1(reason);
        if (out.reason == QStringLiteral("recoveryRequired")) out.disposition = SaveDisposition::recoveryRequired;
        return out;
    };
    const auto step = [&fault](std::string_view name) { return !fault || fault(name); };
    std::vector<const BookmarkNode*> inOrder;
    for (const auto& node : input.nodes) inOrder.push_back(&node);
    const BaseSnapshot base = input.base.value_or(BaseSnapshot{fileRevision(input.path), {}});
    out.plan = planOutlineSave(inOrder, base, input.options);
    if (!out.plan.refusal.empty()) return fail(out.plan.refusal.c_str());
    const QFileInfo targetInfo{target};
    std::error_code identityError;
    const bool sameObject = std::filesystem::equivalent(
        std::filesystem::path{target.toStdU16String()}, std::filesystem::path{input.path.toStdU16String()}, identityError);
    if (target.isEmpty() || sameObject || targetInfo.absoluteFilePath() == QFileInfo{input.path}.absoluteFilePath())
        return fail("sameFile");
    const bool targetExisted = targetInfo.exists();
    const QByteArray targetHash = targetExisted ? sha256(target) : QByteArray{};
    if (targetExisted && targetHash.isEmpty()) return fail("replaceFailed");
    const QByteArray sourceHash = sha256(input.path);
    if (sourceHash.isEmpty()) return fail("changedOutside");
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
    if (!step("space") ||
        (volume.bytesAvailable() >= 0 && volume.bytesAvailable() < 2 * QFileInfo{input.path}.size() + (4 << 20)))
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
    // Confirmation does not authorize overwriting a destination changed while
    // qpdf was running. Preserve both files if either revision has changed.
    if (!step("beforeCopyReplace") || sha256(input.path) != sourceHash ||
        QFileInfo::exists(target) != targetExisted || (targetExisted && sha256(target) != targetHash))
        return fail("changedDuringSave");
    // Into place under the chosen name (the file dialog already confirmed any overwrite).
    if (QFileInfo::exists(target) ? !replaceFile(target, temp.path)
                                  : !QFile::rename(temp.path, target))
        return fail("replaceFailed");
    temp.path.clear();
    out.disposition = SaveDisposition::saved;
    out.ok = true;
    out.sourceRevision = fileRevision(target);
    return out;
}

SafeSaveOutcome saveOutlineSafely(SqliteBookmarkOverlay& store, const SafeSaveInput& input, const SafeSaveFault& fault) {
    SafeSaveOutcome out;
    const auto fail = [&out](const char* reason) {
        out.ok = false;
        out.reason = QString::fromLatin1(reason);
        if (out.reason == QStringLiteral("recoveryRequired")) out.disposition = SaveDisposition::recoveryRequired;
        return out;
    };
    const auto step = [&fault](std::string_view name) { return !fault || fault(name); };

    const auto recovery = recoverOutlineSave(store, input.path, input.documentId, input.backupRoot);
    if (recovery == SaveRecovery::required) return fail("recoveryRequired");
    if (recovery == SaveRecovery::restored) {
        out.disposition = SaveDisposition::restored;
        return fail("replaceUnverified");
    }
    const QString recoveryFolder = qpath(input.backupRoot / input.documentId);
    if (!QDir{}.mkpath(recoveryFolder)) return fail("backupFailed");
    QLockFile saveLock{QDir{recoveryFolder}.filePath(QStringLiteral("save.lock"))};
    if (!saveLock.tryLock(0)) return fail("recoveryRequired");
    const QString journal = QDir{recoveryFolder}.filePath(QStringLiteral("pending-save.json"));
    if (QFileInfo::exists(journal)) return fail("recoveryRequired");

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
    if (!step("space") || (volume.bytesAvailable() >= 0 && volume.bytesAvailable() < 2 * info.size() + (4 << 20)))
        return fail("diskFull");
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
    QJsonObject record{{"schema", 1}, {"operation", QUuid::createUuid().toString(QUuid::Id128)},
        {"document", utf8(input.documentId)}, {"path", QFileInfo{input.path}.absoluteFilePath()},
        {"original", QString::fromLatin1(originalHash.toHex())},
        {"replacement", QString::fromLatin1(verifiedHash.toHex())},
        {"backup", QFileInfo{out.backupPath}.fileName()},
        {"revision", QString::number(input.overlayRevision)}, {"phase", "prepared"}};
    const auto recordPhase = [&](const QString& phase) {
        record.insert(QStringLiteral("phase"), phase);
        QSaveFile journalFile{journal};
        const QByteArray journalBytes = QJsonDocument{record}.toJson(QJsonDocument::Compact);
        return journalFile.open(QIODevice::WriteOnly) && journalFile.write(journalBytes) == journalBytes.size() &&
               journalFile.commit();
    };
    if (!recordPhase(QStringLiteral("prepared"))) return fail("backupFailed");
    if (!step("journalPrepared")) return fail("recoveryRequired");
    if (sha256(input.path) != originalHash || fileRevision(input.path) != originalRevision)
        return fail("recoveryRequired");
    if (!step("replace") || !replaceFile(input.path, temp.path)) {
        if (sha256(input.path) != originalHash) {
            out.pdfChanged = true;
            return fail("recoveryRequired");
        }
        QFile::remove(journal);
        return fail("replaceFailed");
    }
    temp.path.clear(); // it is the PDF now
    out.pdfChanged = true;
    // Never remove the current PDF before a verified recovery copy is ready.
    // If another writer changed it, preserve that version for explicit recovery.
    const auto restore = [&](const char* reason) {
        TempFile recovery;
        recovery.path = input.path + QStringLiteral(".atlas-restore-%1.pdf")
                                        .arg(QRandomGenerator::global()->generate());
        if (sha256(input.path) != verifiedHash || !step("restore") ||
            !QFile::copy(out.backupPath, recovery.path) || sha256(recovery.path) != originalHash ||
            sha256(input.path) != verifiedHash ||
            !replaceFile(input.path, recovery.path) || sha256(input.path) != originalHash) {
            return fail("recoveryRequired");
        }
        recovery.path.clear();
        if (!step("afterRestore")) return fail("recoveryRequired");
        out.pdfChanged = false;
        out.disposition = SaveDisposition::restored;
        if (!QFile::remove(journal)) return fail("recoveryRequired");
        return fail(reason);
    };
    if (!recordPhase(QStringLiteral("replaced")) || sha256(input.path) != verifiedHash || !step("afterReplace")) {
        return restore("replaceUnverified");
    }

    // 7. Re-read the saved PDF the way Atlas opens books, then commit the overlay.
    out.sourceRevision = fileRevision(input.path);
    const auto finalOutline = reopen(input.path, input.password);
    const auto newBase = finalOutline ? baseFromOutline(*finalOutline, out.sourceRevision) : std::nullopt;
    if (!finalOutline || !newBase || !sameAsPlan(*finalOutline, out.plan, keys) || !step("reindex"))
        return restore("replaceUnverified");
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
        if (!step("commitOverlay")) return restore("replaceUnverified");
        out.overlayRevision = store.commitSave(input.documentId, input.overlayRevision, commit);
    } catch (const OverlayRevisionConflict&) {
        return restore("replaceUnverified");
    } catch (const std::exception&) {
        return restore("replaceUnverified");
    }
    if (!recordPhase(QStringLiteral("committed")) || !step("afterOverlayCommit")) return fail("recoveryRequired");
    if (!QFile::remove(journal)) return fail("recoveryRequired");
    QDir backups{recoveryFolder};
    const auto copies = backups.entryList({QStringLiteral("*.pdf")}, QDir::Files, QDir::Name | QDir::Reversed);
    for (qsizetype i = 3; i < copies.size(); ++i) backups.remove(copies.at(i));
    out.disposition = SaveDisposition::saved;
    out.ok = true;
    return out;
}

} // namespace atlas::app
