#include "app/index/SqliteLibraryIndex.h"

#include <sqlite3.h>

#include <array>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace atlas::index {
namespace {

constexpr int applicationId = 0x41544C53; // "ATLS"
constexpr int currentSchemaVersion = 3;
constexpr int maxSearchResults = 100;

[[nodiscard]] std::string pathToUtf8(const std::filesystem::path& path) {
    const auto utf8 = path.generic_u8string();
    return {reinterpret_cast<const char*>(utf8.data()), utf8.size()};
}

[[nodiscard]] std::filesystem::path pathFromUtf8(const unsigned char* value) {
    if (value == nullptr) {
        return {};
    }
    const std::string_view bytes{reinterpret_cast<const char*>(value)};
    std::u8string utf8;
    utf8.reserve(bytes.size());
    for (const auto byte : bytes) {
        utf8.push_back(static_cast<char8_t>(static_cast<unsigned char>(byte)));
    }
    return std::filesystem::path{utf8};
}

[[nodiscard]] const char* availabilityName(document::Availability value) {
    switch (value) {
    case document::Availability::available: return "available";
    case document::Availability::locked: return "locked";
    case document::Availability::readOnly: return "read_only";
    case document::Availability::offline: return "offline";
    case document::Availability::missing: return "missing";
    case document::Availability::cloudPlaceholder: return "cloud_placeholder";
    case document::Availability::unreadable: return "unreadable";
    case document::Availability::unsupported: return "unsupported";
    }
    throw std::invalid_argument("Unknown document availability value.");
}

[[nodiscard]] document::Availability parseAvailability(const unsigned char* value) {
    const std::string_view text{reinterpret_cast<const char*>(value)};
    if (text == "available") return document::Availability::available;
    if (text == "locked") return document::Availability::locked;
    if (text == "read_only") return document::Availability::readOnly;
    if (text == "offline") return document::Availability::offline;
    if (text == "missing") return document::Availability::missing;
    if (text == "cloud_placeholder") return document::Availability::cloudPlaceholder;
    if (text == "unreadable") return document::Availability::unreadable;
    if (text == "unsupported") return document::Availability::unsupported;
    throw std::runtime_error("The library database contains an unknown availability state.");
}

[[nodiscard]] const char* rootAvailabilityName(LibraryRootAvailability value) {
    switch (value) {
    case LibraryRootAvailability::available: return "available";
    case LibraryRootAvailability::offline: return "offline";
    case LibraryRootAvailability::partiallyAvailable: return "partial";
    case LibraryRootAvailability::permissionDenied: return "permission_denied";
    }
    throw std::invalid_argument("Unknown library-root availability value.");
}

[[nodiscard]] LibraryRootAvailability parseRootAvailability(const unsigned char* value) {
    const std::string_view text{reinterpret_cast<const char*>(value)};
    if (text == "available") return LibraryRootAvailability::available;
    if (text == "offline") return LibraryRootAvailability::offline;
    if (text == "partial") return LibraryRootAvailability::partiallyAvailable;
    if (text == "permission_denied") return LibraryRootAvailability::permissionDenied;
    throw std::runtime_error("The library database contains an unknown root state.");
}

void fail(sqlite3* db, std::string_view operation, int result) {
    std::string message{operation};
    message += ": ";
    message += db == nullptr ? sqlite3_errstr(result) : sqlite3_errmsg(db);
    throw std::runtime_error(message);
}

void execute(sqlite3* db, const char* sql) {
    char* error = nullptr;
    const int result = sqlite3_exec(db, sql, nullptr, nullptr, &error);
    if (result != SQLITE_OK) {
        std::string message{"SQLite statement failed"};
        if (error != nullptr) {
            message += ": ";
            message += error;
            sqlite3_free(error);
        }
        throw std::runtime_error(message);
    }
}

class Statement final {
public:
    Statement(sqlite3* db, const char* sql) : db_(db) {
        const int result = sqlite3_prepare_v2(db, sql, -1, &statement_, nullptr);
        if (result != SQLITE_OK) {
            fail(db, "Preparing a library database statement", result);
        }
    }

    ~Statement() { sqlite3_finalize(statement_); }
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    [[nodiscard]] sqlite3_stmt* get() const { return statement_; }

    void bindText(int index, std::string_view value) {
        if (value.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            throw std::length_error("Library text exceeds SQLite's supported value size.");
        }
        const int result = sqlite3_bind_text(statement_, index, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT);
        if (result != SQLITE_OK) fail(db_, "Binding library text", result);
    }

    void bindInteger(int index, std::int64_t value) {
        const int result = sqlite3_bind_int64(statement_, index, value);
        if (result != SQLITE_OK) fail(db_, "Binding a library value", result);
    }

    void bindNull(int index) {
        const int result = sqlite3_bind_null(statement_, index);
        if (result != SQLITE_OK) fail(db_, "Binding a library value", result);
    }

    void run() {
        const int result = sqlite3_step(statement_);
        if (result != SQLITE_DONE) fail(db_, "Writing the library database", result);
    }

private:
    sqlite3* db_{};
    sqlite3_stmt* statement_{};
};

[[nodiscard]] std::int64_t scalarInteger(sqlite3* db, const char* sql) {
    Statement statement{db, sql};
    const int result = sqlite3_step(statement.get());
    if (result != SQLITE_ROW) fail(db, "Reading the library database", result);
    return sqlite3_column_int64(statement.get(), 0);
}

void applyMigration(sqlite3* db, int nextVersion, const char* sql) {
    execute(db, "BEGIN IMMEDIATE");
    try {
        execute(db, sql);
        execute(db, ("PRAGMA user_version = " + std::to_string(nextVersion)).c_str());
        execute(db, "COMMIT");
    } catch (...) {
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
        throw;
    }
}

} // namespace

struct SqliteLibraryIndex::Impl final {
    explicit Impl(const std::filesystem::path& databasePath) {
        if (databasePath.empty()) {
            throw std::invalid_argument("A library database path is required.");
        }
        if (databasePath != std::filesystem::path{":memory:"}) {
            const auto parent = databasePath.parent_path();
            if (!parent.empty()) {
                std::error_code error;
                std::filesystem::create_directories(parent, error);
                if (error) {
                    throw std::runtime_error("Could not prepare the Atlas library profile directory.");
                }
            }
        }

        const auto databasePathUtf8 = pathToUtf8(databasePath);
        const int result = sqlite3_open_v2(
            databasePathUtf8.c_str(), &db,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
        if (result != SQLITE_OK) {
            const auto message = db == nullptr ? std::string{sqlite3_errstr(result)} : std::string{sqlite3_errmsg(db)};
            if (db != nullptr) sqlite3_close_v2(db);
            db = nullptr;
            throw std::runtime_error("Could not open the Atlas library database: " + message);
        }

        try {
        sqlite3_busy_timeout(db, 5000);
        execute(db, "PRAGMA foreign_keys = ON");
        const int storedApplicationId = static_cast<int>(scalarInteger(db, "PRAGMA application_id"));
        if (storedApplicationId != 0 && storedApplicationId != applicationId) {
            throw std::runtime_error("The selected profile database belongs to another application.");
        }

        auto version = static_cast<int>(scalarInteger(db, "PRAGMA user_version"));
        if (version < 0 || version > currentSchemaVersion) {
            throw std::runtime_error("The library database schema is newer than this Atlas build supports.");
        }

        constexpr const char* migration1 = R"sql(
CREATE TABLE library_roots (
    root_id TEXT PRIMARY KEY NOT NULL,
    source_path TEXT NOT NULL UNIQUE,
    availability TEXT NOT NULL DEFAULT 'available'
        CHECK (availability IN ('available', 'offline', 'partial', 'permission_denied')),
    last_completed_scan_utc_ms INTEGER NULL,
    scan_generation INTEGER NOT NULL DEFAULT 0 CHECK (scan_generation >= 0)
);
CREATE TABLE documents (
    document_id TEXT PRIMARY KEY NOT NULL,
    title TEXT NOT NULL DEFAULT '',
    author TEXT NOT NULL DEFAULT '',
    file_name TEXT NOT NULL DEFAULT '',
    availability TEXT NOT NULL DEFAULT 'available'
        CHECK (availability IN ('available', 'locked', 'read_only', 'offline', 'missing', 'cloud_placeholder', 'unreadable', 'unsupported')),
    file_size_bytes INTEGER NULL CHECK (file_size_bytes IS NULL OR file_size_bytes >= 0),
    modified_utc_ms INTEGER NULL,
    first_indexed_utc_ms INTEGER NOT NULL DEFAULT (unixepoch('subsec') * 1000)
);
CREATE TABLE document_locations (
    location_id INTEGER PRIMARY KEY,
    document_id TEXT NOT NULL REFERENCES documents(document_id) ON DELETE RESTRICT,
    root_id TEXT NOT NULL REFERENCES library_roots(root_id) ON DELETE RESTRICT,
    source_path TEXT NOT NULL,
    last_seen_scan_generation INTEGER NOT NULL DEFAULT 0 CHECK (last_seen_scan_generation >= 0),
    UNIQUE (root_id, source_path)
);
CREATE INDEX document_locations_document_id ON document_locations(document_id);
)sql";

        constexpr const char* migration2 = R"sql(
ALTER TABLE documents ADD COLUMN is_favorite INTEGER NOT NULL DEFAULT 0 CHECK (is_favorite IN (0, 1));
ALTER TABLE documents ADD COLUMN last_opened_utc_ms INTEGER NULL;
CREATE INDEX documents_recent ON documents(last_opened_utc_ms DESC) WHERE last_opened_utc_ms IS NOT NULL;
)sql";

        constexpr const char* migration3 = R"sql(
CREATE VIRTUAL TABLE documents_fts USING fts5(
    title, author, file_name,
    content='documents', content_rowid='rowid',
    tokenize='unicode61 remove_diacritics 0'
);
CREATE TRIGGER documents_fts_insert AFTER INSERT ON documents BEGIN
    INSERT INTO documents_fts(rowid, title, author, file_name)
    VALUES (new.rowid, new.title, new.author, new.file_name);
END;
CREATE TRIGGER documents_fts_delete AFTER DELETE ON documents BEGIN
    INSERT INTO documents_fts(documents_fts, rowid, title, author, file_name)
    VALUES ('delete', old.rowid, old.title, old.author, old.file_name);
END;
CREATE TRIGGER documents_fts_update AFTER UPDATE OF title, author, file_name ON documents BEGIN
    INSERT INTO documents_fts(documents_fts, rowid, title, author, file_name)
    VALUES ('delete', old.rowid, old.title, old.author, old.file_name);
    INSERT INTO documents_fts(rowid, title, author, file_name)
    VALUES (new.rowid, new.title, new.author, new.file_name);
END;
)sql";

        const std::array<const char*, 3> migrations{migration1, migration2, migration3};
        while (version < currentSchemaVersion) {
            applyMigration(db, version + 1, migrations.at(static_cast<std::size_t>(version)));
            ++version;
        }
        if (storedApplicationId == 0) {
            execute(db, ("PRAGMA application_id = " + std::to_string(applicationId)).c_str());
        }
        execute(db, "PRAGMA journal_mode = WAL");
        execute(db, "PRAGMA synchronous = FULL");
        } catch (...) {
            sqlite3_close_v2(db);
            db = nullptr;
            throw;
        }
    }

    ~Impl() {
        if (db != nullptr) sqlite3_close_v2(db);
    }

    sqlite3* db{};
    mutable std::mutex mutex;
};

SqliteLibraryIndex::SqliteLibraryIndex(std::filesystem::path databasePath)
    : impl_{std::make_unique<Impl>(databasePath)} {}

SqliteLibraryIndex::~SqliteLibraryIndex() = default;

int SqliteLibraryIndex::schemaVersion() const {
    std::scoped_lock lock{impl_->mutex};
    return static_cast<int>(scalarInteger(impl_->db, "PRAGMA user_version"));
}

void SqliteLibraryIndex::registerRoot(LibraryRootRecord root) {
    if (root.id.empty() || root.source.empty()) {
        throw std::invalid_argument("A library root needs an ID and a source path.");
    }
    std::scoped_lock lock{impl_->mutex};
    Statement statement{impl_->db,
        "INSERT INTO library_roots(root_id, source_path, availability) VALUES(?1, ?2, ?3) "
        "ON CONFLICT(root_id) DO UPDATE SET availability = excluded.availability "
        "WHERE library_roots.source_path = excluded.source_path"};
    statement.bindText(1, root.id);
    statement.bindText(2, pathToUtf8(root.source));
    statement.bindText(3, rootAvailabilityName(root.availability));
    statement.run();
    if (sqlite3_changes(impl_->db) != 1) {
        throw std::runtime_error("A library-root ID cannot be silently reassigned to a different path.");
    }
}

void SqliteLibraryIndex::setRootAvailability(std::string rootId, LibraryRootAvailability availability) {
    std::scoped_lock lock{impl_->mutex};
    Statement statement{impl_->db, "UPDATE library_roots SET availability = ?2 WHERE root_id = ?1"};
    statement.bindText(1, rootId);
    statement.bindText(2, rootAvailabilityName(availability));
    statement.run();
    if (sqlite3_changes(impl_->db) != 1) {
        throw std::invalid_argument("The requested library root does not exist.");
    }
}

std::vector<LibraryRootRecord> SqliteLibraryIndex::roots() const {
    std::scoped_lock lock{impl_->mutex};
    Statement statement{impl_->db, "SELECT root_id, source_path, availability FROM library_roots ORDER BY root_id"};
    std::vector<LibraryRootRecord> result;
    for (;;) {
        const int step = sqlite3_step(statement.get());
        if (step == SQLITE_DONE) break;
        if (step != SQLITE_ROW) fail(impl_->db, "Reading library roots", step);
        const auto* id = sqlite3_column_text(statement.get(), 0);
        const auto* path = sqlite3_column_text(statement.get(), 1);
        const auto* availability = sqlite3_column_text(statement.get(), 2);
        result.push_back({
            id == nullptr ? std::string{} : std::string{reinterpret_cast<const char*>(id)},
            pathFromUtf8(path),
            parseRootAvailability(availability),
        });
    }
    return result;
}

void SqliteLibraryIndex::upsertRecord(LibraryRecord record, std::string rootId) {
    if (record.id.empty() || record.source.empty()) {
        throw std::invalid_argument("A library record needs a stable document ID and source path.");
    }
    std::scoped_lock lock{impl_->mutex};
    const auto sourcePath = pathToUtf8(record.source);

    execute(impl_->db, "BEGIN IMMEDIATE");
    try {
        Statement recordStatement{impl_->db,
            "INSERT INTO documents(document_id, title, author, file_name, availability) "
            "VALUES(?1, ?2, ?3, ?4, ?5) ON CONFLICT(document_id) DO UPDATE SET "
            "title = excluded.title, author = excluded.author, file_name = excluded.file_name, availability = excluded.availability"};
        recordStatement.bindText(1, record.id);
        recordStatement.bindText(2, record.title);
        recordStatement.bindText(3, record.author);
        recordStatement.bindText(4, pathToUtf8(record.source.filename()));
        recordStatement.bindText(5, availabilityName(record.availability));
        recordStatement.run();

        if (!rootId.empty()) {
            Statement locationStatement{impl_->db,
                "INSERT INTO document_locations(document_id, root_id, source_path) VALUES(?1, ?2, ?3) "
                "ON CONFLICT(root_id, source_path) DO UPDATE SET source_path = excluded.source_path "
                "WHERE document_locations.document_id = excluded.document_id"};
            locationStatement.bindText(1, record.id);
            locationStatement.bindText(2, rootId);
            locationStatement.bindText(3, sourcePath);
            locationStatement.run();
            if (sqlite3_changes(impl_->db) != 1) {
                throw std::runtime_error("This path is already linked to a different book; identity reconciliation is required.");
            }
        }
        execute(impl_->db, "COMMIT");
    } catch (...) {
        sqlite3_exec(impl_->db, "ROLLBACK", nullptr, nullptr, nullptr);
        throw;
    }
}

void SqliteLibraryIndex::setFavorite(std::string documentId, bool favorite) {
    std::scoped_lock lock{impl_->mutex};
    Statement statement{impl_->db, "UPDATE documents SET is_favorite = ?2 WHERE document_id = ?1"};
    statement.bindText(1, documentId);
    statement.bindInteger(2, favorite ? 1 : 0);
    statement.run();
    if (sqlite3_changes(impl_->db) != 1) {
        throw std::invalid_argument("The requested library record does not exist.");
    }
}

void SqliteLibraryIndex::recordOpened(std::string documentId, std::int64_t openedAtUtcMs) {
    if (openedAtUtcMs < 0) throw std::invalid_argument("The open time must be a nonnegative UTC timestamp.");
    std::scoped_lock lock{impl_->mutex};
    Statement statement{impl_->db, "UPDATE documents SET last_opened_utc_ms = ?2 WHERE document_id = ?1"};
    statement.bindText(1, documentId);
    statement.bindInteger(2, openedAtUtcMs);
    statement.run();
    if (sqlite3_changes(impl_->db) != 1) {
        throw std::invalid_argument("The requested library record does not exist.");
    }
}

void SqliteLibraryIndex::rebuildSearchIndex() {
    std::scoped_lock lock{impl_->mutex};
    execute(impl_->db, "INSERT INTO documents_fts(documents_fts) VALUES('rebuild')");
}

std::vector<LibraryRecord> SqliteLibraryIndex::search(std::string query) const {
    if (query.empty()) return {};
    std::scoped_lock lock{impl_->mutex};
    Statement statement{impl_->db,
        "SELECT d.document_id, d.title, d.author, d.availability, "
        "(SELECT l.source_path FROM document_locations l WHERE l.document_id = d.document_id ORDER BY l.location_id LIMIT 1) "
        "FROM documents_fts f JOIN documents d ON d.rowid = f.rowid "
        "WHERE documents_fts MATCH ?1 ORDER BY rank LIMIT 100"};
    statement.bindText(1, query);

    std::vector<LibraryRecord> result;
    for (;;) {
        const int step = sqlite3_step(statement.get());
        if (step == SQLITE_DONE) break;
        if (step != SQLITE_ROW) fail(impl_->db, "Searching the library index", step);
        const auto* id = sqlite3_column_text(statement.get(), 0);
        const auto* title = sqlite3_column_text(statement.get(), 1);
        const auto* author = sqlite3_column_text(statement.get(), 2);
        const auto* availability = sqlite3_column_text(statement.get(), 3);
        const auto* path = sqlite3_column_text(statement.get(), 4);
        result.push_back({
            id == nullptr ? std::string{} : std::string{reinterpret_cast<const char*>(id)},
            pathFromUtf8(path),
            title == nullptr ? std::string{} : std::string{reinterpret_cast<const char*>(title)},
            author == nullptr ? std::string{} : std::string{reinterpret_cast<const char*>(author)},
            parseAvailability(availability),
        });
        if (result.size() == static_cast<std::size_t>(maxSearchResults)) break;
    }
    return result;
}

std::vector<LibraryRecord> SqliteLibraryIndex::favorites() const {
    std::scoped_lock lock{impl_->mutex};
    Statement statement{impl_->db,
        "SELECT d.document_id, d.title, d.author, d.availability, "
        "(SELECT l.source_path FROM document_locations l WHERE l.document_id = d.document_id ORDER BY l.location_id LIMIT 1) "
        "FROM documents d WHERE d.is_favorite = 1 ORDER BY d.title COLLATE NOCASE, d.document_id"};
    std::vector<LibraryRecord> result;
    for (;;) {
        const int step = sqlite3_step(statement.get());
        if (step == SQLITE_DONE) break;
        if (step != SQLITE_ROW) fail(impl_->db, "Reading favorites", step);
        const auto* id = sqlite3_column_text(statement.get(), 0);
        const auto* title = sqlite3_column_text(statement.get(), 1);
        const auto* author = sqlite3_column_text(statement.get(), 2);
        const auto* availability = sqlite3_column_text(statement.get(), 3);
        const auto* path = sqlite3_column_text(statement.get(), 4);
        result.push_back({
            id == nullptr ? std::string{} : std::string{reinterpret_cast<const char*>(id)},
            pathFromUtf8(path),
            title == nullptr ? std::string{} : std::string{reinterpret_cast<const char*>(title)},
            author == nullptr ? std::string{} : std::string{reinterpret_cast<const char*>(author)},
            parseAvailability(availability),
        });
    }
    return result;
}

std::vector<LibraryRecord> SqliteLibraryIndex::recentlyOpened(std::size_t limit) const {
    const auto boundedLimit = static_cast<std::int64_t>(std::min<std::size_t>(limit, 1000));
    std::scoped_lock lock{impl_->mutex};
    Statement statement{impl_->db,
        "SELECT d.document_id, d.title, d.author, d.availability, "
        "(SELECT l.source_path FROM document_locations l WHERE l.document_id = d.document_id ORDER BY l.location_id LIMIT 1) "
        "FROM documents d WHERE d.last_opened_utc_ms IS NOT NULL "
        "ORDER BY d.last_opened_utc_ms DESC, d.document_id LIMIT ?1"};
    statement.bindInteger(1, boundedLimit);
    std::vector<LibraryRecord> result;
    for (;;) {
        const int step = sqlite3_step(statement.get());
        if (step == SQLITE_DONE) break;
        if (step != SQLITE_ROW) fail(impl_->db, "Reading recent books", step);
        const auto* id = sqlite3_column_text(statement.get(), 0);
        const auto* title = sqlite3_column_text(statement.get(), 1);
        const auto* author = sqlite3_column_text(statement.get(), 2);
        const auto* availability = sqlite3_column_text(statement.get(), 3);
        const auto* path = sqlite3_column_text(statement.get(), 4);
        result.push_back({
            id == nullptr ? std::string{} : std::string{reinterpret_cast<const char*>(id)},
            pathFromUtf8(path),
            title == nullptr ? std::string{} : std::string{reinterpret_cast<const char*>(title)},
            author == nullptr ? std::string{} : std::string{reinterpret_cast<const char*>(author)},
            parseAvailability(availability),
        });
    }
    return result;
}

} // namespace atlas::index
