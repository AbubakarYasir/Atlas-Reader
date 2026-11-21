#pragma once

// N5.5 portability (docs/DATA_AND_SAFETY.md "Import and export",
// docs/formats/atlas-bookmarks.md): bookmarks to and from files.
//
// - `.atlas-bookmarks.json` is lossless and versioned (format
//   "atlas-bookmarks", schemaVersion "1.x.y"): hierarchy, order, exact
//   titles, notes, tags, Atlas location envelopes, storage state as
//   provenance, and document hints that never bind a file by themselves.
//   A newer major version is refused; unknown fields of a newer minor are
//   ignored. Never contains passwords.
// - Markdown and CSV are readable conveniences, never called lossless.
//
// Pure functions over overlay nodes; files are written and read by the caller.

#include "core/bookmarks/BookmarkEditor.h"

#include <QByteArray>
#include <QString>
#include <QStringList>

#include <optional>
#include <string>
#include <vector>

namespace atlas::app {

enum class ExportScope : std::uint8_t {
    all,      // every visible bookmark (hidden PDF bookmarks are left out)
    subtree,  // one bookmark and everything under it
    localOnly // Atlas's own work: bookmarks not (yet) in the PDF, and changed ones
};

struct DocumentHints final {
    QString title;
    QString fileName;
    int pageCount{};
};

// A bookmark as imported: a tree in reading order (`level` 0 = top).
struct ImportedBookmark final {
    std::string title;
    std::string location;    // a location envelope Atlas can read, or empty
    std::string description;
    std::vector<std::string> tags;
    std::uint32_t level{};
};

struct ImportResult final {
    bool ok{false};
    // notJson, wrongFormat, newerVersion, badVersion, tooLarge, tooMany,
    // tooDeep, badItem: the file is refused whole.
    QString error;
    QString schemaVersion;
    DocumentHints hints;
    QString exportedUtc;
    std::vector<ImportedBookmark> items;
};

inline constexpr const char* kExchangeFormat = "atlas-bookmarks";
inline constexpr const char* kExchangeSchemaVersion = "1.0.0";
inline constexpr qsizetype kMaxImportBytes = 64LL * 1024 * 1024;

// `inOrder`: the overlay depth first (BookmarkEditor::inOrder); `subtreeRoot`
// for ExportScope::subtree.
[[nodiscard]] std::vector<const bookmarks::BookmarkNode*>
exportSelection(const std::vector<const bookmarks::BookmarkNode*>& inOrder, ExportScope scope,
                const std::string& subtreeRoot = {});

[[nodiscard]] QByteArray exportJson(const std::vector<const bookmarks::BookmarkNode*>& selection,
                                    const DocumentHints& hints, ExportScope scope, const QString& exporterVersion,
                                    const QString& exportedUtc);
[[nodiscard]] QByteArray exportMarkdown(const std::vector<const bookmarks::BookmarkNode*>& selection,
                                        const DocumentHints& hints, const QStringList& pageLabels);
[[nodiscard]] QByteArray exportCsv(const std::vector<const bookmarks::BookmarkNode*>& selection,
                                   const QStringList& pageLabels);

[[nodiscard]] ImportResult importJson(const QByteArray& bytes);

} // namespace atlas::app
