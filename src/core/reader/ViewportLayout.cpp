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
    const std::size_t count = pages_.size();
    tops_.assign(count, 0.0);
    heights_.assign(count, 0.0);
    widths_.assign(count, 0.0);
    lefts_.assign(count, 0.0);
    rowOfPage_.assign(count, 0);
    rows_.clear();
    widestPage_ = 0.0;

    for (std::size_t i = 0; i < count; ++i) {
        PageGeometry rotated = pages_[i];
        rotated.rotation = combineRotation(pages_[i].rotation, settings_.viewRotation);
        const SizeF size = rotatedSizePoints(rotated);
        // Degenerate pages still occupy a sliver so navigation stays monotonic.
        widths_[i] = std::max(1.0, size.width * scale);
        heights_[i] = std::max(1.0, size.height * scale);
    }

    // Group pages into rows.
    const bool facing = isFacing(settings_.display);
    std::vector<double> rowWidths;
    for (std::size_t i = 0; i < count;) {
        std::size_t last = i;
        const bool alone = !facing || (settings_.coverPage && i == 0);
        if (!alone && i + 1 < count) last = i + 1;
        PageRow row{i, last, 0.0, 0.0};
        double width = 0.0;
        for (std::size_t p = i; p <= last; ++p) {
            row.height = std::max(row.height, heights_[p]);
            width += widths_[p];
            rowOfPage_[p] = rows_.size();
        }
        width += static_cast<double>(last - i) * settings_.pageGap;
        rows_.push_back(row);
        rowWidths.push_back(width);
        widestPage_ = std::max(widestPage_, width);
        i = last + 1;
    }

    double y = settings_.margin;
    for (std::size_t r = 0; r < rows_.size(); ++r) {
        auto& row = rows_[r];
        row.top = y;
        double x = settings_.margin + (widestPage_ - rowWidths[r]) / 2.0;
        // A spread with a single trailing page keeps that page on its reading side.
        const bool lonelySpreadPage = facing && row.first == row.last && !(settings_.coverPage && row.first == 0);
        if (lonelySpreadPage && widestPage_ > rowWidths[r] + 1.0) {
            const double shift = (widestPage_ - rowWidths[r]) / 2.0;
            x += settings_.rightToLeft ? shift : -shift;
        }
        for (std::size_t k = 0; k <= row.last - row.first; ++k) {
            const std::size_t p = settings_.rightToLeft ? row.last - k : row.first + k;
            lefts_[p] = x;
            tops_[p] = y + (row.height - heights_[p]) / 2.0;
            x += widths_[p] + settings_.pageGap;
        }
        y += row.height;
        if (r + 1 < rows_.size()) y += settings_.pageGap;
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
    return RectF{lefts_[index], tops_[index], widths_[index], heights_[index]};
}

std::size_t ViewportLayout::rowOf(std::size_t pageIndex) const {
    if (pageIndex >= pages_.size()) {
        throw std::out_of_range("page index out of range");
    }
    return rowOfPage_[pageIndex];
}

std::optional<std::size_t> ViewportLayout::pageAt(double y) const {
    if (pages_.empty()) {
        return std::nullopt;
    }
    // Last row whose top is <= y; returns that row's first page in reading order.
    const auto it = std::upper_bound(rows_.begin(), rows_.end(), y,
                                     [](double value, const PageRow& row) { return value < row.top; });
    if (it == rows_.begin()) {
        return 0;
    }
    return std::prev(it)->first;
}

std::optional<PageRange> ViewportLayout::pagesInBand(double top, double height) const {
    if (pages_.empty() || !(height > 0.0) || !std::isfinite(top)) {
        return std::nullopt;
    }
    const double bottom = top + height;
    if (bottom <= rows_.front().top || top >= rows_.back().top + rows_.back().height) {
        return std::nullopt;
    }
    auto firstRow = static_cast<std::size_t>(
        std::distance(rows_.begin(), std::upper_bound(rows_.begin(), rows_.end(), top,
                                                      [](double v, const PageRow& r) { return v < r.top; })));
    firstRow = firstRow == 0 ? 0 : firstRow - 1;
    if (rows_[firstRow].top + rows_[firstRow].height <= top && firstRow + 1 < rows_.size()) {
        ++firstRow; // `top` falls in the gap after this row
    }
    const auto it = std::lower_bound(rows_.begin(), rows_.end(), bottom,
                                     [](const PageRow& r, double v) { return r.top < v; });
    const std::size_t lastRow = it == rows_.begin() ? 0 : static_cast<std::size_t>(std::distance(rows_.begin(), it) - 1);
    if (lastRow < firstRow) {
        return std::nullopt; // band lies entirely inside one gap
    }
    return PageRange{rows_[firstRow].first, rows_[lastRow].last};
}

double ViewportLayout::scrollOffsetForPage(std::size_t index) const {
    if (index >= pages_.size()) {
        throw std::out_of_range("page index out of range");
    }
    const std::size_t row = rowOfPage_[index];
    if (row == 0) return 0.0; // the first page keeps the document margin visible
    return std::max(0.0, rows_[row].top - settings_.pageGap / 2.0);
}

double ViewportLayout::fitWidthZoom(double viewportWidth) const {
    if (pages_.empty() || !(viewportWidth > 0.0)) {
        return settings_.zoom;
    }
    // The widest row decides: points of its pages plus fixed gaps between them.
    double best = kMaxZoom;
    bool any = false;
    for (const auto& row : rows_) {
        double points = 0.0;
        for (std::size_t p = row.first; p <= row.last; ++p) {
            PageGeometry rotated = pages_[p];
            rotated.rotation = combineRotation(rotated.rotation, settings_.viewRotation);
            points += rotatedSizePoints(rotated).width;
        }
        const double available = viewportWidth - 2.0 * settings_.margin -
                                 static_cast<double>(row.last - row.first) * settings_.pageGap;
        if (points <= 0.0 || available <= 0.0) continue;
        best = std::min(best, available / (points * kPointsToLogicalPixels));
        any = true;
    }
    return any ? clampZoom(best) : settings_.zoom;
}

double ViewportLayout::fitPageZoom(SizeF viewport, std::size_t referencePage) const {
    if (pages_.empty() || !(viewport.width > 0.0) || !(viewport.height > 0.0)) {
        return settings_.zoom;
    }
    const auto& row = rows_[rowOfPage_[std::min(referencePage, pages_.size() - 1)]];
    double widthPoints = 0.0;
    double heightPoints = 0.0;
    for (std::size_t p = row.first; p <= row.last; ++p) {
        PageGeometry rotated = pages_[p];
        rotated.rotation = combineRotation(rotated.rotation, settings_.viewRotation);
        const SizeF size = rotatedSizePoints(rotated);
        widthPoints += size.width;
        heightPoints = std::max(heightPoints, size.height);
    }
    const double availableWidth = viewport.width - 2.0 * settings_.margin -
                                  static_cast<double>(row.last - row.first) * settings_.pageGap;
    const double availableHeight = viewport.height - 2.0 * settings_.margin;
    if (widthPoints <= 0.0 || heightPoints <= 0.0 || availableWidth <= 0.0 || availableHeight <= 0.0) {
        return settings_.zoom;
    }
    const double byWidth = availableWidth / (widthPoints * kPointsToLogicalPixels);
    const double byHeight = availableHeight / (heightPoints * kPointsToLogicalPixels);
    return clampZoom(std::min(byWidth, byHeight));
}

} // namespace atlas::reader
