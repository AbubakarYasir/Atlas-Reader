#include "app/index/LibraryIngestionService.h"
#include "app/index/NativeFileIdentityProvider.h"
#include "app/index/NativeLibraryEnumerator.h"
#include "app/index/SqliteLibraryIndex.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

using namespace atlas::index;

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

class TemporaryDirectory final {
public:
    TemporaryDirectory() {
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path()
            / ("atlas-ingestion-test-" + std::to_string(nonce));
        std::filesystem::create_directory(path_);
    }
    ~TemporaryDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }
    [[nodiscard]] const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

class FixtureInspector final : public ILibraryDocumentInspector {
public:
    [[nodiscard]] LibraryDocumentInspection inspect(
        const std::filesystem::path& source) const override {
        return {
            source,
            source.stem().string(),
            "Fixture Author",
            atlas::document::Availability::available,
            std::filesystem::file_size(source),
            std::nullopt,
            1,
        };
    }
};

void writeFixture(const std::filesystem::path& path, std::string_view marker) {
    std::ofstream output{path, std::ios::binary | std::ios::trunc};
    output << "%PDF-1.4\n" << marker << '\n';
}

void checkNewMoveCopyAndReplacementFlow() {
    TemporaryDirectory temporary;
    const auto library = temporary.path() / "library";
    std::filesystem::create_directory(library);
    const auto original = library / "original.pdf";
    const auto renamed = library / "کتاب-renamed.pdf";
    const auto copied = library / "copy.pdf";
    writeFixture(original, "original");

    SqliteLibraryIndex index{temporary.path() / "profile.sqlite3"};
    index.registerRoot({"root", library, LibraryRootAvailability::available});
    auto enumerator = std::make_shared<NativeLibraryEnumerator>();
    LibraryScanner scanner{enumerator, {.batchSize = 2, .maximumDepth = 8, .maximumFiles = 20}};
    NativeFileIdentityProvider identities;
    LibraryScanService scanService{scanner, index, identities};
    FixtureInspector inspector;
    int nextId = 1;
    LibraryIngestionService ingestion{scanService, index, inspector, [&] {
        return "id-" + std::to_string(nextId++);
    }};
    std::atomic_bool cancelled{};

    auto summary = ingestion.ingest({{"root", library}}, cancelled, 1000);
    require(summary.documentsAdded == 1 && summary.reconciliationProposalsAdded == 0,
        "A newly discovered PDF must receive one stable Atlas document ID.");
    auto records = index.allRecords();
    require(records.size() == 1 && records.front().id == "id-1" && records.front().source == original,
        "The first ingestion must persist the inspected document and its original path.");

    std::filesystem::rename(original, renamed);
    summary = ingestion.ingest({{"root", library}}, cancelled, 2000);
    require(summary.documentsAdded == 0 && summary.reconciliationProposalsAdded == 1,
        "A native-identity rename must become a reviewable reconciliation proposal.");
    records = index.allRecords();
    require(records.size() == 1 && records.front().source == original,
        "A move proposal must not mutate the document before explicit application.");
    const auto moveProposal = index.reconciliationProposals().back();
    require(moveProposal.decision == IdentityDecision::confidentMove
            && moveProposal.candidateSource == renamed,
        "The rename proposal must identify a confident move to the Unicode path.");
    index.applyReconciliation(moveProposal.id);
    records = index.allRecords();
    require(records.front().id == "id-1" && records.front().source == renamed,
        "Applying the move must preserve the Atlas document ID at the renamed path.");

    std::filesystem::copy_file(renamed, copied);
    summary = ingestion.ingest({{"root", library}}, cancelled, 3000);
    require(summary.documentsAdded == 1 && index.allRecords().size() == 2,
        "A byte-identical copy with a different native identity must become a distinct document.");

    const auto replacement = library / "replacement.tmp";
    writeFixture(replacement, "replacement");
    std::filesystem::remove(renamed);
    std::filesystem::rename(replacement, renamed);
    summary = ingestion.ingest({{"root", library}}, cancelled, 4000);
    require(summary.reconciliationProposalsAdded == 1,
        "A different object at the same path must create a reconciliation proposal.");
    const auto proposals = index.reconciliationProposals();
    const auto& replacementProposal = proposals.back();
    require(replacementProposal.decision == IdentityDecision::ambiguous
            && !replacementProposal.mayRelinkAutomatically,
        "A same-path replacement must remain ambiguous and never auto-relink.");
    require(index.allRecords().size() == 2,
        "A replacement candidate must not overwrite or duplicate an indexed book automatically.");
}

} // namespace

int main() {
    try {
        checkNewMoveCopyAndReplacementFlow();
    } catch (const std::exception& error) {
        std::cerr << "Library ingestion test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Library ingestion new, move, copy, replacement and explicit-apply checks passed.\n";
}

