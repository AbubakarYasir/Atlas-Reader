#pragma once

#include "app/index/IFileIdentityProvider.h"
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
    std::size_t replacedLocationsFound{};
};

struct IntegratedLibraryScanSummary final {
    std::vector<IntegratedRootScanSummary> roots;
    std::size_t uniqueNewFilesFound{};
    std::size_t duplicateNewFilesSkipped{};
    std::size_t replacedLocationsFound{};
};

enum class DiscoveredLibraryFileKind {
    newPath,
    replacementAtKnownPath,
};

struct DiscoveredLibraryFile final {
    std::filesystem::path source;
    FileIdentityResult filesystemIdentity;
    DiscoveredLibraryFileKind kind{DiscoveredLibraryFileKind::newPath};
    std::string previousDocumentId;
};

// Coordinates filesystem observations with durable root scan state. Newly
// observed paths are published for later identity inspection; this service
// never creates a document ID, relinks a book, or deletes an indexed record.
class LibraryScanService final {
public:
    using NewFilesHandler = std::function<void(
        const LibraryScanRoot&,
        const std::vector<DiscoveredLibraryFile>&,
        const LibraryScanProgress&)>;

    LibraryScanService(
        const LibraryScanner& scanner,
        SqliteLibraryIndex& index,
        const IFileIdentityProvider& fileIdentityProvider);

    [[nodiscard]] IntegratedLibraryScanSummary scan(
        const std::vector<LibraryScanRoot>& roots,
        const std::atomic_bool& cancelled,
        std::int64_t completedAtUtcMs,
        const NewFilesHandler& onNewFiles) const;

private:
    const LibraryScanner& scanner_;
    SqliteLibraryIndex& index_;
    const IFileIdentityProvider& fileIdentityProvider_;
};

} // namespace atlas::index
