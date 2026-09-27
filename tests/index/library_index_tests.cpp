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
        require(index.schemaVersion() == 4, "A new profile must reach the current schema version.");
        index.registerRoot({"root-ar", temporary.path() / "مكتبتي", atlas::index::LibraryRootAvailability::available});
        index.upsertRecord({"book-ar", temporary.path() / "مكتبتي" / "مدخل.pdf", "مدخل إلى القراءة", "مؤلف عربي", atlas::document::Availability::available}, "root-ar");
        index.upsertRecord({"book-ur", temporary.path() / "مكتبتي" / "کتاب.pdf", "کتاب", "مصنف", atlas::document::Availability::available}, "root-ar");
        index.upsertRecord({"book-en", temporary.path() / "مكتبتي" / "research.pdf", "Atlas Research", "Atlas Author", atlas::document::Availability::available}, "root-ar");
        index.setFavorite("book-ar", true);
        index.recordOpened("book-ar", 1000);
        index.recordOpened("book-en", 2000);

        // unicode61 tokenizes Arabic but does not stem inflected forms.
        require(index.search("القراءة").size() == 1, "FTS5 must find Arabic metadata.");
        require(index.search("کتاب").size() == 1, "FTS5 must find Urdu metadata.");
        require(index.search("research").size() == 1, "FTS5 must find English metadata.");
        require(index.favorites().size() == 1 && index.favorites().front().id == "book-ar", "Favorites must round-trip through the repository.");
        require(index.recentlyOpened().size() == 2 && index.recentlyOpened().front().id == "book-en", "Recents must sort by persisted open time.");

        index.upsertRecord({"book-en", temporary.path() / "مكتبتي" / "research.pdf", "Updated Atlas Research", "Atlas Author", atlas::document::Availability::available}, "root-ar");
        require(index.search("research").front().title == "Updated Atlas Research", "Updating metadata must update the FTS index.");
        require(index.search("Updated").size() == 1, "New metadata must be searchable.");
        require(index.search("Updated Atlas").size() == 1,
            "A multi-word library query must match the corresponding metadata phrase.");
        require(index.search("Research\"").size() == 1,
            "User punctuation must be escaped rather than interpreted as invalid FTS syntax.");
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
        require(reopened.schemaVersion() == 4, "Reopening a profile must preserve its schema version.");
        const auto roots = reopened.roots();
        require(roots.size() == 1 && roots.front().availability == atlas::index::LibraryRootAvailability::offline,
            "An offline root must remain in the database after reopening.");
        require(reopened.search("القراءة").size() == 1, "Arabic FTS results must survive reopening.");
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

void checkFilenameOrdering() {
    TemporaryDirectory temporary;
    const auto databasePath = temporary.path() / "filename-order.sqlite3";
    const auto rootPath = temporary.path() / "library";

    atlas::index::SqliteLibraryIndex index{databasePath};
    index.registerRoot({"root", rootPath, atlas::index::LibraryRootAvailability::available});
    index.upsertRecord({"z-title", rootPath / "Alpha.pdf", "Zebra internal title", "", atlas::document::Availability::available}, "root");
    index.upsertRecord({"a-title", rootPath / "Beta.pdf", "Alpha internal title", "", atlas::document::Availability::available}, "root");
    index.setFavorite("z-title", true);
    index.setFavorite("a-title", true);

    const auto all = index.allRecords();
    require(all.size() == 2 && all[0].id == "z-title" && all[1].id == "a-title",
        "The Library must sort by the visible filename rather than hidden embedded title metadata.");
    const auto root = index.recordsForRoot("root");
    require(root.size() == 2 && root[0].id == "z-title" && root[1].id == "a-title",
        "A folder view must use the same visible-filename ordering as the full Library.");
    const auto favorites = index.favorites();
    require(favorites.size() == 2 && favorites[0].id == "z-title" && favorites[1].id == "a-title",
        "Favorites must use the same visible-filename ordering as normal Library views.");
}

void checkScanGenerationSafety() {
    TemporaryDirectory temporary;
    const auto databasePath = temporary.path() / "scan-state.sqlite3";
    const auto rootPath = temporary.path() / "library";
    const auto bookPath = rootPath / "book.pdf";

    {
        atlas::index::SqliteLibraryIndex index{databasePath};
        index.registerRoot({"root", rootPath, atlas::index::LibraryRootAvailability::available});
        index.upsertRecord({"book", bookPath, "Preserved Book", "", atlas::document::Availability::available}, "root");

        const auto firstGeneration = index.beginRootScan("root");
        require(firstGeneration == 1, "A root's first scan generation must start at one.");
        require(index.markExistingLocationSeen("root", bookPath, firstGeneration),
            "A scan must mark an existing indexed location as seen.");
        require(!index.markExistingLocationSeen("root", rootPath / "new.pdf", firstGeneration),
            "A newly discovered path must remain unlinked until identity reconciliation.");
        index.finishRootScan("root", firstGeneration, atlas::index::LibraryRootAvailability::available, 1234);

        auto root = index.roots().front();
        require(root.scanGeneration == 1 && root.lastCompletedScanUtcMs == 1234,
            "A complete scan must persist its generation and completion time.");

        const auto secondGeneration = index.beginRootScan("root");
        require(secondGeneration == 2, "Every new root scan must advance its generation.");
        bool staleSeenRejected = false;
        try {
            static_cast<void>(index.markExistingLocationSeen("root", bookPath, firstGeneration));
        } catch (const std::runtime_error&) {
            staleSeenRejected = true;
        }
        require(staleSeenRejected, "A superseded scan must not mark locations as current.");

        bool staleFinishRejected = false;
        try {
            index.finishRootScan("root", firstGeneration, atlas::index::LibraryRootAvailability::available, 2000);
        } catch (const std::runtime_error&) {
            staleFinishRejected = true;
        }
        require(staleFinishRejected, "A superseded scan must not overwrite the newer root outcome.");

        index.finishRootScan("root", secondGeneration, atlas::index::LibraryRootAvailability::partiallyAvailable);
        root = index.roots().front();
        require(root.availability == atlas::index::LibraryRootAvailability::partiallyAvailable
                && root.lastCompletedScanUtcMs == 1234,
            "A partial scan must preserve the preceding successful completion time.");
        require(index.search("Preserved").size() == 1,
            "A partial scan must not delete an indexed book.");

        const auto cancelledGeneration = index.beginRootScan("root");
        require(cancelledGeneration == 3, "A cancelled scan still needs a distinct generation.");
        // Deliberately do not finish: cancellation must leave the preceding
        // availability and completion evidence untouched.
    }

    {
        atlas::index::SqliteLibraryIndex reopened{databasePath};
        auto root = reopened.roots().front();
        require(root.scanGeneration == 3
                && root.availability == atlas::index::LibraryRootAvailability::partiallyAvailable
                && root.lastCompletedScanUtcMs == 1234,
            "An interrupted scan must preserve the last known root state after restart.");
        require(reopened.search("Preserved").size() == 1,
            "An interrupted scan must preserve indexed books after restart.");
        reopened.finishRootScan("root", 3, atlas::index::LibraryRootAvailability::offline);
        root = reopened.roots().front();
        require(root.availability == atlas::index::LibraryRootAvailability::offline
                && root.lastCompletedScanUtcMs == 1234
                && reopened.search("Preserved").size() == 1,
            "An offline result must preserve both books and prior successful-scan evidence.");
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

void checkDurableReconciliation() {
    TemporaryDirectory temporary;
    const auto databasePath = temporary.path() / "reconciliation.sqlite3";
    const auto rootPath = temporary.path() / "library";
    const auto oldPath = rootPath / "old-name.pdf";
    const auto movedPath = rootPath / "renamed.pdf";

    {
        atlas::index::SqliteLibraryIndex index{databasePath};
        index.registerRoot({"root", rootPath, atlas::index::LibraryRootAvailability::available});
        index.upsertRecord({"book", oldPath, "Durable Identity", "Atlas", atlas::document::Availability::available}, "root");
        index.setLocationFilesystemIdentity("book", "root", oldPath, "volume-1:file-42");

        const auto first = index.beginRootScan("root");
        require(index.markExistingLocationSeen("root", oldPath, first), "The original location must be observed in the first scan.");
        index.finishRootScan("root", first, atlas::index::LibraryRootAvailability::available, 1000);

        const auto second = index.beginRootScan("root");
        index.finishRootScan("root", second, atlas::index::LibraryRootAvailability::available, 2000);
        const auto proposal = index.proposeReconciliation(
            "move-1", "book", "root", oldPath, "root", movedPath, second, "volume-1:file-42");
        require(proposal.decision == atlas::index::IdentityDecision::confidentMove
                && proposal.mayRelinkAutomatically
                && proposal.state == atlas::index::ReconciliationProposalState::pending,
            "A completed scan plus preserved filesystem identity must create a reviewable safe-move proposal.");
        require(index.search("Durable").front().source == oldPath,
            "Creating a reconciliation proposal must not mutate the document location.");
    }

    {
        atlas::index::SqliteLibraryIndex reopened{databasePath};
        const auto pending = reopened.reconciliationProposals();
        require(pending.size() == 1 && pending.front().id == "move-1"
                && pending.front().state == atlas::index::ReconciliationProposalState::pending,
            "A reviewable reconciliation proposal must survive restart before application.");
        reopened.applyReconciliation("move-1");
        const auto result = reopened.search("Durable");
        require(result.size() == 1 && result.front().id == "book" && result.front().source == movedPath,
            "Applying a proven move must retain the stable document ID at the new path.");
        require(reopened.reconciliationProposals().front().state == atlas::index::ReconciliationProposalState::applied,
            "An applied reconciliation must remain auditable after the location changes.");
    }

    const auto ambiguityDatabase = temporary.path() / "ambiguity.sqlite3";
    {
        atlas::index::SqliteLibraryIndex index{ambiguityDatabase};
        const auto oldRoot = temporary.path() / "offline-library";
        const auto candidateRoot = temporary.path() / "online-library";
        const auto source = oldRoot / "book.pdf";
        index.registerRoot({"old-root", oldRoot, atlas::index::LibraryRootAvailability::available});
        index.registerRoot({"candidate-root", candidateRoot, atlas::index::LibraryRootAvailability::available});
        index.upsertRecord({"book", source, "Offline Identity", "", atlas::document::Availability::available}, "old-root");
        index.setLocationFilesystemIdentity("book", "old-root", source, "volume-2:file-9");

        const auto oldGeneration = index.beginRootScan("old-root");
        require(index.markExistingLocationSeen("old-root", source, oldGeneration), "The offline fixture must begin with a known location.");
        index.finishRootScan("old-root", oldGeneration, atlas::index::LibraryRootAvailability::available, 1000);
        index.setRootAvailability("old-root", atlas::index::LibraryRootAvailability::offline);

        const auto candidateGeneration = index.beginRootScan("candidate-root");
        index.finishRootScan("candidate-root", candidateGeneration, atlas::index::LibraryRootAvailability::available, 2000);
        const auto proposal = index.proposeReconciliation(
            "ambiguous-1", "book", "old-root", source, "candidate-root",
            candidateRoot / "book.pdf", candidateGeneration, "volume-2:file-9");
        require(proposal.decision == atlas::index::IdentityDecision::ambiguous
                && !proposal.mayRelinkAutomatically,
            "An offline previous root must produce an ambiguous proposal even when filesystem evidence matches.");
        bool applyRejected = false;
        try {
            index.applyReconciliation("ambiguous-1");
        } catch (const std::runtime_error&) {
            applyRejected = true;
        }
        require(applyRejected && index.search("Offline").front().source == source,
            "An ambiguous proposal must be rejected without changing the indexed location.");
        index.dismissReconciliation("ambiguous-1");
        require(index.reconciliationProposals().front().state == atlas::index::ReconciliationProposalState::dismissed,
            "A dismissed ambiguous proposal must remain auditable.");
    }
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
    require(retried.schemaVersion() == 4, "A later retry must complete the remaining migrations.");
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
        checkFilenameOrdering();
        checkScanGenerationSafety();
        checkMigrationRollback();
        checkDurableReconciliation();
        checkLaterMigrationRollbackAndRetry();
        checkNewerSchemaIsPreserved();
    } catch (const std::exception& error) {
        std::cerr << "Library index test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "SQLite schema, migration, repository, FTS5, Unicode, offline-root, reopen and rollback checks passed.\n";
    return 0;
}
