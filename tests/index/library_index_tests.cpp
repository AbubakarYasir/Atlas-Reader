#include "app/index/SqliteLibraryIndex.h"

#include <sqlite3.h>

#include <atomic>
#include <cstdint>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

class TemporaryDirectory final {
public:
    TemporaryDirectory() {
        static std::atomic<std::uint64_t> sequence{};
        const auto parent = std::filesystem::temp_directory_path();
        for (int attempt = 0; attempt < 100; ++attempt) {
            const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
            path_ = parent / ("atlas-index-test-" + std::to_string(nonce) + "-" + std::to_string(sequence++));
            std::error_code error;
            if (std::filesystem::create_directory(path_, error)) return;
            if (error && error != std::errc::file_exists) {
                throw std::runtime_error("Could not create an isolated index-test directory.");
            }
        }
        throw std::runtime_error("Could not reserve a unique index-test directory.");
    }

    ~TemporaryDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

void executeRaw(sqlite3* db, const char* sql) {
    char* error = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &error) != SQLITE_OK) {
        const std::string message = error == nullptr ? "SQLite test setup failed" : error;
        sqlite3_free(error);
        throw std::runtime_error(message);
    }
}

[[nodiscard]] sqlite3* openRaw(const std::filesystem::path& path) {
    sqlite3* db = nullptr;
    const auto utf8 = path.generic_u8string();
    const std::string filename{reinterpret_cast<const char*>(utf8.data()), utf8.size()};
    if (sqlite3_open(filename.c_str(), &db) != SQLITE_OK) {
        if (db != nullptr) sqlite3_close_v2(db);
        throw std::runtime_error("Could not open an isolated index-test database.");
    }
    return db;
}

void checkSchemaAndFts() {
    TemporaryDirectory temporary;
    const auto databasePath = temporary.path() / "profile.sqlite3";
    require(sqlite3_compileoption_used("ENABLE_FTS5") != 0, "The pinned SQLite build must enable FTS5.");

    {
        atlas::index::SqliteLibraryIndex index{databasePath};
        require(index.schemaVersion() == 3, "A new profile must reach the current schema version.");
        index.registerRoot({"root-ar", temporary.path() / "مكتبتي", atlas::index::LibraryRootAvailability::available});
        index.upsertRecord({"book-ar", temporary.path() / "مكتبتي" / "مدخل.pdf", "مدخل إلى القراءة", "مؤلف عربي", atlas::document::Availability::available}, "root-ar");
        index.upsertRecord({"book-ur", temporary.path() / "مكتبتي" / "کتاب.pdf", "کتاب", "مصنف", atlas::document::Availability::available}, "root-ar");
        index.upsertRecord({"book-en", temporary.path() / "مكتبتي" / "research.pdf", "Atlas Research", "Atlas Author", atlas::document::Availability::available}, "root-ar");
        index.setFavorite("book-ar", true);
        index.recordOpened("book-ar", 1000);
        index.recordOpened("book-en", 2000);

        require(index.search("قراءة").size() == 1, "FTS5 must find Arabic metadata.");
        require(index.search("کتاب").size() == 1, "FTS5 must find Urdu metadata.");
        require(index.search("research").size() == 1, "FTS5 must find English metadata.");
        require(index.favorites().size() == 1 && index.favorites().front().id == "book-ar", "Favorites must round-trip through the repository.");
        require(index.recentlyOpened().size() == 2 && index.recentlyOpened().front().id == "book-en", "Recents must sort by persisted open time.");

        index.upsertRecord({"book-en", temporary.path() / "مكتبتي" / "research.pdf", "Updated Atlas Research", "Atlas Author", atlas::document::Availability::available}, "root-ar");
        require(index.search("research").front().title == "Updated Atlas Research", "Updating metadata must update the FTS index.");
        require(index.search("Updated").size() == 1, "New metadata must be searchable.");
        require(index.search("").empty(), "An empty query must not scan the entire index.");
        index.rebuildSearchIndex();
        require(index.search("Updated").size() == 1, "The derived search table must support a complete rebuild.");
        require(index.roots().size() == 1, "The root repository must return the configured root.");
        bool rootIdentityConflict = false;
        try {
            index.registerRoot({"root-ar", temporary.path() / "different-root", atlas::index::LibraryRootAvailability::available});
        } catch (const std::runtime_error&) {
            rootIdentityConflict = true;
        }
        require(rootIdentityConflict, "A root identity must not be silently reassigned to another source path.");
        index.setRootAvailability("root-ar", atlas::index::LibraryRootAvailability::offline);
    }

    {
        atlas::index::SqliteLibraryIndex reopened{databasePath};
        require(reopened.schemaVersion() == 3, "Reopening a profile must preserve its schema version.");
        const auto roots = reopened.roots();
        require(roots.size() == 1 && roots.front().availability == atlas::index::LibraryRootAvailability::offline,
            "An offline root must remain in the database after reopening.");
        require(reopened.search("قراءة").size() == 1, "Arabic FTS results must survive reopening.");
        require(reopened.favorites().size() == 1, "Favorite state must survive reopening.");
        require(reopened.recentlyOpened().front().id == "book-en", "Recent-opened state must survive reopening.");

        bool identityConflict = false;
        try {
            reopened.upsertRecord({"different-book", temporary.path() / "مكتبتي" / "مدخل.pdf", "Different", "", atlas::document::Availability::available}, "root-ar");
        } catch (const std::runtime_error&) {
            identityConflict = true;
        }
        require(identityConflict, "A path already belonging to another document must not silently be reassigned.");
        require(reopened.search("Different").empty(), "A conflicting identity write must roll back the newly inserted document.");
    }
}

void checkMigrationRollback() {
    TemporaryDirectory temporary;
    const auto databasePath = temporary.path() / "rollback.sqlite3";
    sqlite3* db = openRaw(databasePath);
    try {
        // A version-zero database with a conflicting table makes migration 1
        // fail after BEGIN IMMEDIATE. No partial Atlas schema may escape.
        executeRaw(db, "CREATE TABLE library_roots (legacy_marker TEXT); INSERT INTO library_roots VALUES ('preserve-me');");
    } catch (...) {
        sqlite3_close_v2(db);
        throw;
    }
    sqlite3_close_v2(db);

    bool rejected = false;
    try {
        atlas::index::SqliteLibraryIndex invalid{databasePath};
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    require(rejected, "A schema conflict must fail clearly rather than reset the profile.");

    db = openRaw(databasePath);
    require(sqlite3_exec(db, "PRAGMA user_version", nullptr, nullptr, nullptr) == SQLITE_OK, "The existing profile must remain openable after failed migration.");
    sqlite3_stmt* statement = nullptr;
    require(sqlite3_prepare_v2(db, "PRAGMA user_version", -1, &statement, nullptr) == SQLITE_OK, "Could not inspect the preserved schema version.");
    require(sqlite3_step(statement) == SQLITE_ROW && sqlite3_column_int(statement, 0) == 0, "A failed migration must not advance user_version.");
    sqlite3_finalize(statement);
    require(sqlite3_prepare_v2(db, "SELECT legacy_marker FROM library_roots", -1, &statement, nullptr) == SQLITE_OK, "The user's conflicting table must not be replaced.");
    require(sqlite3_step(statement) == SQLITE_ROW && std::string_view{reinterpret_cast<const char*>(sqlite3_column_text(statement, 0))} == "preserve-me",
        "Existing data must survive a failed migration.");
    sqlite3_finalize(statement);
    sqlite3_close_v2(db);
}

void checkLaterMigrationRollbackAndRetry() {
    TemporaryDirectory temporary;
    const auto databasePath = temporary.path() / "retry.sqlite3";
    sqlite3* db = openRaw(databasePath);
    executeRaw(db, R"sql(
PRAGMA application_id = 1096043603;
CREATE TABLE library_roots (root_id TEXT PRIMARY KEY, source_path TEXT NOT NULL UNIQUE,
    availability TEXT NOT NULL DEFAULT 'available', last_completed_scan_utc_ms INTEGER NULL,
    scan_generation INTEGER NOT NULL DEFAULT 0);
CREATE TABLE documents (document_id TEXT PRIMARY KEY NOT NULL, title TEXT NOT NULL DEFAULT '',
    author TEXT NOT NULL DEFAULT '', file_name TEXT NOT NULL DEFAULT '', availability TEXT NOT NULL DEFAULT 'available',
    file_size_bytes INTEGER NULL, modified_utc_ms INTEGER NULL, first_indexed_utc_ms INTEGER NOT NULL DEFAULT 0);
CREATE TABLE document_locations (location_id INTEGER PRIMARY KEY, document_id TEXT NOT NULL,
    root_id TEXT NOT NULL, source_path TEXT NOT NULL, last_seen_scan_generation INTEGER NOT NULL DEFAULT 0,
    UNIQUE(root_id, source_path));
CREATE TABLE documents_fts (preserve TEXT);
PRAGMA user_version = 1;
)sql");
    sqlite3_close_v2(db);

    bool rejected = false;
    try {
        atlas::index::SqliteLibraryIndex blocked{databasePath};
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    require(rejected, "A failing FTS migration must report failure.");

    db = openRaw(databasePath);
    sqlite3_stmt* statement = nullptr;
    require(sqlite3_prepare_v2(db, "PRAGMA user_version", -1, &statement, nullptr) == SQLITE_OK, "Could not inspect the preceding migration version.");
    require(sqlite3_step(statement) == SQLITE_ROW && sqlite3_column_int(statement, 0) == 2,
        "A failed migration must leave the last successful migration version intact.");
    sqlite3_finalize(statement);
    require(sqlite3_prepare_v2(db, "SELECT preserve FROM documents_fts", -1, &statement, nullptr) == SQLITE_OK,
        "The conflicting table must remain intact after failed migration.");
    sqlite3_finalize(statement);
    executeRaw(db, "DROP TABLE documents_fts");
    sqlite3_close_v2(db);

    atlas::index::SqliteLibraryIndex retried{databasePath};
    require(retried.schemaVersion() == 3, "A later retry must complete the remaining migration.");
}

void checkNewerSchemaIsPreserved() {
    TemporaryDirectory temporary;
    const auto databasePath = temporary.path() / "newer.sqlite3";
    sqlite3* db = openRaw(databasePath);
    executeRaw(db, "CREATE TABLE caller_data (value TEXT); INSERT INTO caller_data VALUES ('keep'); PRAGMA user_version = 99;");
    sqlite3_close_v2(db);

    bool rejected = false;
    try {
        atlas::index::SqliteLibraryIndex unsupported{databasePath};
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    require(rejected, "A newer schema must be rejected without an automatic downgrade.");
    db = openRaw(databasePath);
    sqlite3_stmt* statement = nullptr;
    require(sqlite3_prepare_v2(db, "PRAGMA user_version", -1, &statement, nullptr) == SQLITE_OK, "Could not inspect the newer schema version.");
    require(sqlite3_step(statement) == SQLITE_ROW && sqlite3_column_int(statement, 0) == 99, "The newer schema marker must remain unchanged.");
    sqlite3_finalize(statement);
    sqlite3_close_v2(db);
}

} // namespace

int main() {
    try {
        checkSchemaAndFts();
        checkMigrationRollback();
        checkLaterMigrationRollbackAndRetry();
        checkNewerSchemaIsPreserved();
    } catch (const std::exception& error) {
        std::cerr << "Library index test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "SQLite schema, migration, repository, FTS5, Unicode, offline-root, reopen and rollback checks passed.\n";
    return 0;
}
