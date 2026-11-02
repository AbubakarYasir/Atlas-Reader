#include "core/reader/RenderPlanner.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace atlas::reader {

namespace {

constexpr int kMinBucket = -8 * kBucketStepsPerOctave; // 1/256 px per point
constexpr int kMaxBucket = 8 * kBucketStepsPerOctave;  // 256 px per point

double centreDistance(const RectF& page, double centreY) noexcept {
    if (centreY < page.y) {
        return page.y - centreY;
    }
    if (centreY > page.bottom()) {
        return centreY - page.bottom();
    }
    return 0.0;
}

} // namespace

// ------------------------------------------------------------------ buckets

int scaleBucketFor(double pixelsPerPoint) noexcept {
    if (!(pixelsPerPoint > 0.0) || !std::isfinite(pixelsPerPoint)) {
        return 0;
    }
    const double exact = std::log2(pixelsPerPoint) * kBucketStepsPerOctave;
    const int bucket = static_cast<int>(std::ceil(exact - 1e-9));
    return std::clamp(bucket, kMinBucket, kMaxBucket);
}

double bucketScale(int bucket) noexcept {
    return std::exp2(static_cast<double>(bucket) / kBucketStepsPerOctave);
}

SizeF renderedPixelSize(const PageGeometry& rotatedPage, int bucket) noexcept {
    const SizeF points = rotatedSizePoints(rotatedPage);
    const double scale = bucketScale(bucket);
    return SizeF{std::max(1.0, std::ceil(points.width * scale)),
                 std::max(1.0, std::ceil(points.height * scale))};
}

int cappedBucket(const PageGeometry& rotatedPage, int wanted, int maxSide) noexcept {
    int bucket = std::clamp(wanted, kMinBucket, kMaxBucket);
    const double limit = std::max(1, maxSide);
    while (bucket > kMinBucket) {
        const SizeF size = renderedPixelSize(rotatedPage, bucket);
        if (size.width <= limit && size.height <= limit) {
            break;
        }
        --bucket;
    }
    return bucket;
}

std::size_t TextureKeyHash::operator()(const TextureKey& key) const noexcept {
    std::uint64_t h = key.documentRevision * 0x9E3779B97F4A7C15ULL;
    h ^= (static_cast<std::uint64_t>(key.page) << 20) ^
         (static_cast<std::uint64_t>(static_cast<std::uint16_t>(key.bucket)) << 4) ^
         (static_cast<std::uint64_t>(key.rotation) << 2) ^
         (static_cast<std::uint64_t>(key.theme) << 52);
    h ^= h >> 31;
    h *= 0xBF58476D1CE4E5B9ULL;
    h ^= h >> 29;
    return static_cast<std::size_t>(h);
}

std::size_t textureBytes(SizeF pixelSize) noexcept {
    const double bytes = std::max(0.0, pixelSize.width) * std::max(0.0, pixelSize.height) * 4.0;
    if (bytes >= static_cast<double>(std::numeric_limits<std::size_t>::max())) {
        return std::numeric_limits<std::size_t>::max();
    }
    return static_cast<std::size_t>(bytes);
}

// ----------------------------------------------------------------- planning

RenderPlan planRenders(const ViewportLayout& layout, const ViewportState& state) {
    RenderPlan plan;
    if (layout.pageCount() == 0 || !(state.viewportHeight > 0.0)) {
        return plan;
    }

    const double top = std::max(0.0, state.scrollTop);
    plan.visible = layout.pagesInBand(top, state.viewportHeight);
    if (!plan.visible) {
        // Scrolled into a gap or past the end: treat the nearest page as visible.
        const auto nearest = layout.pageAt(top);
        if (!nearest) {
            return plan;
        }
        plan.visible = PageRange{*nearest, *nearest};
    }

    const double prefetch = std::max(0.0, state.prefetchScreens) * state.viewportHeight;
    plan.nearby = layout.pagesInBand(std::max(0.0, top - prefetch),
                                     state.viewportHeight + 2.0 * prefetch);
    if (!plan.nearby) {
        plan.nearby = plan.visible;
    }
    plan.nearby->first = std::min(plan.nearby->first, plan.visible->first);
    plan.nearby->last = std::max(plan.nearby->last, plan.visible->last);

    const double dpr = state.devicePixelRatio > 0.0 ? state.devicePixelRatio : 1.0;
    const double pixelsPerPoint = layout.settings().zoom * kPointsToLogicalPixels * dpr;
    const int wanted = scaleBucketFor(pixelsPerPoint);
    const double centreY = top + state.viewportHeight / 2.0;

    plan.requests.reserve(plan.nearby->count());
    for (std::size_t i = plan.nearby->first; i <= plan.nearby->last; ++i) {
        PageGeometry rotated = layout.page(i);
        rotated.rotation = layout.effectiveRotation(i);
        const int bucket = cappedBucket(rotated, wanted, state.maxTextureSide);
        RenderRequest request;
        request.key = TextureKey{state.documentRevision, static_cast<std::uint32_t>(i),
                                 static_cast<std::int16_t>(bucket), rotated.rotation, state.theme};
        request.priority = plan.visible->contains(i) ? RenderPriority::visible : RenderPriority::prefetch;
        request.distance = centreDistance(layout.pageRect(i), centreY);
        request.pixelSize = renderedPixelSize(rotated, bucket);
        plan.requests.push_back(request);
    }

    std::stable_sort(plan.requests.begin(), plan.requests.end(),
                     [](const RenderRequest& a, const RenderRequest& b) {
                         if (a.priority != b.priority) {
                             return a.priority < b.priority;
                         }
                         return a.distance < b.distance;
                     });
    return plan;
}

// --------------------------------------------------------------- scheduling

RenderScheduler::RenderScheduler(std::size_t maxConcurrent)
    : maxConcurrent_(std::max<std::size_t>(1, maxConcurrent)) {}

SchedulerActions RenderScheduler::update(const RenderPlan& plan,
                                         const std::function<bool(const TextureKey&)>& isCached) {
    std::unordered_set<TextureKey, TextureKeyHash> wanted;
    queued_.clear();
    for (const auto& request : plan.requests) {
        wanted.insert(request.key);
        if (isCached && isCached(request.key)) {
            continue;
        }
        queued_.push_back(request);
    }

    SchedulerActions actions;
    std::unordered_set<TextureKey, TextureKeyHash> running;
    for (auto it = inFlight_.begin(); it != inFlight_.end();) {
        if (!wanted.contains(it->second)) {
            actions.cancel.push_back(it->first);
            it = inFlight_.erase(it);
        } else {
            running.insert(it->second);
            ++it;
        }
    }
    std::erase_if(queued_, [&](const RenderRequest& r) { return running.contains(r.key); });
    std::sort(actions.cancel.begin(), actions.cancel.end());

    SchedulerActions started = fill();
    actions.start = std::move(started.start);
    return actions;
}

SchedulerActions RenderScheduler::finished(RenderTicket ticket) {
    if (inFlight_.erase(ticket) == 0) {
        return {};
    }
    return fill();
}

bool RenderScheduler::isCurrent(RenderTicket ticket) const {
    return inFlight_.contains(ticket);
}

SchedulerActions RenderScheduler::fill() {
    SchedulerActions actions;
    std::size_t taken = 0;
    while (inFlight_.size() < maxConcurrent_ && taken < queued_.size()) {
        const RenderRequest request = queued_[taken++];
        const RenderTicket ticket = nextTicket_++;
        inFlight_.emplace(ticket, request.key);
        actions.start.emplace_back(ticket, request);
    }
    queued_.erase(queued_.begin(), queued_.begin() + static_cast<std::ptrdiff_t>(taken));
    return actions;
}

// -------------------------------------------------------------------- cache

PageTextureCache::PageTextureCache(std::size_t byteBudget) : budget_(byteBudget) {}

std::vector<TextureKey> PageTextureCache::insert(const TextureKey& key, std::size_t bytes) {
    if (const auto it = entries_.find(key); it != entries_.end()) {
        bytes_ -= it->second->bytes;
        lru_.erase(it->second);
        entries_.erase(it);
    }
    lru_.push_front(Entry{key, bytes});
    entries_.emplace(key, lru_.begin());
    bytes_ += bytes;
    return enforceBudget();
}

bool PageTextureCache::contains(const TextureKey& key) const {
    return entries_.contains(key);
}

void PageTextureCache::touch(const TextureKey& key) {
    if (const auto it = entries_.find(key); it != entries_.end()) {
        lru_.splice(lru_.begin(), lru_, it->second);
    }
}

std::vector<TextureKey> PageTextureCache::pin(const std::vector<TextureKey>& keys) {
    pinned_.clear();
    for (const auto& key : keys) {
        pinned_.insert(key);
        touch(key);
    }
    return enforceBudget();
}

std::vector<TextureKey> PageTextureCache::enforceBudget() {
    std::vector<TextureKey> evicted;
    auto it = lru_.end();
    while (bytes_ > budget_ && it != lru_.begin()) {
        --it;
        if (pinned_.contains(it->key)) {
            continue;
        }
        evicted.push_back(it->key);
        bytes_ -= it->bytes;
        entries_.erase(it->key);
        it = lru_.erase(it);
    }
    return evicted;
}

std::vector<TextureKey> PageTextureCache::retainOnlyRevision(std::uint64_t documentRevision) {
    std::vector<TextureKey> evicted;
    for (auto it = lru_.begin(); it != lru_.end();) {
        if (it->key.documentRevision != documentRevision) {
            evicted.push_back(it->key);
            bytes_ -= it->bytes;
            pinned_.erase(it->key);
            entries_.erase(it->key);
            it = lru_.erase(it);
        } else {
            ++it;
        }
    }
    return evicted;
}

std::vector<TextureKey> PageTextureCache::clear() {
    std::vector<TextureKey> evicted;
    evicted.reserve(lru_.size());
    for (const auto& entry : lru_) {
        evicted.push_back(entry.key);
    }
    lru_.clear();
    entries_.clear();
    pinned_.clear();
    bytes_ = 0;
    return evicted;
}

std::optional<TextureKey> PageTextureCache::bestAvailable(std::uint64_t documentRevision,
                                                          std::uint32_t page, PageRotation rotation,
                                                          std::uint8_t theme,
                                                          int wantedBucket) const {
    std::optional<TextureKey> below;
    std::optional<TextureKey> above;
    for (const auto& entry : lru_) {
        const TextureKey& key = entry.key;
        if (key.documentRevision != documentRevision || key.page != page ||
            key.rotation != rotation || key.theme != theme) {
            continue;
        }
        if (key.bucket <= wantedBucket) {
            if (!below || key.bucket > below->bucket) {
                below = key;
            }
        } else if (!above || key.bucket < above->bucket) {
            above = key;
        }
    }
    return below ? below : above;
}

} // namespace atlas::reader
