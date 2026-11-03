#pragma once

// N5.2 local overlay: Atlas's own record of a document's bookmarks, kept
// beside (never inside) the PDF until a validated write proves otherwise.
// Contract: docs/DATA_AND_SAFETY.md ("Bookmark node", "Local overlay and
// promotion") and ADR-0008. Pure C++; no engine, Qt, SQL or OS types.

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace atlas::bookmarks {

enum class StorageState : std::uint8_t {
    embedded,      // matches the PDF's own outline as last read
    localOnly,     // exists only in Atlas
    pendingEmbed,  // local, and the user asked for it to go into the PDF when possible
    localOverride, // an embedded node with a local rename, move or destination change
    locallyHidden, // hidden in Atlas, still in the PDF
    conflict,      // local work and the PDF disagree; needs the user's decision
};

// What a person or the app did. Transitions are explicit; anything not listed
// in docs/DATA_AND_SAFETY.md (and the table in BookmarkOverlay.cpp) is refused.
enum class StorageEvent : std::uint8_t {
    editLocally,      // rename, move, reorder, change destination, notes, tags
    requestEmbed,     // "put this into the PDF when it becomes possible"
    cancelEmbed,      // keep it local after all
    hide,             // hide an embedded node in Atlas only
    unhideUnchanged,  // show it again; local fields equal the base
    unhideChanged,    // show it again; local fields differ from the base
    revertToSource,   // drop local changes to an embedded node
    detectConflict,   // reconciliation found a disagreement with the PDF
    keepLocal,        // conflict resolved by keeping Atlas's version, local only
    keepOverride,     // conflict on a bookmark the PDF still has: keep Atlas's version over it (N5.4)
    takeSource,       // conflict resolved by taking the PDF's version
    embedVerified,    // a write was validated, reopened and re-indexed (needs EmbedProof)
};

enum class ConflictReason : std::uint8_t {
    sourceChanged,  // the PDF's node changed since the base snapshot
    sourceRemoved,  // the PDF no longer has the node
    locationUnresolved, // the destination no longer exists (page removed, newer envelope)
};

struct ConflictInfo final {
    ConflictReason reason{ConflictReason::sourceChanged};
    StorageState previous{StorageState::localOverride}; // state before the conflict
    std::string observedSourceRevision;
};

// Evidence that N5.4 safe save completed for this node. Every flag must be
// true; nothing becomes Embedded on intent alone.
struct EmbedProof final {
    bool written{false};
    bool validated{false};
    bool reopened{false};
    bool reindexed{false};
    std::string sourceRevisionAfterWrite;
};

struct TransitionResult final {
    bool allowed{false};
    StorageState next{StorageState::localOnly};
};

[[nodiscard]] TransitionResult transition(StorageState from, StorageEvent event,
                                          const EmbedProof* proof = nullptr);

// True when `from` → `to` can be reached by one event other than embedVerified.
// The store uses this to refuse writes that would skip a step or claim Embedded.
[[nodiscard]] bool isOrdinaryStateChange(StorageState from, StorageState to);

// A node's place and content. `location` holds the encoded location envelope
// text; it is kept as text so an envelope from a newer build is never dropped.
// An empty location means a grouping-only node.
struct BookmarkNode final {
    std::string id;         // stable Atlas ID, independent of the title
    std::string documentId; // Atlas document identity (ADR-0005)
    std::optional<std::string> parentId;
    std::uint32_t position{}; // order among siblings, contiguous from 0
    std::string title;        // exact UTF-8; duplicates are valid
    std::string location;     // encoded LocationEnvelope, or empty
    std::string description;  // exact UTF-8
    std::vector<std::string> tags; // exact UTF-8, order kept
    StorageState state{StorageState::localOnly};
    std::optional<std::string> baseKey; // the base-snapshot node this mirrors or overrides
    std::int64_t localRevision{1};      // increases with every local change
    std::optional<ConflictInfo> conflict;
};

// The PDF outline as Atlas last read it: the base for later reconciliation.
struct BaseNode final {
    std::string key; // stable within one snapshot ("0", "0/3", …)
    std::optional<std::string> parentKey;
    std::uint32_t position{};
    std::string title;
    std::string location; // encoded LocationEnvelope, or empty
};

struct BaseSnapshot final {
    std::string sourceRevision; // revision stamp the outline was read from
    std::vector<BaseNode> nodes;
};

// Limits: high enough for real research (10,000+ nodes), low enough to stop a
// hostile import from exhausting memory (docs/DATA_AND_SAFETY.md, resource limits).
struct OverlayLimits final {
    static constexpr std::size_t maxNodesPerDocument = 200'000;
    static constexpr std::size_t maxDepth = 256;
    static constexpr std::size_t maxIdBytes = 128;
    static constexpr std::size_t maxTitleBytes = 16 * 1024;
    static constexpr std::size_t maxDescriptionBytes = 1024 * 1024;
    static constexpr std::size_t maxTags = 256;
    static constexpr std::size_t maxTagBytes = 1024;
};

// Checks one node's own fields. Empty when valid; otherwise a short reason.
[[nodiscard]] std::string validateNode(const BookmarkNode& node);

// Checks a whole document's live nodes: unique IDs, one document, every parent
// present, no cycles, depth limit, contiguous sibling positions, field rules.
[[nodiscard]] std::string validateTree(const std::vector<BookmarkNode>& nodes);

// Only the structural part of validateTree (IDs, one document, parents, cycles,
// depth, positions), for a tree whose node fields were already checked.
[[nodiscard]] std::string validateStructure(const std::vector<BookmarkNode>& nodes);

// Same structural checks for a base snapshot.
[[nodiscard]] std::string validateBase(const BaseSnapshot& base);

// "No false Embedded": every Embedded node must match its base node (title,
// location and parent), Embedded siblings must keep the PDF's relative order
// (local nodes may sit between them), and no two live nodes may claim the same
// base node. Empty when consistent.
[[nodiscard]] std::string validateAgainstBase(const std::vector<BookmarkNode>& nodes,
                                              const std::optional<BaseSnapshot>& base);

// The overlay nodes that mirror a freshly read PDF outline: one Embedded node
// per base node, with new Atlas IDs, same hierarchy and order.
[[nodiscard]] std::vector<BookmarkNode> embeddedNodesFromBase(std::string_view documentId,
                                                              const BaseSnapshot& base);

// 128-bit random ID as 32 lower-case hex digits.
[[nodiscard]] std::string newBookmarkId();

[[nodiscard]] std::string_view stateName(StorageState state);
[[nodiscard]] std::optional<StorageState> parseStateName(std::string_view name);
[[nodiscard]] std::string_view conflictReasonName(ConflictReason reason);
[[nodiscard]] std::optional<ConflictReason> parseConflictReasonName(std::string_view name);

} // namespace atlas::bookmarks
