#pragma once

// Engine-independent page geometry for the Atlas reader viewport (N4.2).
//
// Units: page sizes arrive in PDF points (1/72 inch). Layout coordinates are
// logical (device-independent) pixels at the current zoom, where zoom 1.0 means
// one point maps to 96/72 logical pixels ("actual size" on a 96 DPI display).
// No Qt or PDF-engine types appear here, so the model is unit-testable without
// a GUI and reusable by future non-PDF fixed-layout adapters.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace atlas::reader {

enum class PageRotation : std::uint8_t { r0, r90, r180, r270 };

[[nodiscard]] constexpr int rotationDegrees(PageRotation rotation) noexcept {
    return static_cast<int>(rotation) * 90;
}

// Combines a page's own /Rotate with a user view rotation.
[[nodiscard]] constexpr PageRotation combineRotation(PageRotation page,
                                                     PageRotation view) noexcept {
    return static_cast<PageRotation>((static_cast<int>(page) + static_cast<int>(view)) % 4);
}

struct PageGeometry final {
    // Visible (crop-box) size before rotation, in points.
    double widthPoints{};
    double heightPoints{};
    PageRotation rotation{PageRotation::r0};
};

struct SizeF final {
    double width{};
    double height{};
};

struct RectF final {
    double x{};
    double y{};
    double width{};
    double height{};

    [[nodiscard]] double bottom() const noexcept { return y + height; }
    [[nodiscard]] double right() const noexcept { return x + width; }
};

// Size of a page in points after its rotation is applied.
[[nodiscard]] SizeF rotatedSizePoints(const PageGeometry& page) noexcept;

struct PageRange final {
    std::size_t first{};
    std::size_t last{}; // inclusive

    [[nodiscard]] std::size_t count() const noexcept { return last - first + 1; }
    [[nodiscard]] bool contains(std::size_t index) const noexcept {
        return index >= first && index <= last;
    }
};

// Foxit Page Display modes (View tab, status bar).
enum class PageDisplay {
    singlePage,     // one page (or spread) at a time
    continuous,     // pages one after another (default)
    facing,         // two pages side by side, one spread at a time
    continuousFacing,
};

struct LayoutSettings final {
    double zoom{1.0};        // 1.0 == actual size
    double pageGap{12.0};    // logical px between pages and between spread pages
    double margin{24.0};     // logical px around the document
    PageRotation viewRotation{PageRotation::r0};
    PageDisplay display{PageDisplay::continuous};
    bool coverPage{};        // facing modes: first page alone, like a book cover
    bool rightToLeft{};      // Reverse View: spreads read right to left (Arabic/Urdu books)
};

[[nodiscard]] constexpr bool isFacing(PageDisplay display) noexcept {
    return display == PageDisplay::facing || display == PageDisplay::continuousFacing;
}
[[nodiscard]] constexpr bool isContinuous(PageDisplay display) noexcept {
    return display == PageDisplay::continuous || display == PageDisplay::continuousFacing;
}

// Pages that are shown together on one row (1 page, or 2 in a spread).
struct PageRow final {
    std::size_t first{};
    std::size_t last{}; // inclusive
    double top{};
    double height{};
};

inline constexpr double kPointsToLogicalPixels = 96.0 / 72.0;
inline constexpr double kMinZoom = 0.05;
inline constexpr double kMaxZoom = 64.0;

[[nodiscard]] double clampZoom(double zoom) noexcept;

// Page layout in rows: one page per row, or two-page spreads (facing), with an
// optional cover page and right-to-left spread order. Single-page modes use the
// same geometry; the viewport limits scrolling to one row at a time.
class ViewportLayout final {
public:
    ViewportLayout() = default;
    ViewportLayout(std::vector<PageGeometry> pages, LayoutSettings settings);

    void setSettings(LayoutSettings settings);
    [[nodiscard]] const LayoutSettings& settings() const noexcept { return settings_; }

    [[nodiscard]] std::size_t pageCount() const noexcept { return pages_.size(); }
    [[nodiscard]] const PageGeometry& page(std::size_t index) const;
    [[nodiscard]] PageRotation effectiveRotation(std::size_t index) const;

    // Whole document extent in logical px, including margins.
    [[nodiscard]] SizeF contentSize() const noexcept { return content_; }

    // Page rectangle in content coordinates (logical px), horizontally centred.
    [[nodiscard]] RectF pageRect(std::size_t index) const;

    // Pages intersecting [top, top + height). Empty for an empty document or a
    // band entirely outside the content.
    [[nodiscard]] std::optional<PageRange> pagesInBand(double top, double height) const;

    // Page whose vertical span (including its trailing gap) contains y.
    [[nodiscard]] std::optional<std::size_t> pageAt(double y) const;

    // Scroll offset that places the top of the page at the top of the viewport.
    [[nodiscard]] double scrollOffsetForPage(std::size_t index) const;

    // Zoom values for fit modes, given the viewport size in logical px.
    [[nodiscard]] double fitWidthZoom(double viewportWidth) const;
    [[nodiscard]] double fitPageZoom(SizeF viewport, std::size_t referencePage) const;

    // Rows (pages shown together) and the row holding a page.
    [[nodiscard]] const std::vector<PageRow>& rows() const noexcept { return rows_; }
    [[nodiscard]] std::size_t rowOf(std::size_t pageIndex) const;

private:
    void rebuild();

    std::vector<PageGeometry> pages_;
    LayoutSettings settings_{};
    std::vector<double> tops_;    // page top, logical px
    std::vector<double> heights_; // page height, logical px
    std::vector<double> widths_;  // page width, logical px
    std::vector<double> lefts_;   // page left edge, logical px
    std::vector<std::size_t> rowOfPage_;
    std::vector<PageRow> rows_;
    double widestPage_{};         // widest row, logical px
    SizeF content_{};
};

} // namespace atlas::reader
