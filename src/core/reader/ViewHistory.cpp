#include "core/reader/ViewHistory.h"

namespace atlas::reader {

namespace {
// Two positions on the same page within 2% of a page are the same place.
bool samePlace(const ViewPosition& a, const ViewPosition& b) {
    const double delta = a.fraction - b.fraction;
    return a.page == b.page && delta < 0.02 && delta > -0.02;
}
} // namespace

void ViewHistory::recordJump(const ViewPosition& from) {
    if (back_.empty() || !samePlace(back_.back(), from)) {
        back_.push_back(from);
        if (back_.size() > kMaxEntries) back_.erase(back_.begin());
    }
    forward_.clear();
}

std::optional<ViewPosition> ViewHistory::back(const ViewPosition& current) {
    while (!back_.empty()) {
        const ViewPosition target = back_.back();
        back_.pop_back();
        if (samePlace(target, current)) continue;
        forward_.push_back(current);
        return target;
    }
    return std::nullopt;
}

std::optional<ViewPosition> ViewHistory::forward(const ViewPosition& current) {
    while (!forward_.empty()) {
        const ViewPosition target = forward_.back();
        forward_.pop_back();
        if (samePlace(target, current)) continue;
        back_.push_back(current);
        return target;
    }
    return std::nullopt;
}

void ViewHistory::clear() noexcept {
    back_.clear();
    forward_.clear();
}

} // namespace atlas::reader
