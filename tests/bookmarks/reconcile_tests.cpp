// N5.4 reconcile: the PDF's outline changed outside Atlas. Atlas follows the
// PDF where the reader did nothing, keeps the reader's work, and marks a
// conflict where both changed the same bookmark.

#include "core/bookmarks/BookmarkEditor.h"
#include "core/bookmarks/LocationEnvelope.h"
#include "core/bookmarks/Reconcile.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

using namespace atlas::bookmarks;

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

std::string at(std::uint32_t page) {
    LocationEnvelope envelope;
    envelope.pdf.pageIndex = page;
    envelope.pdf.kind = DestinationKind::fit;
    return encodeLocation(envelope);
}

BaseSnapshot before() {
    BaseSnapshot base;
    base.sourceRevision = "r1";
    base.nodes = {{"0", std::nullopt, 0, "الباب الأول", at(0)},
                  {"0/0", "0", 0, "الفصل الأول", at(1)},
                  {"0/1", "0", 1, "الفصل الثاني", at(2)},
                  {"1", std::nullopt, 1, "Part Two", at(10)},
                  {"2", std::nullopt, 2, "Index", at(40)}};
    return base;
}

const BookmarkNode* byTitle(const ReconcileResult& r, std::string_view title) {
    for (const auto& node : r.nodes) {
        if (node.title == title) return &node;
    }
    return nullptr;
}

struct Overlay final {
    BookmarkEditor editor{"doc", embeddedNodesFromBase("doc", before()), before(), 1};
    std::string id(std::string_view key) {
        for (const auto* node : editor.inOrder()) {
            if (node->baseKey && *node->baseKey == key) return node->id;
        }
        throw std::runtime_error("no node");
    }
    void run(EditResult result) {
        require(result.ok, "command refused: " + result.reason);
        editor.confirm(editor.revision() + 1);
    }
    std::vector<BookmarkNode> nodes() const {
        std::vector<BookmarkNode> out;
        for (const auto* node : editor.inOrder()) out.push_back(*node);
        return out;
    }
};

void checkRevisionOnly() {
    Overlay overlay;
    BaseSnapshot same = before();
    same.sourceRevision = "r2";
    const auto r = reconcile("doc", overlay.nodes(), before(), same);
    require(r.refusal.empty() && r.added == 0 && r.updated == 0 && r.removedFromPdf == 0 && r.conflicts == 0 &&
                r.removed.empty() && r.nodes.size() == 5,
            "The same outline in a new revision changes nothing.");
}

void checkMerge() {
    Overlay overlay;
    const auto note = overlay.editor.create(overlay.id("0"), 9, "my note", at(1));
    overlay.run(note);
    overlay.run(overlay.editor.rename(overlay.id("1"), "Part Two (mine)"));        // the PDF keeps "Part Two"
    overlay.run(overlay.editor.rename(overlay.id("0/0"), "الفصل الأول — ملاحظتي")); // the PDF renames it too
    overlay.run(overlay.editor.setHidden(overlay.id("2"), true));                  // the PDF removes Index

    BaseSnapshot changed;
    changed.sourceRevision = "r2";
    changed.nodes = {{"0", std::nullopt, 0, "Preface", at(0)},
                     {"1", std::nullopt, 1, "الباب الأول", at(0)},
                     {"1/0", "1", 0, "الفصل الأول (منقح)", at(1)},
                     {"1/1", "1", 1, "الفصل الثاني", at(3)},
                     {"2", std::nullopt, 2, "Part Two", at(10)}};
    const auto r = reconcile("doc", overlay.nodes(), before(), changed);
    require(r.refusal.empty(), "Consistent result: " + r.refusal);
    require(r.added == 1 && byTitle(r, "Preface") && byTitle(r, "Preface")->state == StorageState::embedded,
            "A new PDF bookmark appears as Embedded.");
    require(r.updated == 1 && byTitle(r, "الفصل الثاني")->location == at(3), "A moved destination is followed.");
    const auto* mine = byTitle(r, "Part Two (mine)");
    require(mine && mine->state == StorageState::localOverride && mine->baseKey == "2",
            "The reader's rename of an unchanged PDF bookmark is kept.");
    const auto* both = byTitle(r, "الفصل الأول — ملاحظتي");
    require(r.conflicts == 1 && both && both->state == StorageState::conflict &&
                both->conflict->reason == ConflictReason::sourceChanged,
            "Changed on both sides: a conflict, with the reader's version kept.");
    require(r.removed.size() == 1 && r.removedFromPdf == 0, "A hidden bookmark the PDF dropped is gone.");
    const auto* kept = byTitle(r, "my note");
    require(kept && kept->parentId == byTitle(r, "الباب الأول")->id, "The reader's own bookmark stays where it was.");
    const auto* preface = byTitle(r, "Preface");
    require(!preface->parentId && preface->position == 0, "Placed where the PDF puts it.");

    // Settling the conflict: either side.
    for (const bool keepMine : {true, false}) {
        BookmarkEditor editor{"doc", r.nodes, changed, 9};
        const std::string id = both->id;
        const auto result = editor.resolveConflict(id, keepMine);
        require(result.ok, "resolve: " + result.reason);
        editor.confirm(10);
        const auto* node = editor.find(id);
        if (keepMine) {
            require(node->state == StorageState::localOverride && node->title == "الفصل الأول — ملاحظتي" && !node->conflict,
                    "Keep mine: the reader's version stays, over the PDF's bookmark.");
        } else {
            require(node->state == StorageState::embedded && node->title == "الفصل الأول (منقح)",
                    "Take the PDF's: its title, destination and place; Embedded again.");
        }
        std::vector<BookmarkNode> after;
        for (const auto* n : editor.inOrder()) after.push_back(*n);
        require(validateAgainstBase(after, changed).empty(), "No false Embedded after resolving.");
    }
}

void checkRemovedWithLocalWork() {
    Overlay overlay;
    overlay.run(overlay.editor.create(overlay.id("1"), 0, "child note", at(11)));
    overlay.run(overlay.editor.rename(overlay.id("0/1"), "my title"));
    BaseSnapshot changed;
    changed.sourceRevision = "r2";
    changed.nodes = {{"0", std::nullopt, 0, "الباب الأول", at(0)}, {"0/0", "0", 0, "الفصل الأول", at(1)},
                     {"1", std::nullopt, 1, "Index", at(40)}};
    const auto r = reconcile("doc", overlay.nodes(), before(), changed);
    require(r.refusal.empty(), "Consistent: " + r.refusal);
    const auto* renamed = byTitle(r, "my title");
    require(renamed && renamed->state == StorageState::conflict && !renamed->baseKey &&
                renamed->conflict->reason == ConflictReason::sourceRemoved,
            "Local work on a bookmark the PDF dropped is kept as a conflict.");
    require(r.removedFromPdf == 1 && !byTitle(r, "Part Two"), "An untouched bookmark the PDF dropped goes.");
    const auto* child = byTitle(r, "child note");
    require(child && !child->parentId, "Its local child moves up instead of being lost.");
}

} // namespace

int main() {
    try {
        checkRevisionOnly();
        checkMerge();
        checkRemovedWithLocalWork();
    } catch (const std::exception& error) {
        std::cerr << "Reconcile test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Reconcile passed.\n";
    return 0;
}
