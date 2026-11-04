#pragma once

// N5.4 safe save of a book's bookmarks into its PDF (ADR-0009,
// docs/DATA_AND_SAFETY.md "Safe save"). One blocking call, run on the
// bookmark worker thread:
//
//   plan → preflight (same revision the overlay was read from, outline
//   embedding permitted, writable, space) → qpdf write to a temporary file in
//   the same folder → qpdf check and verify → reopen with Qt PDF → recovery
//   backup in the profile → source unchanged? → atomic replace → replaced file
//   equals the verified one → re-read the outline → overlay commit (saved
//   nodes become Embedded with an EmbedProof, one transaction).
//
// Failures before replacement leave PDF and overlay unchanged. Late failures
// attempt verified atomic restoration; recoveryRequired means restoration was
// not proved and the current file and backup must both be retained.

#include "app/bookmarks/SqliteBookmarkOverlay.h"
#include "core/bookmarks/OutlineSavePlan.h"

#include <QString>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace atlas::app {

// A pending journal blocks another write until recovery is resolved. No password
// is persisted. 'restored' requires reopening the reader before further editing.
enum class SaveRecovery { none, completed, restored, required };
[[nodiscard]] SaveRecovery recoverOutlineSave(bookmarks::SqliteBookmarkOverlay& store,
    const QString& path, const std::string& documentId, const std::filesystem::path& backupRoot);

// The reader's way out of `required` (ADR-0009 "Recovery journal"). Nothing is
// deleted: the journal is renamed `resolved-<time>.json` and every backup stays.
// - keepCurrent: accept the PDF as it is now; the overlay keeps the reader's work
//   and reconciles with the file on reopen.
// - restoreOriginal: put back the verified pre-save backup, after a hash-checked
//   copy of the current file is added to the backups. Only while the overlay is
//   still at the journal's revision (the save was never committed), so restoring
//   cannot drop bookmarks the overlay believes are in the PDF.
// Result: kept, restored, or a refusal (busy, noJournal, backupMissing,
// notSafe, copyFailed, replaceFailed).
enum class RecoveryChoice { keepCurrent, restoreOriginal };
[[nodiscard]] QString resolveSaveRecovery(bookmarks::SqliteBookmarkOverlay& store, const QString& path,
    const std::string& documentId, const std::filesystem::path& backupRoot, RecoveryChoice choice);

struct SafeSaveInput final {
    QString path;                                   // the PDF to save into
    QString password;                               // session only; never stored
    std::string documentId;
    std::int64_t overlayRevision{};                 // the overlay revision the nodes come from
    std::vector<bookmarks::BookmarkNode> nodes;     // live nodes, depth first
    std::optional<bookmarks::BaseSnapshot> base;
    bookmarks::SaveOptions options;
    std::filesystem::path backupRoot;               // <profile>/backups
    QString qpdf;                                   // empty: QpdfInspector::locate()
};

enum class SaveDisposition { unchanged, saved, restored, recoveryRequired };

struct SafeSaveOutcome final {
    SaveDisposition disposition{SaveDisposition::unchanged};
    bool ok{false};
    // Stable reason when !ok: nothingToSave, resolveFirst, missingBaseItems,
    // unreadableLocation, changedOutside, notPermitted, diskFull, qpdfUnavailable,
    // folderReadOnly, outlineMismatch, writeFailed, validation, preservation,
    // outlineDiffers, reopenDiffers, backupFailed, changedDuringSave,
    // replaceFailed, replaceUnverified, recoveryRequired, reindexFailed, overlayChanged.
    QString reason;
    bool pdfChanged{false};       // the PDF was replaced (ok, or overlayChanged)
    bookmarks::SavePlan plan;
    QString backupPath;
    std::int64_t overlayRevision{};
    std::string sourceRevision;   // the PDF's revision after the save
};

// Test hook: return false at a named step to make it fail there
// ("space" = too little free space, "write", "verify", "reopen", "backup", "beforeReplace",
// "replace", "afterReplace", "reindex", "commitOverlay", "restore").
using SafeSaveFault = std::function<bool(std::string_view step)>;

[[nodiscard]] SafeSaveOutcome saveOutlineSafely(bookmarks::SqliteBookmarkOverlay& store, const SafeSaveInput& input,
                                                const SafeSaveFault& fault = {});

// N5.4 Save a Copy: the same plan written into a new file (`target`), for
// PDFs Atlas may read but not change in place (read-only, in use, signed,
// certified). The source and the overlay are untouched; the copy is checked by
// qpdf and reopened with Qt PDF before it appears under its name. Refused when
// the PDF's own security does not allow changing its outline.
[[nodiscard]] SafeSaveOutcome saveOutlineCopy(const SafeSaveInput& input, const QString& target,
                                              const SafeSaveFault& fault = {});

// "size=…;mtime=…": the revision stamp the overlay's base snapshot records.
[[nodiscard]] std::string fileRevision(const QString& path);

} // namespace atlas::app
