#include "core/bookmarks/OutlineSavePlan.h"

#include "core/bookmarks/LocationEnvelope.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace atlas::bookmarks {

SavePlan planOutlineSave(const std::vector<const BookmarkNode*>& inOrder, const std::optional<BaseSnapshot>& base,
                         const SaveOptions& options) {
    SavePlan plan;
    std::unordered_map<std::string, const BaseNode*> baseByKey;
    if (base) {
        for (const auto& node : base->nodes) baseByKey.emplace(node.key, &node);
    }
    // Where each overlay node's children go: its own item, or (when it is left
    // out) the item its own children would have gone to.
    std::unordered_map<std::string, int> slotOf;
    std::unordered_set<std::string> seenBase;

    for (const BookmarkNode* node : inOrder) {
        const int parentSlot = node->parentId ? slotOf.at(*node->parentId) : -1;
        if (node->state == StorageState::conflict) {
            plan.refusal = "resolveFirst";
            return plan;
        }
        const BaseNode* source = nullptr;
        if (node->baseKey) {
            const auto found = baseByKey.find(*node->baseKey);
            if (found == baseByKey.end()) {
                plan.refusal = "noBase";
                return plan;
            }
            source = found->second;
            seenBase.insert(*node->baseKey);
        }
        if (node->state == StorageState::locallyHidden && options.removeHidden) {
            ++plan.removed;
            slotOf[node->id] = parentSlot; // children move up
            continue;
        }

        PlannedItem item;
        item.nodeId = node->id;
        item.baseKey = node->baseKey;
        item.title = node->title;
        item.location = node->location;
        item.parent = parentSlot;
        item.hidden = node->state == StorageState::locallyHidden;
        if (source != nullptr && item.hidden) {
            // A hidden item is written as the PDF has it (Atlas may not touch it).
            item.title = source->title;
            item.location = source->location;
        }
        item.titleChanged = source == nullptr || item.title != source->title;
        item.locationChanged = source == nullptr || item.location != source->location;
        if (item.locationChanged && !item.location.empty()) {
            const auto decoded = decodeLocation(item.location);
            if (decoded.status != LocationDecodeStatus::ok || decoded.envelope->scheme != LocationScheme::pdf) {
                plan.refusal = "unreadableLocation";
                return plan;
            }
        }
        slotOf[node->id] = static_cast<int>(plan.items.size());
        plan.items.push_back(std::move(item));
    }
    // A PDF item missing from the overlay would vanish from the PDF; Atlas
    // never deletes the PDF's bookmarks by accident.
    if (base) {
        for (const auto& node : base->nodes) {
            if (!seenBase.contains(node.key)) {
                plan.refusal = "missingBaseItems";
                return plan;
            }
        }
    }
    // Counts for the preview. A PDF item is "changed" when its title,
    // destination, parent or order among the PDF's items differs.
    std::unordered_map<std::string, std::uint32_t> lastPosition; // parent base key → last seen position
    for (const auto& item : plan.items) {
        if (!item.baseKey) {
            ++plan.added;
            continue;
        }
        const BaseNode* source = baseByKey.at(*item.baseKey);
        // The parent as a group name: a PDF item ("k…"), a new item ("n…") or the top.
        std::string group{"top"};
        if (item.parent >= 0) {
            const auto& parent = plan.items[static_cast<std::size_t>(item.parent)];
            group = parent.baseKey ? "k" + *parent.baseKey : "n" + std::to_string(item.parent);
        }
        const std::string sourceGroup = source->parentKey ? "k" + *source->parentKey : std::string{"top"};
        const auto previous = lastPosition.find(group);
        const bool reordered = previous != lastPosition.end() && source->position < previous->second;
        lastPosition[group] = source->position;
        const bool moved = group != sourceGroup || reordered;
        if (item.titleChanged || item.locationChanged || moved) ++plan.changed;
        else ++plan.kept;
    }
    return plan;
}

PdfPoint pdfPointFromFractions(const PageGeometry& page, double left, double top) {
    const double width = page.right - page.left;
    const double height = page.top - page.bottom;
    const int rotate = ((page.rotate % 360) + 360) % 360;
    switch (rotate) {
    case 90: return {page.left + top * width, page.bottom + left * height};
    case 180: return {page.right - left * width, page.bottom + top * height};
    case 270: return {page.right - top * width, page.top - left * height};
    default: return {page.left + left * width, page.top - top * height};
    }
}

std::pair<double, double> fractionsFromPdfPoint(const PageGeometry& page, PdfPoint point) {
    const double width = page.right - page.left;
    const double height = page.top - page.bottom;
    if (!(width > 0) || !(height > 0)) return {0.0, 0.0};
    const double fx = (point.x - page.left) / width;   // 0 at the left edge in user space
    const double fy = (page.top - point.y) / height;   // 0 at the top edge in user space
    const int rotate = ((page.rotate % 360) + 360) % 360;
    switch (rotate) {
    case 90: return {1.0 - fy, fx};
    case 180: return {1.0 - fx, 1.0 - fy};
    case 270: return {fy, 1.0 - fx};
    default: return {fx, fy};
    }
}

} // namespace atlas::bookmarks
