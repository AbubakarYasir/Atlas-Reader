#pragma once

#include "core/index/ILibraryIndex.h"

#include <filesystem>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
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

    // Adding a location already associated with a different document is a
    // reconciliation conflict, not an implicit identity change.
    void upsertRecord(LibraryRecord record, std::string rootId = {});
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
