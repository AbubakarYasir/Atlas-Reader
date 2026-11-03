#include "core/bookmarks/BookmarkEditor.h"

#include "core/bookmarks/LocationEnvelope.h"

#include <algorithm>
#include <unordered_set>

namespace atlas::bookmarks {
namespace {

using S = StorageState;

[[nodiscard]] bool sameNode(const BookmarkNode& a, const BookmarkNode& b) {
    const auto sameConflict = [](const std::optional<ConflictInfo>& x, const std::optional<ConflictInfo>& y) {
        if (x.has_value() != y.has_value()) return false;
        return !x || (x->reason == y->reason && x->previous == y->previous &&
                      x->observedSourceRevision == y->observedSourceRevision);
    };
    return a.id == b.id && a.documentId == b.documentId && a.parentId == b.parentId && a.position == b.position &&
           a.title == b.title && a.location == b.location && a.description == b.description && a.tags == b.tags &&
           a.state == b.state && a.baseKey == b.baseKey && sameConflict(a.conflict, b.conflict);
}

[[nodiscard]] bool sameOptional(const std::optional<BookmarkNode>& a, const std::optional<BookmarkNode>& b) {
    if (a.has_value() != b.has_value()) return false;
    return !a || sameNode(*a, *b);
}

[[nodiscard]] bool frozen(const BookmarkNode& node) {
    return node.state == S::locallyHidden || node.state == S::conflict;
}

[[nodiscard]] std::string frozenReason(const BookmarkNode& node) {
    return node.state == S::conflict ? "resolveFirst" : "unhideFirst";
}

} // namespace

std::string_view editKindName(EditKind kind) {
    switch (kind) {
    case EditKind::create: return "create";
    case EditKind::rename: return "rename";
    case EditKind::setLocation: return "setLocation";
    case EditKind::setNotes: return "setNotes";
    case EditKind::move: return "move";
    case EditKind::remove: return "remove";
    case EditKind::hide: return "hide";
    case EditKind::unhide: return "unhide";
    case EditKind::requestEmbed: return "requestEmbed";
    case EditKind::cancelEmbed: return "cancelEmbed";
    case EditKind::autoCreate: return "autoCreate";
    case EditKind::resolve: return "resolve";
    }
    return "unknown";
}

BookmarkEditor::BookmarkEditor(std::string documentId, std::vector<BookmarkNode> nodes,
                               std::optional<BaseSnapshot> base, std::int64_t revision)
    : documentId_(std::move(documentId)) {
    reset(std::move(nodes), std::move(base), revision);
}

void BookmarkEditor::reset(std::vector<BookmarkNode> nodes, std::optional<BaseSnapshot> base, std::int64_t revision) {
    revision_ = revision;
    base_ = std::move(base);
    baseByKey_.clear();
    if (base_) {
        baseByKey_.reserve(base_->nodes.size());
        for (const auto& node : base_->nodes) baseByKey_.emplace(node.key, &node);
    }
    nodes_.clear();
    nodes_.reserve(nodes.size());
    for (auto& node : nodes) {
        auto id = node.id;
        nodes_.emplace(std::move(id), std::move(node));
    }
    undo_.clear();
    redo_.clear();
    pending_.reset();
    rebuildIndex();
}

std::string BookmarkEditor::groupKey(const std::optional<std::string>& parent) {
    return parent.value_or(std::string{});
}

void BookmarkEditor::rebuildIndex() {
    children_.clear();
    children_[std::string{}];
    for (const auto& [id, node] : nodes_) children_[groupKey(node.parentId)].push_back(id);
    for (auto& [group, ids] : children_) {
        std::sort(ids.begin(), ids.end(), [this](const std::string& a, const std::string& b) {
            return nodes_.at(a).position < nodes_.at(b).position;
        });
    }
}

const BookmarkNode* BookmarkEditor::find(std::string_view id) const {
    const auto found = nodes_.find(std::string{id});
    return found == nodes_.end() ? nullptr : &found->second;
}

const std::vector<std::string>& BookmarkEditor::children(const std::optional<std::string>& parent) const {
    static const std::vector<std::string> none;
    const auto found = children_.find(groupKey(parent));
    return found == children_.end() ? none : found->second;
}

std::vector<const BookmarkNode*> BookmarkEditor::inOrder() const {
    std::vector<const BookmarkNode*> out;
    out.reserve(nodes_.size());
    std::vector<std::pair<const std::vector<std::string>*, std::size_t>> stack{{&children(std::nullopt), 0}};
    while (!stack.empty()) {
        auto& [list, next] = stack.back();
        if (next >= list->size()) {
            stack.pop_back();
            continue;
        }
        const auto& id = (*list)[next++];
        const auto& node = nodes_.at(id);
        out.push_back(&node);
        const auto found = children_.find(id);
        if (found != children_.end() && !found->second.empty()) stack.emplace_back(&found->second, 0);
    }
    return out;
}

std::vector<std::string> BookmarkEditor::breadcrumb(std::string_view id) const {
    std::vector<std::string> titles;
    const BookmarkNode* node = find(id);
    while (node != nullptr && titles.size() <= OverlayLimits::maxDepth) {
        titles.push_back(node->title);
        node = node->parentId ? find(*node->parentId) : nullptr;
    }
    std::reverse(titles.begin(), titles.end());
    return titles;
}

bool BookmarkEditor::matchesBase(const BookmarkNode& node) const {
    if (!node.baseKey) return false;
    const auto found = baseByKey_.find(*node.baseKey);
    if (found == baseByKey_.end()) return false;
    const BaseNode& source = *found->second;
    if (node.title != source.title || node.location != source.location) return false;
    std::optional<std::string> parentBaseKey;
    if (node.parentId) {
        const auto* parent = find(*node.parentId);
        if (parent == nullptr) return false;
        parentBaseKey = parent->baseKey;
    }
    if (parentBaseKey != source.parentKey) return false;
    // Relative order among the Embedded siblings must match the PDF.
    const auto& siblings = children(node.parentId);
    std::optional<std::uint32_t> before;
    std::optional<std::uint32_t> after;
    bool passed = false;
    for (const auto& id : siblings) {
        if (id == node.id) {
            passed = true;
            continue;
        }
        const auto& sibling = nodes_.at(id);
        if (sibling.state != S::embedded || !sibling.baseKey) continue;
        const auto base = baseByKey_.find(*sibling.baseKey);
        if (base == baseByKey_.end()) continue;
        if (!passed) before = base->second->position;
        else if (!after) after = base->second->position;
    }
    return (!before || *before < source.position) && (!after || source.position < *after);
}

void BookmarkEditor::settleSourceState(BookmarkNode& node) const {
    if (node.state != S::embedded && node.state != S::localOverride) return;
    node.state = matchesBase(node) ? S::embedded : S::localOverride;
}

bool BookmarkEditor::isAncestorOrSelf(std::string_view ancestor, std::string_view id) const {
    const BookmarkNode* node = find(id);
    std::size_t steps = 0;
    while (node != nullptr && steps++ <= OverlayLimits::maxDepth + 1) {
        if (node->id == ancestor) return true;
        node = node->parentId ? find(*node->parentId) : nullptr;
    }
    return false;
}

void BookmarkEditor::remember(std::unordered_map<std::string, std::optional<BookmarkNode>>& touched,
                              const std::string& id) const {
    if (touched.contains(id)) return;
    const auto found = nodes_.find(id);
    touched.emplace(id, found == nodes_.end() ? std::nullopt : std::optional<BookmarkNode>{found->second});
}

void BookmarkEditor::renumber(const std::string& group,
                              std::unordered_map<std::string, std::optional<BookmarkNode>>& touched) {
    const auto found = children_.find(group);
    if (found == children_.end()) return;
    for (std::size_t i = 0; i < found->second.size(); ++i) {
        auto& node = nodes_.at(found->second[i]);
        if (node.position != i) {
            remember(touched, node.id);
            node.position = static_cast<std::uint32_t>(i);
        }
    }
}

EditResult BookmarkEditor::refuse(std::string reason) const {
    EditResult result;
    result.reason = std::move(reason);
    return result;
}

EditResult BookmarkEditor::finish(EditKind kind, std::unordered_map<std::string, std::optional<BookmarkNode>>& before,
                                  std::string nodeId) {
    Entry entry;
    entry.kind = kind;
    EditResult result;
    result.ok = true;
    result.nodeId = std::move(nodeId);
    for (auto& [id, previous] : before) {
        const auto current = nodes_.find(id);
        std::optional<BookmarkNode> after;
        if (current != nodes_.end()) after = current->second;
        if (sameOptional(previous, after)) continue;
        if (after) result.change.upserts.push_back(*after);
        else result.change.removals.push_back(id);
        entry.nodes.emplace_back(id, Snapshot{std::move(previous), std::move(after)});
    }
    // Stable order makes the change and the history deterministic.
    std::sort(result.change.upserts.begin(), result.change.upserts.end(),
              [](const BookmarkNode& a, const BookmarkNode& b) { return a.id < b.id; });
    std::sort(result.change.removals.begin(), result.change.removals.end());
    if (!entry.nodes.empty()) {
        pending_ = std::move(entry);
        pendingSource_ = PendingSource::command;
    }
    return result;
}

EditResult BookmarkEditor::create(const std::optional<std::string>& parent, std::size_t index, std::string title,
                                  std::string location, bool requestEmbed) {
    if (pending_) return refuse("busy");
    if (parent) {
        const auto* owner = find(*parent);
        if (owner == nullptr) return refuse("unknownNode");
        if (frozen(*owner)) return refuse(frozenReason(*owner));
        if (breadcrumb(*parent).size() >= OverlayLimits::maxDepth) return refuse("tooDeep");
    }
    if (nodes_.size() >= OverlayLimits::maxNodesPerDocument) return refuse("tooManyNodes");

    BookmarkNode node;
    node.id = newBookmarkId();
    while (nodes_.contains(node.id)) node.id = newBookmarkId();
    node.documentId = documentId_;
    node.parentId = parent;
    node.title = std::move(title);
    node.location = std::move(location);
    node.state = requestEmbed ? S::pendingEmbed : S::localOnly;
    if (auto problem = validateNode(node); !problem.empty()) return refuse(problem);

    std::unordered_map<std::string, std::optional<BookmarkNode>> touched;
    remember(touched, node.id); // absent before
    const auto group = groupKey(parent);
    auto& list = children_[group];
    const auto at = std::min(index, list.size());
    const auto id = node.id;
    nodes_.emplace(id, std::move(node));
    list.insert(list.begin() + static_cast<std::ptrdiff_t>(at), id);
    nodes_.at(id).position = static_cast<std::uint32_t>(at);
    renumber(group, touched);
    return finish(EditKind::create, touched, id);
}

EditResult BookmarkEditor::rename(std::string_view id, std::string title) {
    if (pending_) return refuse("busy");
    const auto found = nodes_.find(std::string{id});
    if (found == nodes_.end()) return refuse("unknownNode");
    if (frozen(found->second)) return refuse(frozenReason(found->second));
    BookmarkNode next = found->second;
    next.title = std::move(title);
    if (auto problem = validateNode(next); !problem.empty()) return refuse(problem);
    std::unordered_map<std::string, std::optional<BookmarkNode>> touched;
    remember(touched, found->first);
    found->second = std::move(next);
    settleSourceState(found->second);
    return finish(EditKind::rename, touched, found->first);
}

EditResult BookmarkEditor::setLocation(std::string_view id, std::string location) {
    if (pending_) return refuse("busy");
    const auto found = nodes_.find(std::string{id});
    if (found == nodes_.end()) return refuse("unknownNode");
    if (frozen(found->second)) return refuse(frozenReason(found->second));
    BookmarkNode next = found->second;
    next.location = std::move(location);
    if (auto problem = validateNode(next); !problem.empty()) return refuse(problem);
    std::unordered_map<std::string, std::optional<BookmarkNode>> touched;
    remember(touched, found->first);
    found->second = std::move(next);
    settleSourceState(found->second);
    return finish(EditKind::setLocation, touched, found->first);
}

EditResult BookmarkEditor::setNotes(std::string_view id, std::string description, std::vector<std::string> tags) {
    if (pending_) return refuse("busy");
    const auto found = nodes_.find(std::string{id});
    if (found == nodes_.end()) return refuse("unknownNode");
    if (frozen(found->second)) return refuse(frozenReason(found->second));
    BookmarkNode next = found->second;
    next.description = std::move(description);
    next.tags = std::move(tags);
    if (auto problem = validateNode(next); !problem.empty()) return refuse(problem);
    std::unordered_map<std::string, std::optional<BookmarkNode>> touched;
    remember(touched, found->first);
    found->second = std::move(next); // notes are Atlas's own: the state does not change
    return finish(EditKind::setNotes, touched, found->first);
}

EditResult BookmarkEditor::move(std::string_view id, const std::optional<std::string>& parent, std::size_t index) {
    if (pending_) return refuse("busy");
    const auto found = nodes_.find(std::string{id});
    if (found == nodes_.end()) return refuse("unknownNode");
    if (frozen(found->second)) return refuse(frozenReason(found->second));
    if (parent) {
        const auto* owner = find(*parent);
        if (owner == nullptr) return refuse("unknownNode");
        if (frozen(*owner)) return refuse(frozenReason(*owner));
        if (isAncestorOrSelf(found->first, *parent)) return refuse("cycle");
        // Depth of the new parent plus the height of the moved subtree.
        std::size_t height = 0;
        std::vector<std::pair<std::string, std::size_t>> stack{{found->first, 1}};
        while (!stack.empty()) {
            auto [current, depth] = stack.back();
            stack.pop_back();
            height = std::max(height, depth);
            const auto kids = children_.find(current);
            if (kids == children_.end()) continue;
            for (const auto& child : kids->second) stack.emplace_back(child, depth + 1);
        }
        if (breadcrumb(*parent).size() + height > OverlayLimits::maxDepth) return refuse("tooDeep");
    }

    std::unordered_map<std::string, std::optional<BookmarkNode>> touched;
    const std::string nodeId = found->first;
    remember(touched, nodeId);
    const auto from = groupKey(found->second.parentId);
    const auto to = groupKey(parent);
    auto& source = children_[from];
    source.erase(std::find(source.begin(), source.end(), nodeId));
    auto& target = children_[to];
    const auto at = std::min(index, target.size());
    target.insert(target.begin() + static_cast<std::ptrdiff_t>(at), nodeId);
    found->second.parentId = parent;
    renumber(from, touched);
    if (to != from) renumber(to, touched);
    settleSourceState(nodes_.at(nodeId));
    return finish(EditKind::move, touched, nodeId);
}

EditResult BookmarkEditor::remove(std::string_view id) {
    if (pending_) return refuse("busy");
    const auto found = nodes_.find(std::string{id});
    if (found == nodes_.end()) return refuse("unknownNode");
    if (isSource(found->second)) return setHidden(id, true); // the PDF keeps it; Atlas hides it
    if (frozen(found->second)) return refuse(frozenReason(found->second));

    std::vector<std::string> subtree{found->first};
    for (std::size_t i = 0; i < subtree.size(); ++i) {
        const auto kids = children_.find(subtree[i]);
        if (kids == children_.end()) continue;
        for (const auto& child : kids->second) {
            if (isSource(nodes_.at(child))) return refuse("containsSourceNodes");
            subtree.push_back(child);
        }
    }
    std::unordered_map<std::string, std::optional<BookmarkNode>> touched;
    for (const auto& member : subtree) remember(touched, member);
    const std::string nodeId = found->first;
    const auto group = groupKey(found->second.parentId);
    auto& list = children_[group];
    list.erase(std::find(list.begin(), list.end(), nodeId));
    for (const auto& member : subtree) {
        children_.erase(member);
        nodes_.erase(member);
    }
    renumber(group, touched);
    return finish(EditKind::remove, touched, nodeId);
}

EditResult BookmarkEditor::setHidden(std::string_view id, bool hidden) {
    if (pending_) return refuse("busy");
    const auto found = nodes_.find(std::string{id});
    if (found == nodes_.end()) return refuse("unknownNode");
    auto& node = found->second;
    if (!isSource(node)) return refuse("notFromPdf");
    if (node.state == S::conflict) return refuse("resolveFirst");
    if (hidden == (node.state == S::locallyHidden)) return refuse("unchanged");
    std::unordered_map<std::string, std::optional<BookmarkNode>> touched;
    remember(touched, found->first);
    if (hidden) {
        node.state = S::locallyHidden;
    } else {
        node.state = S::localOverride;
        settleSourceState(node);
    }
    return finish(hidden ? EditKind::hide : EditKind::unhide, touched, found->first);
}

EditResult BookmarkEditor::setEmbedRequested(std::string_view id, bool requested) {
    if (pending_) return refuse("busy");
    const auto found = nodes_.find(std::string{id});
    if (found == nodes_.end()) return refuse("unknownNode");
    auto& node = found->second;
    if (node.state != S::localOnly && node.state != S::pendingEmbed) return refuse("notLocal");
    const auto wanted = requested ? S::pendingEmbed : S::localOnly;
    if (node.state == wanted) return refuse("unchanged");
    std::unordered_map<std::string, std::optional<BookmarkNode>> touched;
    remember(touched, found->first);
    node.state = wanted;
    return finish(requested ? EditKind::requestEmbed : EditKind::cancelEmbed, touched, found->first);
}

EditResult BookmarkEditor::resolveConflict(std::string_view id, bool keepMine) {
    if (pending_) return refuse("busy");
    const auto found = nodes_.find(std::string{id});
    if (found == nodes_.end()) return refuse("unknownNode");
    if (found->second.state != S::conflict) return refuse("unchanged");
    std::unordered_map<std::string, std::optional<BookmarkNode>> touched;
    remember(touched, found->first);
    BookmarkNode& node = found->second;
    const BaseNode* source = nullptr;
    if (node.baseKey) {
        if (const auto b = baseByKey_.find(*node.baseKey); b != baseByKey_.end()) source = b->second;
    }
    if (keepMine || source == nullptr) {
        if (!keepMine) return refuse("notInPdf"); // nothing in the PDF to take
        node.conflict.reset();
        if (source != nullptr) {
            node.state = S::localOverride;
            settleSourceState(node); // equal to the PDF after all → Embedded
        } else {
            node.state = S::localOnly;
            node.baseKey.reset();
        }
        return finish(EditKind::resolve, touched, found->first);
    }
    // The PDF's version: its title and destination, under its PDF parent,
    // after the PDF sibling that precedes it.
    node.title = source->title;
    node.location = source->location;
    node.conflict.reset();
    std::optional<std::string> parent;
    if (source->parentKey) {
        for (const auto& [otherId, other] : nodes_) {
            if (other.baseKey && *other.baseKey == *source->parentKey) parent = otherId;
        }
        if (!parent) parent = node.parentId; // its PDF parent is not shown: stay put
    }
    const std::string nodeId = found->first;
    const auto from = groupKey(node.parentId);
    auto& oldList = children_[from];
    oldList.erase(std::find(oldList.begin(), oldList.end(), nodeId));
    node.parentId = parent;
    const auto to = groupKey(parent);
    auto& list = children_[to];
    std::size_t at = 0;
    for (std::size_t i = 0; i < list.size(); ++i) {
        const auto& sibling = nodes_.at(list[i]);
        if (sibling.baseKey && baseByKey_.contains(*sibling.baseKey) &&
            baseByKey_.at(*sibling.baseKey)->parentKey == source->parentKey &&
            baseByKey_.at(*sibling.baseKey)->position < source->position)
            at = i + 1;
    }
    list.insert(list.begin() + static_cast<std::ptrdiff_t>(at), nodeId);
    renumber(from, touched);
    if (to != from) renumber(to, touched);
    nodes_.at(nodeId).state = S::localOverride;
    settleSourceState(nodes_.at(nodeId)); // Embedded when it now equals the PDF's bookmark
    return finish(EditKind::resolve, touched, nodeId);
}

EditResult BookmarkEditor::createBatch(std::vector<NewBookmark> items, BatchPlacement placement) {
    if (pending_) return refuse("busy");
    if (items.empty()) return refuse("nothingToAdd");
    if (nodes_.size() + items.size() > OverlayLimits::maxNodesPerDocument) return refuse("tooManyNodes");

    std::unordered_map<std::string, std::optional<BookmarkNode>> touched;
    const std::string top = groupKey(std::nullopt);

    if (placement == BatchPlacement::replace) {
        // Hide every PDF bookmark; remove Atlas's own unless they hold PDF ones.
        std::vector<std::string> localTops;
        for (const auto* node : inOrder()) {
            if (isSource(*node)) {
                if (node->state != S::locallyHidden && node->state != S::conflict) {
                    remember(touched, node->id);
                    nodes_.at(node->id).state = S::locallyHidden;
                }
            }
        }
        for (const auto& id : children(std::nullopt)) {
            if (!isSource(nodes_.at(id))) localTops.push_back(id);
        }
        for (const auto& id : localTops) {
            std::vector<std::string> subtree{id};
            bool holdsSource = false;
            for (std::size_t i = 0; i < subtree.size() && !holdsSource; ++i) {
                const auto kids = children_.find(subtree[i]);
                if (kids == children_.end()) continue;
                for (const auto& child : kids->second) {
                    if (isSource(nodes_.at(child))) holdsSource = true;
                    subtree.push_back(child);
                }
            }
            if (holdsSource || frozen(nodes_.at(id))) continue;
            for (const auto& member : subtree) remember(touched, member);
            auto& list = children_[top];
            list.erase(std::find(list.begin(), list.end(), id));
            for (const auto& member : subtree) {
                children_.erase(member);
                nodes_.erase(member);
            }
        }
    }

    // Build the new nodes, then insert the top-level ones in one place.
    // Open headings by their requested level; a heading nests under the
    // nearest open one with a smaller level, so skipped levels close up and
    // headings of one level stay siblings ("1", "1.1.1", "1.1.2" → both under "1").
    std::vector<std::pair<std::uint32_t, std::string>> open; // (requested level, node id)
    std::vector<std::string> parents;                        // parents[depth] = open node at that depth
    std::vector<std::string> newTops;
    std::unordered_set<std::string> groups{top};
    for (auto& item : items) {
        while (!open.empty() && open.back().first >= item.level) open.pop_back();
        if (open.size() >= OverlayLimits::maxDepth) open.resize(OverlayLimits::maxDepth - 1);
        const auto level = static_cast<std::uint32_t>(open.size());
        BookmarkNode node;
        node.id = newBookmarkId();
        while (nodes_.contains(node.id) || touched.contains(node.id)) node.id = newBookmarkId();
        node.documentId = documentId_;
        if (level > 0) node.parentId = parents[level - 1];
        node.title = std::move(item.title);
        node.location = std::move(item.location);
        node.state = S::localOnly;
        if (auto problem = validateNode(node); !problem.empty()) {
            // Undo what this command did so far, then refuse.
            for (auto& [id, previous] : touched) {
                if (previous) nodes_.insert_or_assign(id, *previous);
                else nodes_.erase(id);
            }
            rebuildIndex();
            return refuse(problem);
        }
        remember(touched, node.id); // absent before
        parents.resize(level + 1);
        parents[level] = node.id;
        open.emplace_back(item.level, node.id);
        const auto group = groupKey(node.parentId);
        const auto id = node.id;
        nodes_.emplace(id, std::move(node));
        if (level == 0) newTops.push_back(id);
        else {
            children_[group].push_back(id);
            groups.insert(group);
        }
    }
    auto& list = children_[top];
    const auto at = placement == BatchPlacement::before ? list.begin() : list.end();
    list.insert(at, newTops.begin(), newTops.end());
    for (const auto& group : groups) renumber(group, touched);
    for (const auto& [id, _] : touched) {
        auto found = nodes_.find(id);
        if (found != nodes_.end()) settleSourceState(found->second);
    }
    return finish(EditKind::autoCreate, touched, newTops.empty() ? std::string{} : newTops.front());
}

std::optional<EditKind> BookmarkEditor::undoKind() const {
    if (undo_.empty()) return std::nullopt;
    return undo_.back().kind;
}

std::optional<EditKind> BookmarkEditor::redoKind() const {
    if (redo_.empty()) return std::nullopt;
    return redo_.back().kind;
}

EditResult BookmarkEditor::replay(const Entry& entry, bool forward, PendingSource source) {
    EditResult result;
    // History is only valid if nothing else changed these nodes since.
    for (const auto& [id, snapshot] : entry.nodes) {
        const auto& expected = forward ? snapshot.before : snapshot.after;
        const auto current = nodes_.find(id);
        std::optional<BookmarkNode> now;
        if (current != nodes_.end()) now = current->second;
        if (!sameOptional(expected, now)) {
            undo_.clear();
            redo_.clear();
            return refuse("historyStale");
        }
    }
    // Update only the sibling lists the entry touches (no full rebuild).
    std::unordered_set<std::string> groups;
    for (const auto& [id, snapshot] : entry.nodes) {
        const auto& target = forward ? snapshot.after : snapshot.before;
        const auto current = nodes_.find(id);
        if (current != nodes_.end()) {
            const auto group = groupKey(current->second.parentId);
            auto& list = children_[group];
            list.erase(std::remove(list.begin(), list.end(), id), list.end());
            groups.insert(group);
        }
        if (target) {
            const bool existed = current != nodes_.end();
            nodes_.insert_or_assign(id, *target);
            result.change.upserts.push_back(*target);
            if (!existed) result.change.restores.push_back(id); // brought back from its tombstone
        } else {
            nodes_.erase(id);
            children_.erase(id);
            result.change.removals.push_back(id);
        }
    }
    for (const auto& [id, snapshot] : entry.nodes) {
        const auto& target = forward ? snapshot.after : snapshot.before;
        if (!target) continue;
        const auto group = groupKey(target->parentId);
        children_[group].push_back(id);
        groups.insert(group);
    }
    for (const auto& group : groups) {
        auto& list = children_[group];
        std::sort(list.begin(), list.end(), [this](const std::string& a, const std::string& b) {
            return nodes_.at(a).position < nodes_.at(b).position;
        });
    }
    result.ok = true;
    result.nodeId = entry.nodes.empty() ? std::string{} : entry.nodes.front().first;
    pending_ = entry;
    pendingSource_ = source;
    return result;
}

EditResult BookmarkEditor::undo() {
    if (pending_) return refuse("busy");
    if (undo_.empty()) return refuse("nothingToUndo");
    const Entry entry = undo_.back();
    return replay(entry, false, PendingSource::undo);
}

EditResult BookmarkEditor::redo() {
    if (pending_) return refuse("busy");
    if (redo_.empty()) return refuse("nothingToRedo");
    const Entry entry = redo_.back();
    return replay(entry, true, PendingSource::redo);
}

void BookmarkEditor::confirm(std::int64_t newRevision) {
    revision_ = newRevision;
    if (!pending_) return;
    switch (pendingSource_) {
    case PendingSource::command:
        undo_.push_back(std::move(*pending_));
        if (undo_.size() > maxHistory) undo_.erase(undo_.begin());
        redo_.clear();
        break;
    case PendingSource::undo:
        undo_.pop_back();
        redo_.push_back(std::move(*pending_));
        break;
    case PendingSource::redo:
        redo_.pop_back();
        undo_.push_back(std::move(*pending_));
        break;
    }
    pending_.reset();
}

void BookmarkEditor::discardPending() {
    pending_.reset();
}

} // namespace atlas::bookmarks
