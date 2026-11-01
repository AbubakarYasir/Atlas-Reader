#pragma once

// Back/Forward history of reading positions (N4.4, Foxit Previous/Next View,
// Alt+Left / Alt+Right). Positions are page-relative so they survive zoom,
// fit and rotation changes. Pure C++; no Qt.

#include <cstddef>
#include <optional>
#include <vector>

namespace atlas::reader {

struct ViewPosition final {
    std::size_t page{};     // zero-based
    double fraction{};      // 0..1 offset of the view top within the page

    friend bool operator==(const ViewPosition&, const ViewPosition&) = default;
};

class ViewHistory final {
public:
    static constexpr std::size_t kMaxEntries = 100;

    // Called before a jump (page box, first/last, link, thumbnail, search
    // result): remembers where the reader was and clears Forward.
    void recordJump(const ViewPosition& from);
    [[nodiscard]] std::optional<ViewPosition> back(const ViewPosition& current);
    [[nodiscard]] std::optional<ViewPosition> forward(const ViewPosition& current);
    [[nodiscard]] bool canGoBack() const noexcept { return !back_.empty(); }
    [[nodiscard]] bool canGoForward() const noexcept { return !forward_.empty(); }
    void clear() noexcept;

private:
    std::vector<ViewPosition> back_;
    std::vector<ViewPosition> forward_;
};

} // namespace atlas::reader
