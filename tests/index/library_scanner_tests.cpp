#include "app/index/LibraryScanner.h"
#include "app/index/LibraryScanService.h"
#include "app/index/NativeLibraryEnumerator.h"
#include "app/index/SqliteLibraryIndex.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using namespace atlas::index;

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

class FixtureEnumerator final : public ILibraryEnumerator {
public:
    struct Directory final {
        bool opened{true};
        bool complete{true};
        std::vector<EnumeratedEntry> entries;
    };

    std::map<std::filesystem::path, Directory> directories;

    [[nodiscard]] DirectoryEnumeration forEachEntry(
        const std::filesystem::path& directory,
        const std::function<bool(const EnumeratedEntry&)>& visitor) const override {
        const auto found = directories.find(directory);
        if (found == directories.end()) return {false, false};
        for (const auto& entry : found->second.entries) {
            if (!visitor(entry)) return {found->second.opened, false};
        }
        return {found->second.opened, found->second.complete};
    }
};

class FixtureFileIdentityProvider final : public IFileIdentityProvider {
public:
    std::map<std::filesystem::path, std::string> identities;

    [[nodiscard]] FileIdentityResult inspect(const std::filesystem::path& source) const override {
        const auto found = identities.find(source);
        return {
            FileIdentityState::available,
            found == identities.end() ? "fixture:" + source.generic_string() : found->second,
        };
    }
};

void checkStreamingPartialAndSymlinkPolicy() {
    auto fixture = std::make_shared<FixtureEnumerator>();
    fixture->directories["root"] = {true, true, {
        {"root/one.PDF", EnumeratedEntryKind::regularFile},
        {"root/notes.txt", EnumeratedEntryKind::regularFile},
        {"root/sub", EnumeratedEntryKind::directory},
        {"root/linked", EnumeratedEntryKind::symlink},
        {"root/denied", EnumeratedEntryKind::inaccessible},
        {"root/two.pdf", EnumeratedEntryKind::regularFile},
    }};
    fixture->directories["root/sub"] = {true, false, {
        {"root/sub/کتاب.pdf", EnumeratedEntryKind::regularFile},
    }};

    LibraryScanner scanner{fixture, {.batchSize = 2, .maximumDepth = 8, .maximumFiles = 20}};
    std::atomic_bool cancelled{};
    std::vector<std::filesystem::path> found;
    std::vector<std::size_t> batchSizes;
    const auto summary = scanner.scan("root", cancelled, [&](const auto& batch, const auto&) {
        batchSizes.push_back(batch.size());
        require(batch.size() <= 2, "A published batch must obey its configured bound.");
        found.insert(found.end(), batch.begin(), batch.end());
    });

    require(summary.outcome == LibraryScanOutcome::partial, "An inaccessible entry or incomplete directory must produce a partial result.");
    require(summary.progress.directoriesVisited == 2, "Nested directories must be visited without following symlinks.");
    require(summary.progress.inaccessibleEntries == 1, "An inaccessible child must be isolated and counted.");
    require(found.size() == 3, "Only PDF files, including Unicode paths, should be emitted.");
    require(batchSizes.size() == 2 && batchSizes.front() == 2 && batchSizes.back() == 1,
        "Results must stream in bounded batches, including the final partial batch.");
    require(std::find(found.begin(), found.end(), std::filesystem::path{"root/linked"}) == found.end(),
        "A symlink entry must not be traversed or indexed.");
}

void checkCancellationAndLimits() {
    auto fixture = std::make_shared<FixtureEnumerator>();
    fixture->directories["root"] = {true, true, {
        {"root/a.pdf", EnumeratedEntryKind::regularFile},
        {"root/b.pdf", EnumeratedEntryKind::regularFile},
        {"root/c.pdf", EnumeratedEntryKind::regularFile},
        {"root/d.pdf", EnumeratedEntryKind::regularFile},
    }};

    LibraryScanner scanner{fixture, {.batchSize = 2, .maximumDepth = 8, .maximumFiles = 3}};
    std::atomic_bool cancelled{};
    const auto limited = scanner.scan("root", cancelled, [](const auto&, const auto&) {});
    require(limited.outcome == LibraryScanOutcome::limitReached && limited.progress.pdfFilesFound == 3,
        "A configured file ceiling must stop scanning explicitly rather than report a complete root.");

    cancelled.store(false);
    std::size_t delivered{};
    const auto stopped = scanner.scan("root", cancelled, [&](const auto&, const auto&) {
        ++delivered;
        cancelled.store(true);
    });
    require(stopped.outcome == LibraryScanOutcome::cancelled, "Cancellation must be reported distinctly.");
    require(delivered == 1 && stopped.progress.pdfFilesFound == 2,
        "Cancellation must stop after the current bounded batch.");
}

void checkUnavailableAndDepthLimit() {
    auto fixture = std::make_shared<FixtureEnumerator>();
    fixture->directories["missing"] = {false, false, {}};
    fixture->directories["root"] = {true, true, {{"root/sub", EnumeratedEntryKind::directory}}};
    fixture->directories["root/sub"] = {true, true, {{"root/sub/deeper", EnumeratedEntryKind::directory}}};
    LibraryScanner scanner{fixture, {.batchSize = 4, .maximumDepth = 1, .maximumFiles = 10}};
    std::atomic_bool cancelled{};
    const auto unavailable = scanner.scan("missing", cancelled, [](const auto&, const auto&) {});
    require(unavailable.outcome == LibraryScanOutcome::unavailable, "An unreadable root must be reported unavailable.");
    const auto depthLimited = scanner.scan("root", cancelled, [](const auto&, const auto&) {});
    require(depthLimited.outcome == LibraryScanOutcome::partial, "Directories beyond the configured depth must not be traversed silently.");
    require(depthLimited.progress.directoriesVisited == 2, "Depth limits must prevent directories beyond the configured depth from being visited.");

    LibraryScanner directoryLimited{fixture, {.batchSize = 4, .maximumDepth = 8, .maximumDirectories = 1, .maximumFiles = 10}};
    const auto boundedQueue = directoryLimited.scan("root", cancelled, [](const auto&, const auto&) {});
    require(boundedQueue.outcome == LibraryScanOutcome::limitReached && boundedQueue.progress.directoriesVisited == 1,
        "The pending directory stack must have a configured cap.");
}

void checkNativeFilesystemTraversal() {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto root = std::filesystem::temp_directory_path() / ("atlas-scan-test-" + std::to_string(nonce)) / "مكتبتي";
    std::error_code error;
    std::filesystem::create_directories(root / "nested", error);
    require(!error, "The native scanner fixture directory must be created.");
    struct Cleanup final {
        std::filesystem::path path;
        ~Cleanup() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
    } cleanup{root.parent_path()};
    std::ofstream{root / "nested" / "كتاب.PDF"}.put('x');
    std::ofstream{root / "nested" / "notes.txt"}.put('x');

    auto enumerator = std::make_shared<NativeLibraryEnumerator>();
    LibraryScanner scanner{enumerator};
    std::atomic_bool cancelled{};
    std::vector<std::filesystem::path> found;
    const auto summary = scanner.scan(root, cancelled, [&](const auto& batch, const auto&) {
        found.insert(found.end(), batch.begin(), batch.end());
    });
    require(summary.outcome == LibraryScanOutcome::complete && found.size() == 1,
        "The native filesystem adapter must discover nested case-insensitive PDF extensions only.");
    require(found.front() == root / "nested" / "كتاب.PDF", "Unicode PDF paths must be preserved exactly.");
}

void checkOverlappingRootsPublishEachPdfOnce() {
    auto fixture = std::make_shared<FixtureEnumerator>();
    fixture->directories["root"] = {true, true, {
        {"root/top.pdf", EnumeratedEntryKind::regularFile},
        {"root/sub", EnumeratedEntryKind::directory},
    }};
    fixture->directories["root/sub"] = {true, true, {
        {"root/sub/shared.pdf", EnumeratedEntryKind::regularFile},
    }};
    fixture->directories["offline"] = {false, false, {}};

    LibraryScanner scanner{fixture, {.batchSize = 2, .maximumDepth = 8, .maximumFiles = 20}};
    std::atomic_bool cancelled{};
    std::vector<std::filesystem::path> found;
    std::vector<std::string> publishingRoots;
    const auto summary = scanner.scanRoots({
        {"parent", "root"},
        {"nested", "root/sub"},
        {"offline", "offline"},
    }, cancelled, [&](const auto& root, const auto& batch, const auto&) {
        publishingRoots.push_back(root.id);
        found.insert(found.end(), batch.begin(), batch.end());
    });

    require(summary.roots.size() == 3, "Every configured root must retain an independent scan outcome.");
    require(summary.roots[0].scan.outcome == LibraryScanOutcome::complete
            && summary.roots[1].scan.outcome == LibraryScanOutcome::complete
            && summary.roots[2].scan.outcome == LibraryScanOutcome::unavailable,
        "An unavailable root must not invalidate successful overlapping roots.");
    require(summary.uniquePdfFilesFound == 2 && summary.duplicatePdfFilesSkipped == 1,
        "Nested roots must publish each physical PDF path only once.");
    require(summary.roots[1].duplicatePdfFilesSkipped == 1,
        "The nested root must report the PDF suppressed by cross-root deduplication.");
    require(found.size() == 2
            && std::count(found.begin(), found.end(), std::filesystem::path{"root/sub/shared.pdf"}) == 1,
        "The overlapping PDF must appear exactly once in published results.");
    require(publishingRoots.size() == 1 && publishingRoots.front() == "parent",
        "A root whose entire batch is duplicate must not emit an empty batch.");

    bool duplicateIdRejected = false;
    try {
        static_cast<void>(scanner.scanRoots({{"same", "root"}, {"same", "root/sub"}}, cancelled,
            [](const auto&, const auto&, const auto&) {}));
    } catch (const std::invalid_argument&) {
        duplicateIdRejected = true;
    }
    require(duplicateIdRejected, "Configured root IDs must be unique before any scan begins.");
}

void checkScanServicePreservesIdentityBoundariesAndRootState() {
    auto fixture = std::make_shared<FixtureEnumerator>();
    fixture->directories["root"] = {true, true, {
        {"root/known.pdf", EnumeratedEntryKind::regularFile},
        {"root/new.pdf", EnumeratedEntryKind::regularFile},
        {"root/sub", EnumeratedEntryKind::directory},
    }};
    fixture->directories["root/sub"] = {true, true, {
        {"root/sub/shared.pdf", EnumeratedEntryKind::regularFile},
    }};
    fixture->directories["offline"] = {false, false, {}};
    fixture->directories["partial"] = {true, false, {
        {"partial/كتاب.pdf", EnumeratedEntryKind::regularFile},
    }};

    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto temporary = std::filesystem::temp_directory_path()
        / ("atlas-scan-service-test-" + std::to_string(nonce));
    std::error_code error;
    std::filesystem::create_directory(temporary, error);
    require(!error, "The scan-service database directory must be created.");
    struct Cleanup final {
        std::filesystem::path path;
        ~Cleanup() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
    } cleanup{temporary};

    SqliteLibraryIndex index{temporary / "profile.sqlite3"};
    index.registerRoot({"parent", "root", LibraryRootAvailability::available});
    index.registerRoot({"nested", "root/sub", LibraryRootAvailability::available});
    index.registerRoot({"offline", "offline", LibraryRootAvailability::available});
    index.registerRoot({"partial", "partial", LibraryRootAvailability::available});
    index.upsertRecord({"known", "root/known.pdf", "Known Book", "", atlas::document::Availability::available}, "parent");

    LibraryScanner scanner{fixture, {.batchSize = 2, .maximumDepth = 8, .maximumFiles = 20}};
    FixtureFileIdentityProvider identities;
    LibraryScanService service{scanner, index, identities};
    std::atomic_bool cancelled{};
    std::vector<std::filesystem::path> newFiles;
    const auto summary = service.scan({
        {"parent", "root"},
        {"nested", "root/sub"},
        {"offline", "offline"},
        {"partial", "partial"},
    }, cancelled, 5000, [&](const auto&, const auto& batch, const auto&) {
        for (const auto& discovered : batch) newFiles.push_back(discovered.source);
    });

    require(summary.roots.size() == 4 && summary.roots[0].knownLocationsSeen == 1,
        "The integrated scan must recognize an existing location without creating a new identity.");
    require(summary.uniqueNewFilesFound == 3 && summary.duplicateNewFilesSkipped == 1,
        "New paths must stream once across overlapping roots while remaining unlinked.");
    require(newFiles.size() == 3
            && std::count(newFiles.begin(), newFiles.end(), std::filesystem::path{"root/sub/shared.pdf"}) == 1,
        "The new-file callback must receive bounded deduplicated discoveries.");
    require(index.search("Known").size() == 1 && index.search("new").empty(),
        "Scanning must preserve known books and must not invent document IDs for new paths.");

    const auto roots = index.roots();
    const auto findRoot = [&](std::string_view id) -> const LibraryRootRecord& {
        const auto found = std::find_if(roots.begin(), roots.end(), [&](const auto& root) { return root.id == id; });
        require(found != roots.end(), "The expected scan-service root must remain persisted.");
        return *found;
    };
    require(findRoot("parent").availability == LibraryRootAvailability::available
            && findRoot("parent").lastCompletedScanUtcMs == 5000,
        "A complete root must record its trusted completion time.");
    require(findRoot("nested").availability == LibraryRootAvailability::available
            && findRoot("nested").lastCompletedScanUtcMs == 5000,
        "A complete overlapping root must retain an independent successful outcome.");
    require(findRoot("offline").availability == LibraryRootAvailability::offline
            && !findRoot("offline").lastCompletedScanUtcMs,
        "An unavailable root must become offline without receiving a completion time.");
    require(findRoot("partial").availability == LibraryRootAvailability::partiallyAvailable
            && !findRoot("partial").lastCompletedScanUtcMs,
        "An incomplete root must become partial without receiving a completion time.");

    cancelled.store(false);
    const auto cancelledSummary = service.scan({{"parent", "root"}}, cancelled, 6000,
        [&](const auto&, const auto&, const auto&) { cancelled.store(true); });
    require(cancelledSummary.roots.front().scan.outcome == LibraryScanOutcome::cancelled,
        "Cancellation during discovery publication must remain distinct from a completed scan.");
    const auto afterCancellation = index.roots();
    const auto parent = std::find_if(afterCancellation.begin(), afterCancellation.end(),
        [](const auto& root) { return root.id == "parent"; });
    require(parent != afterCancellation.end()
            && parent->scanGeneration == 2
            && parent->availability == LibraryRootAvailability::available
            && parent->lastCompletedScanUtcMs == 5000,
        "A cancelled scan must advance its generation without replacing the last trusted root outcome.");
    require(index.search("Known").size() == 1,
        "Cancellation must never remove an existing indexed book.");

    cancelled.store(false);
    identities.identities["root/known.pdf"] = "fixture:replacement-object";
    std::vector<DiscoveredLibraryFile> replacements;
    const auto replacementSummary = service.scan({{"parent", "root"}}, cancelled, 7000,
        [&](const auto&, const auto& batch, const auto&) {
            replacements.insert(replacements.end(), batch.begin(), batch.end());
        });
    const auto replacement = std::find_if(replacements.begin(), replacements.end(), [](const auto& item) {
        return item.source == std::filesystem::path{"root/known.pdf"};
    });
    require(replacementSummary.replacedLocationsFound == 1
            && replacement != replacements.end()
            && replacement->kind == DiscoveredLibraryFileKind::replacementAtKnownPath
            && replacement->previousDocumentId == "known",
        "A different filesystem object at a known path must be published as a replacement candidate.");
    require(index.search("Known").front().source == std::filesystem::path{"root/known.pdf"},
        "Detecting a same-path replacement must not silently reassign or delete the preceding book.");
}

} // namespace

int main() {
    try {
        checkStreamingPartialAndSymlinkPolicy();
        checkCancellationAndLimits();
        checkUnavailableAndDepthLimit();
        checkNativeFilesystemTraversal();
        checkOverlappingRootsPublishEachPdfOnce();
        checkScanServicePreservesIdentityBoundariesAndRootState();
    } catch (const std::exception& error) {
        std::cerr << "Library scanner test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Library scanner bounded-batch, cancellation, Unicode-path, inaccessible-entry, symlink, limit, overlap-deduplication and unavailable-root checks passed.\n";
}
