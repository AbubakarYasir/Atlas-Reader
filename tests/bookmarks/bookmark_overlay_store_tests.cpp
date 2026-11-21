// N5.2: the overlay repository on a real profile database. Persistence of every
// storage state, exact Unicode text, the "no false Embedded" rule, revision
// conflicts, tombstones, migration from schema 4, a 10,000-node overlay, and
// crash recovery: this executable re-runs itself as a child process that ends
// abruptly inside a write transaction, then checks what survived.

#include "app/bookmarks/SqliteBookmarkOverlay.h"
#include "app/index/SqliteLibraryIndex.h"
#include "core/bookmarks/LocationEnvelope.h"

#include <sqlite3.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

using namespace atlas::bookmarks;
namespace fs = std::filesystem;

fs::path selfPath;

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

class TemporaryDirectory final {
public:
    TemporaryDirectory() {
        static std::atomic<std::uint64_t> sequence{};
        const auto parent = fs::temp_directory_path();
        for (int attempt = 0; attempt < 100; ++attempt) {
            const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
            path_ = parent / ("atlas-overlay-test-" + std::to_string(nonce) + "-" + std::to_string(sequence++));
            std::error_code error;
            if (fs::create_directory(path_, error)) return;
        }
        throw std::runtime_error("Could not reserve a unique overlay-test directory.");
    }
    ~TemporaryDirectory() {
        std::error_code ignored;
        fs::remove_all(path_, ignored);
    }
    [[nodiscard]] const fs::path& path() const { return path_; }

private:
    fs::path path_;
};

// A profile with the library schema and two documents.
fs::path makeProfile(const TemporaryDirectory& temporary) {
    const auto database = temporary.path() / "profile.sqlite3";
    atlas::index::SqliteLibraryIndex index{database};
    index.registerRoot({"root", temporary.path() / "books", atlas::index::LibraryRootAvailability::available, 0,
                        std::nullopt, 0});
    index.upsertRecord({"doc-ar", temporary.path() / "books" / "كتاب.pdf", "كتاب", "",
                        atlas::document::Availability::available},
                       "root");
    index.upsertRecord({"doc-en", temporary.path() / "books" / "book.pdf", "Book", "",
                        atlas::document::Availability::available},
                       "root");
    return database;
}

std::string pdfLocation(std::uint32_t page, double top) {
    LocationEnvelope envelope;
    envelope.pdf.pageIndex = page;
    envelope.pdf.kind = DestinationKind::xyz;
    envelope.pdf.left = 0.0;
    envelope.pdf.top = top;
    return encodeLocation(envelope);
}

BaseSnapshot sampleBase() {
    BaseSnapshot base;
    base.sourceRevision = "size=2048;mtime=1700000000000";
    base.nodes = {{"0", std::nullopt, 0, "الفصل الأول", pdfLocation(0, 0.0)},
                  {"0/0", "0", 0, "المبحث الأول", pdfLocation(1, 0.25)},
                  {"1", std::nullopt, 1, "Chapter Two", pdfLocation(5, 0.0)}};
    return base;
}

BookmarkNode local(std::string id, std::string documentId, std::optional<std::string> parent, std::uint32_t position,
                   std::string title, StorageState state = StorageState::localOnly) {
    BookmarkNode node;
    node.id = std::move(id);
    node.documentId = std::move(documentId);
    node.parentId = std::move(parent);
    node.position = position;
    node.title = std::move(title);
    node.state = state;
    return node;
}

const BookmarkNode& find(const OverlaySnapshot& snapshot, std::string_view id) {
    for (const auto& node : snapshot.nodes) {
        if (node.id == id) return node;
    }
    throw std::runtime_error("Node not found: " + std::string{id});
}

const BookmarkNode& findByBase(const OverlaySnapshot& snapshot, std::string_view key) {
    for (const auto& node : snapshot.nodes) {
        if (node.baseKey && *node.baseKey == key) return node;
    }
    throw std::runtime_error("Base node not found: " + std::string{key});
}

template <typename Function>
std::string refusal(Function&& function) {
    try {
        function();
    } catch (const OverlayRuleViolation& violation) {
        return violation.reason();
    }
    return {};
}

std::int64_t rawInteger(const fs::path& database, const char* sql) {
    sqlite3* db = nullptr;
    const auto utf8 = database.generic_u8string();
    require(sqlite3_open_v2(reinterpret_cast<const char*>(utf8.c_str()), &db, SQLITE_OPEN_READONLY, nullptr) ==
                SQLITE_OK,
            "Could not open the profile for inspection.");
    sqlite3_stmt* statement = nullptr;
    require(sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) == SQLITE_OK, "Could not prepare an inspection.");
    require(sqlite3_step(statement) == SQLITE_ROW, "The inspection returned no row.");
    const auto value = sqlite3_column_int64(statement, 0);
    sqlite3_finalize(statement);
    sqlite3_close_v2(db);
    return value;
}

std::string rawText(const fs::path& database, const char* sql) {
    sqlite3* db = nullptr;
    const auto utf8 = database.generic_u8string();
    require(sqlite3_open_v2(reinterpret_cast<const char*>(utf8.c_str()), &db, SQLITE_OPEN_READONLY, nullptr) ==
                SQLITE_OK,
            "Could not open the profile for inspection.");
    sqlite3_stmt* statement = nullptr;
    require(sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) == SQLITE_OK, "Could not prepare an inspection.");
    require(sqlite3_step(statement) == SQLITE_ROW, "The inspection returned no row.");
    std::string value{reinterpret_cast<const char*>(sqlite3_column_text(statement, 0)),
                      static_cast<std::size_t>(sqlite3_column_bytes(statement, 0))};
    sqlite3_finalize(statement);
    sqlite3_close_v2(db);
    return value;
}

void checkOpenRules() {
    TemporaryDirectory temporary;
    const auto missing = temporary.path() / "missing.sqlite3";
    bool refused = false;
    try {
        SqliteBookmarkOverlay overlay{missing};
    } catch (const std::runtime_error&) {
        refused = true;
    }
    require(refused && !fs::exists(missing), "The overlay never creates a profile on its own.");

    const auto stranger = temporary.path() / "stranger.sqlite3";
    {
        sqlite3* db = nullptr;
        const auto utf8 = stranger.generic_u8string();
        sqlite3_open(reinterpret_cast<const char*>(utf8.c_str()), &db);
        sqlite3_exec(db, "CREATE TABLE other(x); PRAGMA user_version = 5;", nullptr, nullptr, nullptr);
        sqlite3_close_v2(db);
    }
    refused = false;
    try {
        SqliteBookmarkOverlay overlay{stranger};
    } catch (const std::runtime_error&) {
        refused = true;
    }
    require(refused, "A database that is not an Atlas profile is refused.");
}

void checkMigrationFromSchema4() {
    TemporaryDirectory temporary;
    const auto database = makeProfile(temporary);
    {
        // Turn the fresh profile back into a schema-4 profile with data.
        sqlite3* db = nullptr;
        const auto utf8 = database.generic_u8string();
        sqlite3_open(reinterpret_cast<const char*>(utf8.c_str()), &db);
        require(sqlite3_exec(db,
                             "DROP TABLE bookmark_node_tags; DROP TABLE bookmark_nodes; DROP TABLE bookmark_base_nodes;"
                             "DROP TABLE bookmark_base_snapshots; DROP TABLE bookmark_overlays; PRAGMA user_version = 4;",
                             nullptr, nullptr, nullptr) == SQLITE_OK,
                "Could not prepare a schema-4 profile.");
        sqlite3_close_v2(db);
    }
    bool refused = false;
    try {
        SqliteBookmarkOverlay overlay{database};
    } catch (const std::runtime_error&) {
        refused = true;
    }
    require(refused, "The overlay waits for the library index to upgrade the profile.");

    atlas::index::SqliteLibraryIndex index{database};
    require(index.schemaVersion() == 5, "The index upgrades a schema-4 profile to 5.");
    require(index.search("كتاب").size() == 1, "Library data survives the upgrade.");
    SqliteBookmarkOverlay overlay{database};
    require(overlay.load("doc-ar").revision == 0, "An upgraded profile starts with an empty overlay.");
}

void checkBaseAdoption() {
    TemporaryDirectory temporary;
    const auto database = makeProfile(temporary);
    SqliteBookmarkOverlay overlay{database};

    // Local work made before the outline was read keeps its place after it.
    const auto first = overlay.apply("doc-ar", 0, {{local("early", "doc-ar", std::nullopt, 0, "ملاحظة")}, {}});
    require(first == 1, "The first change creates revision 1.");

    require(overlay.adoptBase("doc-ar", sampleBase()) == BaseAdoption::adopted, "The first outline is adopted.");
    auto snapshot = overlay.load("doc-ar");
    require(snapshot.base && snapshot.base->nodes.size() == 3, "The base snapshot is stored.");
    require(snapshot.nodes.size() == 4, "Embedded nodes join the local node.");
    require(findByBase(snapshot, "0").state == StorageState::embedded &&
                findByBase(snapshot, "0").title == "الفصل الأول",
            "Seeded nodes are Embedded copies of the outline.");
    require(find(snapshot, "early").position == 2 && find(snapshot, "early").state == StorageState::localOnly,
            "Earlier local nodes follow the outline at the root.");
    require(findByBase(snapshot, "0/0").parentId == findByBase(snapshot, "0").id,
            "The outline hierarchy is kept.");

    require(overlay.adoptBase("doc-ar", sampleBase()) == BaseAdoption::unchanged, "The same outline is unchanged.");
    auto changed = sampleBase();
    changed.nodes[2].title = "Chapter 2 (revised)";
    const auto before = overlay.load("doc-ar").revision;
    require(overlay.adoptBase("doc-ar", changed) == BaseAdoption::needsReconcile,
            "A different outline needs reconciling.");
    auto newer = sampleBase();
    newer.sourceRevision = "size=4096;mtime=1800000000000";
    require(overlay.adoptBase("doc-ar", newer) == BaseAdoption::needsReconcile,
            "A different revision needs reconciling.");
    const auto after = overlay.load("doc-ar");
    const auto chapterTwoTitle = [&] {
        for (const auto& node : after.base->nodes) {
            if (node.key == "1") return node.title;
        }
        return std::string{};
    }();
    require(after.revision == before && chapterTwoTitle == "Chapter Two" &&
                after.base->sourceRevision == sampleBase().sourceRevision,
            "A changed outline never overwrites the base or the overlay here.");

    BaseSnapshot broken = sampleBase();
    broken.nodes[1].parentKey = "missing";
    require(refusal([&] { (void)overlay.adoptBase("doc-en", broken); }) == "missingParent",
            "A broken outline is refused whole.");
    require(overlay.load("doc-en").revision == 0, "A refused outline writes nothing.");

    bool refused = false;
    try {
        (void)overlay.adoptBase("no-such-document", sampleBase());
    } catch (const std::runtime_error&) {
        refused = true;
    }
    require(refused, "An overlay belongs to a document the library knows.");
}

void checkStatesAndRules() {
    TemporaryDirectory temporary;
    const auto database = makeProfile(temporary);
    std::int64_t revision = 0;
    {
        SqliteBookmarkOverlay overlay{database};
        require(overlay.adoptBase("doc-ar", sampleBase()) == BaseAdoption::adopted, "Outline adopted.");
        auto snapshot = overlay.load("doc-ar");
        revision = snapshot.revision;
        const auto chapter = findByBase(snapshot, "0");
        const auto section = findByBase(snapshot, "0/0");
        const auto chapterTwo = findByBase(snapshot, "1");

        // Exact Unicode: duplicates, spaces, harakat, joiners, emoji and U+0000.
        const std::string arabic = "  المبحث الأول  ";
        const std::string urdu = "اُردُو‍متن";
        const std::string mixed = std::string{"Notes \xF0\x9F\x93\x9A "} + std::string{"\0end", 4};
        auto note = local("note-1", "doc-ar", chapter.id, 1, arabic);
        note.description = "وصف طويل\nwith a second line";
        note.tags = {"فقه", "Hadith", "فقه"};
        note.location = pdfLocation(2, 0.5);
        auto duplicate = local("note-2", "doc-ar", chapter.id, 2, arabic, StorageState::pendingEmbed);
        auto urduNode = local("note-3", "doc-ar", std::nullopt, 2, urdu);
        auto mixedNode = local("note-4", "doc-ar", "note-3", 0, mixed);
        mixedNode.location = "atlas-location/9;scheme=pdf;page=4;future=yes";
        revision = overlay.apply("doc-ar", revision, {{note, duplicate, urduNode, mixedNode}, {}});

        // Editing a source node is a LocalOverride, never a quiet Embedded.
        auto renamed = chapterTwo;
        renamed.title = "Chapter 2 — my notes";
        require(refusal([&] { overlay.apply("doc-ar", revision, {{renamed}, {}}); }) == "embeddedDiffersFromSource",
                "A changed source node cannot stay Embedded.");
        renamed.state = StorageState::localOverride;
        revision = overlay.apply("doc-ar", revision, {{renamed}, {}});

        // Hide a source node; a hidden node cannot be edited.
        auto hidden = section;
        hidden.state = StorageState::locallyHidden;
        revision = overlay.apply("doc-ar", revision, {{hidden}, {}});
        auto hiddenEdit = hidden;
        hiddenEdit.title = "Edited while hidden";
        require(refusal([&] { overlay.apply("doc-ar", revision, {{hiddenEdit}, {}}); }) == "editWhileHiddenOrConflict",
                "Hidden nodes are unhidden before editing.");

        // Source nodes are hidden, never removed by Atlas alone.
        require(refusal([&] { overlay.apply("doc-ar", revision, {{}, {chapter.id}}); }) == "sourceNodeMustBeHidden",
                "Removing a node that is in the PDF is refused.");

        // New nodes cannot claim to be Embedded; local work cannot jump there.
        auto fake = local("fake", "doc-ar", std::nullopt, 3, "Fake");
        fake.state = StorageState::embedded;
        fake.baseKey = "1";
        require(refusal([&] { overlay.apply("doc-ar", revision, {{fake}, {}}); }) == "newNodeState",
                "A new node cannot start Embedded.");
        auto promoted = duplicate;
        promoted.state = StorageState::embedded;
        promoted.baseKey = "0/0";
        require(refusal([&] { overlay.apply("doc-ar", revision, {{promoted}, {}}); }) == "illegalTransition",
                "Pending work becomes Embedded only through a verified save (N5.4).");

        // A conflict records what it was and resolves explicitly.
        auto conflicted = find(overlay.load("doc-ar"), renamed.id);
        conflicted.state = StorageState::conflict;
        conflicted.conflict = ConflictInfo{ConflictReason::sourceChanged, StorageState::localOverride, "size=1;mtime=2"};
        revision = overlay.apply("doc-ar", revision, {{conflicted}, {}});

        // Stale writers are refused and change nothing.
        bool stale = false;
        try {
            overlay.apply("doc-ar", revision - 1, {{local("late", "doc-ar", std::nullopt, 3, "Late")}, {}});
        } catch (const OverlayRevisionConflict&) {
            stale = true;
        }
        require(stale, "A change based on an old revision is refused.");

        // Removing a local node leaves a tombstone; its ID is never reused.
        revision = overlay.apply("doc-ar", revision, {{}, {"note-2"}});
        require(refusal([&] { overlay.apply("doc-ar", revision, {{local("note-2", "doc-ar", chapter.id, 2, "Again")}, {}}); }) ==
                    "idInUse",
                "A removed node's ID is retired.");
        // A tree change that would orphan a child is refused whole.
        require(refusal([&] { overlay.apply("doc-ar", revision, {{}, {"note-3"}}); }) == "missingParent",
                "Removing a parent with live children is refused.");
        require(refusal([&] {
                    auto other = local("other-doc", "doc-en", std::nullopt, 0, "x");
                    overlay.apply("doc-ar", revision, {{other}, {}});
                }) == "wrongDocument",
                "A change set belongs to one document.");
    }

    // Everything above survives closing and reopening.
    SqliteBookmarkOverlay reopened{database};
    const auto snapshot = reopened.load("doc-ar");
    require(snapshot.revision == revision, "The revision persists.");
    const auto& note = find(snapshot, "note-1");
    require(note.title == "  المبحث الأول  " && note.description == "وصف طويل\nwith a second line" &&
                note.tags == std::vector<std::string>{"فقه", "Hadith", "فقه"} &&
                note.location == pdfLocation(2, 0.5) && note.state == StorageState::localOnly,
            "Local nodes keep exact text, tags in order and destinations.");
    require(find(snapshot, "note-3").title == "اُردُو‍متن", "Urdu with harakat and a joiner is kept.");
    const auto& mixedBack = find(snapshot, "note-4");
    require(mixedBack.title.size() == std::string{"Notes \xF0\x9F\x93\x9A "}.size() + 4 &&
                mixedBack.title.back() == 'd',
            "Text with U+0000 is kept byte for byte.");
    require(mixedBack.location == "atlas-location/9;scheme=pdf;page=4;future=yes",
            "A location from a newer build is kept unchanged.");
    require(findByBase(snapshot, "0/0").state == StorageState::locallyHidden, "Hidden state persists.");
    const auto& conflicted = findByBase(snapshot, "1");
    require(conflicted.state == StorageState::conflict && conflicted.conflict &&
                conflicted.conflict->reason == ConflictReason::sourceChanged &&
                conflicted.conflict->previous == StorageState::localOverride &&
                conflicted.title == "Chapter 2 — my notes" && conflicted.localRevision == 3,
            "Conflicts persist with their reason, previous state and local revision.");
    for (const auto& node : snapshot.nodes) {
        require(node.id != "note-2", "Removed nodes are not loaded.");
    }
    require(rawInteger(database, "SELECT count(*) FROM bookmark_nodes WHERE node_id = 'note-2' AND "
                                 "deleted_utc_ms IS NOT NULL") == 1,
            "A removed local node is kept as a tombstone.");
    require(reopened.documentsWithLocalWork() == std::vector<std::string>{"doc-ar"},
            "Documents with local research are listed for recovery.");

    // Resolve the conflict by keeping the local version; then unhide.
    auto keep = conflicted;
    keep.state = StorageState::localOnly;
    keep.baseKey.reset();
    keep.conflict.reset();
    auto unhide = findByBase(snapshot, "0/0");
    unhide.state = StorageState::embedded;
    const auto resolved = reopened.apply("doc-ar", snapshot.revision, {{keep, unhide}, {}});
    const auto finalSnapshot = reopened.load("doc-ar");
    require(resolved == snapshot.revision + 1 && find(finalSnapshot, keep.id).state == StorageState::localOnly &&
                !find(finalSnapshot, keep.id).baseKey && findByBase(finalSnapshot, "0/0").state == StorageState::embedded,
            "Keep-local and unhide-unchanged follow the table.");
}

void checkTenThousandNodes() {
    TemporaryDirectory temporary;
    const auto database = makeProfile(temporary);
    SqliteBookmarkOverlay overlay{database};

    // 100 chapters × 99 sections, alternating Arabic and English titles.
    std::vector<BookmarkNode> nodes;
    nodes.reserve(10'000);
    for (std::uint32_t chapter = 0; chapter < 100; ++chapter) {
        const auto chapterId = "c" + std::to_string(chapter);
        nodes.push_back(local(chapterId, "doc-en", std::nullopt, chapter,
                              chapter % 2 == 0 ? "الباب " + std::to_string(chapter) : "Part " + std::to_string(chapter)));
        for (std::uint32_t section = 0; section < 99; ++section) {
            auto node = local(chapterId + "-s" + std::to_string(section), "doc-en", chapterId, section,
                              section % 2 == 0 ? "فصل" : "Section"); // many duplicate titles
            node.location = pdfLocation(chapter * 10 + section % 10, 0.5);
            nodes.push_back(std::move(node));
        }
    }
    using Clock = std::chrono::steady_clock;
    const auto writeStart = Clock::now();
    const auto revision = overlay.apply("doc-en", 0, {nodes, {}});
    const auto writeMs = std::chrono::duration<double, std::milli>(Clock::now() - writeStart).count();

    const auto loadStart = Clock::now();
    const auto snapshot = overlay.load("doc-en");
    const auto loadMs = std::chrono::duration<double, std::milli>(Clock::now() - loadStart).count();
    require(snapshot.nodes.size() == 10'000, "All 10,000 nodes persist.");

    // Move one section to the front of another chapter (positions shift).
    std::vector<BookmarkNode> moves;
    auto moved = find(snapshot, "c5-s3");
    moved.parentId = "c7";
    moved.position = 0;
    moves.push_back(moved);
    for (std::uint32_t s = 4; s < 99; ++s) {
        auto sibling = find(snapshot, "c5-s" + std::to_string(s));
        --sibling.position;
        moves.push_back(sibling);
    }
    for (std::uint32_t s = 0; s < 99; ++s) {
        auto sibling = find(snapshot, "c7-s" + std::to_string(s));
        ++sibling.position;
        moves.push_back(sibling);
    }
    const auto moveStart = Clock::now();
    (void)overlay.apply("doc-en", revision, {moves, {}});
    const auto moveMs = std::chrono::duration<double, std::milli>(Clock::now() - moveStart).count();
    const auto after = overlay.load("doc-en");
    require(find(after, "c5-s3").parentId == "c7" && find(after, "c5-s3").localRevision == 2 &&
                find(after, "c7-s0").position == 1,
            "A move and its sibling shifts persist together.");
    std::cout << "10,000 nodes (build-dependent, informational): write " << writeMs << " ms, load " << loadMs
              << " ms, move with 197 shifts " << moveMs << " ms\n";
}

// Child process: crash at a named step, or right after committing.
int runChild(std::string_view mode, const fs::path& database) {
    SqliteBookmarkOverlay overlay{database};
    std::vector<BookmarkNode> nodes;
    for (std::uint32_t i = 0; i < 500; ++i) {
        nodes.push_back(local("crash-" + std::to_string(i), "doc-ar", std::nullopt, i + 1, "عقدة " + std::to_string(i)));
    }
    if (mode == "--crash-in-apply") {
        overlay.setFaultHookForTesting([](std::string_view step) {
            if (step == "apply:beforeCommit") std::_Exit(41);
        });
        (void)overlay.apply("doc-ar", 1, {nodes, {}});
    } else if (mode == "--crash-in-adopt") {
        overlay.setFaultHookForTesting([](std::string_view step) {
            if (step == "adoptBase:beforeCommit") std::_Exit(42);
        });
        (void)overlay.adoptBase("doc-en", sampleBase());
    } else if (mode == "--crash-after-commit") {
        (void)overlay.apply("doc-ar", 1, {nodes, {}});
        std::_Exit(43); // no destructor, no close: WAL recovery on next open
    }
    return 2; // the child should never get here
}

int runSelf(std::string_view mode, const fs::path& database) {
    std::string command = "\"" + selfPath.string() + "\" " + std::string{mode} + " \"" + database.string() + "\"";
#ifdef _WIN32
    command = "\"" + command + "\""; // cmd.exe strips one outer pair of quotes
#endif
    return std::system(command.c_str());
}

void checkCrashRecovery() {
    TemporaryDirectory temporary;
    const auto database = makeProfile(temporary);
    {
        SqliteBookmarkOverlay overlay{database};
        require(overlay.apply("doc-ar", 0, {{local("keep", "doc-ar", std::nullopt, 0, "يبقى")}, {}}) == 1,
                "Baseline written.");
    }

    require(runSelf("--crash-in-apply", database) != 0, "The child ends abruptly inside the transaction.");
    {
        SqliteBookmarkOverlay overlay{database};
        const auto snapshot = overlay.load("doc-ar");
        require(snapshot.revision == 1 && snapshot.nodes.size() == 1 && snapshot.nodes.front().title == "يبقى",
                "A crash before commit leaves the previous overlay exactly.");
    }
    require(runSelf("--crash-in-adopt", database) != 0, "The child ends abruptly inside base adoption.");
    {
        SqliteBookmarkOverlay overlay{database};
        const auto snapshot = overlay.load("doc-en");
        require(snapshot.revision == 0 && !snapshot.base && snapshot.nodes.empty(),
                "A crash during base adoption leaves no partial base or nodes.");
        require(overlay.adoptBase("doc-en", sampleBase()) == BaseAdoption::adopted,
                "Base adoption succeeds after the crash.");
    }
    require(runSelf("--crash-after-commit", database) != 0, "The child ends abruptly after committing.");
    {
        SqliteBookmarkOverlay overlay{database};
        const auto snapshot = overlay.load("doc-ar");
        require(snapshot.revision == 2 && snapshot.nodes.size() == 501,
                "A committed change survives a crash before the profile is closed.");
    }
    require(rawText(database, "PRAGMA integrity_check") == "ok", "The profile passes SQLite's integrity check.");
    require(rawInteger(database, "SELECT count(*) FROM pragma_foreign_key_check") == 0,
            "No foreign key is left dangling.");
}

} // namespace

int main(int argc, char** argv) {
    selfPath = fs::absolute(argv[0]);
    if (argc == 3) return runChild(argv[1], fs::path{argv[2]});
    try {
        checkOpenRules();
        checkMigrationFromSchema4();
        checkBaseAdoption();
        checkStatesAndRules();
        checkTenThousandNodes();
        checkCrashRecovery();
    } catch (const std::exception& error) {
        std::cerr << "Bookmark overlay store test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Overlay persistence, exact Unicode, state rules, tombstones, migration, 10k nodes and crash "
                 "recovery passed.\n";
    return 0;
}
