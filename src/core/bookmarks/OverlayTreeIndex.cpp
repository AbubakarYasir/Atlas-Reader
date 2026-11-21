#include "core/bookmarks/OverlayTreeIndex.h"

#include <algorithm>
#include <unordered_set>

namespace atlas::bookmarks {

OverlayTreeIndex::OverlayTreeIndex(std::string documentId, std::vector<BookmarkNode> nodes,
                                   std::optional<BaseSnapshot> base)
    : documentId_(std::move(documentId)), base_(std::move(base)) {
    if (base_) {
        baseByKey_.reserve(base_->nodes.size());
        for (const auto& node : base_->nodes) baseByKey_.emplace(node.key, &node);
    }
    nodes_.reserve(nodes.size());
    for (auto& node : nodes) {
        auto id = node.id;
        put(id, std::move(node));
    }
}

std::string OverlayTreeIndex::groupKey(const std::optional<std::string>& parent) {
    return parent.value_or(std::string{});
}

const BookmarkNode* OverlayTreeIndex::find(std::string_view id) const {
    const auto found = nodes_.find(std::string{id});
    return found == nodes_.end() ? nullptr : &found->second;
}

std::vector<BookmarkNode> OverlayTreeIndex::nodes() const {
    std::vector<BookmarkNode> out;
    out.reserve(nodes_.size());
    for (const auto& [id, node] : nodes_) out.push_back(node);
    return out;
}

void OverlayTreeIndex::put(const std::string& id, const std::optional<BookmarkNode>& node) {
    if (const auto current = nodes_.find(id); current != nodes_.end()) {
        auto& siblings = children_[groupKey(current->second.parentId)];
        siblings.erase(std::find(siblings.begin(), siblings.end(), id));
        if (current->second.baseKey) {
            if (--claims_[*current->second.baseKey] == 0) claims_.erase(*current->second.baseKey);
        }
        nodes_.erase(current);
    }
    if (!node) return;
    children_[groupKey(node->parentId)].push_back(id);
    if (node->baseKey) ++claims_[*node->baseKey];
    nodes_.insert_or_assign(id, *node);
}

std::string OverlayTreeIndex::apply(const std::vector<BookmarkNode>& upserts,
                                    const std::vector<std::string>& removals) {
    std::unordered_map<std::string, std::optional<BookmarkNode>> before;
    std::vector<std::string> order; // for an exact rollback
    const auto remember = [&](const std::string& id) {
        if (before.contains(id)) return;
        const auto found = nodes_.find(id);
        before.emplace(id, found == nodes_.end() ? std::nullopt : std::optional<BookmarkNode>{found->second});
        order.push_back(id);
    };
    for (const auto& id : removals) {
        remember(id);
        put(id, std::nullopt);
    }
    for (const auto& node : upserts) {
        remember(node.id);
        put(node.id, node);
    }
    auto problem = check(upserts, removals, before);
    if (!problem.empty()) {
        for (auto it = order.rbegin(); it != order.rend(); ++it) put(*it, before.at(*it));
    }
    return problem;
}

std::string OverlayTreeIndex::check(const std::vector<BookmarkNode>& upserts, const std::vector<std::string>& removals,
                                    const std::unordered_map<std::string, std::optional<BookmarkNode>>& before) const {
    if (nodes_.size() > OverlayLimits::maxNodesPerDocument) return "tooManyNodes";
    std::unordered_set<std::string> groups;

    // Removed nodes leave no live children behind.
    for (const auto& id : removals) {
        if (const auto kids = children_.find(id); kids != children_.end() && !kids->second.empty()) {
            return "missingParent";
        }
        if (const auto& old = before.at(id)) groups.insert(groupKey(old->parentId));
    }

    for (const auto& node : upserts) {
        if (node.documentId != documentId_) return "mixedDocuments";
        if (node.parentId && *node.parentId == node.id) return "badParent";
        if (node.parentId && !nodes_.contains(*node.parentId)) return "missingParent";
        groups.insert(groupKey(node.parentId));
        const auto& old = before.at(node.id);
        if (old) groups.insert(groupKey(old->parentId));

        // Ancestors: no cycle, depth within the limit.
        std::size_t depth = 1;
        const BookmarkNode* walk = &node;
        while (walk->parentId) {
            if (*walk->parentId == node.id) return "cycle";
            const auto parent = nodes_.find(*walk->parentId);
            if (parent == nodes_.end()) return "missingParent";
            walk = &parent->second;
            if (++depth > OverlayLimits::maxDepth) return "tooDeep";
        }
        // A node with a new parent carries its subtree down with it.
        if (!old || old->parentId != node.parentId) {
            std::vector<std::pair<const std::string*, std::size_t>> stack{{&node.id, depth}};
            while (!stack.empty()) {
                const auto [id, level] = stack.back();
                stack.pop_back();
                if (level > OverlayLimits::maxDepth) return "tooDeep";
                if (const auto kids = children_.find(*id); kids != children_.end()) {
                    for (const auto& child : kids->second) stack.emplace_back(&child, level + 1);
                }
            }
        }

        // Base: one claimant per source node; only conflicts may point at a
        // node the PDF no longer has.
        if (node.baseKey) {
            if (claims_.at(*node.baseKey) > 1) return "baseClaimedTwice";
            if (!baseByKey_.contains(*node.baseKey) && node.state != StorageState::conflict) return "unknownBaseKey";
        }
    }

    // Embedded nodes match their source: changed nodes, and the children of
    // nodes whose base key changed (their parent condition depends on it).
    const auto matchesSource = [&](const BookmarkNode& node) -> std::string {
        const auto source = baseByKey_.find(*node.baseKey);
        if (source == baseByKey_.end()) return "unknownBaseKey";
        if (node.title != source->second->title || node.location != source->second->location) {
            return "embeddedDiffersFromSource";
        }
        std::optional<std::string> parentBaseKey;
        if (node.parentId) parentBaseKey = nodes_.at(*node.parentId).baseKey;
        if (parentBaseKey != source->second->parentKey) return "embeddedDiffersFromSource";
        return {};
    };
    for (const auto& node : upserts) {
        const auto& current = nodes_.at(node.id);
        if (current.state == StorageState::embedded) {
            if (auto problem = matchesSource(current); !problem.empty()) return problem;
        }
        const auto& old = before.at(node.id);
        if (old && old->baseKey != current.baseKey) {
            if (const auto kids = children_.find(node.id); kids != children_.end()) {
                for (const auto& child : kids->second) {
                    const auto& kid = nodes_.at(child);
                    if (kid.state != StorageState::embedded) continue;
                    if (auto problem = matchesSource(kid); !problem.empty()) return problem;
                }
            }
        }
    }

    // Sibling positions and the PDF's relative order, per touched list.
    std::vector<bool> used;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> embedded;
    for (const auto& group : groups) {
        const auto found = children_.find(group);
        if (found == children_.end()) continue;
        const auto& members = found->second;
        used.assign(members.size(), false);
        embedded.clear();
        for (const auto& id : members) {
            const auto& node = nodes_.at(id);
            if (node.position >= used.size() || used[node.position]) return "badPosition";
            used[node.position] = true;
            if (node.state == StorageState::embedded && node.baseKey) {
                if (const auto source = baseByKey_.find(*node.baseKey); source != baseByKey_.end()) {
                    embedded.emplace_back(node.position, source->second->position);
                }
            }
        }
        std::sort(embedded.begin(), embedded.end());
        for (std::size_t i = 1; i < embedded.size(); ++i) {
            if (embedded[i - 1].second >= embedded[i].second) return "embeddedOrderDiffersFromSource";
        }
    }
    return {};
}

} // namespace atlas::bookmarks
