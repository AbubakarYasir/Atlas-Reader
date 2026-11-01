// N4.2 viewport geometry, render planning, scheduling and cache-budget tests.
// Pure C++: no Qt, no PDF engine, no GUI.

#include "core/reader/RenderPlanner.h"
#include "core/reader/ViewHistory.h"
#include "core/reader/ViewportLayout.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>

namespace {

using namespace atlas::reader;

void require(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string{message});
    }
}

bool approx(double a, double b, double tolerance = 1e-6) {
    return std::abs(a - b) <= tolerance;
}

constexpr PageGeometry kLetter{612.0, 792.0, PageRotation::r0};
constexpr PageGeometry kLandscape{792.0, 612.0, PageRotation::r0};

std::vector<PageGeometry> uniform(std::size_t count, PageGeometry page = kLetter) {
    return std::vector<PageGeometry>(count, page);
}

void testRotationAndSizes() {
    require(rotatedSizePoints({100, 200, PageRotation::r90}).width == 200,
            "A quarter turn must swap width and height.");
    require(rotatedSizePoints({100, 200, PageRotation::r180}).width == 100,
            "A half turn must keep width and height.");
    require(combineRotation(PageRotation::r270, PageRotation::r180) == PageRotation::r90,
            "Page and view rotations must combine modulo 360 degrees.");
    require(approx(clampZoom(1000.0), kMaxZoom) && approx(clampZoom(0.0), kMinZoom) &&
                approx(clampZoom(std::nan("")), 1.0),
            "Zoom must be clamped and non-finite zoom must fall back to actual size.");
}

void testContinuousLayout() {
    std::vector<PageGeometry> pages{kLetter, kLandscape, {612, 792, PageRotation::r90}};
    const LayoutSettings settings{1.0, 10.0, 20.0, PageRotation::r0};
    const ViewportLayout layout(pages, settings);
    const double s = kPointsToLogicalPixels;

    const RectF first = layout.pageRect(0);
    require(approx(first.y, 20.0) && approx(first.height, 792 * s),
            "The first page must start after the top margin.");
    const RectF second = layout.pageRect(1);
    require(approx(second.y, first.bottom() + 10.0), "Pages must be separated by the page gap.");
    require(approx(second.width, 792 * s) && approx(second.x, 20.0),
            "The widest page defines the column and starts at the margin.");
    require(approx(first.x, 20.0 + (792 * s - 612 * s) / 2.0),
            "Narrower pages must be centred in the column.");
    const RectF third = layout.pageRect(2);
    require(approx(third.width, 792 * s) && approx(third.height, 612 * s),
            "A /Rotate 90 page must be laid out rotated.");
    require(approx(layout.contentSize().height, third.bottom() + 20.0) &&
                approx(layout.contentSize().width, 792 * s + 40.0),
            "Content size must include margins on all sides.");

    LayoutSettings rotated = settings;
    rotated.viewRotation = PageRotation::r90;
    const ViewportLayout turned(pages, rotated);
    require(approx(turned.pageRect(0).width, 792 * s) &&
                turned.effectiveRotation(2) == PageRotation::r180,
            "A user view rotation must rotate every page and combine with /Rotate.");
}

void testBandQueries() {
    const ViewportLayout layout(uniform(10), LayoutSettings{1.0, 10.0, 20.0, PageRotation::r0});
    const RectF p3 = layout.pageRect(3);

    auto range = layout.pagesInBand(p3.y + 1.0, 10.0);
    require(range && range->first == 3 && range->last == 3,
            "A band inside one page must return exactly that page.");
    range = layout.pagesInBand(p3.bottom() - 5.0, 20.0);
    require(range && range->first == 3 && range->last == 4,
            "A band crossing a page boundary must return both pages.");
    require(!layout.pagesInBand(p3.bottom() + 1.0, 5.0),
            "A band entirely inside a gap must return no pages.");
    require(!layout.pagesInBand(layout.contentSize().height + 100.0, 50.0),
            "A band past the end must return no pages.");
    range = layout.pagesInBand(0.0, layout.contentSize().height);
    require(range && range->first == 0 && range->last == 9,
            "A band covering the document must return every page.");

    require(layout.pageAt(-50.0) == 0u && layout.pageAt(p3.y + 2.0) == 3u &&
                layout.pageAt(1e12) == 9u,
            "pageAt must clamp before the first and after the last page.");
    require(approx(layout.scrollOffsetForPage(3), p3.y - 5.0),
            "Jumping to a page must place its top just below the viewport edge.");
    require(!ViewportLayout{}.pagesInBand(0, 100) && !ViewportLayout{}.pageAt(0),
            "An empty document must have no pages anywhere.");
}

void testFitZoom() {
    const ViewportLayout layout(uniform(3), LayoutSettings{1.0, 10.0, 20.0, PageRotation::r0});
    const double width = layout.fitWidthZoom(612 * kPointsToLogicalPixels + 40.0);
    require(approx(width, 1.0), "Fit width must make the widest page exactly fill the width.");
    const double page = layout.fitPageZoom({2000.0, 792 * kPointsToLogicalPixels / 2.0 + 40.0}, 0);
    require(approx(page, 0.5), "Fit page must be limited by the tighter dimension.");
    require(approx(layout.fitWidthZoom(0.0), 1.0),
            "Fit with no viewport must keep the current zoom.");
}

void testScaleBuckets() {
    require(scaleBucketFor(1.0) == 0 && approx(bucketScale(0), 1.0), "Scale 1 is bucket 0.");
    for (double ppp : {0.3, 0.77, 1.0, 1.33, 2.0, 2.66, 5.1, 40.0}) {
        const int bucket = scaleBucketFor(ppp);
        require(bucketScale(bucket) >= ppp - 1e-9,
                "A bucket must never render blurrier than requested.");
        require(bucketScale(bucket) < ppp * std::exp2(1.0 / kBucketStepsPerOctave) + 1e-9,
                "A bucket must be within one quarter-octave of the request.");
    }
    require(scaleBucketFor(1.33) == scaleBucketFor(1.38),
            "Small zoom changes must reuse the same bucket.");

    const PageGeometry poster{14400.0, 14400.0, PageRotation::r0}; // 200 inch square
    const int capped = cappedBucket(poster, scaleBucketFor(4.0), kDefaultMaxTextureSide);
    const SizeF size = renderedPixelSize(poster, capped);
    require(size.width <= kDefaultMaxTextureSide && size.height <= kDefaultMaxTextureSide,
            "Huge pages must be capped to the maximum texture side.");
}

ViewportState stateAt(const ViewportLayout& layout, std::size_t page, double height = 900.0) {
    ViewportState state;
    state.scrollTop = layout.scrollOffsetForPage(page);
    state.viewportHeight = height;
    state.devicePixelRatio = 1.5;
    state.documentRevision = 7;
    return state;
}

void testPlanning() {
    const ViewportLayout layout(uniform(50), LayoutSettings{1.0, 12.0, 24.0, PageRotation::r0});
    const RenderPlan plan = planRenders(layout, stateAt(layout, 20));

    require(plan.visible && plan.visible->first == 20, "The jumped-to page must be visible.");
    require(plan.nearby && plan.nearby->first < plan.visible->first &&
                plan.nearby->last > plan.visible->last,
            "Prefetch must extend above and below the visible pages.");
    require(!plan.requests.empty() && plan.requests.front().priority == RenderPriority::visible,
            "Visible pages must be requested first.");

    bool seenNear = false;
    double lastDistance = -1.0;
    for (const auto& request : plan.requests) {
        if (request.priority == RenderPriority::prefetch) {
            if (!seenNear) {
                lastDistance = -1.0;
            }
            seenNear = true;
        } else {
            require(!seenNear, "No visible request may follow a prefetch request.");
        }
        require(request.distance >= lastDistance,
                "Within a priority, closer pages must render first.");
        lastDistance = request.distance;
        require(request.key.documentRevision == 7, "Requests must carry the document revision.");
    }
    require(plan.requests.size() == plan.nearby->count(),
            "Exactly one request per nearby page must be planned.");
    const int expected = scaleBucketFor(1.0 * kPointsToLogicalPixels * 1.5);
    require(plan.requests.front().key.bucket == expected,
            "Requests must target the bucket for zoom x device pixel ratio.");

    ViewportState empty = stateAt(layout, 0);
    empty.viewportHeight = 0.0;
    require(planRenders(layout, empty).requests.empty(), "No viewport means no work.");
}

void testSchedulerSupersedesStaleWork() {
    const ViewportLayout layout(uniform(500), LayoutSettings{});
    RenderScheduler scheduler(2);
    const auto never = [](const TextureKey&) { return false; };

    auto actions = scheduler.update(planRenders(layout, stateAt(layout, 10)), never);
    require(actions.start.size() == 2 && actions.cancel.empty(),
            "The scheduler must start at most maxConcurrent renders.");
    require(actions.start.front().second.key.page == 10,
            "The first render must be the visible page.");
    const RenderTicket stale = actions.start.front().first;

    // Fast jump far away: everything in flight becomes obsolete.
    actions = scheduler.update(planRenders(layout, stateAt(layout, 400)), never);
    require(actions.cancel.size() == 2, "Obsolete in-flight renders must be cancelled.");
    require(!scheduler.isCurrent(stale), "A cancelled ticket must no longer be current.");
    require(actions.start.size() == 2 && actions.start.front().second.key.page == 400,
            "The new visible page must start immediately after a jump.");
    require(scheduler.finished(stale).start.empty(),
            "A late result from a cancelled ticket must be ignored.");

    const RenderTicket current = actions.start.front().first;
    const auto more = scheduler.finished(current);
    require(more.start.size() == 1 && scheduler.inFlightCount() == 2,
            "Finishing a render must start the next queued request.");

    // Re-planning the same view keeps running work and does not duplicate it.
    const auto again = scheduler.update(planRenders(layout, stateAt(layout, 400)), never);
    require(again.cancel.empty(), "Still-wanted renders must not be cancelled.");
    for (const auto& [ticket, request] : again.start) {
        (void)ticket;
        require(request.key.page != 400, "A finished or running page must not be re-queued.");
    }
    (void)scheduler.update(planRenders(layout, stateAt(layout, 400)),
                           [](const TextureKey& key) { return key.page == 400; });

    // Cached pages are skipped entirely.
    RenderScheduler fresh(4);
    const auto cachedAll = fresh.update(planRenders(layout, stateAt(layout, 5)),
                                        [](const TextureKey&) { return true; });
    require(cachedAll.start.empty() && fresh.queuedCount() == 0,
            "Fully cached views must not render anything.");
}

TextureKey key(std::uint32_t page, std::int16_t bucket = 0, std::uint64_t revision = 1) {
    return TextureKey{revision, page, bucket, PageRotation::r0, 0};
}

void testCacheBudget() {
    PageTextureCache cache(1000);
    require(cache.insert(key(1), 400).empty() && cache.insert(key(2), 400).empty(),
            "Inserts within budget must not evict.");
    cache.touch(key(1));
    const auto evicted = cache.insert(key(3), 400);
    require(evicted.size() == 1 && evicted.front() == key(2),
            "The least recently used texture must be evicted first.");
    require(cache.bytesUsed() == 800 && cache.contains(key(1)) && cache.contains(key(3)),
            "Byte accounting must follow inserts and evictions.");

    (void)cache.pin({key(1), key(3)});
    const auto pinnedInsert = cache.insert(key(4), 400);
    require(pinnedInsert.size() == 1 && pinnedInsert.front() == key(4),
            "Pinned (visible) textures must never be evicted, even for newer ones.");

    (void)cache.insert(key(5, 2), 100);
    (void)cache.insert(key(5, -2), 50);
    const auto best = cache.bestAvailable(1, 5, PageRotation::r0, 0, 1);
    require(best && best->bucket == -2, "Prefer the sharpest texture not above the wanted bucket.");
    const auto above = cache.bestAvailable(1, 5, PageRotation::r0, 0, -4);
    require(above && above->bucket == -2,
            "Fall back to the smallest larger texture when nothing smaller exists.");
    const auto sharp = cache.bestAvailable(1, 5, PageRotation::r0, 0, 3);
    require(sharp && sharp->bucket == 2, "With a sharper request, reuse the sharpest smaller texture.");
    require(!cache.bestAvailable(2, 5, PageRotation::r0, 0, 0),
            "Textures from another document revision must never be reused.");

    const auto dropped = cache.retainOnlyRevision(2);
    require(cache.size() == 0 && cache.bytesUsed() == 0 && !dropped.empty(),
            "Reopening a document must drop every stale texture.");
}

// The N2 limitation "Qt working-set trend needs real-reader proof" requires that
// memory does not grow with page count. Simulate reading through 2,000 pages.
void testTwoThousandPageMemorySeries() {
    const ViewportLayout layout(uniform(2000), LayoutSettings{1.5, 12.0, 24.0, PageRotation::r0});
    constexpr std::size_t budget = 256ULL * 1024ULL * 1024ULL;
    PageTextureCache cache(budget);
    RenderScheduler scheduler(3);
    std::size_t peak = 0;
    std::size_t rendered = 0;

    for (std::size_t page = 0; page < 2000; page += 7) {
        ViewportState state = stateAt(layout, page, 1080.0);
        state.devicePixelRatio = 2.0;
        const RenderPlan plan = planRenders(layout, state);

        std::vector<TextureKey> visibleKeys;
        for (const auto& request : plan.requests) {
            if (request.priority == RenderPriority::visible) {
                visibleKeys.push_back(request.key);
            }
        }
        (void)cache.pin(visibleKeys);

        auto actions = scheduler.update(plan, [&](const TextureKey& k) { return cache.contains(k); });
        // Complete everything that starts, as a fast renderer would.
        while (!actions.start.empty()) {
            SchedulerActions next;
            for (const auto& [ticket, request] : actions.start) {
                (void)cache.insert(request.key, textureBytes(request.pixelSize));
                ++rendered;
                auto more = scheduler.finished(ticket);
                next.start.insert(next.start.end(), more.start.begin(), more.start.end());
            }
            actions = std::move(next);
        }
        peak = std::max(peak, cache.bytesUsed());
    }

    require(rendered > 280, "The series must actually render pages across the whole book.");
    require(peak <= budget, "Texture memory must stay within the cache budget.");
    const std::size_t oneDocumentAtThisZoom =
        2000 * textureBytes(renderedPixelSize(kLetter, scaleBucketFor(1.5 * kPointsToLogicalPixels * 2.0)));
    require(peak * 20 < oneDocumentAtThisZoom,
            "Memory must be bounded by the budget, not proportional to 2,000 pages.");
    std::cout << "2000-page series: rendered " << rendered << " textures, peak "
              << peak / (1024 * 1024) << " MiB of " << budget / (1024 * 1024) << " MiB budget\n";
}

} // namespace

void testViewHistory() {
    ViewHistory history;
    require(!history.canGoBack() && !history.canGoForward(), "New history must be empty.");
    history.recordJump({0, 0.0});
    history.recordJump({9, 0.5});
    const ViewPosition at{40, 0.0};
    auto back = history.back(at);
    require(back && back->page == 9 && history.canGoForward(), "Back must return the last place before a jump.");
    auto again = history.back(*back);
    require(again && again->page == 0, "Back twice must reach the first place.");
    auto forward = history.forward(*again);
    require(forward && forward->page == 9, "Forward must undo Back.");
    history.recordJump(*forward);
    require(!history.canGoForward(), "A new jump must clear Forward.");
    ViewHistory dedupe;
    dedupe.recordJump({3, 0.10});
    dedupe.recordJump({3, 0.11});
    require(dedupe.back({50, 0.0}) && !dedupe.canGoBack(), "Nearby positions must collapse into one entry.");
    ViewHistory capped;
    for (std::size_t i = 0; i < ViewHistory::kMaxEntries + 20; ++i) capped.recordJump({i, 0.0});
    std::size_t steps = 0;
    ViewPosition cursor{100000, 0.0};
    while (auto p = capped.back(cursor)) { cursor = *p; ++steps; }
    require(steps == ViewHistory::kMaxEntries, "History must be capped.");
}

int main() {
    try {
        testRotationAndSizes();
        testContinuousLayout();
        testBandQueries();
        testFitZoom();
        testScaleBuckets();
        testPlanning();
        testSchedulerSupersedesStaleWork();
        testCacheBudget();
        testTwoThousandPageMemorySeries();
        testViewHistory();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "viewport tests passed\n";
    return 0;
}
