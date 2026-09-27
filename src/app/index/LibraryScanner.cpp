#include "app/index/LibraryScanner.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_set>
#include <utility>

namespace atlas::index {
namespace {

[[nodiscard]] bool isPdf(const std::filesystem::path& path) {
    auto extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return extension == ".pdf";
}

[[nodiscard]] std::string pathKey(const std::filesystem::path& path) {
    std::error_code error;
    auto normalized = std::filesystem::absolute(path, error);
    if (error) normalized = path;
    const auto utf8 = normalized.lexically_normal().generic_u8string();
    std::string key{reinterpret_cast<const char*>(utf8.data()), utf8.size()};
#ifdef _WIN32
    // Windows paths are case-insensitive for Atlas's supported local roots.
    // ASCII folding covers drive letters and the ordinary Latin path aliases
    // while preserving Arabic, Urdu and other non-cased scripts byte-for-byte.
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
#endif
    return key;
}

} // namespace

LibraryScanner::LibraryScanner(std::shared_ptr<const ILibraryEnumerator> enumerator, LibraryScanLimits limits)
    : enumerator_{std::move(enumerator)}, limits_{limits} {
    if (!enumerator_) throw std::invalid_argument("A library directory enumerator is required.");
    if (limits_.batchSize == 0 || limits_.maximumDepth == 0 || limits_.maximumDirectories == 0 || limits_.maximumFiles == 0) {
        throw std::invalid_argument("Library scan limits must be greater than zero.");
    }
}

LibraryScanSummary LibraryScanner::scan(
    const std::filesystem::path& root,
    const std::atomic_bool& cancelled,
    const BatchHandler& onBatch) const {
    if (root.empty() || !onBatch) throw std::invalid_argument("A root and batch callback are required for scanning.");

    LibraryScanSummary summary;
    std::vector<std::filesystem::path> pendingDirectories{root};
    std::vector<std::filesystem::path> batch;
    batch.reserve(limits_.batchSize);
    bool sawReadableDirectory = false;
    bool incomplete = false;
    bool hitLimit = false;

    const auto publish = [&] {
        if (!batch.empty()) {
            onBatch(batch, summary.progress);
            batch.clear();
        }
    };

    while (!pendingDirectories.empty()) {
        if (cancelled.load(std::memory_order_relaxed)) {
            publish();
            summary.outcome = LibraryScanOutcome::cancelled;
            return summary;
        }

        auto directory = std::move(pendingDirectories.back());
        pendingDirectories.pop_back();
        const auto relative = directory.lexically_relative(root);
        const auto depth = relative == std::filesystem::path{"."}
            ? std::size_t{0}
            : static_cast<std::size_t>(std::distance(relative.begin(), relative.end()));
        if (depth > limits_.maximumDepth) {
            incomplete = true;
            continue;
        }

        ++summary.progress.directoriesVisited;
        bool stopped = false;
        const auto listing = enumerator_->forEachEntry(directory, [&](const EnumeratedEntry& entry) {
            if (cancelled.load(std::memory_order_relaxed)) {
                stopped = true;
                return false;
            }
            switch (entry.kind) {
            case EnumeratedEntryKind::directory:
                if (depth >= limits_.maximumDepth) incomplete = true;
                else if (summary.progress.directoriesVisited + pendingDirectories.size() >= limits_.maximumDirectories) {
                    stopped = true;
                    hitLimit = true;
                    return false;
                } else pendingDirectories.push_back(entry.path);
                break;
            case EnumeratedEntryKind::regularFile:
                if (isPdf(entry.path)) {
                    if (summary.progress.pdfFilesFound >= limits_.maximumFiles) {
                        stopped = true;
                        return false;
                    }
                    batch.push_back(entry.path);
                    ++summary.progress.pdfFilesFound;
                    if (batch.size() >= limits_.batchSize) publish();
                }
                break;
            case EnumeratedEntryKind::inaccessible:
                ++summary.progress.inaccessibleEntries;
                incomplete = true;
                break;
            case EnumeratedEntryKind::symlink:
            case EnumeratedEntryKind::other:
                break;
            }
            return true;
        });

        sawReadableDirectory = sawReadableDirectory || listing.opened;
        if (!listing.opened || !listing.complete) incomplete = true;
        if (cancelled.load(std::memory_order_relaxed)) {
            publish();
            summary.outcome = LibraryScanOutcome::cancelled;
            return summary;
        }
        if (hitLimit || (stopped && summary.progress.pdfFilesFound >= limits_.maximumFiles)) {
            publish();
            summary.outcome = LibraryScanOutcome::limitReached;
            return summary;
        }
    }

    publish();
    if (!sawReadableDirectory) summary.outcome = LibraryScanOutcome::unavailable;
    else summary.outcome = incomplete ? LibraryScanOutcome::partial : LibraryScanOutcome::complete;
    return summary;
}

LibraryMultiScanSummary LibraryScanner::scanRoots(
    const std::vector<LibraryScanRoot>& roots,
    const std::atomic_bool& cancelled,
    const RootBatchHandler& onBatch) const {
    if (roots.empty() || !onBatch) {
        throw std::invalid_argument("At least one library root and a batch callback are required for scanning.");
    }

    std::unordered_set<std::string> rootIds;
    for (const auto& root : roots) {
        if (root.id.empty() || root.source.empty()) {
            throw std::invalid_argument("Every library scan root needs an ID and a source path.");
        }
        if (!rootIds.insert(root.id).second) {
            throw std::invalid_argument("Library scan root IDs must be unique.");
        }
    }

    LibraryMultiScanSummary result;
    result.roots.reserve(roots.size());
    std::unordered_set<std::string> publishedPaths;

    for (const auto& root : roots) {
        LibraryRootScanSummary rootResult;
        rootResult.root = root;
        rootResult.scan = scan(root.source, cancelled, [&](const auto& batch, const auto& progress) {
            std::vector<std::filesystem::path> uniqueBatch;
            uniqueBatch.reserve(batch.size());
            for (const auto& path : batch) {
                if (publishedPaths.insert(pathKey(path)).second) {
                    uniqueBatch.push_back(path);
                    ++result.uniquePdfFilesFound;
                } else {
                    ++rootResult.duplicatePdfFilesSkipped;
                    ++result.duplicatePdfFilesSkipped;
                }
            }
            if (!uniqueBatch.empty()) onBatch(root, uniqueBatch, progress);
        });
        result.roots.push_back(std::move(rootResult));
        if (result.roots.back().scan.outcome == LibraryScanOutcome::cancelled) break;
    }
    return result;
}

} // namespace atlas::index
