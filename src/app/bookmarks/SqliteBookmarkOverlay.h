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
    // Removed local nodes brought back by undo. Each must also be in `upserts`
    // with its full record; only these may reuse a retired ID.
    std::vector<std::string> restores;
};

// N5.4: the outcome of a verified save (ADR-0009), applied in one transaction.
struct SaveCommit final {
    BaseSnapshot base;                // the outline as read back from the written file
    std::vector<BookmarkNode> nodes;  // every live node afterwards (full records)
    std::vector<std::string> removed; // hidden PDF nodes the save removed from the PDF
    EmbedProof proof;                 // every flag must be true
};

// N5.4: the outcome of reconciling with a PDF changed outside Atlas.
struct ReconcileCommit final {
    BaseSnapshot base;                // the outline as read now
    std::vector<BookmarkNode> nodes;  // every live node afterwards; new ones only Embedded
    std::vector<std::string> removed; // Embedded or hidden nodes the PDF dropped
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

    // N5.4 safe save: replaces the base snapshot with the outline read back
    // from the written file and moves the saved nodes to Embedded (the only
    // way local work becomes Embedded). Every live node must be accounted for;
    // states may only stay as they are or take the embedVerified step with a
    // complete proof; the result must pass "no false Embedded" against the new
    // base. Returns the new overlay revision.
    std::int64_t commitSave(std::string_view documentId, std::int64_t expectedRevision, const SaveCommit& commit);

    // N5.4 reconcile (core/bookmarks/Reconcile): the new outline becomes the
    // base; nodes keep their state or become conflicts; new nodes only as
    // Embedded mirrors; only Embedded or hidden nodes may be dropped.
    std::int64_t commitReconcile(std::string_view documentId, std::int64_t expectedRevision,
                                 const ReconcileCommit& commit);

    // Documents with local research that is not in the PDF (any state other
    // than Embedded): what a restart after a crash must offer to continue.
    [[nodiscard]] std::vector<std::string> documentsWithLocalWork() const;

    // Test hook: called at named steps inside a write transaction
    // ("adoptBase:beforeCommit", "apply:beforeCommit", "commitSave:beforeCommit"). Crash tests end the
    // process there to prove the transaction leaves nothing half-written.
    void setFaultHookForTesting(std::function<void(std::string_view step)> hook);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace atlas::bookmarks
