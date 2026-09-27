#pragma once

#include "app/index/ILibraryEnumerator.h"

#include <atomic>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace atlas::index {

enum class LibraryScanOutcome { complete, partial, unavailable, cancelled, limitReached };

struct LibraryScanProgress final {
    std::size_t directoriesVisited{};
    std::size_t pdfFilesFound{};
    std::size_t inaccessibleEntries{};
};

struct LibraryScanLimits final {
    std::size_t batchSize{64};
    std::size_t maximumDepth{128};
    std::size_t maximumDirectories{100'000};
    std::size_t maximumFiles{1'000'000};
};

struct LibraryScanSummary final {
    LibraryScanOutcome outcome{LibraryScanOutcome::complete};
    LibraryScanProgress progress;
};

struct LibraryScanRoot final {
    std::string id;
    std::filesystem::path source;
};

struct LibraryRootScanSummary final {
    LibraryScanRoot root;
    LibraryScanSummary scan;
    std::size_t duplicatePdfFilesSkipped{};
};

struct LibraryMultiScanSummary final {
    std::vector<LibraryRootScanSummary> roots;
    std::size_t uniquePdfFilesFound{};
    std::size_t duplicatePdfFilesSkipped{};
};

// Enumerates paths only. It never mutates the index or infers removals.
class LibraryScanner final {
public:
    using BatchHandler = std::function<void(const std::vector<std::filesystem::path>&, const LibraryScanProgress&)>;
    using RootBatchHandler = std::function<void(
        const LibraryScanRoot&,
        const std::vector<std::filesystem::path>&,
        const LibraryScanProgress&)>;

    explicit LibraryScanner(std::shared_ptr<const ILibraryEnumerator> enumerator,
                            LibraryScanLimits limits = {});

    [[nodiscard]] LibraryScanSummary scan(
        const std::filesystem::path& root,
        const std::atomic_bool& cancelled,
        const BatchHandler& onBatch) const;

    // Scans every configured root independently and publishes each physical
    // PDF path at most once, even when roots overlap. Root outcomes remain
    // separate so one unavailable root cannot invalidate another root.
    [[nodiscard]] LibraryMultiScanSummary scanRoots(
        const std::vector<LibraryScanRoot>& roots,
        const std::atomic_bool& cancelled,
        const RootBatchHandler& onBatch) const;

private:
    std::shared_ptr<const ILibraryEnumerator> enumerator_;
    LibraryScanLimits limits_;
};

} // namespace atlas::index
