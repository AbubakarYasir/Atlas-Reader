#include "app/index/LibraryScanner.h"
#include "app/index/NativeLibraryEnumerator.h"
#include "app/index/SqliteLibraryIndex.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

[[nodiscard]] std::string utf8(const std::filesystem::path& value) {
    const auto bytes = value.generic_u8string();
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

[[nodiscard]] std::int64_t milliseconds(Clock::time_point start, Clock::time_point finish) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
}

[[nodiscard]] std::int64_t percentile(std::vector<std::int64_t> values, double fraction) {
    if (values.empty()) return 0;
    std::ranges::sort(values);
    const auto index = static_cast<std::size_t>((values.size() - 1) * fraction);
    return values[index];
}

int run(const std::filesystem::path& root, const std::filesystem::path& database, std::string query) {
    if (!std::filesystem::is_directory(root)) {
        std::cerr << "Benchmark root must be an existing directory.\n";
        return 2;
    }
    std::error_code error;
    std::filesystem::remove(database, error);

    atlas::index::SqliteLibraryIndex index{database};
    index.registerRoot({"benchmark-root", root, atlas::index::LibraryRootAvailability::available});
    auto enumerator = std::make_shared<atlas::index::NativeLibraryEnumerator>();
    atlas::index::LibraryScanner scanner{enumerator};
    std::atomic_bool cancelled{};
    std::vector<std::filesystem::path> files;

    const auto scanStart = Clock::now();
    const auto summary = scanner.scan(root, cancelled, [&](const auto& batch, const auto&) {
        files.insert(files.end(), batch.begin(), batch.end());
    });
    const auto scanFinish = Clock::now();
    if (summary.outcome != atlas::index::LibraryScanOutcome::complete) {
        std::cerr << "Benchmark scan did not complete safely.\n";
        return 3;
    }

    const auto indexStart = Clock::now();
    std::size_t ordinal{};
    for (const auto& file : files) {
        index.upsertRecord({
            "benchmark-" + std::to_string(++ordinal),
            file,
            utf8(file.stem()),
            "Atlas benchmark",
            atlas::document::Availability::available,
        }, "benchmark-root");
    }
    const auto indexFinish = Clock::now();

    if (query.empty() && !files.empty()) query = utf8(files.front().stem());
    for (int warmup = 0; warmup < 20; ++warmup) static_cast<void>(index.search(query));
    std::vector<std::int64_t> queryMicroseconds;
    queryMicroseconds.reserve(200);
    std::size_t resultCount{};
    for (int sample = 0; sample < 200; ++sample) {
        const auto start = Clock::now();
        const auto results = index.search(query);
        const auto finish = Clock::now();
        resultCount = results.size();
        queryMicroseconds.push_back(
            std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count());
    }

    std::cout << "{\n"
              << "  \"schema\": 1,\n"
              << "  \"files\": " << files.size() << ",\n"
              << "  \"scanMilliseconds\": " << milliseconds(scanStart, scanFinish) << ",\n"
              << "  \"indexMilliseconds\": " << milliseconds(indexStart, indexFinish) << ",\n"
              << "  \"querySamples\": " << queryMicroseconds.size() << ",\n"
              << "  \"queryResultCount\": " << resultCount << ",\n"
              << "  \"queryP50Microseconds\": " << percentile(queryMicroseconds, 0.50) << ",\n"
              << "  \"queryP95Microseconds\": " << percentile(queryMicroseconds, 0.95) << "\n"
              << "}\n";
    return 0;
}

} // namespace

#ifdef _WIN32
int wmain(int argc, wchar_t* argv[]) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: atlas_n3_library_benchmark <library-root> <temporary-database> [query]\n";
        return 1;
    }
    std::string query;
    if (argc == 4) {
        const std::filesystem::path queryPath{argv[3]};
        query = utf8(queryPath);
    }
    return run(argv[1], argv[2], std::move(query));
}
#else
int main(int argc, char* argv[]) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: atlas_n3_library_benchmark <library-root> <temporary-database> [query]\n";
        return 1;
    }
    return run(argv[1], argv[2], argc == 4 ? argv[3] : std::string{});
}
#endif
