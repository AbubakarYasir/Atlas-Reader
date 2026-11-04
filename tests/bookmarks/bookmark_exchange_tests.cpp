// N5.5 portability: the Atlas bookmarks file round-trips exactly; Markdown and
// CSV are readable; bad or newer files are refused whole; imports place items
// where asked and merge without duplicates.

#include "app/bookmarks/BookmarkExchange.h"
#include "core/bookmarks/BookmarkEditor.h"
#include "core/bookmarks/LocationEnvelope.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

using namespace atlas::app;
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

BookmarkNode node(std::string id, std::optional<std::string> parent, std::string title, std::string location,
                  StorageState state = StorageState::localOnly) {
    BookmarkNode n;
    n.id = std::move(id);
    n.documentId = "doc";
    n.parentId = std::move(parent);
    n.title = std::move(title);
    n.location = std::move(location);
    n.state = state;
    return n;
}

std::vector<BookmarkNode> sample() {
    auto chapter = node("a", std::nullopt, "الباب الأول — \"مقدمة\"", at(0), StorageState::embedded);
    auto section = node("b", "a", "Section 1.1 *bold* [x]", at(3), StorageState::localOverride);
    section.description = "line one\nline two — اردو";
    section.tags = {"ref", "خاص", "=SUM(A1)"};
    auto hidden = node("c", "a", "Hidden from the PDF", at(4), StorageState::locallyHidden);
    auto deep = node("d", "b", "  spaced  title  ", "", StorageState::localOnly);
    auto second = node("e", std::nullopt, "=HYPERLINK(\"x\")", at(9), StorageState::embedded);
    return {chapter, section, hidden, deep, second};
}

std::vector<const BookmarkNode*> pointers(const std::vector<BookmarkNode>& nodes) {
    std::vector<const BookmarkNode*> out;
    for (const auto& n : nodes) out.push_back(&n);
    return out;
}

void checkRoundTrip() {
    const auto nodes = sample();
    const auto selection = exportSelection(pointers(nodes), ExportScope::all);
    require(selection.size() == 4, "hidden PDF bookmarks are not exported");
    const QByteArray json = exportJson(selection, {QStringLiteral("كتاب"), QStringLiteral("book.pdf"), 120},
                                       ExportScope::all, QStringLiteral("0.5.0"), QStringLiteral("2026-11-03T10:00:00Z"));
    require(!json.contains("password"), "no passwords in exports");
    const auto imported = importJson(json);
    require(imported.ok, "own export imports: " + imported.error.toStdString());
    require(imported.schemaVersion == QStringLiteral("1.0.0"), "schema version");
    require(imported.hints.fileName == QStringLiteral("book.pdf") && imported.hints.pageCount == 120, "hints");
    require(imported.items.size() == 4, "all exported items come back");
    require(imported.items[0].title == nodes[0].title && imported.items[0].level == 0, "exact Unicode title");
    require(imported.items[1].title == nodes[1].title && imported.items[1].level == 1, "child level");
    require(imported.items[1].description == nodes[1].description, "notes exact");
    require(imported.items[1].tags == nodes[1].tags, "tags exact and in order");
    require(imported.items[1].location == nodes[1].location, "location exact");
    require(imported.items[2].title == "  spaced  title  " && imported.items[2].level == 2, "whitespace kept, grandchild");
    require(imported.items[2].location.empty(), "no location stays empty");
    require(imported.items[3].level == 0, "second top item");
}

void checkScopes() {
    const auto nodes = sample();
    const auto local = exportSelection(pointers(nodes), ExportScope::localOnly);
    require(local.size() == 2 && local[0]->id == "b" && local[1]->id == "d", "only Atlas's own work");
    const auto json = exportJson(local, {}, ExportScope::localOnly, {}, {});
    const auto imported = importJson(json);
    require(imported.ok && imported.items[0].level == 0 && imported.items[1].level == 1,
            "levels follow exported parents only");
    const auto sub = exportSelection(pointers(nodes), ExportScope::subtree, "b");
    require(sub.size() == 2 && sub[0]->id == "b" && sub[1]->id == "d", "subtree");
}

QByteArray withRoot(const std::function<void(QJsonObject&)>& change) {
    const auto nodes = sample();
    const QByteArray json =
        exportJson(exportSelection(pointers(nodes), ExportScope::all), {}, ExportScope::all, {}, {});
    QJsonObject root = QJsonDocument::fromJson(json).object();
    change(root);
    return QJsonDocument{root}.toJson();
}

void checkRefusals() {
    require(importJson("not json").error == QStringLiteral("notJson"), "not JSON");
    require(importJson("[]").error == QStringLiteral("notJson"), "array root");
    require(importJson(withRoot([](QJsonObject& r) { r[QStringLiteral("format")] = QStringLiteral("other"); })).error ==
                QStringLiteral("wrongFormat"),
            "wrong format");
    require(importJson(withRoot([](QJsonObject& r) { r[QStringLiteral("schemaVersion")] = QStringLiteral("2.0.0"); }))
                    .error == QStringLiteral("newerVersion"),
            "newer major refused");
    require(importJson(withRoot([](QJsonObject& r) { r[QStringLiteral("schemaVersion")] = QStringLiteral("one"); }))
                    .error == QStringLiteral("badVersion"),
            "bad version");
    const auto newerMinor = importJson(withRoot([](QJsonObject& r) {
        r[QStringLiteral("schemaVersion")] = QStringLiteral("1.4.0");
        r[QStringLiteral("futureField")] = QStringLiteral("ignored");
        auto list = r[QStringLiteral("bookmarks")].toArray();
        auto first = list[0].toObject();
        first[QStringLiteral("colour")] = QStringLiteral("red");
        list[0] = first;
        r[QStringLiteral("bookmarks")] = list;
    }));
    require(newerMinor.ok && newerMinor.items.size() == 4, "unknown fields of a newer minor are ignored");
    const auto badItem = importJson(withRoot([](QJsonObject& r) {
        auto list = r[QStringLiteral("bookmarks")].toArray();
        list.append(QJsonObject{{QStringLiteral("title"), 5}});
        r[QStringLiteral("bookmarks")] = list;
    }));
    require(!badItem.ok && badItem.error == QStringLiteral("badItem") && badItem.items.empty(), "refused whole");
    const auto badLocation = importJson(withRoot([](QJsonObject& r) {
        auto list = r[QStringLiteral("bookmarks")].toArray();
        list.append(QJsonObject{{QStringLiteral("title"), QStringLiteral("x")},
                                {QStringLiteral("location"), QStringLiteral("garbage{")}});
        r[QStringLiteral("bookmarks")] = list;
    }));
    require(badLocation.error == QStringLiteral("badItem"), "malformed location refused");
    // Depth: 300 nested levels.
    QJsonObject deepest{{QStringLiteral("title"), QStringLiteral("leaf")}};
    for (int i = 0; i < 300; ++i)
        deepest = QJsonObject{{QStringLiteral("title"), QStringLiteral("n")},
                              {QStringLiteral("children"), QJsonArray{deepest}}};
    const auto deep = importJson(withRoot([&](QJsonObject& r) { r[QStringLiteral("bookmarks")] = QJsonArray{deepest}; }));
    require(!deep.ok, "too deep refused");
    const auto emptyChildren = importJson(withRoot([](QJsonObject& r) {
        r[QStringLiteral("bookmarks")] = QJsonArray{QJsonObject{{QStringLiteral("title"), QStringLiteral("t")},
                                                                {QStringLiteral("children"), QJsonArray{}}}};
    }));
    require(emptyChildren.ok && emptyChildren.items.size() == 1, "empty children are fine");
    QByteArray huge(kMaxImportBytes + 1, ' ');
    require(importJson(huge).error == QStringLiteral("tooLarge"), "size limit");
}

void checkReadable() {
    const auto nodes = sample();
    const auto selection = exportSelection(pointers(nodes), ExportScope::all);
    const QString md = QString::fromUtf8(exportMarkdown(selection, {QStringLiteral("My # Book"), {}, 0},
                                                        {QStringLiteral("i"), QStringLiteral("ii"), {}, QStringLiteral("1")}));
    require(md.startsWith(QStringLiteral("# My \\# Book\n")), "title escaped");
    require(md.contains(QStringLiteral("- الباب الأول — \"مقدمة\" — p. i\n")), "top line with page label");
    require(md.contains(QStringLiteral("  - Section 1.1 \\*bold\\* \\[x\\] — p. 1\n")), "child escaped, label used");
    require(md.contains(QStringLiteral("    > line two — اردو\n")), "notes quoted");
    require(md.contains(QStringLiteral("    - spaced title\n")), "no page when no location");
    require(md.contains(QStringLiteral("- =HYPERLINK(\"x\") — p. 10\n")), "page number when no label");

    const QByteArray csv = exportCsv(selection, {});
    require(csv.startsWith("\xEF\xBB\xBF" "level,path,title,page,state,notes,tags\r\n"), "BOM and header");
    const QString text = QString::fromUtf8(csv);
    require(text.contains(QStringLiteral("0,\"\",\"الباب الأول — \"\"مقدمة\"\"\",\"1\",\"embedded\"")), "quotes doubled");
    require(text.contains(QStringLiteral("\"ref;خاص;=SUM(A1)\"")), "tags joined (not at cell start)");
    require(text.contains(QStringLiteral("\"'=HYPERLINK(\"\"x\"\")\"")), "formula kept as text");
    require(text.contains(QStringLiteral("\"line one\nline two — اردو\"")), "newlines stay inside quotes");
    require(text.contains(QStringLiteral("2,\"الباب الأول — \"\"مقدمة\"\" › Section 1.1 *bold* [x]\"")), "path");
}

BaseSnapshot base() {
    BaseSnapshot snapshot;
    snapshot.sourceRevision = "r1";
    snapshot.nodes = {{"0", std::nullopt, 0, "One", at(0)}, {"1", std::nullopt, 1, "Two", at(5)}};
    return snapshot;
}

void checkPlacement() {
    BookmarkEditor editor{"doc", embeddedNodesFromBase("doc", base()), base(), 1};
    const auto order = editor.inOrder();
    const std::string one = order[0]->id;
    // Under a bookmark: the imported tree goes beneath it, keeping its shape.
    auto result = editor.createBatch({{"A", at(1), 0, "note", {"t"}, std::nullopt}, {"A.1", at(2), 1}, {"B", at(3), 0}},
                                     BatchPlacement::after, one);
    require(result.ok, "batch under: " + result.reason);
    editor.confirm(editor.revision() + 1);
    const auto after = editor.inOrder();
    require(after.size() == 5, "three added");
    require(after[1]->title == "A" && after[1]->parentId == one, "A under One");
    require(after[1]->description == "note" && after[1]->tags == std::vector<std::string>{"t"}, "notes and tags kept");
    require(after[2]->title == "A.1" && after[2]->parentId == after[1]->id, "A.1 under A");
    require(after[3]->title == "B" && after[3]->parentId == one, "B under One");
    require(after[4]->title == "Two", "Two still last");
    // replace is refused under a bookmark.
    require(!editor.createBatch({{"x", {}, 0}}, BatchPlacement::replace, one).ok, "replace only at the top");
    // attachTo: a top item joins an existing bookmark (merge).
    const std::string two = after[4]->id;
    NewBookmark child{"Two.1", at(6), 0};
    child.attachTo = two;
    result = editor.createBatch({child, {"Three", at(8), 0}}, BatchPlacement::after);
    require(result.ok, "attachTo: " + result.reason);
    editor.confirm(editor.revision() + 1);
    bool found = false;
    for (const auto* n : editor.inOrder()) {
        if (n->title == "Two.1") found = n->parentId == two;
    }
    require(found, "attached under Two");
    NewBookmark missing{"x", {}, 0};
    missing.attachTo = std::string{"no-such-id"};
    require(!editor.createBatch({missing}, BatchPlacement::after).ok, "missing attach target refused");
}

QByteArray fixture(const char* name) {
    QFile file{QStringLiteral(ATLAS_BOOKMARK_FIXTURE_DIR "/") + QString::fromLatin1(name)};
    require(file.open(QIODevice::ReadOnly), std::string{"fixture "} + name);
    return file.readAll();
}

// The published fixtures (docs/formats/atlas-bookmarks.md).
void checkFixtures() {
    const auto valid = importJson(fixture("valid-1.0.atlas-bookmarks.json"));
    require(valid.ok && valid.items.size() == 4, "valid fixture: " + valid.error.toStdString());
    require(valid.items[1].title == "مبحثٌ في الأدب — اردو: کتابِ عِلم" && valid.items[1].level == 1, "Unicode fixture title");
    require(valid.items[1].description == "Two lines\nسطر ثانٍ" && valid.items[1].tags.size() == 2, "fixture notes and tags");
    require(valid.items[2].title == "  Spaces kept  " && valid.items[3].title == "Index 🔖", "fixture spaces and emoji");
    require(valid.hints.fileName == QStringLiteral("دليل الباحث.pdf"), "fixture hints");
    const auto future = importJson(fixture("future-minor.atlas-bookmarks.json"));
    require(future.ok && future.items.size() == 2 && future.items[0].location.starts_with("atlas-location/2"),
            "newer minor: unknown fields ignored, a newer location kept as it is");
    require(importJson(fixture("newer-major.atlas-bookmarks.json")).error == QStringLiteral("newerVersion"), "newer major");
    require(importJson(fixture("damaged-item.atlas-bookmarks.json")).error == QStringLiteral("badItem"), "damaged item");
}

// docs/QUALITY.md: Atlas-owned parsers are fuzzed. Deterministic, so a failure
// reproduces; any crash, hang or broken invariant fails the test.
void checkInvariants(const ImportResult& result) {
    if (!result.ok) {
        require(result.items.empty() && !result.error.isEmpty(), "a refusal carries a reason and nothing else");
        return;
    }
    require(result.items.size() <= OverlayLimits::maxNodesPerDocument, "count limit");
    std::uint32_t previous = 0;
    for (std::size_t i = 0; i < result.items.size(); ++i) {
        const auto& item = result.items[i];
        require(i == 0 ? item.level == 0 : item.level <= previous + 1, "levels form a tree in reading order");
        require(item.level < OverlayLimits::maxDepth, "depth limit");
        BookmarkNode probe;
        probe.id = "probe";
        probe.documentId = "probe";
        probe.title = item.title;
        probe.location = item.location;
        probe.description = item.description;
        probe.tags = item.tags;
        require(validateNode(probe).empty(), "every accepted item is a valid bookmark");
        previous = item.level;
    }
}

void checkFuzz() {
    std::uint64_t state = 0x9E3779B97F4A7C15ULL;
    const auto next = [&state] {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        return state;
    };
    const auto nodes = sample();
    const QByteArray seed =
        exportJson(exportSelection(pointers(nodes), ExportScope::all), {QStringLiteral("كتاب"), {}, 3}, ExportScope::all, {}, {});
    static const char raw[] = "{}[]\":,\\ 0123456789-.eEtrufalsn\x00\xff\xd8\xa7" "atlas-location/1;=%";
    const QByteArray alphabet{raw, static_cast<qsizetype>(sizeof raw - 1)}; // keeps the NUL byte
    int accepted = 0;
    int refused = 0;
    for (int round = 0; round < 20000; ++round) {
        QByteArray bytes = seed;
        const int edits = 1 + static_cast<int>(next() % 8);
        for (int e = 0; e < edits && !bytes.isEmpty(); ++e) {
            const auto at = static_cast<qsizetype>(next() % static_cast<std::uint64_t>(bytes.size()));
            switch (next() % 4) {
            case 0: bytes[at] = alphabet[static_cast<qsizetype>(next() % static_cast<std::uint64_t>(alphabet.size()))]; break;
            case 1: bytes.remove(at, static_cast<qsizetype>(1 + next() % 16)); break;
            case 2: bytes.insert(at, alphabet.mid(static_cast<qsizetype>(next() % static_cast<std::uint64_t>(alphabet.size())), 4)); break;
            default: bytes.truncate(at); break;
            }
        }
        const auto result = importJson(bytes);
        checkInvariants(result);
        (result.ok ? accepted : refused) += 1;
    }
    std::cout << "fuzz: " << accepted << " accepted, " << refused << " refused\n";
    require(accepted > 100 && refused > 1000, "the fuzz exercises both outcomes");

    // Structured: random trees of random JSON values in every member.
    const auto randomValue = [&next](int depth, const auto& self) -> QJsonValue {
        switch (next() % (depth > 3 ? 5 : 7)) {
        case 0: return QJsonValue{};
        case 1: return static_cast<double>(static_cast<std::int64_t>(next() % 2001) - 1000);
        case 2: return (next() % 2) == 0;
        case 3: return QString::fromUtf8("عنوان ") + QString::number(next() % 100);
        case 4: return QStringLiteral("atlas-location/1;scheme=pdf;page=%1;kind=fit").arg(static_cast<int>(next() % 5) - 1);
        case 5: {
            QJsonArray list;
            for (int i = 0, n = static_cast<int>(next() % 4); i < n; ++i) list.append(self(depth + 1, self));
            return list;
        }
        default: {
            QJsonObject item;
            const char* keys[] = {"title", "location", "notes", "tags", "children", "id", "state", "x"};
            for (int i = 0, n = static_cast<int>(next() % 6); i < n; ++i)
                item.insert(QString::fromLatin1(keys[next() % 8]), self(depth + 1, self));
            return item;
        }
        }
    };
    for (int round = 0; round < 5000; ++round) {
        QJsonObject root{{QStringLiteral("format"), QStringLiteral("atlas-bookmarks")},
                         {QStringLiteral("schemaVersion"), QStringLiteral("1.0.0")}};
        QJsonArray list;
        for (int i = 0, n = static_cast<int>(next() % 5); i < n; ++i) list.append(randomValue(0, randomValue));
        root.insert(QStringLiteral("bookmarks"), list);
        if (next() % 10 == 0) root.insert(QStringLiteral("document"), randomValue(0, randomValue));
        checkInvariants(importJson(QJsonDocument{root}.toJson(QJsonDocument::Compact)));
    }

    // Location envelopes: a decoded envelope always encodes and decodes to itself.
    const std::string valid = "atlas-location/1;scheme=pdf;page=3;label=iv;kind=xyz;left=0.25;top=0.5;zoom=1.5";
    for (int round = 0; round < 50000; ++round) {
        std::string text = valid;
        for (int e = 0, n = 1 + static_cast<int>(next() % 4); e < n && !text.empty(); ++e) {
            const auto at = static_cast<std::size_t>(next() % text.size());
            if (next() % 2 == 0) text[at] = static_cast<char>(alphabet[static_cast<qsizetype>(next() % static_cast<std::uint64_t>(alphabet.size()))]);
            else text.erase(at, 1 + static_cast<std::size_t>(next() % 6));
        }
        const auto decoded = decodeLocation(text);
        if (decoded.status != LocationDecodeStatus::ok) continue;
        require(decoded.envelope.has_value(), "ok carries an envelope");
        const auto again = decodeLocation(encodeLocation(*decoded.envelope));
        require(again.status == LocationDecodeStatus::ok && encodeLocation(*again.envelope) == encodeLocation(*decoded.envelope),
                "a decoded location re-encodes stably");
    }
}

} // namespace

int main() {
    try {
        checkRoundTrip();
        checkScopes();
        checkRefusals();
        checkReadable();
        checkPlacement();
        checkFixtures();
        checkFuzz();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "bookmark exchange tests passed\n";
    return 0;
}
