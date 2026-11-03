#include "core/bookmarks/BookmarkSearch.h"

#include "core/bookmarks/Utf8.h"
#include "core/reader/TextSearch.h"

namespace atlas::bookmarks {
namespace {

[[nodiscard]] std::u16string normalized(std::string_view utf8) {
    return reader::normalizeForSearch(toUtf16(utf8), reader::SearchOptions{}).text;
}

} // namespace

BookmarkSearchIndex::BookmarkSearchIndex(const std::vector<const BookmarkNode*>& ordered) {
    entries_.reserve(ordered.size());
    for (const auto* node : ordered) {
        Entry entry;
        entry.id = node->id;
        entry.hidden = node->state == StorageState::locallyHidden;
        entry.title = normalized(node->title);
        entry.notes = normalized(node->description);
        std::string tags;
        for (const auto& tag : node->tags) {
            if (!tags.empty()) tags.push_back('\n');
            tags += tag;
        }
        entry.tags = normalized(tags);
        entries_.push_back(std::move(entry));
    }
}

std::vector<BookmarkHit> BookmarkSearchIndex::find(std::string_view query, std::size_t limit,
                                                   bool includeHidden) const {
    std::vector<BookmarkHit> hits;
    if (!isValidUtf8(query)) return hits;
    const auto needle = normalized(query);
    if (needle.empty()) return hits;
    for (const auto& entry : entries_) {
        if (hits.size() >= limit) break;
        if (entry.hidden && !includeHidden) continue;
        if (entry.title.find(needle) != std::u16string::npos) hits.push_back({entry.id, BookmarkField::title});
        else if (entry.notes.find(needle) != std::u16string::npos) hits.push_back({entry.id, BookmarkField::notes});
        else if (entry.tags.find(needle) != std::u16string::npos) hits.push_back({entry.id, BookmarkField::tag});
    }
    return hits;
}

} // namespace atlas::bookmarks
