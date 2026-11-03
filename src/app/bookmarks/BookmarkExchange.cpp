#include "app/bookmarks/BookmarkExchange.h"

#include "core/bookmarks/LocationEnvelope.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace atlas::app {
namespace {

using bookmarks::BookmarkNode;
using bookmarks::StorageState;

QString qs(const std::string& text) { return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size())); }
std::string utf8(const QString& text) {
    const QByteArray bytes = text.toUtf8();
    return {bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

// Levels of the selected nodes: how many selected ancestors each has.
std::vector<int> levelsOf(const std::vector<const BookmarkNode*>& selection) {
    std::unordered_map<std::string, int> levelById;
    std::vector<int> levels;
    levels.reserve(selection.size());
    for (const auto* node : selection) {
        // One deeper than its parent when the parent is exported too; else
        // a top-level item of the export.
        int level = 0;
        if (node->parentId) {
            if (const auto found = levelById.find(*node->parentId); found != levelById.end()) level = found->second + 1;
        }
        levelById.emplace(node->id, level);
        levels.push_back(level);
    }
    return levels;
}

int pageOf(const std::string& location) {
    if (location.empty()) return -1;
    const auto decoded = bookmarks::decodeLocation(location);
    return decoded.status == bookmarks::LocationDecodeStatus::ok ? static_cast<int>(decoded.envelope->pdf.pageIndex) : -1;
}

QString pageText(int page, const QStringList& labels) {
    if (page < 0) return {};
    return page < labels.size() && !labels.at(page).isEmpty() ? labels.at(page) : QString::number(page + 1);
}

} // namespace

std::vector<const BookmarkNode*> exportSelection(const std::vector<const BookmarkNode*>& inOrder, ExportScope scope,
                                                 const std::string& subtreeRoot) {
    std::vector<const BookmarkNode*> out;
    std::unordered_set<std::string> inSubtree;
    for (const auto* node : inOrder) {
        if (scope == ExportScope::subtree) {
            const bool member = node->id == subtreeRoot || (node->parentId && inSubtree.contains(*node->parentId));
            if (!member) continue;
            inSubtree.insert(node->id);
        }
        if (node->state == StorageState::locallyHidden) continue;
        if (scope == ExportScope::localOnly && node->state == StorageState::embedded) continue;
        out.push_back(node);
    }
    return out;
}

QByteArray exportJson(const std::vector<const BookmarkNode*>& selection, const DocumentHints& hints, ExportScope scope,
                      const QString& exporterVersion, const QString& exportedUtc) {
    const auto levels = levelsOf(selection);
    // Build the nested arrays from the depth-first list.
    std::vector<QJsonArray> stack(1);
    std::vector<QJsonObject> open;
    const auto closeTo = [&](std::size_t depth) {
        while (open.size() > depth) {
            QJsonObject item = open.back();
            open.pop_back();
            item.insert(QStringLiteral("children"), stack.back());
            stack.pop_back();
            stack.back().append(item);
        }
    };
    for (std::size_t i = 0; i < selection.size(); ++i) {
        const auto* node = selection[i];
        closeTo(static_cast<std::size_t>(levels[i]));
        QJsonObject item;
        item.insert(QStringLiteral("id"), qs(node->id));
        item.insert(QStringLiteral("title"), qs(node->title));
        if (!node->location.empty()) item.insert(QStringLiteral("location"), qs(node->location));
        if (!node->description.empty()) item.insert(QStringLiteral("notes"), qs(node->description));
        if (!node->tags.empty()) {
            QJsonArray tags;
            for (const auto& tag : node->tags) tags.append(qs(tag));
            item.insert(QStringLiteral("tags"), tags);
        }
        item.insert(QStringLiteral("state"), qs(std::string{bookmarks::stateName(node->state)}));
        open.push_back(item);
        stack.emplace_back();
    }
    closeTo(0);
    QJsonObject document{{QStringLiteral("title"), hints.title},
                         {QStringLiteral("fileName"), hints.fileName},
                         {QStringLiteral("pageCount"), hints.pageCount}};
    QJsonObject root{
        {QStringLiteral("format"), QString::fromLatin1(kExchangeFormat)},
        {QStringLiteral("schemaVersion"), QString::fromLatin1(kExchangeSchemaVersion)},
        {QStringLiteral("exporter"), QJsonObject{{QStringLiteral("name"), QStringLiteral("Atlas Reader")},
                                                 {QStringLiteral("version"), exporterVersion},
                                                 {QStringLiteral("exportedUtc"), exportedUtc}}},
        {QStringLiteral("document"), document},
        {QStringLiteral("scope"), scope == ExportScope::all       ? QStringLiteral("all")
                                  : scope == ExportScope::subtree ? QStringLiteral("subtree")
                                                                  : QStringLiteral("localOnly")},
        {QStringLiteral("bookmarks"), stack.front()},
    };
    return QJsonDocument{root}.toJson(QJsonDocument::Indented);
}

QByteArray exportMarkdown(const std::vector<const BookmarkNode*>& selection, const DocumentHints& hints,
                          const QStringList& pageLabels) {
    // Markdown punctuation in user text is escaped so titles stay text.
    static const QRegularExpression special{QStringLiteral("([\\\\`*_\\[\\]<>#|])")};
    const auto escape = [](QString text) { return text.replace(special, QStringLiteral("\\\\1")); };
    QString out = QStringLiteral("# ") + escape(hints.title.isEmpty() ? hints.fileName : hints.title) + u'\n';
    out += u'\n';
    const auto levels = levelsOf(selection);
    for (std::size_t i = 0; i < selection.size(); ++i) {
        const auto* node = selection[i];
        const QString indent(levels[i] * 2, u' ');
        QString line = indent + QStringLiteral("- ") + escape(qs(node->title).simplified());
        const QString page = pageText(pageOf(node->location), pageLabels);
        if (!page.isEmpty()) line += QStringLiteral(" — p. ") + page;
        out += line + u'\n';
        if (!node->description.empty()) {
            for (const auto& noteLine : qs(node->description).split(u'\n'))
                out += indent + QStringLiteral("  > ") + escape(noteLine) + u'\n';
        }
        if (!node->tags.empty()) {
            QStringList tags;
            for (const auto& tag : node->tags) tags << QStringLiteral("`") + qs(tag).replace(u'`', u'\'') + u'`';
            out += indent + QStringLiteral("  ") + tags.join(u' ') + u'\n';
        }
    }
    return out.toUtf8();
}

QByteArray exportCsv(const std::vector<const BookmarkNode*>& selection, const QStringList& pageLabels) {
    const auto cell = [](QString text) {
        // A cell that a spreadsheet would run as a formula is kept as text.
        if (!text.isEmpty() && QStringLiteral("=+-@\t\r").contains(text.front())) text.prepend(u'\'');
        return u'"' + text.replace(u'"', QStringLiteral("\"\"")) + u'"';
    };
    QString out = QStringLiteral("level,path,title,page,state,notes,tags\r\n");
    const auto levels = levelsOf(selection);
    std::vector<QString> path; // titles of the open ancestors
    for (std::size_t i = 0; i < selection.size(); ++i) {
        const auto* node = selection[i];
        path.resize(static_cast<std::size_t>(levels[i]));
        QStringList crumbs;
        for (const auto& title : path) crumbs << title;
        const QString title = qs(node->title);
        QStringList tags;
        for (const auto& tag : node->tags) tags << qs(tag);
        out += QString::number(levels[i]) + u',' + cell(crumbs.join(QStringLiteral(" › "))) + u',' + cell(title) + u',' +
               cell(pageText(pageOf(node->location), pageLabels)) + u',' +
               cell(QString::fromLatin1(bookmarks::stateName(node->state).data(),
                                        static_cast<qsizetype>(bookmarks::stateName(node->state).size()))) +
               u',' + cell(qs(node->description)) + u',' + cell(tags.join(u';')) + QStringLiteral("\r\n");
        path.push_back(title);
    }
    return QByteArray{"\xEF\xBB\xBF"} + out.toUtf8(); // BOM: spreadsheets then read UTF-8
}

ImportResult importJson(const QByteArray& bytes) {
    ImportResult result;
    const auto fail = [&result](const char* error) {
        result.ok = false;
        result.error = QString::fromLatin1(error);
        result.items.clear();
        return result;
    };
    if (bytes.size() > kMaxImportBytes) return fail("tooLarge");
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) return fail("notJson");
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("format")).toString() != QString::fromLatin1(kExchangeFormat)) return fail("wrongFormat");
    result.schemaVersion = root.value(QStringLiteral("schemaVersion")).toString();
    static const QRegularExpression version{QStringLiteral("^(\\d+)\\.(\\d+)\\.(\\d+)$")};
    const auto match = version.match(result.schemaVersion);
    if (!match.hasMatch()) return fail("badVersion");
    const int major = match.captured(1).toInt();
    if (major > 1) return fail("newerVersion");
    if (major < 1) return fail("badVersion");
    const QJsonObject hints = root.value(QStringLiteral("document")).toObject();
    result.hints = {hints.value(QStringLiteral("title")).toString(), hints.value(QStringLiteral("fileName")).toString(),
                    hints.value(QStringLiteral("pageCount")).toInt()};
    result.exportedUtc = root.value(QStringLiteral("exporter")).toObject().value(QStringLiteral("exportedUtc")).toString();
    if (!root.value(QStringLiteral("bookmarks")).isArray()) return fail("wrongFormat");

    bool bad = false;
    const char* why = "badItem";
    std::function<void(const QJsonArray&, std::uint32_t)> walk = [&](const QJsonArray& list, std::uint32_t level) {
        if (list.isEmpty()) return;
        if (level >= bookmarks::OverlayLimits::maxDepth) {
            bad = true;
            why = "tooDeep";
            return;
        }
        for (const auto& value : list) {
            if (bad) return;
            if (result.items.size() >= bookmarks::OverlayLimits::maxNodesPerDocument) {
                bad = true;
                why = "tooMany";
                return;
            }
            const QJsonObject item = value.toObject();
            if (!value.isObject() || !item.value(QStringLiteral("title")).isString()) {
                bad = true;
                return;
            }
            ImportedBookmark imported;
            imported.level = level;
            imported.title = utf8(item.value(QStringLiteral("title")).toString());
            if (item.contains(QStringLiteral("location"))) {
                const QJsonValue location = item.value(QStringLiteral("location"));
                if (!location.isString()) {
                    bad = true;
                    return;
                }
                imported.location = utf8(location.toString());
                // A location from a newer Atlas is kept as it is; a broken one refuses the file.
                if (!imported.location.empty() &&
                    bookmarks::decodeLocation(imported.location).status == bookmarks::LocationDecodeStatus::malformed) {
                    bad = true;
                    return;
                }
            }
            if (item.contains(QStringLiteral("notes"))) {
                if (!item.value(QStringLiteral("notes")).isString()) {
                    bad = true;
                    return;
                }
                imported.description = utf8(item.value(QStringLiteral("notes")).toString());
            }
            if (item.contains(QStringLiteral("tags"))) {
                const QJsonValue tags = item.value(QStringLiteral("tags"));
                if (!tags.isArray()) {
                    bad = true;
                    return;
                }
                for (const auto& tag : tags.toArray()) {
                    if (!tag.isString()) {
                        bad = true;
                        return;
                    }
                    imported.tags.push_back(utf8(tag.toString()));
                }
            }
            BookmarkNode probe; // the overlay's own field rules
            probe.id = "probe";
            probe.documentId = "probe";
            probe.title = imported.title;
            probe.location = imported.location;
            probe.description = imported.description;
            probe.tags = imported.tags;
            if (!bookmarks::validateNode(probe).empty()) {
                bad = true;
                return;
            }
            result.items.push_back(std::move(imported));
            const QJsonValue children = item.value(QStringLiteral("children"));
            if (!children.isUndefined() && !children.isArray()) {
                bad = true;
                return;
            }
            walk(children.toArray(), level + 1);
        }
    };
    walk(root.value(QStringLiteral("bookmarks")).toArray(), 0);
    if (bad) return fail(why);
    result.ok = true;
    return result;
}

} // namespace atlas::app
