#include "core/bookmarks/BookmarkOverlay.h"

#include "core/bookmarks/LocationEnvelope.h"
#include "core/bookmarks/Utf8.h"

#include <algorithm>
#include <array>
#include <random>
#include <unordered_map>
#include <unordered_set>

namespace atlas::bookmarks {
namespace {

using S = StorageState;
using E = StorageEvent;

struct Rule final {
    S from;
    E event;
    S to;
};

// The complete transition table. Anything not here is refused; in particular
// a hidden node or a conflict cannot be edited until it is unhidden or resolved.
constexpr std::array<Rule, 22> rules{{
    {S::embedded, E::editLocally, S::localOverride},
    {S::localOverride, E::editLocally, S::localOverride},
    {S::localOnly, E::editLocally, S::localOnly},
    {S::pendingEmbed, E::editLocally, S::pendingEmbed},

    {S::localOnly, E::requestEmbed, S::pendingEmbed},
    {S::pendingEmbed, E::cancelEmbed, S::localOnly},

    {S::embedded, E::hide, S::locallyHidden},
    {S::localOverride, E::hide, S::locallyHidden},
    {S::locallyHidden, E::unhideUnchanged, S::embedded},
    {S::locallyHidden, E::unhideChanged, S::localOverride},

    {S::localOverride, E::revertToSource, S::embedded},

    {S::embedded, E::detectConflict, S::conflict},
    {S::localOnly, E::detectConflict, S::conflict},
    {S::pendingEmbed, E::detectConflict, S::conflict},
    {S::localOverride, E::detectConflict, S::conflict},
    {S::locallyHidden, E::detectConflict, S::conflict},

    {S::conflict, E::keepLocal, S::localOnly},
    {S::conflict, E::takeSource, S::embedded},

    {S::localOnly, E::embedVerified, S::embedded},
    {S::pendingEmbed, E::embedVerified, S::embedded},
    {S::localOverride, E::embedVerified, S::embedded},
    // Re-embedding an unchanged node is a no-op, recorded for completeness.
    {S::embedded, E::embedVerified, S::embedded},
}};

[[nodiscard]] bool proofComplete(const EmbedProof* proof) {
    return proof != nullptr && proof->written && proof->validated && proof->reopened && proof->reindexed &&
           !proof->sourceRevisionAfterWrite.empty();
}

[[nodiscard]] bool isPrintableAscii(std::string_view text) {
    for (const char c : text) {
        if (c <= 0x20 || c >= 0x7F) return false;
    }
    return true;
}

[[nodiscard]] bool requiresBaseKey(S state) {
    return state == S::embedded || state == S::localOverride || state == S::locallyHidden;
}

[[nodiscard]] bool forbidsBaseKey(S state) {
    return state == S::localOnly || state == S::pendingEmbed;
}

template <typename Node, typename KeyOf, typename ParentOf>
[[nodiscard]] std::string validateHierarchy(const std::vector<Node>& nodes, KeyOf keyOf, ParentOf parentOf) {
    if (nodes.size() > OverlayLimits::maxNodesPerDocument) return "tooManyNodes";
    std::unordered_map<std::string_view, const Node*> byKey;
    byKey.reserve(nodes.size());
    for (const auto& node : nodes) {
        if (!byKey.emplace(keyOf(node), &node).second) return "duplicateId";
    }
    // Sibling positions must be exactly 0..n-1 under each parent.
    std::unordered_map<std::string_view, std::vector<bool>> slots;
    std::unordered_map<std::string_view, std::size_t> counts;
    constexpr std::string_view rootKey{"\x01root"};
    for (const auto& node : nodes) {
        const auto& parent = parentOf(node);
        if (parent && !byKey.contains(*parent)) return "missingParent";
        ++counts[parent ? std::string_view{*parent} : rootKey];
    }
    for (const auto& node : nodes) {
        const auto& parent = parentOf(node);
        const auto key = parent ? std::string_view{*parent} : rootKey;
        auto& used = slots[key];
        if (used.empty()) used.resize(counts[key], false);
        if (node.position >= used.size() || used[node.position]) return "badPosition";
        used[node.position] = true;
    }
    // Depth and cycles: walk up from each node, memoizing known depths.
    std::unordered_map<std::string_view, std::size_t> depth;
    depth.reserve(nodes.size());
    std::vector<std::string_view> chain;
    for (const auto& node : nodes) {
        chain.clear();
        std::string_view current = keyOf(node);
        std::size_t base = 0;
        while (true) {
            if (const auto known = depth.find(current); known != depth.end()) {
                base = known->second;
                break;
            }
            chain.push_back(current);
            if (chain.size() > OverlayLimits::maxDepth + 1) return "tooDeepOrCycle";
            const auto& parent = parentOf(*byKey.at(current));
            if (!parent) {
                base = 0;
                break;
            }
            current = *parent;
            for (const auto earlier : chain) {
                if (earlier == current) return "cycle";
            }
        }
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            ++base;
            if (base > OverlayLimits::maxDepth) return "tooDeep";
            depth[*it] = base;
        }
    }
    return {};
}

[[nodiscard]] std::string validateLocationText(const std::string& location) {
    if (location.empty()) return {};
    // A newer envelope or scheme is kept as it is; only garbage is refused.
    if (decodeLocation(location).status == LocationDecodeStatus::malformed) return "malformedLocation";
    return {};
}

} // namespace

TransitionResult transition(StorageState from, StorageEvent event, const EmbedProof* proof) {
    if (event == E::embedVerified && !proofComplete(proof)) return {false, from};
    for (const auto& rule : rules) {
        if (rule.from == from && rule.event == event) return {true, rule.to};
    }
    return {false, from};
}

bool isOrdinaryStateChange(StorageState from, StorageState to) {
    if (from == to) return true;
    for (const auto& rule : rules) {
        if (rule.event != E::embedVerified && rule.from == from && rule.to == to) return true;
    }
    return false;
}

std::string validateNode(const BookmarkNode& node) {
    if (node.id.empty() || node.id.size() > OverlayLimits::maxIdBytes || !isPrintableAscii(node.id)) return "badId";
    if (node.documentId.empty() || node.documentId.size() > OverlayLimits::maxIdBytes) return "badDocumentId";
    if (node.parentId && (node.parentId->empty() || *node.parentId == node.id)) return "badParent";
    if (node.title.size() > OverlayLimits::maxTitleBytes || !isValidUtf8(node.title)) return "badTitle";
    if (node.description.size() > OverlayLimits::maxDescriptionBytes || !isValidUtf8(node.description)) {
        return "badDescription";
    }
    if (node.tags.size() > OverlayLimits::maxTags) return "tooManyTags";
    for (const auto& tag : node.tags) {
        if (tag.empty() || tag.size() > OverlayLimits::maxTagBytes || !isValidUtf8(tag)) return "badTag";
    }
    if (auto problem = validateLocationText(node.location); !problem.empty()) return problem;
    if (node.localRevision < 1) return "badRevision";
    if ((node.state == S::conflict) != node.conflict.has_value()) return "conflictMismatch";
    if (node.conflict && node.conflict->previous == S::conflict) return "conflictMismatch";
    if (requiresBaseKey(node.state) && (!node.baseKey || node.baseKey->empty())) return "missingBaseKey";
    if (forbidsBaseKey(node.state) && node.baseKey) return "unexpectedBaseKey";
    return {};
}

std::string validateTree(const std::vector<BookmarkNode>& nodes) {
    if (nodes.empty()) return {};
    const auto& documentId = nodes.front().documentId;
    for (const auto& node : nodes) {
        if (auto problem = validateNode(node); !problem.empty()) return problem;
        if (node.documentId != documentId) return "mixedDocuments";
    }
    return validateHierarchy(
        nodes, [](const BookmarkNode& n) -> std::string_view { return n.id; },
        [](const BookmarkNode& n) -> const std::optional<std::string>& { return n.parentId; });
}

std::string validateStructure(const std::vector<BookmarkNode>& nodes) {
    if (nodes.empty()) return {};
    const auto& documentId = nodes.front().documentId;
    for (const auto& node : nodes) {
        if (node.documentId != documentId) return "mixedDocuments";
        if (node.parentId && *node.parentId == node.id) return "badParent";
    }
    return validateHierarchy(
        nodes, [](const BookmarkNode& n) -> std::string_view { return n.id; },
        [](const BookmarkNode& n) -> const std::optional<std::string>& { return n.parentId; });
}

std::string validateBase(const BaseSnapshot& base) {
    if (base.sourceRevision.empty()) return "missingSourceRevision";
    for (const auto& node : base.nodes) {
        if (node.key.empty() || node.key.size() > OverlayLimits::maxIdBytes || !isPrintableAscii(node.key)) {
            return "badKey";
        }
        if (node.title.size() > OverlayLimits::maxTitleBytes || !isValidUtf8(node.title)) return "badTitle";
        if (auto problem = validateLocationText(node.location); !problem.empty()) return problem;
    }
    return validateHierarchy(
        base.nodes, [](const BaseNode& n) -> std::string_view { return n.key; },
        [](const BaseNode& n) -> const std::optional<std::string>& { return n.parentKey; });
}

std::string validateAgainstBase(const std::vector<BookmarkNode>& nodes, const std::optional<BaseSnapshot>& base) {
    std::unordered_map<std::string_view, const BaseNode*> baseByKey;
    if (base) {
        baseByKey.reserve(base->nodes.size());
        for (const auto& node : base->nodes) baseByKey.emplace(node.key, &node);
    }
    std::unordered_map<std::string_view, const BookmarkNode*> byId;
    byId.reserve(nodes.size());
    for (const auto& node : nodes) byId.emplace(node.id, &node);

    // Embedded siblings, in overlay order, must keep the PDF's relative order.
    // Local nodes may sit anywhere between them.
    constexpr std::string_view rootKey{"\x01root"};
    std::unordered_map<std::string_view, std::vector<std::pair<std::uint32_t, std::uint32_t>>> embeddedOrder;

    std::unordered_set<std::string_view> claimed;
    for (const auto& node : nodes) {
        if (!node.baseKey) continue;
        if (!claimed.insert(*node.baseKey).second) return "baseClaimedTwice";
        const auto found = baseByKey.find(*node.baseKey);
        if (found == baseByKey.end()) {
            // Only a conflict may point at a node the PDF no longer has.
            if (node.state != S::conflict) return "unknownBaseKey";
            continue;
        }
        if (node.state != S::embedded) continue;
        const BaseNode& source = *found->second;
        if (node.title != source.title || node.location != source.location) return "embeddedDiffersFromSource";
        std::optional<std::string> parentBaseKey;
        if (node.parentId) {
            const auto parent = byId.find(*node.parentId);
            if (parent == byId.end()) return "missingParent";
            parentBaseKey = parent->second->baseKey;
        }
        if (parentBaseKey != source.parentKey) return "embeddedDiffersFromSource";
        const auto group = node.parentId ? std::string_view{*node.parentId} : rootKey;
        embeddedOrder[group].emplace_back(node.position, source.position);
    }
    for (auto& [group, order] : embeddedOrder) {
        std::sort(order.begin(), order.end());
        for (std::size_t i = 1; i < order.size(); ++i) {
            if (order[i - 1].second >= order[i].second) return "embeddedOrderDiffersFromSource";
        }
    }
    return {};
}

std::vector<BookmarkNode> embeddedNodesFromBase(std::string_view documentId, const BaseSnapshot& base) {
    std::unordered_map<std::string_view, std::string> idForKey;
    idForKey.reserve(base.nodes.size());
    for (const auto& node : base.nodes) idForKey.emplace(node.key, newBookmarkId());

    std::vector<BookmarkNode> nodes;
    nodes.reserve(base.nodes.size());
    for (const auto& source : base.nodes) {
        BookmarkNode node;
        node.id = idForKey.at(source.key);
        node.documentId = std::string{documentId};
        if (source.parentKey) node.parentId = idForKey.at(*source.parentKey);
        node.position = source.position;
        node.title = source.title;
        node.location = source.location;
        node.state = S::embedded;
        node.baseKey = source.key;
        nodes.push_back(std::move(node));
    }
    return nodes;
}

std::string newBookmarkId() {
    static thread_local std::random_device device;
    constexpr std::string_view hex = "0123456789abcdef";
    std::string id(32, '0');
    for (std::size_t i = 0; i < id.size(); i += 8) {
        auto value = static_cast<std::uint32_t>(device());
        for (std::size_t k = 0; k < 8; ++k) {
            id[i + k] = hex[value & 0x0F];
            value >>= 4;
        }
    }
    return id;
}

std::string_view stateName(StorageState state) {
    switch (state) {
    case S::embedded: return "embedded";
    case S::localOnly: return "local_only";
    case S::pendingEmbed: return "pending_embed";
    case S::localOverride: return "local_override";
    case S::locallyHidden: return "locally_hidden";
    case S::conflict: return "conflict";
    }
    return "unknown";
}

std::optional<StorageState> parseStateName(std::string_view name) {
    for (const auto state : {S::embedded, S::localOnly, S::pendingEmbed, S::localOverride, S::locallyHidden,
                             S::conflict}) {
        if (stateName(state) == name) return state;
    }
    return std::nullopt;
}

std::string_view conflictReasonName(ConflictReason reason) {
    switch (reason) {
    case ConflictReason::sourceChanged: return "source_changed";
    case ConflictReason::sourceRemoved: return "source_removed";
    case ConflictReason::locationUnresolved: return "location_unresolved";
    }
    return "unknown";
}

std::optional<ConflictReason> parseConflictReasonName(std::string_view name) {
    for (const auto reason : {ConflictReason::sourceChanged, ConflictReason::sourceRemoved,
                              ConflictReason::locationUnresolved}) {
        if (conflictReasonName(reason) == name) return reason;
    }
    return std::nullopt;
}

} // namespace atlas::bookmarks
