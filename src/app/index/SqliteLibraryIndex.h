#pragma once

#include "core/index/ILibraryIndex.h"
#include "core/index/DocumentIdentityReconciler.h"

#include <filesystem>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace atlas::index {

enum class LibraryRootAvailability {
    available,
    offline,
    partiallyAvailable,
    permissionDenied,
};

struct LibraryRootRecord final {
    std::string id;
    std::filesystem::path source;
    LibraryRootAvailability availability{LibraryRootAvailability::available};
    std::int64_t scanGeneration{};
    std::optional<std::int64_t> lastCompletedScanUtcMs;
    std::int64_t lastCompletedScanGeneration{};
};

enum class ReconciliationProposalState {
    pending,
    applied,
    dismissed,
};

struct ReconciliationProposalRecord final {
    std::string id;
    std::string documentId;
    std::string previousRootId;
    std::filesystem::path previousSource;
    std::string candidateRootId;
    std::filesystem::path candidateSource;
    std::int64_t candidateScanGeneration{};
    IdentityDecision decision{IdentityDecision::ambiguous};
    bool mayRelinkAutomatically{};
    ReconciliationProposalState state{ReconciliationProposalState::pending};
};

enum class IndexedLocationObservationState {
    unlinked,
    unchanged,
    identityInitialized,
    replaced,
};

struct IndexedLocationObservation final {
    IndexedLocationObservationState state{IndexedLocationObservationState::unlinked};
    std::string documentId;
};

struct IndexedLocationRecord final {
    std::string documentId;
    std::string rootId;
    std::filesystem::path source;
    std::string filesystemIdentity;
    std::int64_t lastSeenScanGeneration{};
};

// SQLite-backed persistence adapter. SQLite and its statements stay private to
// this application-layer implementation; callers use Atlas-owned index types.
class SqliteLibraryIndex final : public ILibraryIndex {
public:
    explicit SqliteLibraryIndex(std::filesystem::path databasePath);
    ~SqliteLibraryIndex() override;

    SqliteLibraryIndex(const SqliteLibraryIndex&) = delete;
    SqliteLibraryIndex& operator=(const SqliteLibraryIndex&) = delete;

    [[nodiscard]] int schemaVersion() const;
    void registerRoot(LibraryRootRecord root);
    void setRootAvailability(std::string rootId, LibraryRootAvailability availability);
    [[nodiscard]] std::vector<LibraryRootRecord> roots() const;

    // Scan generations prevent an older/cancelled scan from overwriting the
    // outcome of a newer scan. Only a complete scan may provide a completion
    // timestamp; partial/offline outcomes preserve the preceding timestamp.
    [[nodiscard]] std::int64_t beginRootScan(std::string rootId);
    [[nodiscard]] bool markExistingLocationSeen(
        std::string rootId,
        const std::filesystem::path& source,
        std::int64_t scanGeneration);
    [[nodiscard]] IndexedLocationObservation observeLocation(
        std::string rootId,
        const std::filesystem::path& source,
        std::int64_t scanGeneration,
        std::optional<std::string> filesystemIdentity);
    void finishRootScan(
        std::string rootId,
        std::int64_t scanGeneration,
        LibraryRootAvailability availability,
        std::optional<std::int64_t> completedAtUtcMs = std::nullopt);

    // Adding a location already associated with a different document is a
    // reconciliation conflict, not an implicit identity change.
    void upsertRecord(LibraryRecord record, std::string rootId = {});
    [[nodiscard]] std::vector<LibraryRecord> allRecords() const;
    [[nodiscard]] std::vector<IndexedLocationRecord> locationsByFilesystemIdentity(
        std::string filesystemIdentity) const;
    void setLocationFilesystemIdentity(
        std::string documentId,
        std::string rootId,
        const std::filesystem::path& source,
        std::string filesystemIdentity);
    [[nodiscard]] ReconciliationProposalRecord proposeReconciliation(
        std::string proposalId,
        std::string documentId,
        std::string previousRootId,
        const std::filesystem::path& previousSource,
        std::string candidateRootId,
        const std::filesystem::path& candidateSource,
        std::int64_t candidateScanGeneration,
        std::string candidateFilesystemIdentity);
    [[nodiscard]] std::vector<ReconciliationProposalRecord> reconciliationProposals() const;
    void applyReconciliation(std::string proposalId);
    void dismissReconciliation(std::string proposalId);
    void setFavorite(std::string documentId, bool favorite);
    void recordOpened(std::string documentId, std::int64_t openedAtUtcMs);
    void rebuildSearchIndex();
    [[nodiscard]] std::vector<LibraryRecord> search(std::string query) const override;
    [[nodiscard]] std::vector<LibraryRecord> favorites() const;
    [[nodiscard]] std::vector<LibraryRecord> recentlyOpened(std::size_t limit = 20) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace atlas::index
