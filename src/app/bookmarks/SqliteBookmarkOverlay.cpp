#include "app/bookmarks/SqliteBookmarkOverlay.h"

#include "core/bookmarks/OverlayTreeIndex.h"

#include <sqlite3.h>

#include <algorithm>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace atlas::bookmarks {
namespace {

constexpr int profileApplicationId = 0x41544C53; // "ATLS", shared with the library index
constexpr int minimumSchemaVersion = 5;          // migration 5 adds the overlay tables

[[nodiscard]] std::string pathToUtf8(const std::filesystem::path& path) {
    const auto utf8 = path.generic_u8string();
    return {reinterpret_cast<const char*>(utf8.data()), utf8.size()};
}

[[noreturn]] void fail(sqlite3* db, std::string_view operation) {
    std::string message{operation};
    message += ": ";
    message += sqlite3_errmsg(db);
    throw std::runtime_error(message);
}

void execute(sqlite3* db, const char* sql) {
    char* error = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &error) != SQLITE_OK) {
        std::string message{"Bookmark overlay statement failed"};
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
        if (sqlite3_prepare_v2(db, sql, -1, &statement_, nullptr) != SQLITE_OK) {
            fail(db, "Preparing a bookmark overlay statement");
        }
    }
    ~Statement() { sqlite3_finalize(statement_); }
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    void text(int index, std::string_view value) {
        if (value.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            throw std::length_error("Bookmark text exceeds SQLite's supported value size.");
        }
        // An empty view may have a null data pointer, which SQLite reads as NULL.
        const char* data = value.data() == nullptr ? "" : value.data();
        if (sqlite3_bind_text(statement_, index, data, static_cast<int>(value.size()), SQLITE_TRANSIENT) != SQLITE_OK) {
            fail(db_, "Binding bookmark text");
        }
    }
    void optionalText(int index, const std::optional<std::string>& value) {
        if (value) text(index, *value);
        else null(index);
    }
    void textOrNull(int index, std::string_view value) {
        if (value.empty()) null(index);
        else text(index, value);
    }
    void integer(int index, std::int64_t value) {
        if (sqlite3_bind_int64(statement_, index, value) != SQLITE_OK) fail(db_, "Binding a bookmark value");
    }
    void null(int index) {
        if (sqlite3_bind_null(statement_, index) != SQLITE_OK) fail(db_, "Binding a bookmark value");
    }

    [[nodiscard]] bool row() {
        const int result = sqlite3_step(statement_);
        if (result == SQLITE_ROW) return true;
        if (result == SQLITE_DONE) return false;
        fail(db_, "Reading the bookmark overlay");
    }
    void run() {
        if (sqlite3_step(statement_) != SQLITE_DONE) fail(db_, "Writing the bookmark overlay");
    }
    void reset() {
        sqlite3_reset(statement_);
        sqlite3_clear_bindings(statement_);
    }

    [[nodiscard]] bool isNull(int column) const { return sqlite3_column_type(statement_, column) == SQLITE_NULL; }
    [[nodiscard]] std::string string(int column) const {
        const auto* data = reinterpret_cast<const char*>(sqlite3_column_text(statement_, column));
        const int size = sqlite3_column_bytes(statement_, column);
        return data == nullptr ? std::string{} : std::string{data, static_cast<std::size_t>(size)};
    }
    [[nodiscard]] std::optional<std::string> optionalString(int column) const {
        if (isNull(column)) return std::nullopt;
        return string(column);
    }
    [[nodiscard]] std::int64_t int64(int column) const { return sqlite3_column_int64(statement_, column); }

private:
    sqlite3* db_{};
    sqlite3_stmt* statement_{};
};

[[nodiscard]] std::int64_t scalar(sqlite3* db, const char* sql) {
    Statement statement{db, sql};
    if (!statement.row()) throw std::runtime_error("The profile database returned no value.");
    return statement.int64(0);
}

class Transaction final {
public:
    explicit Transaction(sqlite3* db) : db_(db) { execute(db_, "BEGIN IMMEDIATE"); }
    ~Transaction() {
        if (!done_) sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
    }
    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    void commit() {
        execute(db_, "COMMIT");
        done_ = true;
    }

private:
    sqlite3* db_{};
    bool done_{false};
};

[[nodiscard]] StorageState storedState(const std::string& name) {
    const auto state = parseStateName(name);
    if (!state) throw std::runtime_error("The profile contains an unknown bookmark state.");
    return *state;
}

[[nodiscard]] bool sameContent(const BookmarkNode& a, const BookmarkNode& b) {
    return a.parentId == b.parentId && a.position == b.position && a.title == b.title && a.location == b.location &&
           a.description == b.description && a.tags == b.tags && a.state == b.state && a.baseKey == b.baseKey &&
           a.conflict.has_value() == b.conflict.has_value() &&
           (!a.conflict || (a.conflict->reason == b.conflict->reason && a.conflict->previous == b.conflict->previous &&
                            a.conflict->observedSourceRevision == b.conflict->observedSourceRevision));
}

// Hidden nodes and conflicts may move only because their siblings moved.
[[nodiscard]] bool onlyPositionChanged(const BookmarkNode& a, const BookmarkNode& b) {
    BookmarkNode copy = b;
    copy.position = a.position;
    return sameContent(a, copy);
}

[[nodiscard]] bool sameBase(const BaseSnapshot& a, const BaseSnapshot& b) {
    if (a.sourceRevision != b.sourceRevision || a.nodes.size() != b.nodes.size()) return false;
    const auto sorted = [](const BaseSnapshot& base) {
        std::vector<const BaseNode*> nodes;
        nodes.reserve(base.nodes.size());
        for (const auto& node : base.nodes) nodes.push_back(&node);
        std::sort(nodes.begin(), nodes.end(), [](const BaseNode* x, const BaseNode* y) { return x->key < y->key; });
        return nodes;
    };
    const auto left = sorted(a);
    const auto right = sorted(b);
    for (std::size_t i = 0; i < left.size(); ++i) {
        const auto& x = *left[i];
        const auto& y = *right[i];
        if (x.key != y.key || x.parentKey != y.parentKey || x.position != y.position || x.title != y.title ||
            x.location != y.location) {
            return false;
        }
    }
    return true;
}

} // namespace

struct SqliteBookmarkOverlay::Impl final {
    sqlite3* db{};
    mutable std::mutex mutex;
    std::function<void(std::string_view)> faultHook;

    // The last checked tree per document, valid only while the stored revision
    // is unchanged. Every write anywhere bumps the revision, so a stale entry is
    // detected inside the write transaction and simply reloaded.
    struct Checked final {
        std::int64_t revision{};
        OverlayTreeIndex tree; // nodes without notes and tags
    };
    std::unordered_map<std::string, Checked> checked;

    void step(std::string_view name) const {
        if (faultHook) faultHook(name);
    }

    [[nodiscard]] std::int64_t revision(std::string_view documentId) const {
        Statement statement{db, "SELECT revision FROM bookmark_overlays WHERE document_id = ?1"};
        statement.text(1, documentId);
        return statement.row() ? statement.int64(0) : 0;
    }

    void ensureOverlayRow(std::string_view documentId) const {
        Statement statement{db, "INSERT INTO bookmark_overlays(document_id, revision) VALUES (?1, 0) "
                                "ON CONFLICT(document_id) DO NOTHING"};
        statement.text(1, documentId);
        statement.run();
    }

    [[nodiscard]] std::int64_t bumpRevision(std::string_view documentId) const {
        Statement update{db, "UPDATE bookmark_overlays SET revision = revision + 1, "
                             "updated_utc_ms = unixepoch('subsec') * 1000 WHERE document_id = ?1"};
        update.text(1, documentId);
        update.run();
        return revision(documentId);
    }

    [[nodiscard]] std::optional<BaseSnapshot> base(std::string_view documentId) const {
        Statement snapshot{db, "SELECT source_revision FROM bookmark_base_snapshots WHERE document_id = ?1"};
        snapshot.text(1, documentId);
        if (!snapshot.row()) return std::nullopt;
        BaseSnapshot result;
        result.sourceRevision = snapshot.string(0);
        Statement nodes{db, "SELECT base_key, parent_base_key, position, title, location FROM bookmark_base_nodes "
                            "WHERE document_id = ?1 ORDER BY parent_base_key, position"};
        nodes.text(1, documentId);
        while (nodes.row()) {
            BaseNode node;
            node.key = nodes.string(0);
            node.parentKey = nodes.optionalString(1);
            node.position = static_cast<std::uint32_t>(nodes.int64(2));
            node.title = nodes.string(3);
            node.location = nodes.optionalString(4).value_or(std::string{});
            result.nodes.push_back(std::move(node));
        }
        return result;
    }

    // Live nodes without notes and tags: what a change needs to check the tree,
    // the states and "no false Embedded". Changed nodes arrive complete.
    [[nodiscard]] std::vector<BookmarkNode> structure(std::string_view documentId) const {
        std::vector<BookmarkNode> result;
        Statement nodes{db,
            "SELECT node_id, parent_id, position, title, location, storage_state, base_key, local_revision, "
            "conflict_reason, conflict_previous_state, conflict_source_revision "
            "FROM bookmark_nodes WHERE document_id = ?1 AND deleted_utc_ms IS NULL"};
        nodes.text(1, documentId);
        while (nodes.row()) {
            BookmarkNode node;
            node.id = nodes.string(0);
            node.documentId = std::string{documentId};
            node.parentId = nodes.optionalString(1);
            node.position = static_cast<std::uint32_t>(nodes.int64(2));
            node.title = nodes.string(3);
            node.location = nodes.optionalString(4).value_or(std::string{});
            node.state = storedState(nodes.string(5));
            node.baseKey = nodes.optionalString(6);
            node.localRevision = nodes.int64(7);
            if (!nodes.isNull(8)) {
                const auto reason = parseConflictReasonName(nodes.string(8));
                if (!reason) throw std::runtime_error("The profile contains an unknown bookmark conflict.");
                node.conflict = ConflictInfo{*reason, storedState(nodes.string(9)),
                                             nodes.optionalString(10).value_or(std::string{})};
            }
            result.push_back(std::move(node));
        }
        return result;
    }

    // Notes and tags of one node, to compare a changed node with what is stored.
    void fillNotes(BookmarkNode& node) const {
        node.tags.clear();
        Statement description{db, "SELECT description FROM bookmark_nodes WHERE node_id = ?1"};
        description.text(1, node.id);
        if (description.row()) node.description = description.string(0);
        Statement tags{db, "SELECT tag FROM bookmark_node_tags WHERE node_id = ?1 ORDER BY position"};
        tags.text(1, node.id);
        while (tags.row()) node.tags.push_back(tags.string(0));
    }

    [[nodiscard]] std::vector<BookmarkNode> liveNodes(std::string_view documentId) const {
        std::vector<BookmarkNode> result;
        std::unordered_map<std::string, std::size_t> indexById;
        Statement nodes{db,
            "SELECT node_id, parent_id, position, title, location, description, storage_state, base_key, "
            "local_revision, conflict_reason, conflict_previous_state, conflict_source_revision "
            "FROM bookmark_nodes WHERE document_id = ?1 AND deleted_utc_ms IS NULL ORDER BY parent_id, position"};
        nodes.text(1, documentId);
        while (nodes.row()) {
            BookmarkNode node;
            node.id = nodes.string(0);
            node.documentId = std::string{documentId};
            node.parentId = nodes.optionalString(1);
            node.position = static_cast<std::uint32_t>(nodes.int64(2));
            node.title = nodes.string(3);
            node.location = nodes.optionalString(4).value_or(std::string{});
            node.description = nodes.string(5);
            node.state = storedState(nodes.string(6));
            node.baseKey = nodes.optionalString(7);
            node.localRevision = nodes.int64(8);
            if (!nodes.isNull(9)) {
                const auto reason = parseConflictReasonName(nodes.string(9));
                if (!reason) throw std::runtime_error("The profile contains an unknown bookmark conflict.");
                node.conflict = ConflictInfo{*reason, storedState(nodes.string(10)),
                                             nodes.optionalString(11).value_or(std::string{})};
            }
            indexById.emplace(node.id, result.size());
            result.push_back(std::move(node));
        }
        Statement tags{db, "SELECT t.node_id, t.tag FROM bookmark_node_tags t JOIN bookmark_nodes n "
                           "ON n.node_id = t.node_id WHERE n.document_id = ?1 AND n.deleted_utc_ms IS NULL "
                           "ORDER BY t.node_id, t.position"};
        tags.text(1, documentId);
        while (tags.row()) {
            const auto found = indexById.find(tags.string(0));
            if (found != indexById.end()) result[found->second].tags.push_back(tags.string(1));
        }
        return result;
    }

};

namespace {

// Prepared once per transaction; writes a node and replaces its tags.
class NodeWriter final {
public:
    explicit NodeWriter(sqlite3* db)
        : upsert_{db,
              "INSERT INTO bookmark_nodes(node_id, document_id, parent_id, position, title, location, description, "
              "storage_state, base_key, local_revision, conflict_reason, conflict_previous_state, "
              "conflict_source_revision) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13) "
              "ON CONFLICT(node_id) DO UPDATE SET parent_id = excluded.parent_id, position = excluded.position, "
              "title = excluded.title, location = excluded.location, description = excluded.description, "
              "storage_state = excluded.storage_state, base_key = excluded.base_key, "
              "local_revision = excluded.local_revision, conflict_reason = excluded.conflict_reason, "
              "conflict_previous_state = excluded.conflict_previous_state, "
              "conflict_source_revision = excluded.conflict_source_revision, "
              "updated_utc_ms = unixepoch('subsec') * 1000, deleted_utc_ms = NULL"},
          clearTags_{db, "DELETE FROM bookmark_node_tags WHERE node_id = ?1"},
          tag_{db, "INSERT INTO bookmark_node_tags(node_id, position, tag) VALUES (?1, ?2, ?3)"} {}

    void write(const BookmarkNode& node) {
        upsert_.reset();
        upsert_.text(1, node.id);
        upsert_.text(2, node.documentId);
        upsert_.optionalText(3, node.parentId);
        upsert_.integer(4, node.position);
        upsert_.text(5, node.title);
        upsert_.textOrNull(6, node.location);
        upsert_.text(7, node.description);
        upsert_.text(8, stateName(node.state));
        upsert_.optionalText(9, node.baseKey);
        upsert_.integer(10, node.localRevision);
        if (node.conflict) {
            upsert_.text(11, conflictReasonName(node.conflict->reason));
            upsert_.text(12, stateName(node.conflict->previous));
            upsert_.text(13, node.conflict->observedSourceRevision);
        } else {
            upsert_.null(11);
            upsert_.null(12);
            upsert_.null(13);
        }
        upsert_.run();

        clearTags_.reset();
        clearTags_.text(1, node.id);
        clearTags_.run();
        for (std::size_t i = 0; i < node.tags.size(); ++i) {
            tag_.reset();
            tag_.text(1, node.id);
            tag_.integer(2, static_cast<std::int64_t>(i));
            tag_.text(3, node.tags[i]);
            tag_.run();
        }
    }

private:
    Statement upsert_;
    Statement clearTags_;
    Statement tag_;
};

} // namespace

SqliteBookmarkOverlay::SqliteBookmarkOverlay(const std::filesystem::path& databasePath)
    : impl_{std::make_unique<Impl>()} {
    const auto path = pathToUtf8(databasePath);
    // No CREATE flag: the overlay never makes a profile on its own.
    if (sqlite3_open_v2(path.c_str(), &impl_->db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_FULLMUTEX, nullptr) != SQLITE_OK) {
        const std::string message = impl_->db == nullptr ? "out of memory" : sqlite3_errmsg(impl_->db);
        sqlite3_close_v2(impl_->db);
        impl_->db = nullptr;
        throw std::runtime_error("Could not open the Atlas profile for bookmarks: " + message);
    }
    try {
        sqlite3_busy_timeout(impl_->db, 5000);
        execute(impl_->db, "PRAGMA foreign_keys = ON");
        execute(impl_->db, "PRAGMA synchronous = FULL");
        if (scalar(impl_->db, "PRAGMA application_id") != profileApplicationId) {
            throw std::runtime_error("The selected database is not an Atlas profile.");
        }
        if (scalar(impl_->db, "PRAGMA user_version") < minimumSchemaVersion) {
            throw std::runtime_error("The Atlas profile has not been upgraded for bookmarks yet.");
        }
    } catch (...) {
        sqlite3_close_v2(impl_->db);
        impl_->db = nullptr;
        throw;
    }
}

SqliteBookmarkOverlay::~SqliteBookmarkOverlay() {
    if (impl_ && impl_->db != nullptr) sqlite3_close_v2(impl_->db);
}

void SqliteBookmarkOverlay::setFaultHookForTesting(std::function<void(std::string_view)> hook) {
    std::lock_guard lock{impl_->mutex};
    impl_->faultHook = std::move(hook);
}

OverlaySnapshot SqliteBookmarkOverlay::load(std::string_view documentId) const {
    std::lock_guard lock{impl_->mutex};
    // One read transaction so revision, base and nodes come from the same moment.
    execute(impl_->db, "BEGIN");
    try {
        OverlaySnapshot snapshot;
        snapshot.revision = impl_->revision(documentId);
        snapshot.base = impl_->base(documentId);
        snapshot.nodes = impl_->liveNodes(documentId);
        execute(impl_->db, "COMMIT");
        return snapshot;
    } catch (...) {
        sqlite3_exec(impl_->db, "ROLLBACK", nullptr, nullptr, nullptr);
        throw;
    }
}

BaseAdoption SqliteBookmarkOverlay::adoptBase(std::string_view documentId, const BaseSnapshot& base) {
    if (auto problem = validateBase(base); !problem.empty()) throw OverlayRuleViolation{problem};
    std::lock_guard lock{impl_->mutex};
    Transaction transaction{impl_->db};

    if (const auto stored = impl_->base(documentId)) {
        return sameBase(*stored, base) ? BaseAdoption::unchanged : BaseAdoption::needsReconcile;
    }

    impl_->ensureOverlayRow(documentId);
    {
        Statement snapshot{impl_->db,
            "INSERT INTO bookmark_base_snapshots(document_id, source_revision) VALUES (?1, ?2)"};
        snapshot.text(1, documentId);
        snapshot.text(2, base.sourceRevision);
        snapshot.run();
        Statement node{impl_->db,
            "INSERT INTO bookmark_base_nodes(document_id, base_key, parent_base_key, position, title, location) "
            "VALUES (?1, ?2, ?3, ?4, ?5, ?6)"};
        for (const auto& source : base.nodes) {
            node.reset();
            node.text(1, documentId);
            node.text(2, source.key);
            node.optionalText(3, source.parentKey);
            node.integer(4, source.position);
            node.text(5, source.title);
            node.textOrNull(6, source.location);
            node.run();
        }
    }

    // Local nodes made before the outline was first read stay, after it.
    auto existing = impl_->liveNodes(documentId);
    const auto embeddedRoots = static_cast<std::uint32_t>(
        std::count_if(base.nodes.begin(), base.nodes.end(), [](const BaseNode& n) { return !n.parentKey; }));
    auto embedded = embeddedNodesFromBase(documentId, base);
    std::vector<BookmarkNode> combined = embedded;
    NodeWriter writer{impl_->db};
    for (auto& node : existing) {
        if (!node.parentId && embeddedRoots > 0) {
            node.position += embeddedRoots;
            ++node.localRevision;
            writer.write(node);
        }
        combined.push_back(node);
    }
    if (auto problem = validateTree(combined); !problem.empty()) throw OverlayRuleViolation{problem};
    if (auto problem = validateAgainstBase(combined, base); !problem.empty()) throw OverlayRuleViolation{problem};
    for (const auto& node : embedded) writer.write(node);
    (void)impl_->bumpRevision(documentId);

    impl_->step("adoptBase:beforeCommit");
    impl_->checked.erase(std::string{documentId});
    transaction.commit();
    return BaseAdoption::adopted;
}

std::int64_t SqliteBookmarkOverlay::apply(std::string_view documentId, std::int64_t expectedRevision,
                                          const OverlayChangeSet& change) {
    std::lock_guard lock{impl_->mutex};
    Transaction transaction{impl_->db};

    const auto current = impl_->revision(documentId);
    if (current != expectedRevision) {
        throw OverlayRevisionConflict{"The bookmark overlay changed since it was loaded."};
    }
    if (change.upserts.empty() && change.removals.empty() && change.restores.empty()) return current;
    impl_->ensureOverlayRow(documentId);

    const std::string key{documentId};
    auto cached = impl_->checked.find(key);
    if (cached == impl_->checked.end() || cached->second.revision != current) {
        Impl::Checked fresh{current, OverlayTreeIndex{key, impl_->structure(documentId), impl_->base(documentId)}};
        cached = impl_->checked.insert_or_assign(key, std::move(fresh)).first;
    }
    OverlayTreeIndex& tree = cached->second.tree;

    std::unordered_set<std::string_view> touched;
    for (const auto& id : change.removals) {
        if (!touched.insert(id).second) throw OverlayRuleViolation{"duplicateChange"};
        const auto* found = tree.find(id);
        if (found == nullptr) throw OverlayRuleViolation{"unknownNode"};
        // A node that is in the PDF is hidden in Atlas, never silently dropped.
        if (found->baseKey) throw OverlayRuleViolation{"sourceNodeMustBeHidden"};
    }

    std::vector<BookmarkNode> prepared;
    prepared.reserve(change.upserts.size());
    std::unordered_set<std::string_view> restoring;
    for (const auto& id : change.restores) {
        if (!restoring.insert(id).second) throw OverlayRuleViolation{"duplicateChange"};
    }
    Statement owner{impl_->db,
        "SELECT document_id, deleted_utc_ms IS NOT NULL, local_revision FROM bookmark_nodes WHERE node_id = ?1"};
    for (const auto& requested : change.upserts) {
        if (!touched.insert(requested.id).second) throw OverlayRuleViolation{"duplicateChange"};
        if (requested.documentId != documentId) throw OverlayRuleViolation{"wrongDocument"};
        if (auto problem = validateNode(requested); !problem.empty()) throw OverlayRuleViolation{problem};

        BookmarkNode next = requested;
        if (const auto* stored = tree.find(requested.id); stored != nullptr) {
            BookmarkNode previous = *stored;
            impl_->fillNotes(previous);
            if (!isOrdinaryStateChange(previous.state, next.state)) throw OverlayRuleViolation{"illegalTransition"};
            const bool keepLocalFromConflict =
                previous.state == StorageState::conflict && next.state == StorageState::localOnly && !next.baseKey;
            if (next.baseKey != previous.baseKey && !keepLocalFromConflict) {
                throw OverlayRuleViolation{"baseKeyChanged"};
            }
            const bool frozen = previous.state == StorageState::locallyHidden ||
                                previous.state == StorageState::conflict;
            if (frozen && next.state == previous.state && !onlyPositionChanged(previous, next)) {
                throw OverlayRuleViolation{"editWhileHiddenOrConflict"};
            }
            next.localRevision = previous.localRevision + (sameContent(previous, next) ? 0 : 1);
        } else {
            owner.reset();
            owner.text(1, requested.id);
            std::int64_t revisionBase = 0;
            if (owner.row()) {
                // A retired ID comes back only as an explicit restore of the same
                // document's removed local node (undo of a removal).
                const bool restorable = restoring.erase(requested.id) == 1 && owner.string(0) == documentId &&
                                        owner.int64(1) != 0;
                if (!restorable) throw OverlayRuleViolation{"idInUse"};
                revisionBase = owner.int64(2);
            } else if (restoring.contains(requested.id)) {
                throw OverlayRuleViolation{"restoreWithoutTombstone"};
            }
            if (next.state != StorageState::localOnly && next.state != StorageState::pendingEmbed) {
                throw OverlayRuleViolation{"newNodeState"};
            }
            next.localRevision = revisionBase + 1;
        }
        prepared.push_back(std::move(next));
    }
    if (!restoring.empty()) throw OverlayRuleViolation{"restoreWithoutRecord"};

    // Only what the change touches is checked; on refusal the tree is unchanged.
    std::vector<BookmarkNode> structural = prepared;
    for (auto& node : structural) {
        node.description.clear();
        node.tags.clear();
    }
    if (auto problem = tree.apply(structural, change.removals); !problem.empty()) {
        throw OverlayRuleViolation{problem};
    }
    try {
#ifndef NDEBUG
        // Debug builds prove the incremental verdict against the full rules.
        const auto all = tree.nodes();
        if (!validateStructure(all).empty() || !validateAgainstBase(all, tree.base()).empty()) {
            throw std::logic_error("Incremental overlay validation accepted a change the full rules refuse.");
        }
#endif
        {
            Statement retire{impl_->db, "UPDATE bookmark_nodes SET deleted_utc_ms = unixepoch('subsec') * 1000, "
                                        "updated_utc_ms = unixepoch('subsec') * 1000 WHERE node_id = ?1"};
            for (const auto& id : change.removals) {
                retire.reset();
                retire.text(1, id);
                retire.run();
            }
        }
        {
            NodeWriter writer{impl_->db};
            for (const auto& node : prepared) writer.write(node);
        }
        const auto next = impl_->bumpRevision(documentId);
        impl_->step("apply:beforeCommit");
        transaction.commit();
        cached->second.revision = next;
        return next;
    } catch (...) {
        impl_->checked.erase(key); // the tree may be ahead of the database
        throw;
    }
}

std::vector<std::string> SqliteBookmarkOverlay::documentsWithLocalWork() const {
    std::lock_guard lock{impl_->mutex};
    Statement statement{impl_->db, "SELECT DISTINCT document_id FROM bookmark_nodes WHERE deleted_utc_ms IS NULL "
                                   "AND storage_state <> 'embedded' ORDER BY document_id"};
    std::vector<std::string> result;
    while (statement.row()) result.push_back(statement.string(0));
    return result;
}

} // namespace atlas::bookmarks
