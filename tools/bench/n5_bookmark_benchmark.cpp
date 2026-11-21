// N5 bookmark benchmark: a 10,000-node outline (Arabic and English, 4 levels)
// through the real profile database, editor and search index. Prints
// p50/p95/p99 and fails (exit 1) when a gate in docs/QUALITY.md §5 is missed:
//   - 10,000-bookmark search < 100 ms p95 (gate)
//   - an editor command on the UI thread < 2 ms p95 (GUI callback budget)
// Storage writes run on a worker thread in the app; they are reported, not gated.
//
// usage: atlas_n5_bookmark_benchmark [work-directory]

#include "app/bookmarks/SqliteBookmarkOverlay.h"
#include "app/index/SqliteLibraryIndex.h"
#include "core/bookmarks/BookmarkEditor.h"
#include "core/bookmarks/BookmarkSearch.h"
#include "core/bookmarks/LocationEnvelope.h"
#include "core/bookmarks/Reconcile.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

namespace {

using namespace atlas::bookmarks;
using Clock = std::chrono::steady_clock;
namespace fs = std::filesystem;

struct Series final {
    std::string name;
    std::vector<double> ms;
    double gate{0.0}; // 0 = reported only

    [[nodiscard]] double percentile(double fraction) const {
        auto sorted = ms;
        std::sort(sorted.begin(), sorted.end());
        if (sorted.empty()) return 0.0;
        return sorted[static_cast<std::size_t>(static_cast<double>(sorted.size() - 1) * fraction)];
    }
};

template <typename Function>
double timed(Function&& function) {
    const auto start = Clock::now();
    function();
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

std::string at(std::uint32_t page, double top) {
    LocationEnvelope envelope;
    envelope.pdf.pageIndex = page;
    envelope.pdf.kind = DestinationKind::xyz;
    envelope.pdf.left = 0.0;
    envelope.pdf.top = top;
    return encodeLocation(envelope);
}

// 10 parts × 10 chapters × 10 sections × 9 subsections + headers = 10,000 nodes.
BaseSnapshot largeOutline() {
    BaseSnapshot base;
    base.sourceRevision = "size=104857600;mtime=1";
    const char* arabic[] = {"الباب", "الفصل", "المبحث", "المطلب"};
    const char* english[] = {"Part", "Chapter", "Section", "Topic"};
    std::uint32_t page = 0;
    std::size_t count = 0;
    std::function<void(const std::optional<std::string>&, int, int)> add =
        [&](const std::optional<std::string>& parent, int depth, int children) {
            for (int i = 0; i < children && count < 10'000; ++i) {
                BaseNode node;
                node.key = (parent ? *parent + "/" : std::string{}) + std::to_string(i);
                node.parentKey = parent;
                node.position = static_cast<std::uint32_t>(i);
                node.title = std::string{(i % 2 == 0) ? arabic[depth] : english[depth]} + " " + std::to_string(i + 1);
                node.location = at(page++ % 2000, 0.25);
                base.nodes.push_back(node);
                ++count;
                if (depth < 3) add(node.key, depth + 1, depth == 2 ? 9 : 10);
            }
        };
    add(std::nullopt, 0, 10);
    // Top up to exactly 10,000 with top-level appendices.
    for (std::uint32_t i = 10; count < 10'000; ++i, ++count) {
        base.nodes.push_back({std::to_string(i), std::nullopt, i, "Appendix " + std::to_string(i), at(i, 0.0)});
    }
    return base;
}

void print(const Series& series, bool& failed) {
    const auto p95 = series.percentile(0.95);
    const bool over = series.gate > 0.0 && p95 >= series.gate;
    failed = failed || over;
    std::cout << std::left << std::setw(34) << series.name << std::right << std::fixed << std::setprecision(3)
              << std::setw(10) << series.percentile(0.50) << std::setw(10) << p95 << std::setw(10)
              << series.percentile(0.99) << std::setw(7) << series.ms.size();
    if (series.gate > 0.0) std::cout << "   gate < " << series.gate << " ms: " << (over ? "OVER" : "PASS");
    std::cout << '\n';
}

} // namespace

int main(int argc, char** argv) {
    const fs::path work = argc > 1 ? fs::path{argv[1]} : fs::temp_directory_path() / "atlas-n5-benchmark";
    std::error_code error;
    fs::remove_all(work, error);
    fs::create_directories(work);
    const auto database = work / "profile.sqlite3";

    {
        atlas::index::SqliteLibraryIndex index{database};
        index.registerRoot({"root", work, atlas::index::LibraryRootAvailability::available, 0, std::nullopt, 0});
        index.upsertRecord({"doc", work / "large.pdf", "Large", "", atlas::document::Availability::available}, "root");
    }
    SqliteBookmarkOverlay store{database};
    const auto base = largeOutline();
    std::vector<Series> results;

    results.push_back({"adopt 10,000-node outline (store)", {timed([&] { (void)store.adoptBase("doc", base); })}});

    OverlaySnapshot snapshot;
    Series load{"load overlay (store)", {}};
    for (int i = 0; i < 20; ++i) load.ms.push_back(timed([&] { snapshot = store.load("doc"); }));
    results.push_back(load);
    if (snapshot.nodes.size() != 10'000) {
        std::cerr << "Expected 10,000 nodes, got " << snapshot.nodes.size() << '\n';
        return 2;
    }

    Series reconcileSeries{"reconcile 10,000 bookmarks", {}};
    auto external = base;
    external.sourceRevision = "external";
    external.nodes.back().title += " updated";
    for (int i = 0; i < 20; ++i) {
        reconcileSeries.ms.push_back(timed([&] {
            const auto result = reconcile("doc", snapshot.nodes, base, external);
            if (!result.refusal.empty() || result.nodes.size() != 10000)
                throw std::runtime_error("Large reconciliation did not preserve the tree.");
        }));
    }
    results.push_back(reconcileSeries);

    std::unique_ptr<BookmarkEditor> editor;
    results.push_back({"build editor", {timed([&] {
                           editor = std::make_unique<BookmarkEditor>("doc", snapshot.nodes, snapshot.base,
                                                                     snapshot.revision);
                       })}});

    std::unique_ptr<BookmarkSearchIndex> search;
    Series indexBuild{"build search index", {}};
    for (int i = 0; i < 10; ++i) {
        indexBuild.ms.push_back(timed([&] { search = std::make_unique<BookmarkSearchIndex>(editor->inOrder()); }));
    }
    results.push_back(indexBuild);

    Series query{"search 10,000 bookmarks", {}, 100.0};
    const std::vector<std::string> queries{"ا", "الفصل", "المطلب 9", "Section", "topic 7", "appendix", "zzz-none",
                                           "الْفَصْلُ", "Chapter 1", "ب"};
    for (int round = 0; round < 20; ++round) {
        for (const auto& text : queries) query.ms.push_back(timed([&] { (void)search->find(text); }));
    }
    results.push_back(query);

    // Editor commands (UI thread) and their writes (worker thread in the app).
    Series command{"editor command (UI thread)", {}, 2.0};
    Series write{"write change (store, worker)", {}};
    Series undoCommand{"editor undo (UI thread)", {}, 2.0};
    const auto run = [&](Series& ui, const std::function<EditResult()>& action) {
        EditResult result;
        ui.ms.push_back(timed([&] { result = action(); }));
        if (!result.ok) throw std::runtime_error("Benchmark command refused: " + result.reason);
        write.ms.push_back(timed([&] {
            editor->confirm(store.apply("doc", editor->revision(),
                                        {result.change.upserts, result.change.removals, result.change.restores}));
        }));
    };
    const auto order = editor->inOrder();
    std::vector<std::string> ids;
    ids.reserve(order.size());
    for (const auto* node : order) ids.push_back(node->id);
    for (int i = 0; i < 100; ++i) {
        const auto& target = ids[static_cast<std::size_t>(i * 97) % ids.size()];
        run(command, [&] { return editor->rename(target, "عنوان جديد " + std::to_string(i)); });
        run(command, [&] { return editor->create(std::nullopt, 5, "Local note " + std::to_string(i), at(7, 0.5)); });
        const auto& mover = ids[static_cast<std::size_t>(i * 131 + 7) % ids.size()];
        const auto* moving = editor->find(mover);
        if (moving != nullptr && moving->state != StorageState::locallyHidden) {
            run(command, [&] { return editor->move(mover, std::nullopt, 0); });
        }
        run(undoCommand, [&] { return editor->undo(); });
    }
    results.push_back(command);
    results.push_back(undoCommand);
    results.push_back(write);

    std::cout << "N5 bookmark benchmark: 10,000-node outline, " << std::thread::hardware_concurrency()
              << " hardware threads\n";
    std::cout << std::left << std::setw(34) << "operation" << std::right << std::setw(10) << "p50 ms" << std::setw(10)
              << "p95 ms" << std::setw(10) << "p99 ms" << std::setw(7) << "n" << '\n';
    bool failed = false;
    for (const auto& series : results) print(series, failed);
    fs::remove_all(work, error);
    if (failed) {
        std::cout << "N5 bookmark benchmark: a gate was missed.\n";
        return 1;
    }
    std::cout << "N5 bookmark benchmark: all gates PASS.\n";
    return 0;
}
