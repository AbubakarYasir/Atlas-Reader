#pragma once

// N5.4 reconcile: the PDF's outline changed outside Atlas (another program,
// another copy). Atlas follows the PDF where the reader did nothing, keeps the
// reader's own work, and marks a conflict where both changed the same bookmark
// (docs/DATA_AND_SAFETY.md, ADR-0005). Pure C++.
//
// Matching uses unique title/destination, title/page, title, then matched-parent/page
// candidates. Indistinguishable candidates are refused; ordering is not identity.

#include "core/bookmarks/BookmarkOverlay.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace atlas::bookmarks {

struct ReconcileResult final {
    std::vector<BookmarkNode> nodes;  // every live node afterwards (full records)
    std::vector<std::string> removed; // nodes that followed the PDF out (tombstoned)
    std::size_t added{};              // new in the PDF, now shown
    std::size_t updated{};            // changed in the PDF, followed
    std::size_t removedFromPdf{};     // gone from the PDF, gone here (nothing local lost)
    std::size_t conflicts{};          // both changed: the reader decides
    std::string refusal;              // empty when the result is consistent
};

[[nodiscard]] ReconcileResult reconcile(std::string_view documentId, const std::vector<BookmarkNode>& overlay,
                                        const BaseSnapshot& oldBase, const BaseSnapshot& newBase);

} // namespace atlas::bookmarks
