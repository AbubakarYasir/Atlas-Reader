#include "core/reader/ViewportLayout.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace atlas::reader {

SizeF rotatedSizePoints(const PageGeometry& page) noexcept {
    const bool quarterTurn =
        page.rotation == PageRotation::r90 || page.rotation == PageRotation::r270;
    return quarterTurn ? SizeF{page.heightPoints, page.widthPoints}
                       : SizeF{page.widthPoints, page.heightPoints};
}

double clampZoom(double zoom) noexcept {
    if (!std::isfinite(zoom)) {
        return 1.0;
    }
    return std::clamp(zoom, kMinZoom, kMaxZoom);
}

ViewportLayout::ViewportLayout(std::vector<PageGeometry> pages, LayoutSettings settings)
    : pages_(std::move(pages)), settings_(settings) {
    rebuild();
}

void ViewportLayout::setSettings(LayoutSettings settings) {
    settings_ = settings;
    rebuild();
}

const PageGeometry& ViewportLayout::page(std::size_t index) const {
    if (index >= pages_.size()) {
        throw std::out_of_range("page index out of range");
    }
    return pages_[index];
}

PageRotation ViewportLayout::effectiveRotation(std::size_t index) const {
    return combineRotation(page(index).rotation, settings_.viewRotation);
}

void ViewportLayout::rebuild() {
    settings_.zoom = clampZoom(settings_.zoom);
    settings_.pageGap = std::max(0.0, settings_.pageGap);
    settings_.margin = std::max(0.0, settings_.margin);

    const double scale = settings_.zoom * kPointsToLogicalPixels;
    tops_.assign(pages_.size(), 0.0);
    heights_.assign(pages_.size(), 0.0);
    widths_.assign(pages_.size(), 0.0);
    widestPage_ = 0.0;

    double y = settings_.margin;
    for (std::size_t i = 0; i < pages_.size(); ++i) {
        PageGeometry rotated = pages_[i];
        rotated.rotation = combineRotation(pages_[i].rotation, settings_.viewRotation);
        const SizeF size = rotatedSizePoints(rotated);
        // Degenerate pages still occupy a sliver so navigation stays monotonic.
        widths_[i] = std::max(1.0, size.width * scale);
        heights_[i] = std::max(1.0, size.height * scale);
        tops_[i] = y;
        y += heights_[i];
        if (i + 1 < pages_.size()) {
            y += settings_.pageGap;
        }
        widestPage_ = std::max(widestPage_, widths_[i]);
    }

    if (pages_.empty()) {
        content_ = {};
        return;
    }
    content_ = SizeF{widestPage_ + 2.0 * settings_.margin, y + settings_.margin};
}

RectF ViewportLayout::pageRect(std::size_t index) const {
    if (index >= pages_.size()) {
        throw std::out_of_range("page index out of range");
    }
    const double x = settings_.margin + (widestPage_ - widths_[index]) / 2.0;
    return RectF{x, tops_[index], widths_[index], heights_[index]};
}

std::optional<std::size_t> ViewportLayout::pageAt(double y) const {
    if (pages_.empty()) {
        return std::nullopt;
    }
    // Last page whose top is <= y; pages before the first top map to page 0.
    const auto it = std::upper_bound(tops_.begin(), tops_.end(), y);
    if (it == tops_.begin()) {
        return 0;
    }
    return static_cast<std::size_t>(std::distance(tops_.begin(), it) - 1);
}

std::optional<PageRange> ViewportLayout::pagesInBand(double top, double height) const {
    if (pages_.empty() || !(height > 0.0) || !std::isfinite(top)) {
        return std::nullopt;
    }
    const double bottom = top + height;
    if (bottom <= tops_.front() || top >= tops_.back() + heights_.back()) {
        return std::nullopt;
    }

    // First page whose bottom edge is below `top`.
    std::size_t first = *pageAt(top);
    if (tops_[first] + heights_[first] <= top && first + 1 < pages_.size()) {
        ++first; // `top` falls in the gap after `first`
    }
    // Last page whose top edge is above `bottom`.
    const auto it = std::lower_bound(tops_.begin(), tops_.end(), bottom);
    std::size_t last = it == tops_.begin()
                           ? 0
                           : static_cast<std::size_t>(std::distance(tops_.begin(), it) - 1);
    if (last < first) {
        return std::nullopt; // band lies entirely inside one gap
    }
    return PageRange{first, last};
}

double ViewportLayout::scrollOffsetForPage(std::size_t index) const {
    if (index >= pages_.size()) {
        throw std::out_of_range("page index out of range");
    }
    return std::max(0.0, tops_[index] - settings_.pageGap / 2.0);
}

double ViewportLayout::fitWidthZoom(double viewportWidth) const {
    if (pages_.empty() || !(viewportWidth > 0.0)) {
        return settings_.zoom;
    }
    double widestPoints = 0.0;
    for (const auto& page : pages_) {
        PageGeometry rotated = page;
        rotated.rotation = combineRotation(page.rotation, settings_.viewRotation);
        widestPoints = std::max(widestPoints, rotatedSizePoints(rotated).width);
    }
    const double available = viewportWidth - 2.0 * settings_.margin;
    if (widestPoints <= 0.0 || available <= 0.0) {
        return settings_.zoom;
    }
    return clampZoom(available / (widestPoints * kPointsToLogicalPixels));
}

double ViewportLayout::fitPageZoom(SizeF viewport, std::size_t referencePage) const {
    if (pages_.empty() || !(viewport.width > 0.0) || !(viewport.height > 0.0)) {
        return settings_.zoom;
    }
    PageGeometry rotated = page(std::min(referencePage, pages_.size() - 1));
    rotated.rotation = combineRotation(rotated.rotation, settings_.viewRotation);
    const SizeF size = rotatedSizePoints(rotated);
    const double availableWidth = viewport.width - 2.0 * settings_.margin;
    const double availableHeight = viewport.height - 2.0 * settings_.margin;
    if (size.width <= 0.0 || size.height <= 0.0 || availableWidth <= 0.0 ||
        availableHeight <= 0.0) {
        return settings_.zoom;
    }
    const double byWidth = availableWidth / (size.width * kPointsToLogicalPixels);
    const double byHeight = availableHeight / (size.height * kPointsToLogicalPixels);
    return clampZoom(std::min(byWidth, byHeight));
}

} // namespace atlas::reader
