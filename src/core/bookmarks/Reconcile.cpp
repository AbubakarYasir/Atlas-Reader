#include "core/bookmarks/Reconcile.h"

#include "core/bookmarks/LocationEnvelope.h"

#include <algorithm>
#include <deque>
#include <map>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace atlas::bookmarks {
namespace {

int pageOf(const std::string& location) {
    if (location.empty()) return -1;
    const auto decoded = decodeLocation(location);
    return decoded.status == LocationDecodeStatus::ok ? static_cast<int>(decoded.envelope->pdf.pageIndex) : -2;
}

// Base nodes in reading order (depth first).
std::vector<const BaseNode*> readingOrder(const BaseSnapshot& base) {
    std::unordered_map<std::string, std::vector<const BaseNode*>> children;
    for (const auto& node : base.nodes) children[node.parentKey.value_or("")].push_back(&node);
    for (auto& [_, list] : children) {
        std::sort(list.begin(), list.end(), [](const BaseNode* a, const BaseNode* b) { return a->position < b->position; });
    }
    std::vector<const BaseNode*> out;
    out.reserve(base.nodes.size());
    std::vector<std::pair<const std::vector<const BaseNode*>*, std::size_t>> stack;
    if (const auto top = children.find(""); top != children.end()) stack.emplace_back(&top->second, 0);
    while (!stack.empty() && out.size() <= base.nodes.size()) {
        auto& [list, next] = stack.back();
        if (next >= list->size()) {
            stack.pop_back();
            continue;
        }
        const BaseNode* node = (*list)[next++];
        out.push_back(node);
        if (const auto kids = children.find(node->key); kids != children.end()) stack.emplace_back(&kids->second, 0);
    }
    return out;
}

// Old base key → new base key: title and page in reading order, then title.
std::unordered_map<std::string, std::string> matchOutlines(const std::vector<const BaseNode*>& before,
                                                           const std::vector<const BaseNode*>& after) {
    std::unordered_map<std::string, std::string> matched;
    std::vector<bool> used(after.size(), false);
    const auto pass = [&](auto signature) {
        std::map<decltype(signature(*after.front())), std::deque<std::size_t>> candidates;
        for (std::size_t i = 0; i < after.size(); ++i) {
            if (!used[i]) candidates[signature(*after[i])].push_back(i);
        }
        std::size_t last = 0;
        bool any = false;
        for (const BaseNode* node : before) {
            if (matched.contains(node->key)) continue;
            const auto found = candidates.find(signature(*node));
            if (found == candidates.end()) continue;
            auto& queue = found->second;
            while (!queue.empty() && used[queue.front()]) queue.pop_front();
            if (queue.empty()) continue;
            // Prefer the first candidate after the previous match (keeps order).
            auto pick = std::find_if(queue.begin(), queue.end(), [&](std::size_t i) { return !any || i > last; });
            if (pick == queue.end()) pick = queue.begin();
            const std::size_t index = *pick;
            queue.erase(pick);
            used[index] = true;
            matched.emplace(node->key, after[index]->key);
            last = index;
            any = true;
        }
    };
    if (after.empty()) return matched;
    pass([](const BaseNode& n) { return std::pair{n.title, pageOf(n.location)}; });
    pass([](const BaseNode& n) { return n.title; });
    // A renamed bookmark: same (matched) parent and same destination page.
    std::map<std::pair<std::string, int>, std::deque<std::size_t>> byPlace;
    for (std::size_t i = 0; i < after.size(); ++i) {
        if (!used[i]) byPlace[{after[i]->parentKey.value_or("~"), pageOf(after[i]->location)}].push_back(i);
    }
    for (const BaseNode* node : before) {
        if (matched.contains(node->key)) continue;
        std::string parent = "~";
        if (node->parentKey) {
            const auto found = matched.find(*node->parentKey);
            if (found == matched.end()) continue;
            parent = found->second;
        }
        auto candidates = byPlace.find({parent, pageOf(node->location)});
        if (candidates == byPlace.end()) continue;
        while (!candidates->second.empty() && used[candidates->second.front()]) candidates->second.pop_front();
        if (candidates->second.empty()) continue;
        const std::size_t index = candidates->second.front();
        candidates->second.pop_front();
        used[index] = true;
        matched.emplace(node->key, after[index]->key);
    }
    return matched;
}

} // namespace

ReconcileResult reconcile(std::string_view documentId, const std::vector<BookmarkNode>& overlay,
                          const BaseSnapshot& oldBase, const BaseSnapshot& newBase) {
    ReconcileResult result;
    const auto before = readingOrder(oldBase);
    const auto after = readingOrder(newBase);
    const auto oldToNew = matchOutlines(before, after);
    std::unordered_map<std::string, std::string> newToOld;
    for (const auto& [o, n] : oldToNew) newToOld.emplace(n, o);
    std::unordered_map<std::string, const BaseNode*> oldByKey;
    for (const auto& node : oldBase.nodes) oldByKey.emplace(node.key, &node);
    std::unordered_map<std::string, const BaseNode*> newByKey;
    std::unordered_map<std::string, std::size_t> newIndex;
    for (std::size_t i = 0; i < after.size(); ++i) {
        newByKey.emplace(after[i]->key, after[i]);
        newIndex.emplace(after[i]->key, i);
    }

    std::unordered_map<std::string, const BookmarkNode*> byId;
    std::unordered_map<std::string, const BookmarkNode*> byOldKey;
    for (const auto& node : overlay) {
        byId.emplace(node.id, &node);
        if (node.baseKey) byOldKey.emplace(*node.baseKey, &node);
    }
    // Reading order of the overlay as it was, for keeping sibling order.
    std::unordered_map<std::string, std::size_t> oldOrder;
    {
        std::unordered_map<std::string, std::vector<const BookmarkNode*>> kids;
        for (const auto& node : overlay) kids[node.parentId.value_or("")].push_back(&node);
        for (auto& [_, list] : kids) {
            std::sort(list.begin(), list.end(), [](auto* a, auto* b) { return a->position < b->position; });
        }
        std::vector<const BookmarkNode*> stack;
        if (const auto top = kids.find(""); top != kids.end()) {
            for (auto it = top->second.rbegin(); it != top->second.rend(); ++it) stack.push_back(*it);
        }
        while (!stack.empty() && oldOrder.size() <= overlay.size()) {
            const auto* node = stack.back();
            stack.pop_back();
            oldOrder.emplace(node->id, oldOrder.size());
            if (const auto k = kids.find(node->id); k != kids.end()) {
                for (auto it = k->second.rbegin(); it != k->second.rend(); ++it) stack.push_back(*it);
            }
        }
    }

    std::unordered_map<std::string, BookmarkNode> next;       // by id
    std::unordered_map<std::string, std::string> idForNewKey; // new base key → node id
    std::unordered_set<std::string> followsPdf;               // placed where the PDF puts them

    // Nodes the PDF still has (or newly has).
    for (const BaseNode* source : after) {
        const auto oldKey = newToOld.find(source->key);
        const BookmarkNode* mine =
            oldKey == newToOld.end() ? nullptr
                                     : (byOldKey.contains(oldKey->second) ? byOldKey.at(oldKey->second) : nullptr);
        if (mine == nullptr) {
            BookmarkNode node;
            node.id = newBookmarkId();
            while (byId.contains(node.id) || next.contains(node.id)) node.id = newBookmarkId();
            node.documentId = std::string{documentId};
            node.title = source->title;
            node.location = source->location;
            node.state = StorageState::embedded;
            node.baseKey = source->key;
            idForNewKey.emplace(source->key, node.id);
            followsPdf.insert(node.id);
            next.emplace(node.id, std::move(node));
            ++result.added;
            continue;
        }
        const BaseNode* was = oldByKey.at(oldKey->second);
        const std::optional<std::string> wasParentNow =
            was->parentKey && oldToNew.contains(*was->parentKey) ? std::optional{oldToNew.at(*was->parentKey)} : std::nullopt;
        const bool pdfChanged = was->title != source->title || was->location != source->location ||
                                wasParentNow != source->parentKey || (was->parentKey.has_value() && !wasParentNow);
        BookmarkNode node = *mine;
        node.baseKey = source->key;
        switch (mine->state) {
        case StorageState::embedded:
            if (pdfChanged) ++result.updated;
            node.title = source->title;
            node.location = source->location;
            followsPdf.insert(node.id);
            break;
        case StorageState::localOverride:
            if (pdfChanged) {
                node.state = StorageState::conflict;
                node.conflict = ConflictInfo{ConflictReason::sourceChanged, StorageState::localOverride, newBase.sourceRevision};
                ++result.conflicts;
            }
            break;
        default: // hidden and conflicts keep their state and their local fields
            break;
        }
        idForNewKey.emplace(source->key, node.id);
        next.emplace(node.id, std::move(node));
    }
    // Nodes the PDF no longer has.
    for (const auto& node : overlay) {
        if (next.contains(node.id)) continue;
        if (!node.baseKey) {
            next.emplace(node.id, node); // the reader's own
            continue;
        }
        if (node.state == StorageState::embedded || node.state == StorageState::locallyHidden) {
            result.removed.push_back(node.id);
            if (node.state == StorageState::embedded) ++result.removedFromPdf;
            continue;
        }
        BookmarkNode kept = node; // local work on a bookmark the PDF dropped
        kept.baseKey.reset();
        if (kept.state != StorageState::conflict) {
            kept.conflict = ConflictInfo{ConflictReason::sourceRemoved, kept.state, newBase.sourceRevision};
            kept.state = StorageState::conflict;
            ++result.conflicts;
        } else if (kept.conflict) {
            kept.conflict->reason = ConflictReason::sourceRemoved;
        }
        next.emplace(kept.id, std::move(kept));
    }

    // Parents: PDF-placed nodes go under the node for their PDF parent; the
    // rest keep theirs, or the nearest surviving ancestor.
    for (auto& [id, node] : next) {
        if (followsPdf.contains(id)) {
            const BaseNode* source = newByKey.at(*node.baseKey);
            node.parentId = source->parentKey ? std::optional{idForNewKey.at(*source->parentKey)} : std::nullopt;
            continue;
        }
        std::optional<std::string> parent = node.parentId;
        std::size_t guard = 0;
        while (parent && !next.contains(*parent) && guard++ <= OverlayLimits::maxDepth) {
            const auto found = byId.find(*parent);
            parent = found == byId.end() ? std::nullopt : found->second->parentId;
        }
        node.parentId = parent && next.contains(*parent) ? parent : std::nullopt;
    }

    // Sibling order: as before; new PDF bookmarks after their PDF neighbour;
    // PDF-placed nodes in the PDF's order among themselves.
    std::unordered_map<std::string, std::vector<std::string>> groups;
    std::vector<std::string> fresh;
    for (const auto& [id, node] : next) {
        if (oldOrder.contains(id)) groups[node.parentId.value_or("")].push_back(id);
        else fresh.push_back(id);
    }
    for (auto& [_, list] : groups) {
        std::sort(list.begin(), list.end(), [&](const std::string& a, const std::string& b) { return oldOrder.at(a) < oldOrder.at(b); });
    }
    std::sort(fresh.begin(), fresh.end(), [&](const std::string& a, const std::string& b) {
        return newIndex.at(*next.at(a).baseKey) < newIndex.at(*next.at(b).baseKey);
    });
    for (const auto& id : fresh) {
        auto& list = groups[next.at(id).parentId.value_or("")];
        // After the closest earlier PDF bookmark already in this group.
        const std::size_t mine = newIndex.at(*next.at(id).baseKey);
        std::size_t at = 0;
        for (std::size_t i = 0; i < list.size(); ++i) {
            const auto& other = next.at(list[i]);
            if (other.baseKey && newIndex.contains(*other.baseKey) && newIndex.at(*other.baseKey) < mine) at = i + 1;
        }
        list.insert(list.begin() + static_cast<std::ptrdiff_t>(at), id);
    }
    for (auto& [_, list] : groups) {
        std::vector<std::size_t> slots;
        std::vector<std::string> placed;
        for (std::size_t i = 0; i < list.size(); ++i) {
            if (next.at(list[i]).state == StorageState::embedded) {
                slots.push_back(i);
                placed.push_back(list[i]);
            }
        }
        std::sort(placed.begin(), placed.end(), [&](const std::string& a, const std::string& b) {
            return newIndex.at(*next.at(a).baseKey) < newIndex.at(*next.at(b).baseKey);
        });
        for (std::size_t k = 0; k < slots.size(); ++k) list[slots[k]] = placed[k];
        for (std::size_t i = 0; i < list.size(); ++i) {
            auto& node = next.at(list[i]);
            if (node.position != i) node.position = static_cast<std::uint32_t>(i);
        }
    }

    result.nodes.reserve(next.size());
    for (auto& [_, node] : next) result.nodes.push_back(std::move(node));
    std::sort(result.nodes.begin(), result.nodes.end(), [](const BookmarkNode& a, const BookmarkNode& b) {
        return a.parentId.value_or("") != b.parentId.value_or("") ? a.parentId.value_or("") < b.parentId.value_or("")
                                                                  : a.position < b.position;
    });
    if (auto problem = validateTree(result.nodes); !problem.empty()) result.refusal = "cannotMerge:" + problem;
    else if (auto mismatch = validateAgainstBase(result.nodes, newBase); !mismatch.empty())
        result.refusal = "cannotMerge:" + mismatch;
    return result;
}

} // namespace atlas::bookmarks
