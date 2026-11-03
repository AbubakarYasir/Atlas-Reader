// N5.4 safe save, end to end (ADR-0009): a copy of an N2 fixture in a real
// profile, edited through the editor and store, saved through qpdf, verified,
// backed up, replaced and committed. Failures injected at every step must
// leave the PDF and the overlay exactly as they were.

#include "app/bookmarks/BookmarkModel.h"
#include "app/bookmarks/BookmarkService.h"
#include "app/bookmarks/OutlineSafeSave.h"
#include "app/bookmarks/SqliteBookmarkOverlay.h"
#include "app/document/DocumentCapabilityCheck.h"
#include "app/index/SqliteLibraryIndex.h"
#include "app/reader/DocumentTextWorker.h"
#include "core/bookmarks/BookmarkEditor.h"
#include "core/bookmarks/LocationEnvelope.h"

#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QPdfDocument>
#include <QTemporaryDir>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

using namespace atlas::bookmarks;
using atlas::app::SafeSaveInput;
using atlas::app::SafeSaveOutcome;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

QString fixture(const char* name) { return QStringLiteral(ATLAS_FIXTURE_DIR "/") + QString::fromLatin1(name); }

std::filesystem::path fsPath(const QString& path) { return std::filesystem::path{path.toStdU16String()}; }

QByteArray sha(const QString& path) {
    QFile file{path};
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256);
}

std::string at(std::uint32_t page, double top) {
    LocationEnvelope envelope;
    envelope.pdf.pageIndex = page;
    envelope.pdf.kind = DestinationKind::xyz;
    envelope.pdf.left = 0.1;
    envelope.pdf.top = top;
    return encodeLocation(envelope);
}

QVariantList qtOutline(const QString& path, const QString& password = {}) {
    QPdfDocument document;
    if (!password.isEmpty()) document.setPassword(password);
    document.load(path);
    require(document.status() == QPdfDocument::Status::Ready, "Qt PDF opens " + path.toStdString());
    return atlas::app::readOutline(document);
}

QStringList titles(const QVariantList& outline) {
    QStringList out;
    for (const auto& entry : outline) {
        const auto map = entry.toMap();
        out << map.value(QStringLiteral("key")).toString() + u' ' + map.value(QStringLiteral("title")).toString();
    }
    return out;
}

QStringList folderFiles(const QString& folder) { return QDir{folder}.entryList(QDir::Files | QDir::Hidden); }

// A book in a library folder, its overlay opened the way the app opens it.
struct Book final {
    QTemporaryDir dir;
    QString folder;
    QString pdf;
    QString password;
    std::filesystem::path database;
    std::unique_ptr<SqliteBookmarkOverlay> store;
    std::unique_ptr<BookmarkEditor> editor;

    explicit Book(const char* source, QString pass = {}) : password{std::move(pass)} {
        require(dir.isValid(), "temporary folder");
        folder = dir.filePath(QStringLiteral("books"));
        QDir{}.mkpath(folder);
        pdf = QDir{folder}.filePath(QStringLiteral("كتاب البحث.pdf"));
        require(QFile::copy(fixture(source), pdf), "copy fixture");
        QFile::setPermissions(pdf, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser | QFile::WriteUser);
        database = fsPath(dir.filePath(QStringLiteral("library.sqlite3")));
        {
            atlas::index::SqliteLibraryIndex index{database};
            index.registerRoot({"root", fsPath(folder), atlas::index::LibraryRootAvailability::available, 0,
                                std::nullopt, 0});
            index.upsertRecord({"doc", fsPath(pdf), "Book", "", atlas::document::Availability::available}, "root");
        }
        store = std::make_unique<SqliteBookmarkOverlay>(database);
        open();
    }
    void open() {
        const auto base = atlas::app::baseFromOutline(qtOutline(pdf, password), atlas::app::fileRevision(pdf));
        require(base.has_value(), "base from outline");
        lastAdoption = store->adoptBase("doc", *base);
        const auto snapshot = store->load("doc");
        editor = std::make_unique<BookmarkEditor>("doc", snapshot.nodes, snapshot.base, snapshot.revision);
    }
    BaseAdoption lastAdoption{BaseAdoption::adopted};
    void run(EditResult result) {
        require(result.ok, "command refused: " + result.reason);
        editor->confirm(store->apply("doc", editor->revision(),
                                     {result.change.upserts, result.change.removals, result.change.restores}));
    }
    std::string id(std::string_view key) {
        for (const auto* node : editor->inOrder()) {
            if (node->baseKey && *node->baseKey == key) return node->id;
        }
        throw std::runtime_error("no node");
    }
    SafeSaveOutcome save(SaveOptions options = {}, const atlas::app::SafeSaveFault& fault = {}) {
        SafeSaveInput input;
        input.path = pdf;
        input.password = password;
        input.documentId = "doc";
        input.overlayRevision = editor->revision();
        for (const auto* node : editor->inOrder()) input.nodes.push_back(*node);
        input.base = store->load("doc").base;
        input.options = options;
        input.backupRoot = fsPath(dir.filePath(QStringLiteral("backups")));
        return atlas::app::saveOutlineSafely(*store, input, fault);
    }
};

void editA003(Book& book) {
    book.run(book.editor->rename(book.id("1"), "الفصل الثاني — محفوظ"));
    const auto folder = book.editor->create(std::nullopt, 0, "Atlas notes", "");
    book.run(folder);
    book.run(book.editor->create(folder.nodeId, 0, "On page 2", at(1, 0.3)));
}

void checkSave() {
    Book book{"A003_outlines_links.pdf"};
    const QByteArray original = sha(book.pdf);
    editA003(book);
    const auto saved = book.save();
    require(saved.ok, "Save succeeds: " + saved.reason.toStdString());
    require(saved.pdfChanged && sha(book.pdf) != original, "The PDF was replaced.");
    require(sha(saved.backupPath) == original, "The backup is the original, byte for byte.");
    require(folderFiles(book.folder) == QStringList{QStringLiteral("كتاب البحث.pdf")}, "No temporary files are left.");
    const QStringList expected{QStringLiteral("0 Atlas notes"), QStringLiteral("0/0 On page 2"),
                               QStringLiteral("1 Chapter 1"), QStringLiteral("1/0 Section 1.1"),
                               QStringLiteral("2 الفصل الثاني — محفوظ")};
    require(titles(qtOutline(book.pdf)) == expected, "Qt PDF reads the saved outline: " +
                                                      titles(qtOutline(book.pdf)).join(u" | ").toStdString());
    const auto snapshot = book.store->load("doc");
    require(snapshot.revision == saved.overlayRevision, "The overlay moved to the committed revision.");
    for (const auto& node : snapshot.nodes)
        require(node.state == StorageState::embedded, "Every saved bookmark is Embedded.");
    require(validateAgainstBase(snapshot.nodes, snapshot.base).empty(), "No false Embedded.");
    // Opening the book again finds nothing to reconcile.
    book.open();
    require(book.lastAdoption == BaseAdoption::unchanged, "Reopening the saved PDF needs no reconciling.");
    require(book.save().reason == QStringLiteral("nothingToSave"), "A second save has nothing to do.");
}

void checkHidden() {
    {
        Book book{"A003_outlines_links.pdf"};
        book.run(book.editor->setHidden(book.id("0"), true));
        book.run(book.editor->create(std::nullopt, 9, "New", at(2, 0.5)));
        const auto kept = book.save();
        require(kept.ok && titles(qtOutline(book.pdf)).size() == 4, "A hidden PDF bookmark stays in the PDF.");
        const auto snapshot = book.store->load("doc");
        int hidden = 0;
        for (const auto& node : snapshot.nodes) hidden += node.state == StorageState::locallyHidden ? 1 : 0;
        require(hidden == 1, "It stays hidden in Atlas.");
    }
    {
        Book book{"A003_outlines_links.pdf"};
        book.run(book.editor->setHidden(book.id("0"), true));
        const auto removed = book.save({true});
        require(removed.ok, "Save with removal: " + removed.reason.toStdString());
        require(titles(qtOutline(book.pdf)) == QStringList({QStringLiteral("0 Section 1.1"), QStringLiteral("1 Chapter 2")}),
                "Removing a hidden bookmark moves its child up.");
        const auto snapshot = book.store->load("doc");
        require(snapshot.nodes.size() == 2 && !snapshot.nodes[0].parentId && !snapshot.nodes[1].parentId,
                "The overlay matches the PDF's new tree.");
    }
}

void checkFailures() {
    for (const char* failing : {"write", "verify", "reopen", "backup", "beforeReplace", "replace"}) {
        Book book{"A003_outlines_links.pdf"};
        editA003(book);
        const QByteArray original = sha(book.pdf);
        const auto revision = book.editor->revision();
        const auto outcome = book.save({}, [failing](std::string_view step) { return step != failing; });
        require(!outcome.ok && !outcome.pdfChanged, std::string{"Fails at "} + failing);
        require(sha(book.pdf) == original, std::string{"The PDF is untouched after a failure at "} + failing);
        require(book.store->load("doc").revision == revision, "The overlay is untouched.");
        require(folderFiles(book.folder).size() == 1, std::string{"No temporary file is left after "} + failing);
    }
    {
        Book book{"A003_outlines_links.pdf"};
        editA003(book);
        const QByteArray original = sha(book.pdf);
        const auto outcome = book.save({}, [](std::string_view step) { return step != "afterReplace"; });
        require(outcome.reason == QStringLiteral("replaceUnverified") && !outcome.pdfChanged && sha(book.pdf) == original,
                "A replaced file that does not verify is put back from the backup.");
    }
    {
        Book book{"A003_outlines_links.pdf"};
        editA003(book);
        QFile file{book.pdf};
        require(file.open(QIODevice::Append) && file.write("\n% changed outside Atlas\n") > 0, "append");
        file.close();
        const QByteArray changed = sha(book.pdf);
        require(book.save().reason == QStringLiteral("changedOutside") && sha(book.pdf) == changed,
                "A PDF changed outside Atlas is never overwritten.");
    }
}

void checkProtected() {
    {
        Book book{"A015_encrypted_outline_rc4_40.pdf", QStringLiteral("atlas-user")};
        book.run(book.editor->rename(book.id("0"), "جذر محفوظ"));
        const auto saved = book.save();
        require(saved.ok, "An encrypted PDF that permits changes saves: " + saved.reason.toStdString());
        require(titles(qtOutline(book.pdf, QStringLiteral("atlas-user"))) == QStringList{QStringLiteral("0 جذر محفوظ")},
                "Still opens with its password.");
    }
    {
        Book book{"A010_certification_structure.pdf"};
        book.run(book.editor->create(std::nullopt, 0, "x", at(0, 0.1)));
        const QByteArray original = sha(book.pdf);
        require(book.save().reason == QStringLiteral("notPermitted") && sha(book.pdf) == original,
                "A certified PDF is never written.");
    }
    {
        Book book{"A003_outlines_links.pdf"};
        editA003(book);
        QFile::setPermissions(book.pdf, QFile::ReadOwner | QFile::ReadUser);
        const QByteArray original = sha(book.pdf);
        if (QFileInfo{book.pdf}.isWritable()) {
            std::cout << "Running with rights that ignore read-only files; read-only case skipped.\n";
        } else {
            const auto outcome = book.save();
            require(outcome.reason == QStringLiteral("notPermitted") && sha(book.pdf) == original,
                    "A read-only file is never written: " + outcome.reason.toStdString());
        }
        QFile::setPermissions(book.pdf, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser | QFile::WriteUser);
    }
}

// The app's path: BookmarkModel → BookmarkService worker → safe save → reload.
void checkThroughModel() {
    Book book{"A003_outlines_links.pdf"};
    book.store.reset(); // the service opens its own connection
    atlas::app::BookmarkService service{book.database};
    atlas::app::BookmarkModel model;
    model.setService(&service);
    model.attach(book.pdf, qtOutline(book.pdf));
    const auto waitFor = [](const std::function<bool()>& condition) {
        QElapsedTimer timer;
        timer.start();
        while (!condition() && timer.elapsed() < 60000) QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        return condition();
    };
    require(waitFor([&] { return model.status() == QStringLiteral("editable"); }), "The model opens the book.");
    model.addBookmark(-1, false, QStringLiteral("من النموذج"), {{QStringLiteral("page"), 2}, {QStringLiteral("fx"), 0.1},
                                                                 {QStringLiteral("fy"), 0.4}});
    require(model.unsavedCount() == 1, "One bookmark waits to be saved.");
    bool finished = false;
    bool saved = false;
    QObject::connect(&model, &atlas::app::BookmarkModel::saveFinished,
                     [&](bool ok, const QString&, bool, int, int, int) { finished = true; saved = ok; });
    model.save(false);
    require(model.saving() && !model.editable(), "Editing pauses while saving.");
    require(waitFor([&] { return finished; }) && saved, "The model saves into the PDF.");
    require(!model.saving() && model.editable() && model.unsavedCount() == 0, "Everything is Embedded afterwards.");
    require(titles(qtOutline(book.pdf)).last() == QStringLiteral("2 من النموذج"), "The PDF holds the new bookmark.");
}

} // namespace

int main(int argc, char** argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app{argc, argv};
    if (atlas::app::QpdfInspector::locate().isEmpty()) {
        std::cout << "qpdf not found; safe save tests skipped.\n";
        return 0;
    }
    try {
        checkSave();
        checkHidden();
        checkFailures();
        checkProtected();
        checkThroughModel();
    } catch (const std::exception& error) {
        std::cerr << "Safe save test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Safe save: write, verify, backup, replace, commit, hidden, failures and protected files passed.\n";
    return 0;
}
