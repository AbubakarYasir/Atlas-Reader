#pragma once

// N5.3 editing engine: every bookmark command the UI offers, with undo and
// redo, on one document's overlay. Pure C++; no Qt, SQL or engine types.
//
// A command changes the editor's in-memory tree at once (so the panel can
// redraw immediately) and returns the change to persist. The caller writes it
// with SqliteBookmarkOverlay::apply on a worker thread, then calls confirm()
// with the new revision, or discardPending() and reloads if the write failed.
//
// States follow the overlay rules: editing a node that is in the PDF makes it a
// LocalOverride; putting it back the way the PDF has it makes it Embedded
// again; nodes from the PDF are hidden, never deleted; hidden nodes and
// conflicts must be unhidden or resolved before they are edited.

#include "core/bookmarks/BookmarkOverlay.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace atlas::bookmarks {

struct EditChange final {
    std::vector<BookmarkNode> upserts;
    std::vector<std::string> removals;
    std::vector<std::string> restores;
    [[nodiscard]] bool empty() const { return upserts.empty() && removals.empty() && restores.empty(); }
};

struct EditResult final {
    bool ok{false};
    std::string reason; // stable machine name when refused ("unhideFirst", "cycle", …)
    EditChange change;
    std::string nodeId; // the node the command created or acted on
};

enum class EditKind : std::uint8_t {
    create,
    rename,
    setLocation,
    setNotes,
    move,
    remove,
    hide,
    unhide,
    requestEmbed,
    cancelEmbed,
    autoCreate,
    resolve,
};

// One bookmark of a batch (Auto Create Bookmarks). `level` 0 is the top; a
// level may go at most one deeper than the bookmark before it.
struct NewBookmark final {
    std::string title;
    std::string location;
    std::uint32_t level{};
    std::string description{};       // notes (imports keep them)
    std::vector<std::string> tags{};
    std::optional<std::string> attachTo{}; // a top item goes under this existing bookmark (merge imports)
};

// Where a batch goes among the top-level bookmarks.
enum class BatchPlacement : std::uint8_t {
    after,   // after the existing bookmarks
    before,  // before them
    replace, // in place of them: the PDF's own are hidden, Atlas's own removed
};

class BookmarkEditor final {
public:
    static constexpr std::size_t maxHistory = 500;

    BookmarkEditor(std::string documentId, std::vector<BookmarkNode> nodes, std::optional<BaseSnapshot> base,
                   std::int64_t revision);

    [[nodiscard]] const std::string& documentId() const noexcept { return documentId_; }
    [[nodiscard]] std::int64_t revision() const noexcept { return revision_; }
    [[nodiscard]] std::size_t size() const noexcept { return nodes_.size(); }
    [[nodiscard]] const BookmarkNode* find(std::string_view id) const;
    // Ordered child IDs; nullopt = top level.
    [[nodiscard]] const std::vector<std::string>& children(const std::optional<std::string>& parent) const;
    // Every node in reading order (depth first), for export, search and tests.
    [[nodiscard]] std::vector<const BookmarkNode*> inOrder() const;
    // Titles from the top level down to the node (duplicates are fine).
    [[nodiscard]] std::vector<std::string> breadcrumb(std::string_view id) const;
    [[nodiscard]] bool hasPending() const noexcept { return pending_.has_value(); }
    [[nodiscard]] const std::optional<BaseSnapshot>& base() const noexcept { return base_; }

    // Commands. `index` is the position among the new parent's children
    // (clamped to the end). Titles, notes and tags are kept exactly as given.
    EditResult create(const std::optional<std::string>& parent, std::size_t index, std::string title,
                      std::string location, bool requestEmbed = false);
    EditResult rename(std::string_view id, std::string title);
    EditResult setLocation(std::string_view id, std::string location);
    EditResult setNotes(std::string_view id, std::string description, std::vector<std::string> tags);
    EditResult move(std::string_view id, const std::optional<std::string>& parent, std::size_t index);
    // Local node: removed with its subtree. Node from the PDF: hidden instead.
    EditResult remove(std::string_view id);
    EditResult setHidden(std::string_view id, bool hidden);
    EditResult setEmbedRequested(std::string_view id, bool requested);
    // Settles a conflict. keepMine: Atlas's version stays (over the PDF's
    // bookmark if the PDF still has it, else as the reader's own). Otherwise
    // the PDF's version: title, destination and place as in the PDF.
    EditResult resolveConflict(std::string_view id, bool keepMine);
    // Adds a whole tree in one step (one undo). Refused if empty or too large.
    // `under`: add the tree's top items as children of this bookmark instead
    // of at the top level (after/before its existing children; replace is top-level only).
    EditResult createBatch(std::vector<NewBookmark> items, BatchPlacement placement,
                           const std::optional<std::string>& under = std::nullopt);

    [[nodiscard]] bool canUndo() const noexcept { return !undo_.empty() && !pending_; }
    [[nodiscard]] bool canRedo() const noexcept { return !redo_.empty() && !pending_; }
    [[nodiscard]] std::optional<EditKind> undoKind() const;
    [[nodiscard]] std::optional<EditKind> redoKind() const;
    EditResult undo();
    EditResult redo();

    // The pending change was stored; the overlay is now at `newRevision`.
    void confirm(std::int64_t newRevision);
    // The pending change was not stored. The caller reloads with reset().
    void discardPending();
    // Replace everything with a freshly loaded overlay (clears history).
    void reset(std::vector<BookmarkNode> nodes, std::optional<BaseSnapshot> base, std::int64_t revision);

private:
    struct Snapshot final {
        std::optional<BookmarkNode> before; // absent: the node did not exist
        std::optional<BookmarkNode> after;  // absent: the node was removed
    };
    struct Entry final {
        EditKind kind{EditKind::rename};
        std::vector<std::pair<std::string, Snapshot>> nodes;
    };
    enum class PendingSource : std::uint8_t { command, undo, redo };

    void rebuildIndex();
    [[nodiscard]] static std::string groupKey(const std::optional<std::string>& parent);
    [[nodiscard]] bool isSource(const BookmarkNode& node) const { return node.baseKey.has_value(); }
    [[nodiscard]] bool matchesBase(const BookmarkNode& node) const;
    [[nodiscard]] bool isAncestorOrSelf(std::string_view ancestor, std::string_view node) const;
    void renumber(const std::string& group, std::unordered_map<std::string, std::optional<BookmarkNode>>& touched);
    void remember(std::unordered_map<std::string, std::optional<BookmarkNode>>& touched, const std::string& id) const;
    EditResult finish(EditKind kind, std::unordered_map<std::string, std::optional<BookmarkNode>>& before,
                      std::string nodeId);
    EditResult refuse(std::string reason) const;
    EditResult replay(const Entry& entry, bool forward, PendingSource source);
    void settleSourceState(BookmarkNode& node) const;

    std::string documentId_;
    std::int64_t revision_{};
    std::optional<BaseSnapshot> base_;
    std::unordered_map<std::string, const BaseNode*> baseByKey_;
    std::unordered_map<std::string, BookmarkNode> nodes_;
    std::unordered_map<std::string, std::vector<std::string>> children_; // "" = top level
    std::vector<Entry> undo_;
    std::vector<Entry> redo_;
    std::optional<Entry> pending_;
    PendingSource pendingSource_{PendingSource::command};
};

[[nodiscard]] std::string_view editKindName(EditKind kind);

} // namespace atlas::bookmarks
