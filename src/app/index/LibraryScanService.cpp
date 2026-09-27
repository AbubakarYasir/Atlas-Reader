#include "app/index/LibraryScanService.h"

#include "app/index/LibraryPathKey.h"
#include "app/index/SqliteLibraryIndex.h"

#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace atlas::index {

LibraryScanService::LibraryScanService(const LibraryScanner& scanner, SqliteLibraryIndex& index)
    : scanner_{scanner}, index_{index} {}

IntegratedLibraryScanSummary LibraryScanService::scan(
    const std::vector<LibraryScanRoot>& roots,
    const std::atomic_bool& cancelled,
    std::int64_t completedAtUtcMs,
    const NewFilesHandler& onNewFiles) const {
    if (roots.empty() || !onNewFiles || completedAtUtcMs < 0) {
        throw std::invalid_argument("Library scanning needs roots, a callback and a nonnegative UTC timestamp.");
    }

    std::unordered_set<std::string> rootIds;
    for (const auto& root : roots) {
        if (root.id.empty() || root.source.empty() || !rootIds.insert(root.id).second) {
            throw std::invalid_argument("Every library root needs a unique ID and source path.");
        }
    }

    IntegratedLibraryScanSummary result;
    result.roots.reserve(roots.size());
    std::unordered_set<std::string> newPathsPublished;

    for (const auto& root : roots) {
        IntegratedRootScanSummary rootResult;
        rootResult.root = root;
        rootResult.generation = index_.beginRootScan(root.id);
        rootResult.scan = scanner_.scan(root.source, cancelled, [&](const auto& batch, const auto& progress) {
            std::vector<std::filesystem::path> newFiles;
            newFiles.reserve(batch.size());
            for (const auto& path : batch) {
                if (index_.markExistingLocationSeen(root.id, path, rootResult.generation)) {
                    ++rootResult.knownLocationsSeen;
                } else if (newPathsPublished.insert(libraryPathKey(path)).second) {
                    newFiles.push_back(path);
                    ++rootResult.newFilesFound;
                    ++result.uniqueNewFilesFound;
                } else {
                    ++rootResult.duplicateNewFilesSkipped;
                    ++result.duplicateNewFilesSkipped;
                }
            }
            if (!newFiles.empty()) onNewFiles(root, newFiles, progress);
        });

        switch (rootResult.scan.outcome) {
        case LibraryScanOutcome::complete:
            index_.finishRootScan(
                root.id, rootResult.generation, LibraryRootAvailability::available, completedAtUtcMs);
            break;
        case LibraryScanOutcome::partial:
        case LibraryScanOutcome::limitReached:
            index_.finishRootScan(
                root.id, rootResult.generation, LibraryRootAvailability::partiallyAvailable);
            break;
        case LibraryScanOutcome::unavailable:
            index_.finishRootScan(root.id, rootResult.generation, LibraryRootAvailability::offline);
            break;
        case LibraryScanOutcome::cancelled:
            break;
        }

        result.roots.push_back(std::move(rootResult));
        if (result.roots.back().scan.outcome == LibraryScanOutcome::cancelled) break;
    }
    return result;
}

} // namespace atlas::index
