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
constexpr int currentSchemaVersion = 4;
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

[[nodiscard]] std::string safeFtsPhrase(std::string_view query) {
    std::string result{"\""};
    result.reserve(query.size() + 2);
    for (const char value : query) {
        if (value == '"') result += "\"\"";
        else result.push_back(value);
    }
    result.push_back('"');
    return result;
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

[[nodiscard]] const char* identityDecisionName(IdentityDecision value) {
    switch (value) {
    case IdentityDecision::sameLocationUnchanged: return "same_location_unchanged";
    case IdentityDecision::sameLocationChanged: return "same_location_changed";
    case IdentityDecision::confidentMove: return "confident_move";
    case IdentityDecision::distinctCopy: return "distinct_copy";
    case IdentityDecision::newDocument: return "new_document";
    case IdentityDecision::ambiguous: return "ambiguous";
    }
    throw std::invalid_argument("Unknown identity decision.");
}

[[nodiscard]] IdentityDecision parseIdentityDecision(const unsigned char* value) {
    const std::string_view text{reinterpret_cast<const char*>(value)};
    if (text == "same_location_unchanged") return IdentityDecision::sameLocationUnchanged;
    if (text == "same_location_changed") return IdentityDecision::sameLocationChanged;
    if (text == "confident_move") return IdentityDecision::confidentMove;
    if (text == "distinct_copy") return IdentityDecision::distinctCopy;
    if (text == "new_document") return IdentityDecision::newDocument;
    if (text == "ambiguous") return IdentityDecision::ambiguous;
    throw std::runtime_error("The library database contains an unknown identity decision.");
}

[[nodiscard]] ReconciliationProposalState parseProposalState(const unsigned char* value) {
    const std::string_view text{reinterpret_cast<const char*>(value)};
    if (text == "pending") return ReconciliationProposalState::pending;
    if (text == "applied") return ReconciliationProposalState::applied;
    if (text == "dismissed") return ReconciliationProposalState::dismissed;
    throw std::runtime_error("The library database contains an unknown reconciliation state.");
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

        constexpr const char* migration4 = R"sql(
ALTER TABLE library_roots ADD COLUMN last_completed_scan_generation INTEGER NOT NULL DEFAULT 0
    CHECK (last_completed_scan_generation >= 0);
ALTER TABLE document_locations ADD COLUMN filesystem_identity TEXT NULL;
CREATE TABLE reconciliation_proposals (
    proposal_id TEXT PRIMARY KEY NOT NULL,
    document_id TEXT NOT NULL REFERENCES documents(document_id) ON DELETE RESTRICT,
    previous_root_id TEXT NOT NULL REFERENCES library_roots(root_id) ON DELETE RESTRICT,
    previous_source_path TEXT NOT NULL,
    candidate_root_id TEXT NOT NULL REFERENCES library_roots(root_id) ON DELETE RESTRICT,
    candidate_source_path TEXT NOT NULL,
    candidate_scan_generation INTEGER NOT NULL CHECK (candidate_scan_generation > 0),
    candidate_filesystem_identity TEXT NOT NULL,
    decision TEXT NOT NULL CHECK (decision IN (
        'same_location_unchanged', 'same_location_changed', 'confident_move',
        'distinct_copy', 'new_document', 'ambiguous')),
    may_relink_automatically INTEGER NOT NULL CHECK (may_relink_automatically IN (0, 1)),
    state TEXT NOT NULL DEFAULT 'pending' CHECK (state IN ('pending', 'applied', 'dismissed')),
    created_utc_ms INTEGER NOT NULL DEFAULT (unixepoch('subsec') * 1000)
);
CREATE INDEX reconciliation_proposals_state ON reconciliation_proposals(state, created_utc_ms);
)sql";

        const std::array<const char*, 4> migrations{migration1, migration2, migration3, migration4};
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
    Statement statement{impl_->db,
        "SELECT root_id, source_path, availability, scan_generation, last_completed_scan_utc_ms, last_completed_scan_generation "
        "FROM library_roots ORDER BY root_id"};
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
            sqlite3_column_int64(statement.get(), 3),
            sqlite3_column_type(statement.get(), 4) == SQLITE_NULL
                ? std::nullopt
                : std::optional<std::int64_t>{sqlite3_column_int64(statement.get(), 4)},
            sqlite3_column_int64(statement.get(), 5),
        });
    }
    return result;
}

std::int64_t SqliteLibraryIndex::beginRootScan(std::string rootId) {
    if (rootId.empty()) throw std::invalid_argument("A library root ID is required to begin a scan.");
    std::scoped_lock lock{impl_->mutex};
    Statement statement{impl_->db,
        "UPDATE library_roots SET scan_generation = scan_generation + 1 "
        "WHERE root_id = ?1 AND scan_generation < 9223372036854775807 RETURNING scan_generation"};
    statement.bindText(1, rootId);
    const int step = sqlite3_step(statement.get());
    if (step == SQLITE_DONE) {
        throw std::invalid_argument("The requested library root does not exist or exhausted its scan generation.");
    }
    if (step != SQLITE_ROW) fail(impl_->db, "Beginning a library-root scan", step);
    const auto generation = sqlite3_column_int64(statement.get(), 0);
    const int completionStep = sqlite3_step(statement.get());
    if (completionStep != SQLITE_DONE) fail(impl_->db, "Completing the library-root scan start", completionStep);
    return generation;
}

bool SqliteLibraryIndex::markExistingLocationSeen(
    std::string rootId,
    const std::filesystem::path& source,
    std::int64_t scanGeneration) {
    return observeLocation(std::move(rootId), source, scanGeneration, std::nullopt).state
        != IndexedLocationObservationState::unlinked;
}

IndexedLocationObservation SqliteLibraryIndex::observeLocation(
    std::string rootId,
    const std::filesystem::path& source,
    std::int64_t scanGeneration,
    std::optional<std::string> filesystemIdentity) {
    if (rootId.empty() || source.empty() || scanGeneration <= 0) {
        throw std::invalid_argument("A root, source path and positive scan generation are required.");
    }
    if (filesystemIdentity && filesystemIdentity->empty()) {
        throw std::invalid_argument("A supplied filesystem identity cannot be empty.");
    }
    std::scoped_lock lock{impl_->mutex};

    Statement generationStatement{impl_->db,
        "SELECT scan_generation FROM library_roots WHERE root_id = ?1"};
    generationStatement.bindText(1, rootId);
    const int generationStep = sqlite3_step(generationStatement.get());
    if (generationStep == SQLITE_DONE) throw std::invalid_argument("The requested library root does not exist.");
    if (generationStep != SQLITE_ROW) fail(impl_->db, "Reading the library-root scan generation", generationStep);
    if (sqlite3_column_int64(generationStatement.get(), 0) != scanGeneration) {
        throw std::runtime_error("This scan was superseded by a newer library-root scan.");
    }

    const auto sourceText = pathToUtf8(source);
    Statement location{impl_->db,
        "SELECT document_id, filesystem_identity FROM document_locations "
        "WHERE root_id = ?1 AND source_path = ?2"};
    location.bindText(1, rootId);
    location.bindText(2, sourceText);
    const int locationStep = sqlite3_step(location.get());
    if (locationStep == SQLITE_DONE) {
        return {IndexedLocationObservationState::unlinked, {}};
    }
    if (locationStep != SQLITE_ROW) fail(impl_->db, "Reading an indexed location observation", locationStep);
    const auto* documentValue = sqlite3_column_text(location.get(), 0);
    const auto* identityValue = sqlite3_column_text(location.get(), 1);
    const std::string documentId = documentValue == nullptr
        ? std::string{}
        : std::string{reinterpret_cast<const char*>(documentValue)};
    const std::string storedIdentity = identityValue == nullptr
        ? std::string{}
        : std::string{reinterpret_cast<const char*>(identityValue)};

    if (filesystemIdentity && !storedIdentity.empty() && storedIdentity != *filesystemIdentity) {
        return {IndexedLocationObservationState::replaced, documentId};
    }

    const bool initializeIdentity = filesystemIdentity && storedIdentity.empty();
    Statement update{impl_->db,
        "UPDATE document_locations SET last_seen_scan_generation = ?3, "
        "filesystem_identity = CASE WHEN filesystem_identity IS NULL THEN ?4 ELSE filesystem_identity END "
        "WHERE root_id = ?1 AND source_path = ?2"};
    update.bindText(1, rootId);
    update.bindText(2, sourceText);
    update.bindInteger(3, scanGeneration);
    if (filesystemIdentity) update.bindText(4, *filesystemIdentity);
    else update.bindNull(4);
    update.run();
    if (sqlite3_changes(impl_->db) != 1) {
        throw std::runtime_error("The indexed location changed while recording a scan observation.");
    }
    return {
        initializeIdentity
            ? IndexedLocationObservationState::identityInitialized
            : IndexedLocationObservationState::unchanged,
        documentId,
    };
}

void SqliteLibraryIndex::finishRootScan(
    std::string rootId,
    std::int64_t scanGeneration,
    LibraryRootAvailability availability,
    std::optional<std::int64_t> completedAtUtcMs) {
    if (rootId.empty() || scanGeneration <= 0) {
        throw std::invalid_argument("A root and positive scan generation are required.");
    }
    if (availability == LibraryRootAvailability::available) {
        if (!completedAtUtcMs || *completedAtUtcMs < 0) {
            throw std::invalid_argument("A completed root scan needs a nonnegative UTC timestamp.");
        }
    } else if (completedAtUtcMs) {
        throw std::invalid_argument("Only a completed root scan may update its completion timestamp.");
    }

    std::scoped_lock lock{impl_->mutex};
    Statement statement{impl_->db,
        "UPDATE library_roots SET availability = ?3, "
        "last_completed_scan_utc_ms = CASE WHEN ?4 IS NULL THEN last_completed_scan_utc_ms ELSE ?4 END, "
        "last_completed_scan_generation = CASE WHEN ?4 IS NULL THEN last_completed_scan_generation ELSE ?2 END "
        "WHERE root_id = ?1 AND scan_generation = ?2"};
    statement.bindText(1, rootId);
    statement.bindInteger(2, scanGeneration);
    statement.bindText(3, rootAvailabilityName(availability));
    if (completedAtUtcMs) statement.bindInteger(4, *completedAtUtcMs);
    else statement.bindNull(4);
    statement.run();
    if (sqlite3_changes(impl_->db) != 1) {
        throw std::runtime_error("This scan was superseded or its library root no longer exists.");
    }
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

std::vector<LibraryRecord> SqliteLibraryIndex::allRecords() const {
    std::scoped_lock lock{impl_->mutex};
    Statement statement{impl_->db,
        "SELECT d.document_id, d.title, d.author, d.availability, "
        "(SELECT l.source_path FROM document_locations l WHERE l.document_id = d.document_id "
        " ORDER BY l.location_id LIMIT 1) "
        "FROM documents d ORDER BY d.file_name COLLATE NOCASE, d.document_id"};
    std::vector<LibraryRecord> result;
    for (;;) {
        const int step = sqlite3_step(statement.get());
        if (step == SQLITE_DONE) break;
        if (step != SQLITE_ROW) fail(impl_->db, "Reading all library records", step);
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

std::vector<LibraryRecord> SqliteLibraryIndex::recordsForRoot(std::string rootId) const {
    if (rootId.empty()) return allRecords();
    std::scoped_lock lock{impl_->mutex};
    Statement statement{impl_->db,
        "SELECT DISTINCT d.document_id, d.title, d.author, d.availability, l.source_path "
        "FROM documents d JOIN document_locations l ON l.document_id = d.document_id "
        "WHERE l.root_id = ?1 ORDER BY d.file_name COLLATE NOCASE, d.document_id"};
    statement.bindText(1, rootId);
    std::vector<LibraryRecord> result;
    for (;;) {
        const int step = sqlite3_step(statement.get());
        if (step == SQLITE_DONE) break;
        if (step != SQLITE_ROW) fail(impl_->db, "Reading records for a library root", step);
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

std::vector<IndexedLocationRecord> SqliteLibraryIndex::locationsByFilesystemIdentity(
    std::string filesystemIdentity) const {
    if (filesystemIdentity.empty()) return {};
    std::scoped_lock lock{impl_->mutex};
    Statement statement{impl_->db,
        "SELECT document_id, root_id, source_path, filesystem_identity, last_seen_scan_generation "
        "FROM document_locations WHERE filesystem_identity = ?1 ORDER BY location_id"};
    statement.bindText(1, filesystemIdentity);
    std::vector<IndexedLocationRecord> result;
    for (;;) {
        const int step = sqlite3_step(statement.get());
        if (step == SQLITE_DONE) break;
        if (step != SQLITE_ROW) fail(impl_->db, "Reading locations by filesystem identity", step);
        const auto text = [&](int column) {
            const auto* value = sqlite3_column_text(statement.get(), column);
            return value == nullptr ? std::string{} : std::string{reinterpret_cast<const char*>(value)};
        };
        result.push_back({
            text(0), text(1), pathFromUtf8(sqlite3_column_text(statement.get(), 2)), text(3),
            sqlite3_column_int64(statement.get(), 4),
        });
    }
    return result;
}

void SqliteLibraryIndex::setLocationFilesystemIdentity(
    std::string documentId,
    std::string rootId,
    const std::filesystem::path& source,
    std::string filesystemIdentity) {
    if (documentId.empty() || rootId.empty() || source.empty() || filesystemIdentity.empty()) {
        throw std::invalid_argument("A document, root, source path and filesystem identity are required.");
    }
    std::scoped_lock lock{impl_->mutex};
    Statement statement{impl_->db,
        "UPDATE document_locations SET filesystem_identity = ?4 "
        "WHERE document_id = ?1 AND root_id = ?2 AND source_path = ?3"};
    statement.bindText(1, documentId);
    statement.bindText(2, rootId);
    statement.bindText(3, pathToUtf8(source));
    statement.bindText(4, filesystemIdentity);
    statement.run();
    if (sqlite3_changes(impl_->db) != 1) {
        throw std::invalid_argument("The requested document location does not exist.");
    }
}

ReconciliationProposalRecord SqliteLibraryIndex::proposeReconciliation(
    std::string proposalId,
    std::string documentId,
    std::string previousRootId,
    const std::filesystem::path& previousSource,
    std::string candidateRootId,
    const std::filesystem::path& candidateSource,
    std::int64_t candidateScanGeneration,
    std::string candidateFilesystemIdentity) {
    if (proposalId.empty() || documentId.empty() || previousRootId.empty() || previousSource.empty()
        || candidateRootId.empty() || candidateSource.empty() || candidateScanGeneration <= 0) {
        throw std::invalid_argument("A complete reconciliation proposal is required.");
    }

    std::scoped_lock lock{impl_->mutex};
    Statement previous{impl_->db,
        "SELECT l.filesystem_identity, l.last_seen_scan_generation, r.availability, "
        "r.last_completed_scan_generation "
        "FROM document_locations l JOIN library_roots r ON r.root_id = l.root_id "
        "WHERE l.document_id = ?1 AND l.root_id = ?2 AND l.source_path = ?3"};
    previous.bindText(1, documentId);
    previous.bindText(2, previousRootId);
    previous.bindText(3, pathToUtf8(previousSource));
    const int previousStep = sqlite3_step(previous.get());
    if (previousStep == SQLITE_DONE) throw std::invalid_argument("The previous document location does not exist.");
    if (previousStep != SQLITE_ROW) fail(impl_->db, "Reading the previous document location", previousStep);
    const auto* previousFilesystemValue = sqlite3_column_text(previous.get(), 0);
    const std::string previousFilesystemIdentity = previousFilesystemValue == nullptr
        ? std::string{}
        : std::string{reinterpret_cast<const char*>(previousFilesystemValue)};
    const auto lastSeenGeneration = sqlite3_column_int64(previous.get(), 1);
    const auto previousRootAvailability = parseRootAvailability(sqlite3_column_text(previous.get(), 2));
    const auto previousCompletedGeneration = sqlite3_column_int64(previous.get(), 3);

    Statement candidateRoot{impl_->db,
        "SELECT availability, scan_generation, last_completed_scan_generation "
        "FROM library_roots WHERE root_id = ?1"};
    candidateRoot.bindText(1, candidateRootId);
    const int candidateRootStep = sqlite3_step(candidateRoot.get());
    if (candidateRootStep == SQLITE_DONE) throw std::invalid_argument("The candidate library root does not exist.");
    if (candidateRootStep != SQLITE_ROW) fail(impl_->db, "Reading the candidate library root", candidateRootStep);
    const auto candidateAvailability = parseRootAvailability(sqlite3_column_text(candidateRoot.get(), 0));
    const auto candidateCurrentGeneration = sqlite3_column_int64(candidateRoot.get(), 1);
    const auto candidateCompletedGeneration = sqlite3_column_int64(candidateRoot.get(), 2);
    if (candidateCurrentGeneration != candidateScanGeneration
        || candidateCompletedGeneration < candidateScanGeneration
        || candidateAvailability != LibraryRootAvailability::available) {
        throw std::runtime_error("The candidate must come from the current successfully completed root scan.");
    }

    PreviousLocationState previousState = PreviousLocationState::available;
    if (previousRootAvailability == LibraryRootAvailability::offline) {
        previousState = PreviousLocationState::rootOffline;
    } else if (previousRootAvailability != LibraryRootAvailability::available) {
        previousState = PreviousLocationState::scanIncomplete;
    } else if (previousCompletedGeneration > lastSeenGeneration) {
        previousState = PreviousLocationState::missingAfterCompleteScan;
    }

    std::optional<bool> sameFilesystemIdentity;
    if (!previousFilesystemIdentity.empty() && !candidateFilesystemIdentity.empty()) {
        sameFilesystemIdentity = previousFilesystemIdentity == candidateFilesystemIdentity;
    }
    const auto resolution = reconcileDocumentIdentity({
        .samePath = previousRootId == candidateRootId && previousSource == candidateSource,
        .previousLocation = previousState,
        .sameFilesystemIdentity = sameFilesystemIdentity,
    });

    Statement insert{impl_->db,
        "INSERT INTO reconciliation_proposals("
        "proposal_id, document_id, previous_root_id, previous_source_path, "
        "candidate_root_id, candidate_source_path, candidate_scan_generation, "
        "candidate_filesystem_identity, decision, may_relink_automatically) "
        "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10)"};
    insert.bindText(1, proposalId);
    insert.bindText(2, documentId);
    insert.bindText(3, previousRootId);
    insert.bindText(4, pathToUtf8(previousSource));
    insert.bindText(5, candidateRootId);
    insert.bindText(6, pathToUtf8(candidateSource));
    insert.bindInteger(7, candidateScanGeneration);
    insert.bindText(8, candidateFilesystemIdentity);
    insert.bindText(9, identityDecisionName(resolution.decision));
    insert.bindInteger(10, resolution.mayRelinkAutomatically ? 1 : 0);
    insert.run();

    return {
        std::move(proposalId), std::move(documentId), std::move(previousRootId), previousSource,
        std::move(candidateRootId), candidateSource, candidateScanGeneration, resolution.decision,
        resolution.mayRelinkAutomatically, ReconciliationProposalState::pending,
    };
}

std::vector<ReconciliationProposalRecord> SqliteLibraryIndex::reconciliationProposals() const {
    std::scoped_lock lock{impl_->mutex};
    Statement statement{impl_->db,
        "SELECT proposal_id, document_id, previous_root_id, previous_source_path, "
        "candidate_root_id, candidate_source_path, candidate_scan_generation, decision, "
        "may_relink_automatically, state FROM reconciliation_proposals "
        "ORDER BY created_utc_ms, proposal_id"};
    std::vector<ReconciliationProposalRecord> result;
    for (;;) {
        const int step = sqlite3_step(statement.get());
        if (step == SQLITE_DONE) break;
        if (step != SQLITE_ROW) fail(impl_->db, "Reading reconciliation proposals", step);
        const auto text = [&](int column) {
            const auto* value = sqlite3_column_text(statement.get(), column);
            return value == nullptr ? std::string{} : std::string{reinterpret_cast<const char*>(value)};
        };
        result.push_back({
            text(0), text(1), text(2), pathFromUtf8(sqlite3_column_text(statement.get(), 3)),
            text(4), pathFromUtf8(sqlite3_column_text(statement.get(), 5)),
            sqlite3_column_int64(statement.get(), 6),
            parseIdentityDecision(sqlite3_column_text(statement.get(), 7)),
            sqlite3_column_int(statement.get(), 8) != 0,
            parseProposalState(sqlite3_column_text(statement.get(), 9)),
        });
    }
    return result;
}

void SqliteLibraryIndex::applyReconciliation(std::string proposalId) {
    if (proposalId.empty()) throw std::invalid_argument("A reconciliation proposal ID is required.");
    std::scoped_lock lock{impl_->mutex};
    execute(impl_->db, "BEGIN IMMEDIATE");
    try {
        Statement proposal{impl_->db,
            "SELECT p.document_id, p.previous_root_id, p.previous_source_path, "
            "p.candidate_root_id, p.candidate_source_path, p.candidate_scan_generation, "
            "p.candidate_filesystem_identity, p.decision, p.may_relink_automatically, p.state, "
            "l.filesystem_identity, l.last_seen_scan_generation, previous_root.availability, "
            "previous_root.last_completed_scan_generation, candidate_root.availability, "
            "candidate_root.scan_generation, candidate_root.last_completed_scan_generation "
            "FROM reconciliation_proposals p "
            "JOIN document_locations l ON l.document_id = p.document_id "
            " AND l.root_id = p.previous_root_id AND l.source_path = p.previous_source_path "
            "JOIN library_roots previous_root ON previous_root.root_id = p.previous_root_id "
            "JOIN library_roots candidate_root ON candidate_root.root_id = p.candidate_root_id "
            "WHERE p.proposal_id = ?1"};
        proposal.bindText(1, proposalId);
        const int step = sqlite3_step(proposal.get());
        if (step == SQLITE_DONE) throw std::invalid_argument("The reconciliation proposal or previous location no longer exists.");
        if (step != SQLITE_ROW) fail(impl_->db, "Reading the reconciliation proposal", step);
        const auto text = [&](int column) {
            const auto* value = sqlite3_column_text(proposal.get(), column);
            return value == nullptr ? std::string{} : std::string{reinterpret_cast<const char*>(value)};
        };
        const auto documentId = text(0);
        const auto previousRootId = text(1);
        const auto previousSource = text(2);
        const auto candidateRootId = text(3);
        const auto candidateSourceText = text(4);
        const auto candidateSource = pathFromUtf8(
            reinterpret_cast<const unsigned char*>(candidateSourceText.c_str()));
        const auto candidateGeneration = sqlite3_column_int64(proposal.get(), 5);
        const auto candidateFilesystemIdentity = text(6);
        const auto decision = parseIdentityDecision(sqlite3_column_text(proposal.get(), 7));
        const bool mayRelink = sqlite3_column_int(proposal.get(), 8) != 0;
        const auto state = parseProposalState(sqlite3_column_text(proposal.get(), 9));
        const auto previousFilesystemIdentity = text(10);
        const auto previousLastSeen = sqlite3_column_int64(proposal.get(), 11);
        const auto previousAvailability = parseRootAvailability(sqlite3_column_text(proposal.get(), 12));
        const auto previousCompletedGeneration = sqlite3_column_int64(proposal.get(), 13);
        const auto candidateAvailability = parseRootAvailability(sqlite3_column_text(proposal.get(), 14));
        const auto candidateCurrentGeneration = sqlite3_column_int64(proposal.get(), 15);
        const auto candidateCompletedGeneration = sqlite3_column_int64(proposal.get(), 16);

        if (state != ReconciliationProposalState::pending
            || decision != IdentityDecision::confidentMove || !mayRelink
            || previousAvailability != LibraryRootAvailability::available
            || previousCompletedGeneration <= previousLastSeen
            || candidateAvailability != LibraryRootAvailability::available
            || candidateCurrentGeneration != candidateGeneration
            || candidateCompletedGeneration < candidateGeneration
            || previousFilesystemIdentity.empty()
            || previousFilesystemIdentity != candidateFilesystemIdentity) {
            throw std::runtime_error("The reconciliation proposal is not a currently proven safe move.");
        }

        Statement move{impl_->db,
            "UPDATE document_locations SET root_id = ?2, source_path = ?3, "
            "last_seen_scan_generation = ?4, filesystem_identity = ?5 "
            "WHERE document_id = ?1 AND root_id = ?6 AND source_path = ?7"};
        move.bindText(1, documentId);
        move.bindText(2, candidateRootId);
        move.bindText(3, candidateSourceText);
        move.bindInteger(4, candidateGeneration);
        move.bindText(5, candidateFilesystemIdentity);
        move.bindText(6, previousRootId);
        move.bindText(7, previousSource);
        move.run();
        if (sqlite3_changes(impl_->db) != 1) {
            throw std::runtime_error("The move target conflicts with another indexed location.");
        }

        Statement document{impl_->db, "UPDATE documents SET file_name = ?2 WHERE document_id = ?1"};
        document.bindText(1, documentId);
        document.bindText(2, pathToUtf8(candidateSource.filename()));
        document.run();

        Statement applied{impl_->db,
            "UPDATE reconciliation_proposals SET state = 'applied' "
            "WHERE proposal_id = ?1 AND state = 'pending'"};
        applied.bindText(1, proposalId);
        applied.run();
        if (sqlite3_changes(impl_->db) != 1) throw std::runtime_error("The reconciliation proposal changed concurrently.");
        execute(impl_->db, "COMMIT");
    } catch (...) {
        sqlite3_exec(impl_->db, "ROLLBACK", nullptr, nullptr, nullptr);
        throw;
    }
}

void SqliteLibraryIndex::dismissReconciliation(std::string proposalId) {
    if (proposalId.empty()) throw std::invalid_argument("A reconciliation proposal ID is required.");
    std::scoped_lock lock{impl_->mutex};
    Statement statement{impl_->db,
        "UPDATE reconciliation_proposals SET state = 'dismissed' "
        "WHERE proposal_id = ?1 AND state = 'pending'"};
    statement.bindText(1, proposalId);
    statement.run();
    if (sqlite3_changes(impl_->db) != 1) {
        throw std::runtime_error("Only a pending reconciliation proposal can be dismissed.");
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
    statement.bindText(1, safeFtsPhrase(query));

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
        "FROM documents d WHERE d.is_favorite = 1 ORDER BY d.file_name COLLATE NOCASE, d.document_id"};
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
