#pragma once

// N5.4 safe save, step one: which bookmarks the PDF's outline will hold after
// Save, in which order and nesting (ADR-0009). Pure C++; the qpdf writer turns
// the plan into objects, and the verifier compares what it reads back with it.

#include "core/bookmarks/BookmarkOverlay.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace atlas::bookmarks {

struct SaveOptions final {
    // A PDF bookmark the reader deleted stays in the PDF unless this is set.
    bool removeHidden{false};
};

struct PlannedItem final {
    std::string nodeId;                 // overlay node
    std::optional<std::string> baseKey; // the PDF item it continues, if any
    std::string title;                  // exact UTF-8
    std::string location;               // encoded envelope, or empty (no destination)
    int parent{-1};                     // index into SavePlan::items; -1 = top level
    bool titleChanged{false};           // differs from the base item (or new)
    bool locationChanged{false};
    bool hidden{false};                 // kept in the PDF although hidden in Atlas
};

struct SavePlan final {
    std::vector<PlannedItem> items; // depth first, in reading order
    std::size_t added{};            // new in the PDF
    std::size_t changed{};          // base items with a new title, place or destination
    std::size_t kept{};             // base items written exactly as they were
    std::size_t removed{};          // base items left out (hidden, removeHidden)
    std::string refusal;            // empty when Save may go ahead

    [[nodiscard]] bool hasChanges() const { return added + changed + removed > 0; }
};

// `inOrder`: the overlay's live nodes depth first (BookmarkEditor::inOrder).
// Refusals: "resolveFirst" (a conflict), "unreadableLocation" (an envelope
// Atlas cannot convert), "noBase" (base items without a base snapshot).
[[nodiscard]] SavePlan planOutlineSave(const std::vector<const BookmarkNode*>& inOrder,
                                       const std::optional<BaseSnapshot>& base, const SaveOptions& options = {});

// Page geometry as the PDF defines it (user space, points).
struct PageGeometry final {
    double left{}, bottom{}, right{}, top{}; // crop box (or media box)
    int rotate{};                            // 0, 90, 180 or 270
};

struct PdfPoint final {
    double x{};
    double y{};
};

// Envelope fractions (displayed page after /Rotate and the crop box, origin
// top-left) → PDF user space, and back. Rotation is normalized to 0–270.
[[nodiscard]] PdfPoint pdfPointFromFractions(const PageGeometry& page, double left, double top);
[[nodiscard]] std::pair<double, double> fractionsFromPdfPoint(const PageGeometry& page, PdfPoint point);

} // namespace atlas::bookmarks
