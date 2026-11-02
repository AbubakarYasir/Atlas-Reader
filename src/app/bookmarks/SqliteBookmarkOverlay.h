#pragma once

// N5.2 overlay repository: persists a document's bookmark overlay in the Atlas
// profile database (schema owned by SqliteLibraryIndex, migration 5). Every
// change is one SQLite transaction (WAL, synchronous=FULL), so after a crash
// the overlay is either the state before the change or the state after it.
// SQLite stays private to this file; callers use core bookmark types.
// Thread-safe; call it from a worker thread, never the UI thread (AGENTS.md §2).

#include "core/bookmarks/BookmarkOverlay.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace atlas::bookmarks {

struct OverlaySnapshot final {
    std::int64_t revision{}; // 0 = nothing stored yet for this document
    std::vector<BookmarkNode> nodes; // live nodes, ordered by parent then position
    std::optional<BaseSnapshot> base;
};

struct OverlayChangeSet final {
    std::vector<BookmarkNode> upserts; // new or changed nodes (full records)
    std::vector<std::string> removals; // local nodes to remove (kept as tombstones)
};

enum class BaseAdoption : std::uint8_t {
    adopted,        // first outline seen: base stored, Embedded nodes created
    unchanged,      // same revision and outline as the stored base
    needsReconcile, // the PDF's outline or revision differs: nothing changed (N5.4 reconciles)
};

// Another writer changed the overlay since the caller loaded it.
class OverlayRevisionConflict final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// The change breaks an overlay rule (tree, state transition, "no false
// Embedded"). `reason()` is a stable machine name for tests and logs.
class OverlayRuleViolation final : public std::runtime_error {
public:
    explicit OverlayRuleViolation(std::string reason)
        : std::runtime_error("Bookmark overlay change refused: " + reason), reason_(std::move(reason)) {}
    [[nodiscard]] const std::string& reason() const noexcept { return reason_; }

private:
    std::string reason_;
};

class SqliteBookmarkOverlay final {
public:
    // Opens the profile database. The library index must already have migrated
    // it to schema 5 or later (it is opened first at startup).
    explicit SqliteBookmarkOverlay(const std::filesystem::path& databasePath);
    ~SqliteBookmarkOverlay();

    SqliteBookmarkOverlay(const SqliteBookmarkOverlay&) = delete;
    SqliteBookmarkOverlay& operator=(const SqliteBookmarkOverlay&) = delete;

    [[nodiscard]] OverlaySnapshot load(std::string_view documentId) const;

    // Records the outline read from the PDF at `base.sourceRevision`. The first
    // time, Embedded nodes mirroring it are created ahead of any local nodes.
    // Later, a different outline is never applied here: it needs reconciling.
    [[nodiscard]] BaseAdoption adoptBase(std::string_view documentId, const BaseSnapshot& base);

    // Applies a change atomically if the overlay is still at `expectedRevision`.
    // The store assigns local revisions. New nodes must be LocalOnly or
    // PendingEmbed; state changes must follow the transition table; Embedded
    // nodes must match the base; nodes from the PDF are hidden, not removed.
    // Returns the new overlay revision.
    std::int64_t apply(std::string_view documentId, std::int64_t expectedRevision, const OverlayChangeSet& change);

    // Documents with local research that is not in the PDF (any state other
    // than Embedded): what a restart after a crash must offer to continue.
    [[nodiscard]] std::vector<std::string> documentsWithLocalWork() const;

    // Test hook: called at named steps inside a write transaction
    // ("adoptBase:beforeCommit", "apply:beforeCommit"). Crash tests end the
    // process there to prove the transaction leaves nothing half-written.
    void setFaultHookForTesting(std::function<void(std::string_view step)> hook);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace atlas::bookmarks
