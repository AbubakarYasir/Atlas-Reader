#pragma once

// N5.3 bookmark search: titles, notes and tags of one document's overlay, with
// the same matching as in-document search (N4.5): Latin case folded; Arabic and
// Urdu harakat, tatweel and joiners ignored; letter variants and presentation
// forms unified. The index is built once (off the UI thread) and queried as
// the user types. Pure C++.

#include "core/bookmarks/BookmarkOverlay.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace atlas::bookmarks {

enum class BookmarkField : std::uint8_t {
    title,
    notes,
    tag,
};

struct BookmarkHit final {
    std::string id;
    BookmarkField field{BookmarkField::title}; // where the first match is
};

class BookmarkSearchIndex final {
public:
    // `ordered` is the reading order (BookmarkEditor::inOrder()); results keep it.
    explicit BookmarkSearchIndex(const std::vector<const BookmarkNode*>& ordered);

    [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }
    // Hidden nodes are skipped unless asked for. At most `limit` hits.
    [[nodiscard]] std::vector<BookmarkHit> find(std::string_view query, std::size_t limit = 500,
                                                bool includeHidden = false) const;

private:
    struct Entry final {
        std::string id;
        bool hidden{false};
        std::u16string title;
        std::u16string notes;
        std::u16string tags; // joined with U+000A
    };
    std::vector<Entry> entries_;
};

} // namespace atlas::bookmarks
