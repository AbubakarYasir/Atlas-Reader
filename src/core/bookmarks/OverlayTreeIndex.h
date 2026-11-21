#pragma once

// N5.3: an indexed overlay tree that checks a change by looking only at what
// the change touches (its sibling lists, ancestors, subtree and base nodes)
// instead of the whole document. The verdict is the same as running
// validateStructure + validateAgainstBase on the result; the repository keeps
// one per document between writes, and Debug builds cross-check every verdict
// against the full validation. Pure C++.

#include "core/bookmarks/BookmarkOverlay.h"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace atlas::bookmarks {

class OverlayTreeIndex final {
public:
    OverlayTreeIndex(std::string documentId, std::vector<BookmarkNode> nodes, std::optional<BaseSnapshot> base);

    [[nodiscard]] const BookmarkNode* find(std::string_view id) const;
    [[nodiscard]] std::size_t size() const noexcept { return nodes_.size(); }
    [[nodiscard]] const std::optional<BaseSnapshot>& base() const noexcept { return base_; }
    [[nodiscard]] std::vector<BookmarkNode> nodes() const;

    // Applies upserts (complete, individually valid records of this document)
    // and removals. Returns an empty string and keeps the result when the tree
    // stays valid; otherwise returns the reason and leaves the index unchanged.
    [[nodiscard]] std::string apply(const std::vector<BookmarkNode>& upserts, const std::vector<std::string>& removals);

private:
    void put(const std::string& id, const std::optional<BookmarkNode>& node);
    [[nodiscard]] std::string check(const std::vector<BookmarkNode>& upserts, const std::vector<std::string>& removals,
                                    const std::unordered_map<std::string, std::optional<BookmarkNode>>& before) const;
    [[nodiscard]] static std::string groupKey(const std::optional<std::string>& parent);

    std::string documentId_;
    std::optional<BaseSnapshot> base_;
    std::unordered_map<std::string, const BaseNode*> baseByKey_;
    std::unordered_map<std::string, BookmarkNode> nodes_;
    std::unordered_map<std::string, std::vector<std::string>> children_; // "" = top level, unordered
    std::unordered_map<std::string, std::size_t> claims_;                // base key → live claimants
};

} // namespace atlas::bookmarks
