// N5.3 editing engine: every command, the automatic state rules, undo/redo
// (including restoring removed nodes), and search. Each change is written to a
// real profile so the repository's rules check the editor's output.

#include "app/bookmarks/SqliteBookmarkOverlay.h"
#include "app/index/SqliteLibraryIndex.h"
#include "core/bookmarks/BookmarkEditor.h"
#include "core/bookmarks/BookmarkSearch.h"
#include "core/bookmarks/LocationEnvelope.h"

#include <atomic>
#include <chrono>
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

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

class TemporaryDirectory final {
public:
    TemporaryDirectory() {
        static std::atomic<std::uint64_t> sequence{};
        for (int attempt = 0; attempt < 100; ++attempt) {
            const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
            path_ = fs::temp_directory_path() /
                    ("atlas-editor-test-" + std::to_string(nonce) + "-" + std::to_string(sequence++));
            std::error_code error;
            if (fs::create_directory(path_, error)) return;
        }
        throw std::runtime_error("Could not reserve a unique editor-test directory.");
    }
    ~TemporaryDirectory() {
        std::error_code ignored;
        fs::remove_all(path_, ignored);
    }
    [[nodiscard]] const fs::path& path() const { return path_; }

private:
    fs::path path_;
};

std::string at(std::uint32_t page) {
    LocationEnvelope envelope;
    envelope.pdf.pageIndex = page;
    envelope.pdf.kind = DestinationKind::fit;
    return encodeLocation(envelope);
}

BaseSnapshot outline() {
    BaseSnapshot base;
    base.sourceRevision = "size=4096;mtime=1";
    base.nodes = {{"0", std::nullopt, 0, "الباب الأول", at(0)},
                  {"0/0", "0", 0, "الفصل الأول", at(1)},
                  {"0/1", "0", 1, "الفصل الثاني", at(2)},
                  {"1", std::nullopt, 1, "Part Two", at(10)},
                  {"2", std::nullopt, 2, "Index", at(40)}};
    return base;
}

struct Fixture final {
    TemporaryDirectory temporary;
    fs::path database;
    std::unique_ptr<SqliteBookmarkOverlay> store;
    std::unique_ptr<BookmarkEditor> editor;

    Fixture() {
        database = temporary.path() / "profile.sqlite3";
        {
            atlas::index::SqliteLibraryIndex index{database};
            index.registerRoot({"root", temporary.path(), atlas::index::LibraryRootAvailability::available, 0,
                                std::nullopt, 0});
            index.upsertRecord({"doc", temporary.path() / "book.pdf", "Book", "",
                                atlas::document::Availability::available},
                               "root");
        }
        store = std::make_unique<SqliteBookmarkOverlay>(database);
        require(store->adoptBase("doc", outline()) == BaseAdoption::adopted, "Outline adopted.");
        auto snapshot = store->load("doc");
        editor = std::make_unique<BookmarkEditor>("doc", snapshot.nodes, snapshot.base, snapshot.revision);
    }

    // Persist the editor's pending change, as the app does on its worker.
    EditResult persist(EditResult result) {
        require(result.ok, "Command refused: " + result.reason);
        const auto revision = store->apply("doc", editor->revision(),
                                           {result.change.upserts, result.change.removals, result.change.restores});
        editor->confirm(revision);
        return result;
    }

    [[nodiscard]] const BookmarkNode& source(std::string_view key) const {
        for (const auto* node : editor->inOrder()) {
            if (node->baseKey && *node->baseKey == key) return *node;
        }
        throw std::runtime_error("No node for base key " + std::string{key});
    }

    // The store and the editor agree on every node.
    void requireInSync(std::string_view where) const {
        const auto stored = store->load("doc");
        require(stored.nodes.size() == editor->size(), std::string{where} + ": node counts differ.");
        for (const auto& node : stored.nodes) {
            const auto* mine = editor->find(node.id);
            require(mine != nullptr && mine->title == node.title && mine->parentId == node.parentId &&
                        mine->position == node.position && mine->state == node.state &&
                        mine->location == node.location && mine->description == node.description &&
                        mine->tags == node.tags,
                    std::string{where} + ": editor and store differ for a node.");
        }
    }
};

void checkCommandsAndStates() {
    Fixture f;
    const auto chapter = f.source("0").id;
    const auto partTwo = f.source("1").id;
    const auto index = f.source("2").id;

    // A local node before the chapters keeps them Embedded.
    const auto note = f.persist(f.editor->create(std::nullopt, 0, "  ملاحظاتي  ", at(5))).nodeId;
    require(f.editor->find(note)->state == StorageState::localOnly && f.editor->find(note)->position == 0,
            "New bookmarks are local and placed where asked.");
    require(f.source("0").state == StorageState::embedded && f.source("2").position == 3,
            "Source nodes after it shift but stay Embedded.");

    // Rename a source node: LocalOverride; rename it back: Embedded again.
    f.persist(f.editor->rename(partTwo, "Part Two — revised"));
    require(f.source("1").state == StorageState::localOverride, "A renamed PDF bookmark is a LocalOverride.");
    f.persist(f.editor->rename(partTwo, "Part Two"));
    require(f.source("1").state == StorageState::embedded, "Restoring the PDF's title makes it Embedded again.");

    // Destination change and notes.
    f.persist(f.editor->setLocation(index, at(41)));
    require(f.source("2").state == StorageState::localOverride, "A new destination is a LocalOverride.");
    f.persist(f.editor->setLocation(index, at(40)));
    f.persist(f.editor->setNotes(chapter, "ملاحظة على الباب", {"فقه", "مهم"}));
    require(f.source("0").state == StorageState::embedded && f.source("0").tags.size() == 2,
            "Notes and tags are Atlas's own and do not change the state.");

    // Reorder source siblings: the moved one is a LocalOverride; moving it back restores it.
    f.persist(f.editor->move(index, std::nullopt, 1)); // before the chapter
    require(f.source("2").state == StorageState::localOverride && f.source("0").state == StorageState::embedded,
            "Only the moved PDF bookmark becomes a LocalOverride.");
    f.persist(f.editor->move(index, std::nullopt, 3));
    require(f.source("2").state == StorageState::embedded, "Moving it back makes it Embedded again.");

    // Nest a section under Part Two and back.
    const auto section = f.source("0/1").id;
    f.persist(f.editor->move(section, partTwo, 0));
    require(f.source("0/1").state == StorageState::localOverride && f.source("0/1").parentId == partTwo,
            "A PDF bookmark under another parent is a LocalOverride.");
    f.persist(f.editor->move(section, chapter, 1));
    require(f.source("0/1").state == StorageState::embedded, "Back under its PDF parent it is Embedded.");

    // Local folders, nesting, and refusals.
    const auto folder = f.persist(f.editor->create(std::nullopt, 0, "مجلد", {})).nodeId;
    const auto inner = f.persist(f.editor->create(folder, 0, "داخل", at(3))).nodeId;
    require(!f.editor->move(folder, inner, 0).ok && f.editor->move(folder, inner, 0).reason == "cycle",
            "A node cannot move into its own subtree.");
    require(f.editor->create(std::string{"missing"}, 0, "x", {}).reason == "unknownNode", "Parents must exist.");
    require(f.editor->rename(note, "\xC0\xAF").reason == "badTitle", "Invalid UTF-8 is refused.");
    require(f.editor->setLocation(note, "page 3").reason == "malformedLocation", "Garbage destinations are refused.");

    // Pending embed requests on local nodes only.
    f.persist(f.editor->setEmbedRequested(note, true));
    require(f.editor->find(note)->state == StorageState::pendingEmbed, "A local node can wait for the PDF.");
    require(f.editor->setEmbedRequested(chapter, true).reason == "notLocal", "PDF bookmarks are already there.");
    f.persist(f.editor->setEmbedRequested(note, false));

    // Delete: local subtree removed; PDF bookmark hidden; hidden ones are frozen.
    f.persist(f.editor->move(section, folder, 1)); // a PDF bookmark inside a local folder
    require(f.editor->remove(folder).reason == "containsSourceNodes",
            "A local folder holding a PDF bookmark is not deleted.");
    f.persist(f.editor->move(section, chapter, 1));
    f.persist(f.editor->remove(folder));
    require(f.editor->find(folder) == nullptr && f.editor->find(inner) == nullptr, "The local subtree is removed.");
    f.persist(f.editor->remove(partTwo));
    require(f.source("1").state == StorageState::locallyHidden, "Deleting a PDF bookmark hides it.");
    require(f.editor->rename(partTwo, "x").reason == "unhideFirst", "Hidden bookmarks are unhidden before editing.");
    require(f.editor->move(section, partTwo, 0).reason == "unhideFirst", "Nothing moves into a hidden bookmark.");
    f.persist(f.editor->setHidden(partTwo, false));
    require(f.source("1").state == StorageState::embedded, "Unhiding an unchanged PDF bookmark makes it Embedded.");

    // Breadcrumbs follow titles from the top, duplicates allowed.
    const auto crumb = f.editor->breadcrumb(section);
    require(crumb.size() == 2 && crumb[0] == "الباب الأول" && crumb[1] == "الفصل الثاني",
            "Breadcrumbs go from the top level down.");
    f.requireInSync("after commands");
}

void checkUndoRedo() {
    Fixture f;
    const auto initial = f.store->load("doc");
    const auto chapter = f.source("0").id;
    const auto partTwo = f.source("1").id;

    const auto a = f.persist(f.editor->create(chapter, 1, "Local A", at(2))).nodeId;
    const auto b = f.persist(f.editor->create(a, 0, "Local B", {})).nodeId;
    f.persist(f.editor->rename(partTwo, "Renamed"));
    f.persist(f.editor->move(a, std::nullopt, 0));
    f.persist(f.editor->remove(a)); // removes A and B
    f.persist(f.editor->remove(partTwo)); // hides
    f.persist(f.editor->setNotes(chapter, "نص", {"وسم"}));
    require(f.editor->undoKind() == EditKind::setNotes, "Undo names the last command.");

    // Undo everything, writing each step: back to the PDF's outline exactly.
    int steps = 0;
    while (f.editor->canUndo()) {
        f.persist(f.editor->undo());
        ++steps;
    }
    require(steps == 7, "Every command can be undone.");
    f.requireInSync("after undoing all");
    const auto restored = f.store->load("doc");
    require(restored.nodes.size() == initial.nodes.size(), "Undo returns to the original node set.");
    for (const auto& node : initial.nodes) {
        const auto* now = f.editor->find(node.id);
        require(now != nullptr && now->title == node.title && now->state == StorageState::embedded &&
                    now->position == node.position && now->parentId == node.parentId &&
                    now->description.empty() && now->tags.empty(),
                "Each PDF bookmark is back as it was.");
    }

    // Redo everything: the removed nodes come back from their tombstones and go again.
    while (f.editor->canRedo()) f.persist(f.editor->redo());
    require(f.editor->find(a) == nullptr && f.editor->find(b) == nullptr &&
                f.source("1").state == StorageState::locallyHidden && f.source("0").tags.size() == 1,
            "Redo replays every command.");
    f.requireInSync("after redoing all");

    // Undo the removal: A and B are restored with their IDs.
    f.persist(f.editor->undo()); // notes
    f.persist(f.editor->undo()); // hide
    f.persist(f.editor->undo()); // remove A+B
    require(f.editor->find(a) != nullptr && f.editor->find(b) != nullptr &&
                f.editor->find(b)->parentId == a,
            "Undoing a removal restores the subtree with the same IDs.");
    f.requireInSync("after restoring");

    // A new command clears the redo list.
    require(f.editor->canRedo(), "Redo is available after undo.");
    f.persist(f.editor->rename(a, "Local A (2)"));
    require(!f.editor->canRedo(), "A new command clears redo.");

    // A failed write: the change is discarded and the editor reloads.
    auto result = f.editor->rename(a, "Will fail");
    require(result.ok && f.editor->hasPending(), "The command is pending until written.");
    require(f.editor->rename(a, "Second").reason == "busy", "One change at a time.");
    bool conflict = false;
    try {
        (void)f.store->apply("doc", f.editor->revision() - 1, {result.change.upserts, {}, {}});
    } catch (const OverlayRevisionConflict&) {
        conflict = true;
    }
    require(conflict, "A stale write is refused by the store.");
    f.editor->discardPending();
    const auto fresh = f.store->load("doc");
    f.editor->reset(fresh.nodes, fresh.base, fresh.revision);
    require(f.editor->find(a)->title == "Local A (2)" && !f.editor->canUndo(),
            "After a failed write the editor shows what is stored.");

    // History that no longer matches the tree is dropped, not replayed.
    f.persist(f.editor->rename(a, "One"));
    {
        auto node = *f.editor->find(a);
        node.title = "Changed elsewhere";
        const auto revision = f.store->apply("doc", f.editor->revision(), {{node}, {}, {}});
        const auto other = f.store->load("doc");
        f.editor->reset(other.nodes, other.base, revision);
    }
    require(!f.editor->canUndo(), "Reloading clears history.");
}

void checkStoreRestoreRules() {
    Fixture f;
    const auto a = f.persist(f.editor->create(std::nullopt, 0, "A", {})).nodeId;
    f.persist(f.editor->remove(a));
    BookmarkNode back;
    back.id = a;
    back.documentId = "doc";
    back.position = 3; // after the three top-level PDF bookmarks
    back.title = "A";
    bool refused = false;
    try {
        (void)f.store->apply("doc", f.editor->revision(), {{back}, {}, {}});
    } catch (const OverlayRuleViolation& violation) {
        refused = violation.reason() == "idInUse";
    }
    require(refused, "A removed ID returns only as an explicit restore.");
    refused = false;
    try {
        (void)f.store->apply("doc", f.editor->revision(), {{}, {}, {a}});
    } catch (const OverlayRuleViolation& violation) {
        refused = violation.reason() == "restoreWithoutRecord";
    }
    require(refused, "A restore needs the node's record.");
    refused = false;
    try {
        BookmarkNode fresh = back;
        fresh.id = "never-existed";
        (void)f.store->apply("doc", f.editor->revision(), {{fresh}, {}, {fresh.id}});
    } catch (const OverlayRuleViolation& violation) {
        refused = violation.reason() == "restoreWithoutTombstone";
    }
    require(refused, "Only removed nodes can be restored.");
    const auto revision = f.store->apply("doc", f.editor->revision(), {{back}, {}, {a}});
    require(revision == f.editor->revision() + 1 && f.store->load("doc").nodes.size() == 6,
            "An explicit restore brings the node back.");
}

void checkSearch() {
    Fixture f;
    const auto chapter = f.source("0").id;
    f.persist(f.editor->create(chapter, 2, "الْعِلْمُ وَالْعَمَلُ", at(3)));
    f.persist(f.editor->create(std::nullopt, 3, "Reading list", {}));
    f.persist(f.editor->setNotes(f.source("1").id, "Contains the CASE studies", {"أصول"}));
    f.persist(f.editor->remove(f.source("2").id)); // hidden

    const BookmarkSearchIndex index{f.editor->inOrder()};
    const auto arabic = index.find("العلم");
    require(arabic.size() == 1 && f.editor->find(arabic[0].id)->title == "الْعِلْمُ وَالْعَمَلُ",
            "Arabic search ignores harakat.");
    const auto chapters = index.find("الفصل");
    require(chapters.size() == 2 && f.editor->find(chapters[0].id)->baseKey == std::optional<std::string>{"0/0"},
            "Results keep reading order.");
    const auto notes = index.find("case studies");
    require(notes.size() == 1 && notes[0].field == BookmarkField::notes, "Notes are searched, case-insensitively.");
    const auto tag = index.find("اصول");
    require(tag.size() == 1 && tag[0].field == BookmarkField::tag, "Tags are searched with alef variants unified.");
    require(index.find("Index").empty() && index.find("Index", 10, true).size() == 1,
            "Hidden bookmarks are found only when asked.");
    require(index.find("").empty() && index.find("\xFF").empty(), "Empty or invalid queries find nothing.");
    require(index.find("ال", 1).size() == 1, "The result limit is respected.");
}

} // namespace

// Auto Create Bookmarks: a whole tree in one step, three placements, one undo.
void checkBatch() {
    {
        Fixture f;
        const auto topsBefore = f.editor->children(std::nullopt);
        std::vector<NewBookmark> items{{"Chapter 1", at(2), 0}, {"1.1 Methods", at(3), 1}, {"1.1.1 Detail", at(3), 2},
                                       {"Chapter 2", at(5), 0}, {"Too deep jumps clamp", at(6), 3}};
        const auto result = f.persist(f.editor->createBatch(items, BatchPlacement::after));
        f.requireInSync("batch after");
        const auto& tops = f.editor->children(std::nullopt);
        require(tops.size() == topsBefore.size() + 2 && tops[topsBefore.size()] == result.nodeId,
                "New top-level bookmarks go after the existing ones.");
        const auto& chapterOne = tops[topsBefore.size()];
        const auto& kids = f.editor->children(chapterOne);
        require(kids.size() == 1 && f.editor->find(kids[0])->title == "1.1 Methods" &&
                    f.editor->children(kids[0]).size() == 1,
                "Levels become a tree.");
        const auto& chapterTwo = tops[topsBefore.size() + 1];
        require(f.editor->children(chapterTwo).size() == 1, "A level that jumps two deeper is clamped to one deeper.");
        for (const auto* node : f.editor->inOrder()) {
            if (!node->baseKey) require(node->state == StorageState::localOnly, "New bookmarks are Atlas's own.");
            else require(node->state == StorageState::embedded, "The PDF's bookmarks are untouched.");
        }
        require(f.editor->undoKind() == EditKind::autoCreate, "One undo step.");
        f.persist(f.editor->undo());
        f.requireInSync("batch undone");
        require(f.editor->children(std::nullopt) == topsBefore && f.editor->size() == 5, "Undo removes the whole batch.");
        f.persist(f.editor->redo());
        require(f.editor->size() == 10, "Redo brings it back.");
    }
    {
        Fixture f;
        const auto topsBefore = f.editor->children(std::nullopt);
        f.persist(f.editor->createBatch({{"Preface", at(0), 0}}, BatchPlacement::before));
        const auto& tops = f.editor->children(std::nullopt);
        require(f.editor->find(tops.front())->title == "Preface" && tops.size() == topsBefore.size() + 1,
                "\"Before\" puts the batch first.");
        require(f.source("0").state == StorageState::embedded,
                "Atlas's own bookmarks may sit before the PDF's: they stay Embedded.");
        f.requireInSync("batch before");
    }
    {
        Fixture f;
        const auto mine = f.persist(f.editor->create(std::nullopt, 9, "my note", at(7))).nodeId;
        f.persist(f.editor->createBatch({{"الفصل الأول", at(1), 0}, {"مبحث", at(1), 1}}, BatchPlacement::replace));
        f.requireInSync("batch replace");
        require(f.editor->find(mine) == nullptr, "Replace removes Atlas's own bookmarks.");
        for (const auto* node : f.editor->inOrder()) {
            if (node->baseKey) require(node->state == StorageState::locallyHidden, "Replace hides the PDF's own.");
        }
        f.persist(f.editor->undo());
        f.requireInSync("replace undone");
        require(f.editor->find(mine) != nullptr && f.source("0").state == StorageState::embedded &&
                    f.source("0/1").state == StorageState::embedded,
                "Undo restores everything replace touched.");
    }
    {
        // Headings of one level stay siblings even when a level is skipped.
        Fixture f;
        f.persist(f.editor->createBatch({{"Part", at(7), 0}, {"7.1", at(7), 2}, {"7.2", at(8), 2}, {"8", at(9), 1}},
                                        BatchPlacement::after));
        const auto part = f.editor->children(std::nullopt).back();
        require(f.editor->children(part).size() == 3, "Skipped levels close up: 7.1, 7.2 and 8 all sit under Part.");
        f.requireInSync("skipped levels");
    }
    {
        Fixture f;
        require(f.editor->createBatch({}, BatchPlacement::after).reason == "nothingToAdd", "An empty batch is refused.");
        const auto before = f.editor->size();
        const auto refused = f.editor->createBatch({{"ok", at(1), 0}, {"\xC0\xAF", at(1), 0}}, BatchPlacement::replace);
        require(!refused.ok && refused.reason == "badTitle" && f.editor->size() == before && !f.editor->hasPending() &&
                    f.source("0").state == StorageState::embedded,
                "A refused batch leaves nothing behind.");
        f.requireInSync("refused batch");
    }
}

int main() {
    try {
        checkCommandsAndStates();
        checkBatch();
        checkUndoRedo();
        checkStoreRestoreRules();
        checkSearch();
    } catch (const std::exception& error) {
        std::cerr << "Bookmark editor test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Bookmark editor commands, state rules, undo/redo with restores, and search passed.\n";
    return 0;
}
