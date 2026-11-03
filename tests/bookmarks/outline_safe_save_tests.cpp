// N5.4 safe save, end to end (ADR-0009): a copy of an N2 fixture in a real
// profile, edited through the editor and store, saved through qpdf, verified,
// backed up, replaced and committed. Failures injected at every step must
// leave the PDF and the overlay exactly as they were.

#include "app/bookmarks/BookmarkModel.h"
#include "app/bookmarks/BookmarkService.h"
#include "app/bookmarks/OutlineSafeSave.h"
#include "app/bookmarks/SqliteBookmarkOverlay.h"
#include "app/document/DocumentCapabilityCheck.h"
#include "app/document/QpdfOutlineWriter.h"
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
#include <QProcess>
#include <cstdlib>

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

void checkProcessRecovery() {
    for (const QString& phase : {QStringLiteral("journalPrepared"), QStringLiteral("afterReplace"),
                                 QStringLiteral("commitSave:beforeCommit"), QStringLiteral("afterOverlayCommit")}) {
        Book book{"A003_outlines_links.pdf"};
        editA003(book);
        const auto original = sha(book.pdf);
        const auto revision = book.editor->revision();
        book.store.reset();
        QProcess child;
        child.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--crash-save"),
            book.dir.path(), book.pdf, phase});
        const bool finished = child.waitForFinished(60000);
        require(finished && child.exitCode() == 73, "Child exits at " + phase.toStdString() + ": " +
                std::to_string(child.exitCode()) + " " + child.readAllStandardError().toStdString());
        book.store = std::make_unique<SqliteBookmarkOverlay>(book.database);
        const auto result = atlas::app::recoverOutlineSave(*book.store, book.pdf, "doc",
            fsPath(book.dir.filePath(QStringLiteral("backups"))));
        const bool committed = phase == QStringLiteral("afterOverlayCommit");
        require(result == (phase == QStringLiteral("journalPrepared") || committed
                    ? atlas::app::SaveRecovery::completed : atlas::app::SaveRecovery::restored),
                "Restart resolves the exact interrupted boundary.");
        require((sha(book.pdf) == original) != committed, "Restart preserves the correct PDF version.");
        require(book.store->load("doc").revision == revision + (committed ? 1 : 0),
                "Restart keeps PDF and SQLite commit state consistent.");
        require(atlas::app::recoverOutlineSave(*book.store, book.pdf, "doc",
                    fsPath(book.dir.filePath(QStringLiteral("backups")))) == atlas::app::SaveRecovery::none,
                "Recovery is idempotent.");
    }
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
    {
        Book book{"A003_outlines_links.pdf"};
        editA003(book);
        const auto original = sha(book.pdf);
        const auto revision = book.editor->revision();
        book.store->setFaultHookForTesting([](std::string_view step) {
            if (step == "commitSave:beforeCommit") throw std::runtime_error("injected database failure");
        });
        const auto outcome = book.save();
        require(!outcome.ok && !outcome.pdfChanged && sha(book.pdf) == original,
                "A database transaction exception restores the PDF.");
        require(book.store->load("doc").revision == revision, "Failed database transaction rolls back.");
    }
    for (const char* failing : {"reindex", "commitOverlay"}) {
        Book book{"A003_outlines_links.pdf"};
        editA003(book);
        const auto original = sha(book.pdf);
        const auto revision = book.editor->revision();
        const auto outcome = book.save({}, [failing](std::string_view step) { return step != failing; });
        require(!outcome.ok && !outcome.pdfChanged && sha(book.pdf) == original,
                "Late failure restores the original PDF.");
        require(book.store->load("doc").revision == revision, "Late failure preserves overlay revision.");
    }
    {
        Book book{"A003_outlines_links.pdf"};
        editA003(book);
        const auto original = sha(book.pdf);
        const auto outcome = book.save({}, [](std::string_view step) {
            return step != "afterReplace" && step != "restore";
        });
        require(!outcome.ok && outcome.pdfChanged && outcome.reason == QStringLiteral("recoveryRequired"),
                "Failed recovery must never claim the original was restored.");
        require(QFileInfo::exists(book.pdf) && sha(outcome.backupPath) == original,
                "Failed recovery retains both the current PDF and original backup.");
        QFile external{book.pdf};
        require(external.open(QIODevice::Append), "Open external change.");
        external.write("\n% external edit after interrupted save\n");
        external.close();
        const auto changed = sha(book.pdf);
        require(atlas::app::recoverOutlineSave(*book.store, book.pdf, "doc",
                    fsPath(book.dir.filePath(QStringLiteral("backups")))) == atlas::app::SaveRecovery::required,
                "Recovery preserves an unknown external revision.");
        require(book.save().reason == QStringLiteral("recoveryRequired") && sha(book.pdf) == changed,
                "An unresolved journal blocks another save.");
    }
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

// Another program changes the PDF's outline; opening the book reconciles.
void checkReconcileOnOpen() {
    Book book{"A003_outlines_links.pdf"};
    book.run(book.editor->create(std::nullopt, 9, "my own", at(1, 0.2)));
    book.run(book.editor->rename(book.id("1"), "Chapter 2 — mine"));
    {
        // "Another program": rename Chapter 1 and add a bookmark, written with qpdf.
        const QString qpdf = atlas::app::QpdfInspector::locate();
        const auto source = atlas::app::QpdfOutlineWriter::read(qpdf, book.pdf, {});
        BaseSnapshot base = *book.store->load("doc").base;
        BookmarkEditor other{"x", embeddedNodesFromBase("x", base), base, 1};
        std::string chapterOne;
        for (const auto* node : other.inOrder()) if (node->baseKey == std::optional<std::string>{"0"}) chapterOne = node->id;
        require(other.rename(chapterOne, "Chapter 1 (revised)").ok, "rename");
        other.confirm(2);
        require(other.create(std::nullopt, 9, "Appendix", at(2, 0.5)).ok, "create");
        other.confirm(3);
        const auto plan = planOutlineSave(other.inOrder(), base);
        QString reason;
        const auto update = atlas::app::QpdfOutlineWriter::buildUpdate(source, base, plan, &reason);
        require(update.has_value(), "update: " + reason.toStdString());
        const QString changed = book.pdf + QStringLiteral(".other.pdf");
        require(atlas::app::QpdfOutlineWriter::write(qpdf, book.pdf, {}, *update, changed).isEmpty(), "other write");
        QFile::remove(book.pdf);
        require(QFile::rename(changed, book.pdf), "other program replaces the file");
    }
    book.store.reset();
    atlas::app::BookmarkService service{book.database};
    atlas::app::BookmarkModel model;
    model.setService(&service);
    model.attach(book.pdf, qtOutline(book.pdf));
    QElapsedTimer timer;
    timer.start();
    while (model.status() != QStringLiteral("editable") && timer.elapsed() < 30000)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    require(model.status() == QStringLiteral("editable"), "The book opens.");
    const QVariantMap info = model.reconcileInfo();
    require(info.value(QStringLiteral("done")).toBool() && info.value(QStringLiteral("added")).toInt() == 1 &&
                info.value(QStringLiteral("updated")).toInt() == 1 && info.value(QStringLiteral("conflicts")).toInt() == 0,
            "Reconciled: one new, one updated, no conflicts.");
    require(!model.outlineChangedOutside() && model.unsavedCount() == 2,
            "The reader's own bookmark and rename are kept as unsaved local work.");
    model.expandAll();
    QStringList rows;
    for (int row = 0; row < model.rowCount(); ++row)
        rows << model.data(model.index(row), atlas::app::BookmarkModel::TitleRole).toString();
    require(rows.contains(QStringLiteral("Chapter 1 (revised)")) && rows.contains(QStringLiteral("Appendix")) &&
                rows.contains(QStringLiteral("Chapter 2 — mine")) && rows.contains(QStringLiteral("my own")),
            "Both sides' changes are shown: " + rows.join(u" | ").toStdString());
}

void checkCopy() {
    {
        Book book{"A010_certification_structure.pdf"};
        book.run(book.editor->create(std::nullopt, 0, "نسخة العمل", at(0, 0.2)));
        const QByteArray original = sha(book.pdf);
        SafeSaveInput input;
        input.path = book.pdf;
        input.documentId = "doc";
        for (const auto* node : book.editor->inOrder()) input.nodes.push_back(*node);
        input.base = book.store->load("doc").base;
        const QString target = QDir{book.folder}.filePath(QStringLiteral("working copy.pdf"));
        const auto copy = atlas::app::saveOutlineCopy(input, target);
        require(copy.ok, "A certified PDF can be saved as a copy: " + copy.reason.toStdString());
        require(sha(book.pdf) == original, "The certified original is untouched.");
        require(titles(qtOutline(target)).first() == QStringLiteral("0 نسخة العمل"), "The copy holds the bookmarks.");
        require(book.store->load("doc").revision == book.editor->revision(), "The overlay is untouched by a copy.");
        require(atlas::app::saveOutlineCopy(input, book.pdf).reason == QStringLiteral("sameFile"),
                "A copy never overwrites its own source.");
        require(folderFiles(book.folder).size() == 2, "Only the original and the copy are in the folder.");
          const QString alias = QDir{book.folder}.filePath(QStringLiteral("source alias.pdf"));
          std::error_code linkError;
          std::filesystem::create_hard_link(fsPath(book.pdf), fsPath(alias), linkError);
          require(!linkError, "The test filesystem supports a hard-link alias.");
          require(atlas::app::saveOutlineCopy(input, alias).reason == QStringLiteral("sameFile"),
                  "A source hard-link is not a separate copy destination.");
          require(sha(book.pdf) == original && sha(alias) == original, "Source aliases remain unchanged.");
          QByteArray externalHash;
          const auto raced = atlas::app::saveOutlineCopy(input, target, [&](std::string_view step) {
              if (step == "beforeCopyReplace") {
                  QFile external{target};
                  require(external.open(QIODevice::Append), "Open target for an external edit.");
                  external.write("\n% external change\n");
                  external.close();
                  externalHash = sha(target);
              }
              return true;
          });
          require(!raced.ok && raced.reason == QStringLiteral("changedDuringSave") && sha(target) == externalHash,
                  "Save a Copy preserves a destination edited during generation.");
    }
    {
        Book book{"A009_restricted_permissions_rc4_40.pdf", QStringLiteral("atlas-user")};
        book.run(book.editor->create(std::nullopt, 0, "x", at(0, 0.2)));
        SafeSaveInput input;
        input.path = book.pdf;
        input.password = book.password;
        input.documentId = "doc";
        for (const auto* node : book.editor->inOrder()) input.nodes.push_back(*node);
        input.base = book.store->load("doc").base;
        const auto copy = atlas::app::saveOutlineCopy(input, QDir{book.folder}.filePath(QStringLiteral("c.pdf")));
        require(copy.reason == QStringLiteral("notPermitted"), "A PDF whose owner forbids changes gets no changed copy either: " +
                                                                 copy.reason.toStdString());
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
    if (argc == 5 && QString::fromLocal8Bit(argv[1]) == QStringLiteral("--crash-save")) {
        const auto args = app.arguments();
        const QDir root{args.at(2)};
        SqliteBookmarkOverlay store{fsPath(root.filePath(QStringLiteral("library.sqlite3")))};
        const auto snapshot = store.load("doc");
        SafeSaveInput input;
        input.path = args.at(3);
        input.documentId = "doc";
        input.overlayRevision = snapshot.revision;
        input.base = snapshot.base;
        BookmarkEditor editor{"doc", snapshot.nodes, snapshot.base, snapshot.revision};
        for (const auto* node : editor.inOrder()) input.nodes.push_back(*node);
        input.backupRoot = fsPath(root.filePath(QStringLiteral("backups")));
        const auto phase = args.at(4).toStdString();
        store.setFaultHookForTesting([phase](std::string_view step) { if (step == phase) std::_Exit(73); });
        const auto outcome = atlas::app::saveOutlineSafely(store, input, [phase](std::string_view step) {
            if (step == phase) std::_Exit(73);
            return true;
        });
        std::cerr << outcome.reason.toStdString();
        return 74;
    }
    if (atlas::app::QpdfInspector::locate().isEmpty()) {
        std::cout << "qpdf not found; safe save tests skipped.\n";
        return 0;
    }
    try {
        checkSave();
        checkProcessRecovery();
        checkHidden();
        checkFailures();
        checkProtected();
        checkCopy();
        checkReconcileOnOpen();
        checkThroughModel();
    } catch (const std::exception& error) {
        std::cerr << "Safe save test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Safe save: write, verify, backup, replace, commit, hidden, failures and protected files passed.\n";
    return 0;
}
