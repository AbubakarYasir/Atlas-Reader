#pragma once

// Render planning, scheduling and texture-cache policy for the reader viewport
// (N4.2). Pure C++: the Qt adapter executes the requests and owns real textures,
// while every decision about *what* to render, in which order, what to cancel
// and what to evict lives here and is unit-tested.

#include "core/reader/ViewportLayout.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace atlas::reader {

// ---------------------------------------------------------------- scale buckets

// Renders are quantised to quarter-octave steps (~19%) so small zoom changes
// reuse textures and the cache does not fill with near-duplicates.
inline constexpr int kBucketStepsPerOctave = 4;
inline constexpr int kDefaultMaxTextureSide = 8192;

// Smallest bucket whose scale is >= pixelsPerPoint (never blurrier than asked).
[[nodiscard]] int scaleBucketFor(double pixelsPerPoint) noexcept;
[[nodiscard]] double bucketScale(int bucket) noexcept; // pixels per point

// Pixel size of a page rendered at a bucket (rotation applied).
[[nodiscard]] SizeF renderedPixelSize(const PageGeometry& rotatedPage, int bucket) noexcept;

// Largest bucket <= wanted whose render fits within maxSide on both axes.
// Pages beyond that are rendered at the capped bucket; tiling is a later step.
[[nodiscard]] int cappedBucket(const PageGeometry& rotatedPage, int wanted, int maxSide) noexcept;

// --------------------------------------------------------------------- keys

struct TextureKey final {
    std::uint64_t documentRevision{};
    std::uint32_t page{};
    std::int16_t bucket{};
    PageRotation rotation{PageRotation::r0};
    std::uint8_t theme{}; // render-affecting appearance (for example inverted page colours)

    friend bool operator==(const TextureKey&, const TextureKey&) = default;
};

struct TextureKeyHash final {
    [[nodiscard]] std::size_t operator()(const TextureKey& key) const noexcept;
};

// ----------------------------------------------------------------- planning

enum class RenderPriority : std::uint8_t { visible = 0, prefetch = 1 };

struct RenderRequest final {
    TextureKey key;
    RenderPriority priority{RenderPriority::visible};
    double distance{}; // logical px from the viewport centre; lower renders sooner
    SizeF pixelSize;   // expected output size

    friend bool operator==(const RenderRequest&, const RenderRequest&) = default;
};

struct ViewportState final {
    double scrollTop{};      // logical px, content coordinates
    double viewportHeight{}; // logical px
    double devicePixelRatio{1.0};
    double prefetchScreens{1.0}; // extra viewport heights above and below
    std::uint64_t documentRevision{};
    std::uint8_t theme{};
    int maxTextureSide{kDefaultMaxTextureSide};
};

struct RenderPlan final {
    std::optional<PageRange> visible;
    std::optional<PageRange> nearby; // includes `visible`
    std::vector<RenderRequest> requests; // ordered: visible first, then by distance
};

[[nodiscard]] RenderPlan planRenders(const ViewportLayout& layout, const ViewportState& state);

// ---------------------------------------------------------------- scheduling

using RenderTicket = std::uint64_t;

struct SchedulerActions final {
    std::vector<std::pair<RenderTicket, RenderRequest>> start;
    std::vector<RenderTicket> cancel;
};

// Keeps at most `maxConcurrent` renders in flight. Each new plan supersedes the
// last: in-flight work no longer wanted is cancelled, and queued work is
// replaced, so a fast scroll never drains a backlog of stale pages.
class RenderScheduler final {
public:
    explicit RenderScheduler(std::size_t maxConcurrent = 2);

    // `isCached` lets the scheduler skip requests whose texture already exists.
    [[nodiscard]] SchedulerActions update(const RenderPlan& plan,
                                          const std::function<bool(const TextureKey&)>& isCached);

    // Called by the adapter when a render finishes, fails or acknowledges a
    // cancel. Returns further work that may now start. Unknown tickets (already
    // cancelled) are ignored, so late results can never be mistaken as current.
    [[nodiscard]] SchedulerActions finished(RenderTicket ticket);

    // True while `ticket` belongs to the current plan; adapters drop results
    // whose ticket is no longer current.
    [[nodiscard]] bool isCurrent(RenderTicket ticket) const;

    [[nodiscard]] std::size_t inFlightCount() const noexcept { return inFlight_.size(); }
    [[nodiscard]] std::size_t queuedCount() const noexcept { return queued_.size(); }

private:
    [[nodiscard]] SchedulerActions fill();

    std::size_t maxConcurrent_;
    RenderTicket nextTicket_{1};
    std::vector<RenderRequest> queued_;
    std::unordered_map<RenderTicket, TextureKey> inFlight_;
};

// -------------------------------------------------------------------- cache

// Byte-budgeted LRU of page textures. Pinned keys (currently visible) are never
// evicted; the cache may exceed its budget only while pins alone require it.
// Memory is therefore bounded by the budget plus the visible set, never by the
// document's page count.
class PageTextureCache final {
public:
    explicit PageTextureCache(std::size_t byteBudget);

    // Records a texture. Returns keys evicted to respect the budget; the
    // adapter must release those textures.
    [[nodiscard]] std::vector<TextureKey> insert(const TextureKey& key, std::size_t bytes);
    [[nodiscard]] bool contains(const TextureKey& key) const;
    void touch(const TextureKey& key);

    // Replaces the pin set; returns keys evicted now that older pins are free.
    [[nodiscard]] std::vector<TextureKey> pin(const std::vector<TextureKey>& keys);

    // Drops every texture that is not from `documentRevision` (reopen/reload).
    [[nodiscard]] std::vector<TextureKey> retainOnlyRevision(std::uint64_t documentRevision);
    [[nodiscard]] std::vector<TextureKey> clear();

    // Best existing texture for a page while a sharper one renders: the largest
    // bucket <= wanted, otherwise the smallest larger bucket.
    [[nodiscard]] std::optional<TextureKey> bestAvailable(std::uint64_t documentRevision,
                                                          std::uint32_t page,
                                                          PageRotation rotation, std::uint8_t theme,
                                                          int wantedBucket) const;

    [[nodiscard]] std::size_t bytesUsed() const noexcept { return bytes_; }
    [[nodiscard]] std::size_t byteBudget() const noexcept { return budget_; }
    [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }
    void setByteBudget(std::size_t budget) { budget_ = budget; }
    [[nodiscard]] std::vector<TextureKey> enforceBudget();

private:
    struct Entry final {
        TextureKey key;
        std::size_t bytes{};
    };
    using Lru = std::list<Entry>; // front = most recently used

    std::size_t budget_;
    std::size_t bytes_{};
    Lru lru_;
    std::unordered_map<TextureKey, Lru::iterator, TextureKeyHash> entries_;
    std::unordered_set<TextureKey, TextureKeyHash> pinned_;
};

[[nodiscard]] std::size_t textureBytes(SizeF pixelSize) noexcept; // RGBA8

} // namespace atlas::reader
