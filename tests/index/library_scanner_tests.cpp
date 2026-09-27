#include "app/index/LibraryScanner.h"
#include "app/index/NativeLibraryEnumerator.h"

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

} // namespace

int main() {
    try {
        checkStreamingPartialAndSymlinkPolicy();
        checkCancellationAndLimits();
        checkUnavailableAndDepthLimit();
        checkNativeFilesystemTraversal();
    } catch (const std::exception& error) {
        std::cerr << "Library scanner test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Library scanner bounded-batch, cancellation, Unicode-path, inaccessible-entry, symlink, limit and unavailable-root checks passed.\n";
}
