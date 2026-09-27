#pragma once

#include "app/index/LibraryScanner.h"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace atlas::index {

class SqliteLibraryIndex;

struct IntegratedRootScanSummary final {
    LibraryScanRoot root;
    std::int64_t generation{};
    LibraryScanSummary scan;
    std::size_t knownLocationsSeen{};
    std::size_t newFilesFound{};
    std::size_t duplicateNewFilesSkipped{};
};

struct IntegratedLibraryScanSummary final {
    std::vector<IntegratedRootScanSummary> roots;
    std::size_t uniqueNewFilesFound{};
    std::size_t duplicateNewFilesSkipped{};
};

// Coordinates filesystem observations with durable root scan state. Newly
// observed paths are published for later identity inspection; this service
// never creates a document ID, relinks a book, or deletes an indexed record.
class LibraryScanService final {
public:
    using NewFilesHandler = std::function<void(
        const LibraryScanRoot&,
        const std::vector<std::filesystem::path>&,
        const LibraryScanProgress&)>;

    LibraryScanService(const LibraryScanner& scanner, SqliteLibraryIndex& index);

    [[nodiscard]] IntegratedLibraryScanSummary scan(
        const std::vector<LibraryScanRoot>& roots,
        const std::atomic_bool& cancelled,
        std::int64_t completedAtUtcMs,
        const NewFilesHandler& onNewFiles) const;

private:
    const LibraryScanner& scanner_;
    SqliteLibraryIndex& index_;
};

} // namespace atlas::index
