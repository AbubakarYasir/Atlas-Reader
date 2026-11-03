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

struct SafeSaveOutcome final {
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
// ("write", "verify", "reopen", "backup", "beforeReplace", "replace", "afterReplace").
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
