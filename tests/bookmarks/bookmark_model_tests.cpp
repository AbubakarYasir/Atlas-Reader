#include "core/bookmarks/LocationEnvelope.h"
// N5.3 slice 2: the Bookmarks panel's model and the storage service, with a
// real event loop and profile. Identity is resolved by the library (never
// guessed); every command reaches SQLite on the worker; reopening shows the
// same tree; unknown or ambiguous books stay read-only.

#include "app/bookmarks/BookmarkModel.h"
#include "app/bookmarks/BookmarkService.h"
#include "app/bookmarks/SqliteBookmarkOverlay.h"
#include "app/index/SqliteLibraryIndex.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QUrl>
#include <QVariantMap>
#include <QTimer>
#include <QSemaphore>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace {

using atlas::app::BookmarkModel;
using atlas::app::BookmarkService;
namespace fs = std::filesystem;

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

void waitFor(const std::function<bool()>& condition, std::string_view what, qint64 timeoutMs = 10000) {
    QElapsedTimer timer;
    timer.start();
    while (!condition()) {
        if (timer.elapsed() > timeoutMs) throw std::runtime_error("Timed out waiting for " + std::string{what});
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    }
}

class TemporaryDirectory final {
public:
    TemporaryDirectory() {
        static std::atomic<std::uint64_t> sequence{};
        for (int attempt = 0; attempt < 100; ++attempt) {
            const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
            path_ = fs::temp_directory_path() /
                    ("atlas-model-test-" + std::to_string(nonce) + "-" + std::to_string(sequence++));
            std::error_code error;
            if (fs::create_directory(path_, error)) return;
        }
        throw std::runtime_error("Could not reserve a unique model-test directory.");
    }
    ~TemporaryDirectory() {
        std::error_code ignored;
        fs::remove_all(path_, ignored);
    }
    [[nodiscard]] const fs::path& path() const { return path_; }

private:
    fs::path path_;
};

QString qpath(const fs::path& path) {
    const auto text = path.generic_u16string();
    return QString::fromUtf16(text.data(), static_cast<qsizetype>(text.size()));
}

QVariantMap entry(const QString& key, const QString& parentKey, int position, int level, const QString& title,
                  int page, int children, double fy = -1.0) {
    return {{QStringLiteral("key"), key},           {QStringLiteral("parentKey"), parentKey},
            {QStringLiteral("position"), position}, {QStringLiteral("level"), level},
            {QStringLiteral("title"), title},       {QStringLiteral("page"), page},
            {QStringLiteral("children"), children}, {QStringLiteral("hasPoint"), fy > 0},
            {QStringLiteral("fx"), 0.0},            {QStringLiteral("fy"), fy > 0 ? fy : 0.0},
            {QStringLiteral("zoom"), 0.0}};
}

QVariantList outline() {
    return {entry("0", "", 0, 0, QStringLiteral("الباب الأول"), 0, 2),
            entry("0/0", "0", 0, 1, QStringLiteral("الفصل الأول"), 1, 0, 0.4),
            entry("0/1", "0", 1, 1, QStringLiteral("الفصل الثاني"), 3, 0),
            entry("1", "", 1, 0, QStringLiteral("Part Two"), 10, 0),
            entry("2", "", 2, 0, QStringLiteral("Index"), 40, 0)};
}

QVariantMap at(int page, double fy) {
    return {{QStringLiteral("page"), page}, {QStringLiteral("fx"), 0.0}, {QStringLiteral("fy"), fy}};
}

QString titleAt(const BookmarkModel& model, int row) {
    return model.data(model.index(row), BookmarkModel::TitleRole).toString();
}

QString storageAt(const BookmarkModel& model, int row) {
    return model.data(model.index(row), BookmarkModel::StorageRole).toString();
}

struct Profile final {
    TemporaryDirectory temporary;
    fs::path database;
    fs::path book;
    fs::path stranger;

    Profile() {
        database = temporary.path() / "library.sqlite3";
        book = temporary.path() / "books" / "كتاب.pdf";
        stranger = temporary.path() / "elsewhere.pdf";
        fs::create_directories(book.parent_path());
        for (const auto& file : {book, stranger}) {
            QFile out{qpath(file)};
            require(out.open(QIODevice::WriteOnly) && out.write("%PDF-1.7\n") > 0, "Could not write a test file.");
        }
        atlas::index::SqliteLibraryIndex index{database};
        index.registerRoot({"root", temporary.path() / "books", atlas::index::LibraryRootAvailability::available, 0,
                            std::nullopt, 0});
        index.upsertRecord({"doc", book, "كتاب", "", atlas::document::Availability::available}, "root");
    }
};

// Records refusal reasons (Qt Test is not a project dependency).
struct Refusals final {
    QStringList reasons;
    explicit Refusals(BookmarkModel& model) {
        QObject::connect(&model, &BookmarkModel::refused, [this](const QString& reason) { reasons.push_back(reason); });
    }
};

void waitIdle(BookmarkModel& model, BookmarkService& service) {
    service.waitForIdle();
    waitFor([&] { return !model.busy(); }, "the write");
}

void checkEditing() {
    Profile profile;
    BookmarkService service{profile.database};
    BookmarkModel model;
    model.setService(&service);
    model.attach(qpath(profile.book), outline());
    waitFor([&] { return model.status() == QStringLiteral("editable"); }, "the overlay");
    require(model.totalCount() == 5 && model.rowCount() == 3, "Top-level PDF bookmarks show first, collapsed.");
    require(titleAt(model, 0) == QStringLiteral("الباب الأول") && storageAt(model, 0) == QStringLiteral("embedded"),
            "Rows carry exact titles and the Embedded state.");

    model.toggle(0);
    require(model.rowCount() == 5 && model.data(model.index(1), BookmarkModel::DepthRole).toInt() == 1,
            "Expanding shows children one level deeper.");
    require(model.data(model.index(1), BookmarkModel::PageRole).toInt() == 1 &&
                std::abs(model.data(model.index(1), BookmarkModel::FyRole).toDouble() - 0.4) < 1e-9,
            "Destinations keep page and position.");

    // Add after "Part Two" (row 3), then a child inside it.
    const int added = model.addBookmark(3, false, QStringLiteral("  ملاحظتي  "), at(12, 0.25));
    require(added == 4 && titleAt(model, 4) == QStringLiteral("  ملاحظتي  ") &&
                storageAt(model, 4) == QStringLiteral("local_only"),
            "A new bookmark is local, placed after the selection, with its title kept exactly.");
    const int child = model.addBookmark(added, true, QStringLiteral("Child note"), at(13, 0.5));
    require(child == 5 && model.data(model.index(5), BookmarkModel::DepthRole).toInt() == 1,
            "A child bookmark appears inside its parent, which opens.");
    waitIdle(model, service);

    // Rename a PDF bookmark: LocalOverride. Rename back: Embedded.
    require(model.rename(3, QStringLiteral("Part Two (notes)")) == 3 &&
                storageAt(model, 3) == QStringLiteral("local_override"),
            "Renaming a PDF bookmark marks it changed in Atlas.");
    waitIdle(model, service);
    model.rename(3, QStringLiteral("Part Two"));
    require(storageAt(model, 3) == QStringLiteral("embedded"), "Renaming it back matches the PDF again.");
    waitIdle(model, service);

    // Move, indent, outdent.
    require(model.moveUp(4) == 3 && titleAt(model, 3) == QStringLiteral("  ملاحظتي  "), "Move up.");
    waitIdle(model, service);
    require(model.moveDown(3) == 4, "Move down.");
    waitIdle(model, service);
    const int indented = model.indent(4); // into "Part Two"
    require(indented == 4 && model.data(model.index(4), BookmarkModel::DepthRole).toInt() == 1,
            "Indent makes it the last child of the bookmark above.");
    waitIdle(model, service);
    require(model.outdent(4) == 4 && model.data(model.index(4), BookmarkModel::DepthRole).toInt() == 0,
            "Outdent puts it back after its parent.");
    waitIdle(model, service);
    require(model.outdent(0) == -1, "A top-level bookmark cannot be outdented.");

    // Delete a PDF bookmark: hidden, can be shown and restored.
    Refusals refusals{model};
    const int before = model.rowCount();
    model.remove(6); // "Index"
    waitIdle(model, service);
    require(model.rowCount() == before - 1 && model.hiddenCount() == 1, "Deleting a PDF bookmark hides it.");
    model.setShowHidden(true);
    const int hiddenRow = model.rowCount() - 1;
    require(storageAt(model, hiddenRow) == QStringLiteral("locally_hidden"), "Hidden bookmarks can be shown.");
    model.setHidden(hiddenRow, false);
    waitIdle(model, service);
    require(model.hiddenCount() == 0 && storageAt(model, hiddenRow) == QStringLiteral("embedded"),
            "Unhiding restores it.");
    model.setShowHidden(false);

    // Notes and tags; search.
    model.setNotes(0, QStringLiteral("أهم باب في الكتاب"), {QStringLiteral("فقه"), QStringLiteral("مهم")});
    waitIdle(model, service);
    model.setFilter(QStringLiteral("فقه"));
    require(model.rowCount() == 1 && model.data(model.index(0), BookmarkModel::MatchRole).toString() == "tag",
            "Search finds a tag.");
    model.setFilter(QStringLiteral("الفصل"));
    require(model.rowCount() == 2 &&
                model.data(model.index(0), BookmarkModel::BreadcrumbRole).toStringList() ==
                    QStringList{QStringLiteral("الباب الأول")},
            "Search results show where each bookmark sits.");
    model.setFilter({});

    // Undo/redo through the model.
    require(model.canUndo() && model.undoKind() == QStringLiteral("setNotes"), "Undo names the last change.");
    model.undo();
    waitIdle(model, service);
    require(model.data(model.index(0), BookmarkModel::NotesRole).toString().isEmpty(), "Undo removes the notes.");
    model.redo();
    waitIdle(model, service);

    // Current-page highlight: the deepest bookmark at or before the page.
    model.setCurrentPage(2);
    require(model.currentId() == model.data(model.index(1), BookmarkModel::NodeIdRole).toString(),
            "The bookmark for the current page is found.");

    // Position within a page: before the section's point (fy 0.4) the chapter is
    // current; at or after it, the section is.
    const QString chapterId = model.data(model.index(0), BookmarkModel::NodeIdRole).toString();
    const QString sectionId = model.data(model.index(1), BookmarkModel::NodeIdRole).toString();
    model.setPosition(1, 0.1);
    require(model.currentId() == chapterId, "Above a bookmark's point, the previous bookmark is current.");
    model.setPosition(1, 0.45);
    require(model.currentId() == sectionId, "Past a bookmark's point, that bookmark is current.");

    // Command Center search: reading order, breadcrumb and destination.
    const auto found = model.findBookmarks(QStringLiteral("الفصل"), 8);
    require(found.size() == 2 && found[0].toMap().value(QStringLiteral("breadcrumb")).toString() == QStringLiteral("الباب الأول") &&
                found[0].toMap().value(QStringLiteral("page")).toInt() == 1 &&
                std::abs(found[0].toMap().value(QStringLiteral("fy")).toDouble() - 0.4) < 1e-9,
            "Bookmark search returns where each one is and where it goes.");
    require(model.findBookmarks(QStringLiteral("الفصل"), 1).size() == 1 && model.findBookmarks(QStringLiteral(" "), 8).isEmpty(),
            "Search respects its limit and ignores blank text.");

    // Refusals are reported, not silent.
    refusals.reasons.clear();
    require(model.moveUp(0) == -1 && refusals.reasons == QStringList{QStringLiteral("atTop")},
            "Refusals carry a reason for the panel.");

    // Everything is stored: a new model on the same profile shows the same tree.
    const auto expected = [&] {
        QStringList titles;
        model.expandAll();
        for (int row = 0; row < model.rowCount(); ++row) titles.push_back(titleAt(model, row) + "|" + storageAt(model, row));
        return titles;
    }();
    BookmarkModel reopened;
    reopened.setService(&service);
    reopened.attach(qpath(profile.book), outline());
    waitFor([&] { return reopened.status() == QStringLiteral("editable"); }, "the reopened overlay");
    reopened.expandAll();
    QStringList actual;
    for (int row = 0; row < reopened.rowCount(); ++row) actual.push_back(titleAt(reopened, row) + "|" + storageAt(reopened, row));
    require(actual == expected, "Reopening shows exactly what was edited.");
    require(!reopened.outlineChangedOutside(), "An unchanged PDF needs no reconciling.");
}

// Auto Create Bookmarks: a previewed list becomes a tree in one write and one undo.
void checkAddMany() {
    Profile profile;
    BookmarkService service{profile.database};
    BookmarkModel model;
    model.setService(&service);
    model.attach(qpath(profile.book), outline());
    waitFor([&] { return model.status() == QStringLiteral("editable"); }, "the overlay");
    const auto heading = [](const QString& title, int page, int level) {
        QVariantMap map = at(page, 0.12);
        map.insert(QStringLiteral("title"), title);
        map.insert(QStringLiteral("level"), level);
        map.insert(QStringLiteral("fx"), 0.1);
        return QVariant{map};
    };
    const int first = model.addMany({heading(QStringLiteral("الفصل الأول"), 2, 0), heading(QStringLiteral("مبحث"), 3, 1),
                                     heading(QStringLiteral("  "), 3, 1), heading(QStringLiteral("Chapter 2"), 9, 0)},
                                    QStringLiteral("before"));
    waitIdle(model, service);
    require(first == 0 && titleAt(model, 0) == QStringLiteral("الفصل الأول") && storageAt(model, 0) == QStringLiteral("local_only"),
            "The batch goes first and is Atlas's own; blank titles are skipped.");
    require(model.totalCount() == 8 && model.undoKind() == QStringLiteral("autoCreate"), "Three added, one undo step.");
    model.undo();
    waitIdle(model, service);
    require(model.totalCount() == 5, "Undo removes the whole batch.");
    Refusals refusals{model};
    require(model.addMany({}, QStringLiteral("after")) == -1 && refusals.reasons.contains(QStringLiteral("nothingToAdd")),
            "An empty batch is refused with a reason.");
}

void checkExchange() {
    Profile profile;
    BookmarkService service{profile.database};
    BookmarkModel model;
    model.setService(&service);
    model.attach(qpath(profile.book), outline());
    waitFor([&] { return model.status() == QStringLiteral("editable"); }, "the overlay");
    const QString file = qpath(profile.temporary.path() / "out.atlas-bookmarks.json");
    const QVariantMap hints{{QStringLiteral("title"), QStringLiteral("كتاب")}, {QStringLiteral("pageCount"), 50}};
    require(model.exportTo(QUrl::fromLocalFile(file), QStringLiteral("json"), QStringLiteral("all"), -1, hints).isEmpty(),
            "Export writes the Atlas bookmarks file.");
    require(model.exportTo(QUrl::fromLocalFile(file + QStringLiteral(".md")), QStringLiteral("markdown"),
                           QStringLiteral("all"), -1, hints).isEmpty() &&
                model.exportTo(QUrl::fromLocalFile(file + QStringLiteral(".csv")), QStringLiteral("csv"),
                               QStringLiteral("all"), -1, hints).isEmpty(),
            "Markdown and CSV export too.");
    require(model.exportTo(QUrl::fromLocalFile(file), QStringLiteral("json"), QStringLiteral("local"), -1, hints) ==
                QStringLiteral("nothing"),
            "Nothing of Atlas's own to export yet.");
    QVariantMap info = model.readImport(QUrl::fromLocalFile(file));
    require(info.value(QStringLiteral("ok")).toBool() && info.value(QStringLiteral("count")).toInt() == 5 &&
                info.value(QStringLiteral("sameBook")).toBool(),
            "The file reads back: five bookmarks from this book.");
    Refusals refusals{model};
    require(model.applyImport(QStringLiteral("merge"), -1) == -1 && refusals.reasons.contains(QStringLiteral("nothingNew")),
            "Merging the same bookmarks adds nothing.");
    info = model.readImport(QUrl::fromLocalFile(file));
    require(model.applyImport(QStringLiteral("end"), -1) >= 0, "Import at the end.");
    waitIdle(model, service);
    require(model.totalCount() == 10 && model.undoKind() == QStringLiteral("autoCreate"), "Five added, one undo step.");
    model.undo();
    waitIdle(model, service);
    require(model.totalCount() == 5, "Undo takes the whole import out.");
    // Merge where only a child is new: it joins the bookmark already here.
    QFile edited{file};
    require(edited.open(QIODevice::ReadOnly), "read export");
    QByteArray bytes = edited.readAll();
    edited.close();
    bytes.replace("\"title\": \"الفصل الثاني\"", "\"title\": \"الفصل الثالث\"");
    require(edited.open(QIODevice::WriteOnly | QIODevice::Truncate) && edited.write(bytes) == bytes.size(), "write");
    edited.close();
    info = model.readImport(QUrl::fromLocalFile(file));
    require(info.value(QStringLiteral("ok")).toBool(), "edited file reads");
    refusals.reasons.clear();
    require(model.applyImport(QStringLiteral("merge"), -1) >= 0,
            "Merge with one new bookmark: " + refusals.reasons.join(u',').toStdString());
    waitIdle(model, service);
    require(model.totalCount() == 6, "Only the new bookmark is added.");
    model.expandAll();
    bool underParent = false;
    for (int row = 0; row < model.rowCount(); ++row) {
        if (titleAt(model, row) == QStringLiteral("الفصل الثالث"))
            underParent = model.get(row).value(QStringLiteral("depth")).toInt() == 1;
    }
    require(underParent, "It goes under its parent already here.");
    QFile junk{file + QStringLiteral(".bad")};
    require(junk.open(QIODevice::WriteOnly) && junk.write("{\"format\":\"x\"}") > 0, "write junk");
    junk.close();
    require(model.readImport(QUrl::fromLocalFile(junk.fileName())).value(QStringLiteral("error")).toString() ==
                QStringLiteral("wrongFormat"),
            "A file that isn't Atlas bookmarks is refused.");
    require(model.applyImport(QStringLiteral("end"), -1) == -1, "Nothing pending after a refused file.");
}

void checkAsyncExchange() {
    Profile profile;
    BookmarkService service{profile.database};
    BookmarkModel model;
    model.setService(&service);
    model.attach(qpath(profile.book), outline());
    waitFor([&] { return model.editable(); }, "async overlay");
    const auto source = QUrl::fromLocalFile(qpath(profile.temporary.path() / "large.json"));
    const auto target = QUrl::fromLocalFile(qpath(profile.temporary.path() / "export.json"));
    std::vector<atlas::bookmarks::BookmarkNode> nodes(10000);
    std::vector<const atlas::bookmarks::BookmarkNode*> pointers;
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        nodes[i].id = std::to_string(i);
        nodes[i].title = "كتاب اردو " + std::to_string(i);
        nodes[i].position = static_cast<std::uint32_t>(i % 100 == 0 ? i / 100 : i % 100 - 1);
        if (i % 100 != 0) nodes[i].parentId = std::to_string(i - i % 100);
        nodes[i].location = atlas::bookmarks::encodeLocation(atlas::bookmarks::LocationEnvelope{});
        nodes[i].state = atlas::bookmarks::StorageState::localOnly;
        pointers.push_back(&nodes[i]);
    }
    QFile fixture{source.toLocalFile()};
    const auto bytes = atlas::app::exportJson(pointers, {}, atlas::app::ExportScope::all, "test", "test");
    require(fixture.open(QIODevice::WriteOnly) && fixture.write(bytes) == bytes.size(), "large fixture");
    fixture.close();
    int previews = 0;
    QVariantMap info;
    QObject::connect(&model, &BookmarkModel::importRead, [&](const QVariantMap& value) { info = value; ++previews; });
    int applied = 0;
    QObject::connect(&model, &BookmarkModel::importApplied, [&](int row) { require(row >= 0, "import row"); ++applied; });
    int exports = 0;
    QObject::connect(&model, &BookmarkModel::exportFinished, [&](const QString& error, const QUrl&) {
        require(error.isEmpty(), "async export succeeds"); ++exports;
    });
    QSemaphore barrier;
    service.exchangeWork([&] { barrier.acquire(); });
    model.readImportAsync(source);
    require(model.exchanging() && !model.editable(), "visible busy state prevents concurrent editing");
    model.cancelExchange();
    barrier.release();
    service.waitForIdle();
    QCoreApplication::processEvents();
    require(previews == 0 && model.totalCount() == 5, "cancelled read has no stale preview or mutation");
    int ticks = 0;
    qint64 maximumGap = 0;
    QElapsedTimer heartbeat;
    heartbeat.start();
    QTimer timer;
    timer.setInterval(1);
    QObject::connect(&timer, &QTimer::timeout, [&] {
        maximumGap = std::max(maximumGap, heartbeat.restart()); ++ticks;
    });
    timer.start();
    QElapsedTimer elapsed;
    elapsed.start();
    model.readImportAsync(source);
    waitFor([&] { return previews == 1; }, "10k import parsing");
    const auto readMs = elapsed.elapsed();
    require(info.value("ok").toBool() && info.value("count").toInt() == 10000, "10k preview");
    service.exchangeWork([&] { barrier.acquire(); });
    model.applyImportAsync("end", -1);
    model.cancelExchange();
    barrier.release();
    service.waitForIdle();
    QCoreApplication::processEvents();
    require(model.totalCount() == 5 && applied == 0, "cancelled apply has no partial import");
    model.readImportAsync(source);
    waitFor([&] { return previews == 2; }, "second preview");
    elapsed.restart();
    model.applyImportAsync("end", -1);
    waitFor([&] { return applied == 1; }, "10k import apply");
    waitFor([&] { return !model.busy(); }, "transaction committed", 60000);
    const auto applyMs = elapsed.elapsed();
    require(model.totalCount() == 10005 && model.canUndo(), "whole import is present with undo");
    service.exchangeWork([&] { barrier.acquire(); });
    model.exportAsync(target, "json", "all", -1, {});
    model.cancelExchange();
    barrier.release();
    service.waitForIdle();
    require(!QFile::exists(target.toLocalFile()), "cancelled export creates no destination");
    elapsed.restart();
    model.exportAsync(target, "json", "all", -1, {});
    waitFor([&] { return exports == 1; }, "10k export");
    const auto exportMs = elapsed.elapsed();
    require(ticks > 0, "interface heartbeat runs during exchange");
    timer.stop();
    model.expandAll();
    const QString jumpId = model.get(model.rowCount() - 1).value("nodeId").toString();
    std::vector<double> expandTimes, collapseTimes, revealTimes;
    for (int sample = 0; sample < 30; ++sample) {
        elapsed.restart(); model.collapseAll(); collapseTimes.push_back(elapsed.nsecsElapsed() / 1e6);
        elapsed.restart(); model.expandAll(); expandTimes.push_back(elapsed.nsecsElapsed() / 1e6);
        model.collapseAll();
        elapsed.restart();
        const int row = model.reveal(jumpId);
        require(row >= 0 && model.get(row).value("nodeId").toString() == jumpId, "reveal exact bookmark");
        revealTimes.push_back(elapsed.nsecsElapsed() / 1e6);
    }
    const auto report = [](const char* name, std::vector<double> values) {
        std::sort(values.begin(), values.end());
        std::cout << "Panel 10005 " << name << " n=30 p50_ms=" << values[14]
                  << " p95_ms=" << values[28] << " p99_ms=" << values[29] << '\n';
    };
    report("expand", expandTimes); report("collapse", collapseTimes); report("reveal", revealTimes);
    // CHECKPOINTS N5 automated QA: 10,000-node navigation under 100 ms p95.
    // Timing is gated on optimised builds only; Debug builds report the numbers.
#ifdef NDEBUG
    for (auto* values : {&expandTimes, &collapseTimes, &revealTimes}) {
        std::sort(values->begin(), values->end());
        require((*values)[28] < 100.0, "10,000-bookmark panel navigation under 100 ms p95");
    }
#endif
    model.undo();
    waitIdle(model, service);
    require(model.totalCount() == 5, "one undo removes the entire asynchronous import");
    model.readImportAsync(source);
    service.waitForIdle(); // response is queued, but not yet delivered
    model.detach();
    QCoreApplication::processEvents();
    require(previews == 2 && model.totalCount() == 0, "closing tab ignores completed stale import");
    std::cout << "Exchange 10000: read_ms=" << readMs << " apply_commit_ms=" << applyMs
              << " export_ms=" << exportMs << " max_heartbeat_gap_ms=" << maximumGap << '\n';
}

void checkReadOnly() {
    Profile profile;
    BookmarkService service{profile.database};
    BookmarkModel model;
    model.setService(&service);
    Refusals refusals{model};

    model.attach(qpath(profile.stranger), outline());
    waitFor([&] { return model.status() == QStringLiteral("readOnly"); }, "read-only mode");
    require(model.statusReason() == QStringLiteral("notInLibrary") && model.rowCount() == 3,
            "A book outside the library shows its outline read-only.");
    require(model.addBookmark(-1, false, QStringLiteral("x"), at(1, 0.5)) == -1 &&
                refusals.reasons == QStringList{QStringLiteral("readOnly")},
            "Read-only books refuse edits with a reason.");
    model.toggle(0);
    require(model.rowCount() == 5, "Read-only outlines still expand.");

    // Two library documents at one path: ambiguous, never guessed.
    {
        atlas::index::SqliteLibraryIndex index{profile.database};
        index.registerRoot({"root2", profile.temporary.path(), atlas::index::LibraryRootAvailability::available, 0,
                            std::nullopt, 0});
        index.upsertRecord({"doc-copy", profile.temporary.path() / "books" / "كتاب.pdf", "كتاب", "",
                            atlas::document::Availability::available},
                           "root2");
    }
    BookmarkModel twice;
    twice.setService(&service);
    twice.attach(qpath(profile.book), outline());
    waitFor([&] { return twice.status() != QStringLiteral("loading"); }, "the ambiguous book");
    require(twice.status() == QStringLiteral("readOnly") && twice.statusReason() == QStringLiteral("ambiguous"),
            "An ambiguous identity keeps the book read-only.");

    // Detaching drops late answers.
    BookmarkModel late;
    late.setService(&service);
    late.attach(qpath(profile.stranger), outline());
    late.detach();
    service.waitForIdle();
    QCoreApplication::processEvents();
    require(late.status() == QStringLiteral("none") && late.rowCount() == 0, "A detached model ignores late answers.");
}

void checkOutlineConversion() {
    const auto base = atlas::app::baseFromOutline(outline(), "size=1;mtime=2");
    require(base && base->nodes.size() == 5, "The outline converts to a base snapshot.");
    require(base->nodes[1].location.find("kind=xyz") != std::string::npos &&
                base->nodes[1].location.find("top=0.4") != std::string::npos,
            "A point on the page becomes an XYZ destination.");
    require(base->nodes[0].location.find("kind=fit") != std::string::npos,
            "An entry without a usable point goes to the top of its page.");
    auto broken = outline();
    auto orphan = broken[1].toMap();
    orphan.insert(QStringLiteral("parentKey"), QStringLiteral("9"));
    broken[1] = orphan;
    require(!atlas::app::baseFromOutline(broken, "rev"), "A broken outline is refused whole.");
}

void checkQueuedWritesAndRecovery() {
    Profile profile;
    BookmarkService service{profile.database};
    BookmarkModel model;
    model.setService(&service);
    model.attach(qpath(profile.book), outline());
    waitFor([&] { return model.status() == QStringLiteral("editable"); }, "the overlay");

    // 30 edits without waiting: none refused, all stored in order.
    Refusals refusals{model};
    int row = -1;
    for (int i = 0; i < 30; ++i) {
        row = model.addBookmark(row, false, QStringLiteral("سريع %1").arg(i), at(i, 0.1));
        require(row >= 0, "Fast edits are never refused while saves are pending.");
    }
    require(refusals.reasons.isEmpty(), "No refusals for queued edits.");
    waitIdle(model, service);
    {
        atlas::bookmarks::SqliteBookmarkOverlay check{profile.database};
        const auto stored = check.load("doc");
        require(stored.nodes.size() == 35 && stored.revision == 31, "All queued edits reached the profile in order.");
    }

    // Someone else changes the profile: the next save fails and the panel
    // shows what is stored, with no half-applied change.
    {
        atlas::bookmarks::SqliteBookmarkOverlay other{profile.database};
        auto snapshot = other.load("doc");
        auto node = snapshot.nodes.back();
        for (const auto& candidate : snapshot.nodes) {
            if (!candidate.baseKey) node = candidate;
        }
        node.title = "Changed by another window";
        (void)other.apply("doc", snapshot.revision, {{node}, {}, {}});
    }
    QStringList reloads;
    QObject::connect(&model, &BookmarkModel::reloaded, [&](const QString& reason) { reloads.push_back(reason); });
    model.rename(0, QStringLiteral("Will not stick"));
    waitFor([&] { return !reloads.isEmpty(); }, "the reload after a failed save");
    waitFor([&] { return !model.busy(); }, "the reload");
    require(reloads == QStringList{QStringLiteral("revisionConflict")}, "A stale save reports a revision conflict.");
    require(titleAt(model, 0) == QStringLiteral("الباب الأول"), "The failed change is not shown.");
    bool found = false;
    model.expandAll();
    for (int r = 0; r < model.rowCount(); ++r) found = found || titleAt(model, r) == "Changed by another window";
    require(found && !model.canUndo(), "The other window's change is shown; history starts fresh.");

    // Closing the tab with saves queued still stores them.
    for (int i = 0; i < 5; ++i) model.addBookmark(-1, false, QStringLiteral("last %1").arg(i), at(1, 0.2));
    model.detach();
    service.waitForIdle();
    atlas::bookmarks::SqliteBookmarkOverlay check{profile.database};
    require(check.load("doc").nodes.size() == 40, "Saves queued when the tab closed are not lost.");
}

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app{argc, argv};
    try {
        checkOutlineConversion();
        checkEditing();
        checkAddMany();
        checkExchange();
        checkAsyncExchange();
        checkReadOnly();
        checkQueuedWritesAndRecovery();
    } catch (const std::exception& error) {
        std::cerr << "Bookmark model test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Bookmark panel model: identity, editing through the worker, reopen, read-only and search passed.\n";
    return 0;
}
