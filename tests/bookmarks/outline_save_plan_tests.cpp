// N5.4 safe save: what the PDF's outline will hold after Save (ADR-0009), and
// destinations converted between Atlas fractions and PDF user space.

#include "core/bookmarks/BookmarkEditor.h"
#include "core/bookmarks/LocationEnvelope.h"
#include "core/bookmarks/OutlineSavePlan.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

using namespace atlas::bookmarks;

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

std::string at(std::uint32_t page, double top = 0.25) {
    LocationEnvelope envelope;
    envelope.pdf.pageIndex = page;
    envelope.pdf.kind = DestinationKind::xyz;
    envelope.pdf.left = 0.1;
    envelope.pdf.top = top;
    return encodeLocation(envelope);
}

BaseSnapshot outline() {
    BaseSnapshot base;
    base.sourceRevision = "size=1;mtime=1";
    base.nodes = {{"0", std::nullopt, 0, "الباب الأول", at(0)},
                  {"0/0", "0", 0, "الفصل الأول", at(1)},
                  {"1", std::nullopt, 1, "Part Two", at(10)},
                  {"2", std::nullopt, 2, "Index", at(40)}};
    return base;
}

struct Book final {
    BookmarkEditor editor;
    Book() : editor{"doc", embeddedNodesFromBase("doc", outline()), outline(), 1} {}
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
    SavePlan plan(SaveOptions options = {}) { return planOutlineSave(editor.inOrder(), outline(), options); }
};

void checkPlans() {
    {
        Book book;
        const auto plan = book.plan();
        require(plan.refusal.empty() && !plan.hasChanges() && plan.kept == 4 && plan.items.size() == 4,
                "An untouched outline has nothing to save.");
    }
    {
        Book book;
        book.run(book.editor.rename(book.id("1"), "الجزء الثاني"));
        book.run(book.editor.create(std::nullopt, 1, "My chapter", at(5)));
        book.run(book.editor.create(std::nullopt, 9, "Folder only", ""));
        const auto plan = book.plan();
        require(plan.refusal.empty() && plan.added == 2 && plan.changed == 1 && plan.kept == 3,
                "Counts: two added, one renamed, three kept.");
        require(plan.items[2].title == "My chapter" && plan.items[2].parent == -1 && !plan.items[2].baseKey,
                "New bookmarks keep their place.");
        require(plan.items.back().location.empty(), "A grouping bookmark is written without a destination.");
        require(plan.items[1].parent == 0 && plan.items[1].baseKey == "0/0", "Nesting is kept.");
    }
    {
        Book book;
        book.run(book.editor.move(book.id("2"), std::nullopt, 0)); // Index first
        const auto plan = book.plan();
        require(plan.changed == 1 && plan.kept == 3 && plan.items.front().title == "Index",
                "A reordered PDF bookmark counts as changed.");
    }
    {
        Book book;
        const auto folder = book.editor.create(std::nullopt, 0, "Folder", "");
        book.run(folder);
        book.run(book.editor.move(book.id("1"), folder.nodeId, 0));
        const auto plan = book.plan();
        require(plan.changed == 1 && plan.added == 1, "A PDF bookmark moved into a new folder counts as changed.");
    }
    {
        Book book;
        book.run(book.editor.rename(book.id("0"), "changed"));
        book.run(book.editor.setHidden(book.id("0"), true));
        const auto keep = book.plan();
        require(keep.removed == 0 && keep.items.size() == 4 && keep.items[0].hidden &&
                    keep.items[0].title == "الباب الأول",
                "A hidden PDF bookmark stays in the PDF exactly as the PDF has it.");
        const auto remove = book.plan({true});
        require(remove.removed == 1 && remove.items.size() == 3 && remove.items[0].baseKey == "0/0" &&
                    remove.items[0].parent == -1,
                "Removing a hidden bookmark moves its children up.");
    }
    {
        Book book;
        auto nodes = book.editor.inOrder();
        BookmarkNode conflicted = *nodes[0];
        conflicted.state = StorageState::conflict;
        nodes[0] = &conflicted;
        require(planOutlineSave(nodes, outline()).refusal == "resolveFirst", "A conflict blocks Save.");
        nodes = book.editor.inOrder();
        nodes.pop_back(); // "Index" missing from the overlay
        require(planOutlineSave(nodes, outline()).refusal == "missingBaseItems",
                "A PDF bookmark missing from Atlas is never deleted by accident.");
        BookmarkNode future = *book.editor.inOrder()[3];
        future.location = "atlas-location/2;scheme=pdf;page=1";
        future.state = StorageState::localOverride;
        nodes = book.editor.inOrder();
        nodes[3] = &future;
        require(planOutlineSave(nodes, outline()).refusal == "unreadableLocation",
                "A destination Atlas cannot read is not written.");
    }
}

void checkCoordinates() {
    const PageGeometry plain{0, 0, 612, 792, 0};
    const auto p = pdfPointFromFractions(plain, 0.25, 0.1);
    require(std::abs(p.x - 153) < 1e-9 && std::abs(p.y - 712.8) < 1e-9, "Rotation 0: top-left origin.");
    const PageGeometry cropped{36, 18, 576, 774, 0};
    const auto c = pdfPointFromFractions(cropped, 0, 0);
    require(c.x == 36 && c.y == 774, "The crop box's top-left is (0, 0).");
    for (const int rotate : {0, 90, 180, 270, -90, 450}) {
        const PageGeometry page{36, 18, 576, 774, rotate};
        for (const auto& [u, v] : {std::pair{0.0, 0.0}, {0.25, 0.75}, {1.0, 1.0}, {0.6, 0.1}}) {
            const auto point = pdfPointFromFractions(page, u, v);
            const auto [u2, v2] = fractionsFromPdfPoint(page, point);
            require(std::abs(u - u2) < 1e-9 && std::abs(v - v2) < 1e-9,
                    "Round trip at rotation " + std::to_string(rotate));
        }
    }
    // Rotation 90: the displayed top-left is the user-space bottom-left.
    const auto r90 = pdfPointFromFractions({0, 0, 600, 800, 90}, 0, 0);
    require(r90.x == 0 && r90.y == 0, "Rotation 90 corner.");
    const auto r90b = pdfPointFromFractions({0, 0, 600, 800, 90}, 1, 0); // displayed top-right
    require(r90b.x == 0 && r90b.y == 800, "Rotation 90: across the display is up the page.");
    const auto r270 = pdfPointFromFractions({0, 0, 600, 800, 270}, 0, 0);
    require(r270.x == 600 && r270.y == 800, "Rotation 270 corner.");
}

} // namespace

int main() {
    try {
        checkPlans();
        checkCoordinates();
    } catch (const std::exception& error) {
        std::cerr << "Outline save plan test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Outline save plan and destination conversion passed.\n";
    return 0;
}
