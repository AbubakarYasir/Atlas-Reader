#pragma once

#include "app/index/ILibraryEnumerator.h"

#include <atomic>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
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

// Enumerates paths only. It never mutates the index or infers removals.
class LibraryScanner final {
public:
    using BatchHandler = std::function<void(const std::vector<std::filesystem::path>&, const LibraryScanProgress&)>;

    explicit LibraryScanner(std::shared_ptr<const ILibraryEnumerator> enumerator,
                            LibraryScanLimits limits = {});

    [[nodiscard]] LibraryScanSummary scan(
        const std::filesystem::path& root,
        const std::atomic_bool& cancelled,
        const BatchHandler& onBatch) const;

private:
    std::shared_ptr<const ILibraryEnumerator> enumerator_;
    LibraryScanLimits limits_;
};

} // namespace atlas::index
