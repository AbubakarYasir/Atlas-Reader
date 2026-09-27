#pragma once

#include "app/index/ILibraryDocumentInspector.h"
#include "app/index/LibraryScanService.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace atlas::index {

class SqliteLibraryIndex;

struct LibraryIngestionSummary final {
    IntegratedLibraryScanSummary scan;
    std::size_t documentsAdded{};
    std::size_t reconciliationProposalsAdded{};
    std::size_t repeatedCandidatesSkipped{};
    std::size_t inspectionFailures{};
};

// Converts bounded scan discoveries into either a new stable Atlas document or
// a durable reconciliation proposal. It never applies a proposal implicitly.
class LibraryIngestionService final {
public:
    using IdFactory = std::function<std::string()>;

    LibraryIngestionService(
        const LibraryScanService& scanService,
        SqliteLibraryIndex& index,
        const ILibraryDocumentInspector& documentInspector,
        IdFactory idFactory);

    [[nodiscard]] LibraryIngestionSummary ingest(
        const std::vector<LibraryScanRoot>& roots,
        const std::atomic_bool& cancelled,
        std::int64_t completedAtUtcMs) const;

private:
    const LibraryScanService& scanService_;
    SqliteLibraryIndex& index_;
    const ILibraryDocumentInspector& documentInspector_;
    IdFactory idFactory_;
};

} // namespace atlas::index

