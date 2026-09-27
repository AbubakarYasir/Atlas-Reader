#include "app/index/LibraryIngestionService.h"

#include "app/index/SqliteLibraryIndex.h"

#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace atlas::index {
namespace {

struct PendingDiscovery final {
    LibraryScanRoot root;
    DiscoveredLibraryFile file;
};

[[nodiscard]] std::string candidateKey(
    const std::string& rootId,
    const std::filesystem::path& source) {
    const auto utf8 = source.generic_u8string();
    return rootId + '\n' + std::string{reinterpret_cast<const char*>(utf8.data()), utf8.size()};
}

} // namespace

LibraryIngestionService::LibraryIngestionService(
    const LibraryScanService& scanService,
    SqliteLibraryIndex& index,
    const ILibraryDocumentInspector& documentInspector,
    IdFactory idFactory)
    : scanService_{scanService},
      index_{index},
      documentInspector_{documentInspector},
      idFactory_{std::move(idFactory)} {
    if (!idFactory_) throw std::invalid_argument("Library ingestion needs a stable ID factory.");
}

LibraryIngestionSummary LibraryIngestionService::ingest(
    const std::vector<LibraryScanRoot>& roots,
    const std::atomic_bool& cancelled,
    std::int64_t completedAtUtcMs) const {
    LibraryIngestionSummary result;
    std::vector<PendingDiscovery> discoveries;
    result.scan = scanService_.scan(roots, cancelled, completedAtUtcMs,
        [&](const auto& root, const auto& batch, const auto&) {
            for (const auto& file : batch) discoveries.push_back({root, file});
        });

    std::map<std::string, std::int64_t> completedGenerations;
    for (const auto& root : result.scan.roots) {
        if (root.scan.outcome == LibraryScanOutcome::complete) {
            completedGenerations.emplace(root.root.id, root.generation);
        }
    }

    std::set<std::string> pendingCandidates;
    for (const auto& proposal : index_.reconciliationProposals()) {
        if (proposal.state == ReconciliationProposalState::pending) {
            pendingCandidates.insert(candidateKey(proposal.candidateRootId, proposal.candidateSource));
        }
    }

    for (const auto& discovery : discoveries) {
        if (cancelled.load()) break;
        const auto generation = completedGenerations.find(discovery.root.id);
        if (generation == completedGenerations.end()) continue;
        const auto key = candidateKey(discovery.root.id, discovery.file.source);
        if (pendingCandidates.contains(key)) {
            ++result.repeatedCandidatesSkipped;
            continue;
        }

        bool createDocument = discovery.file.kind == DiscoveredLibraryFileKind::newPath;
        if (discovery.file.kind == DiscoveredLibraryFileKind::replacementAtKnownPath) {
            const auto proposal = index_.proposeReconciliation(
                idFactory_(), discovery.file.previousDocumentId,
                discovery.root.id, discovery.file.source,
                discovery.root.id, discovery.file.source,
                generation->second, discovery.file.filesystemIdentity.identity);
            pendingCandidates.insert(key);
            ++result.reconciliationProposalsAdded;
            createDocument = proposal.decision == IdentityDecision::newDocument
                || proposal.decision == IdentityDecision::distinctCopy;
        } else if (discovery.file.filesystemIdentity.state == FileIdentityState::available
                   && !discovery.file.filesystemIdentity.identity.empty()) {
            const auto matches = index_.locationsByFilesystemIdentity(
                discovery.file.filesystemIdentity.identity);
            if (matches.size() == 1) {
                const auto& previous = matches.front();
                const auto proposal = index_.proposeReconciliation(
                    idFactory_(), previous.documentId, previous.rootId, previous.source,
                    discovery.root.id, discovery.file.source, generation->second,
                    discovery.file.filesystemIdentity.identity);
                pendingCandidates.insert(key);
                ++result.reconciliationProposalsAdded;
                createDocument = proposal.decision == IdentityDecision::newDocument
                    || proposal.decision == IdentityDecision::distinctCopy;
            }
        }

        if (!createDocument) continue;
        try {
            const auto inspected = documentInspector_.inspect(discovery.file.source);
            const auto documentId = idFactory_();
            index_.upsertRecord({
                documentId,
                inspected.source,
                inspected.title,
                inspected.author,
                inspected.availability,
            }, discovery.root.id);
            if (discovery.file.filesystemIdentity.state == FileIdentityState::available
                && !discovery.file.filesystemIdentity.identity.empty()) {
                index_.setLocationFilesystemIdentity(
                    documentId, discovery.root.id, discovery.file.source,
                    discovery.file.filesystemIdentity.identity);
            }
            ++result.documentsAdded;
        } catch (const std::exception&) {
            ++result.inspectionFailures;
        }
    }
    return result;
}

} // namespace atlas::index

