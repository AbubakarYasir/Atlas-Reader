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
#include <QLockFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QPdfDocument>
#include <QTemporaryDir>
#include <QProcess>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif
#include <cstdlib>

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

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
    QFile source{path};
    require(source.open(QIODevice::ReadOnly), "read outline source");
    return atlas::app::readOutline(document, source.readAll(), password);
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
                                 QStringLiteral("commitSave:beforeCommit"), QStringLiteral("afterOverlayCommit"),
                                 QStringLiteral("restore"), QStringLiteral("afterRestore")}) {
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
        require(result == (phase == QStringLiteral("journalPrepared") || phase == QStringLiteral("afterRestore") || committed
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
        const auto outcome = book.save({}, [&](std::string_view step) {
            if (step != "replace") return true;
            QFile external{book.pdf};
            require(external.open(QIODevice::Append), "Simulate a changed destination on failed replacement.");
            external.write("\n% changed at replacement\n");
            return false;
        });
        require(outcome.disposition == atlas::app::SaveDisposition::recoveryRequired && outcome.pdfChanged &&
                    sha(outcome.backupPath) == original,
                "A failed replacement cannot claim unchanged when the file differs; retain recovery.");
    }
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
        const auto backupRoot = fsPath(book.dir.filePath(QStringLiteral("backups")));
        const auto overlayRevision = book.store->load("doc").revision;
        require(!atlas::app::keepCurrentAfterRecovery(book.pdf, {}, "doc", backupRoot, [](std::string_view) { return false; }),
                "A failure after archiving preserves the recovery hold.");
        require(atlas::app::recoverOutlineSave(*book.store, book.pdf, "doc", backupRoot) == atlas::app::SaveRecovery::required,
                "An incomplete resolution does not permit another save.");
        require(atlas::app::keepCurrentAfterRecovery(book.pdf, {}, "doc", backupRoot),
                "Explicit keep-current archives both versions and resolves the hold.");
        require(sha(book.pdf) == changed && sha(outcome.backupPath) == original &&
                book.store->load("doc").revision == overlayRevision,
                "Keeping current never changes the PDF, old backup or local research.");
        const QDir recoveryFolder{book.dir.filePath(QStringLiteral("backups/doc"))};
        const auto archives = recoveryFolder.entryList({QStringLiteral("resolved-*")}, QDir::Dirs);
        require(archives.size() == 2, "Failed and completed resolution evidence is retained.");
        for (const auto& archiveName : archives) {
            const QDir archive{recoveryFolder.filePath(archiveName)};
            require(sha(archive.filePath(QStringLiteral("before-save.pdf"))) == original &&
                    sha(archive.filePath(QStringLiteral("kept-current.pdf"))) == changed,
                    "The resolution archive contains verified copies of both revisions.");
        }
        require(atlas::app::recoverOutlineSave(*book.store, book.pdf, "doc", backupRoot) == atlas::app::SaveRecovery::none,
                "The next open can reconcile the chosen file.");
    }
    // The reader's way out of an unresolved journal: restore the original or keep
    // the current file. Nothing is deleted either way.
    for (const auto choice : {atlas::app::RecoveryChoice::restoreOriginal, atlas::app::RecoveryChoice::keepCurrent}) {
        Book book{"A003_outlines_links.pdf"};
        editA003(book);
        const auto original = sha(book.pdf);
        const auto revision = book.editor->revision();
        const auto outcome = book.save({}, [](std::string_view step) { return step != "afterReplace" && step != "restore"; });
        require(outcome.reason == QStringLiteral("recoveryRequired"), "set up an unresolved save");
        QFile external{book.pdf};
        require(external.open(QIODevice::Append), "Open external change.");
        external.write("\n% external edit after interrupted save\n");
        external.close();
        const auto changed = sha(book.pdf);
        const auto backups = fsPath(book.dir.filePath(QStringLiteral("backups")));
        const QString folder = book.dir.filePath(QStringLiteral("backups/doc"));
        {
            QLockFile busy{QDir{folder}.filePath(QStringLiteral("save.lock"))};
            require(busy.tryLock(0), "hold the save lock");
            require(atlas::app::resolveSaveRecovery(*book.store, book.pdf, "doc", backups, choice) == QStringLiteral("busy"),
                    "Recovery waits for a running save.");
        }
        const QString result = atlas::app::resolveSaveRecovery(*book.store, book.pdf, "doc", backups, choice);
        const QStringList resolved = choice == atlas::app::RecoveryChoice::keepCurrent
            ? QDir{folder}.entryList({QStringLiteral("resolved-*")}, QDir::Dirs)
            : QDir{folder}.entryList({QStringLiteral("resolved-*.json")}, QDir::Files);
        require(!QFileInfo::exists(QDir{folder}.filePath(QStringLiteral("pending-save.json"))) && resolved.size() == 1,
                "The journal is kept as a resolved record, not deleted.");
        require(book.store->load("doc").revision == revision, "Recovery never changes the reader's bookmarks.");
        if (choice == atlas::app::RecoveryChoice::restoreOriginal) {
            require(result == QStringLiteral("restored") && sha(book.pdf) == original, "The original is put back.");
            bool keptChanged = false;
            for (const auto& name : QDir{folder}.entryList({QStringLiteral("restored-*")}, QDir::Dirs))
                keptChanged = keptChanged || sha(QDir{QDir{folder}.filePath(name)}.filePath(QStringLiteral("displaced-current.pdf"))) == changed;
            require(keptChanged, "The file it replaced is kept among the backups.");
        } else {
            require(result == QStringLiteral("kept") && sha(book.pdf) == changed, "The current file is kept as it is.");
        }
        require(atlas::app::recoverOutlineSave(*book.store, book.pdf, "doc", backups) == atlas::app::SaveRecovery::none,
                "Nothing is pending afterwards.");
        require(atlas::app::resolveSaveRecovery(*book.store, book.pdf, "doc", backups, choice) == QStringLiteral("noJournal"),
                "A second resolution finds nothing to do.");
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
    {   // Too little free space: refused before anything is written.
        Book book{"A003_outlines_links.pdf"};
        editA003(book);
        const QByteArray original = sha(book.pdf);
        const auto revision = book.editor->revision();
        const auto outcome = book.save({}, [](std::string_view step) { return step != "space"; });
        require(outcome.reason == QStringLiteral("diskFull") && !outcome.pdfChanged && sha(book.pdf) == original &&
                    book.store->load("doc").revision == revision && folderFiles(book.folder).size() == 1,
                "No space: the PDF, overlay and folder are untouched.");
    }
#ifdef Q_OS_WIN
    {   // Another program holds the PDF open without sharing write or delete
        // (as an editor does): the save fails cleanly and nothing is lost.
        Book book{"A003_outlines_links.pdf"};
        editA003(book);
        const QByteArray original = sha(book.pdf);
        const auto revision = book.editor->revision();
        const std::wstring native = QDir::toNativeSeparators(book.pdf).toStdWString();
        HANDLE holder = CreateFileW(native.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL, nullptr);
        require(holder != INVALID_HANDLE_VALUE, "hold the PDF open");
        const auto outcome = book.save();
        CloseHandle(holder);
        require(!outcome.ok && !outcome.pdfChanged && sha(book.pdf) == original,
                "A PDF locked by another program is left exactly as it was: " + outcome.reason.toStdString());
        require(book.store->load("doc").revision == revision, "The overlay keeps the reader's work.");
        require(folderFiles(book.folder).size() == 1, "No temporary file is left beside a locked PDF.");
        require(book.save().ok, "Once the other program lets go, the save goes through.");
    }
#endif
}

void checkProtected() {
#ifdef Q_OS_WIN
    {
        Book book{"A003_outlines_links.pdf"};
        editA003(book);
        const auto original = sha(book.pdf);
        HANDLE held = INVALID_HANDLE_VALUE;
        const auto outcome = book.save({}, [&](std::string_view step) {
            if (step == "afterReplace") {
                held = CreateFileW(reinterpret_cast<LPCWSTR>(book.pdf.utf16()), GENERIC_READ,
                                   FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr);
            }
            return true; // no injected failure: the actual Windows lock denies reopen
        });
        const bool locked = held != INVALID_HANDLE_VALUE;
        if (locked) CloseHandle(held);
        require(locked && outcome.disposition == atlas::app::SaveDisposition::recoveryRequired,
                "A real sharing lock prevents final reopen and retains recovery evidence.");
        require(atlas::app::recoverOutlineSave(*book.store, book.pdf, "doc",
                    fsPath(book.dir.filePath(QStringLiteral("backups")))) == atlas::app::SaveRecovery::restored &&
                    sha(book.pdf) == original,
                "After releasing the real sharing lock, restart restores the verified original.");
    }
#endif
    {
        Book book{"A018_self_signed.pdf"};
        book.run(book.editor->create(std::nullopt, 0, "signed original stays intact", at(0, 0.2)));
        const auto original = sha(book.pdf);
        require(book.save().reason == QStringLiteral("notPermitted") && sha(book.pdf) == original,
                "A cryptographically signed original is never changed in place.");
        SafeSaveInput input;
        input.path = book.pdf;
        input.documentId = "doc";
        input.base = book.store->load("doc").base;
        for (const auto* node : book.editor->inOrder()) input.nodes.push_back(*node);
        const QString target = book.dir.filePath(QStringLiteral("unsigned-working-copy.pdf"));
        const auto copy = atlas::app::saveOutlineCopy(input, target);
        require(copy.ok && sha(book.pdf) == original && !qtOutline(target).isEmpty(),
                "A signed file permits a working copy, with no signature-validity claim: " + copy.reason.toStdString());
    }
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

// docs/QUALITY.md N5 corpus: every writable fixture takes a deep Arabic/English
// tree with duplicate names under different parents, saves, verifies (qpdf and Qt
// PDF inside the pipeline), reopens without a reconcile and has nothing left to
// save. Protected originals are never written; a malformed outline never
// leads to a write that is not verified.
void growTree(Book& book, std::uint32_t page) {
    std::optional<std::string> parent;
    for (int depth = 0; depth < 12; ++depth) {
        const std::string title = depth % 2 == 0 ? "Chapter — الفصل" : "الفصل · Chapter";
        auto main = book.editor->create(parent, 0, title, at(page, 0.1 + 0.05 * (depth % 6)));
        const auto id = main.nodeId;
        book.run(std::move(main));
        book.run(book.editor->create(parent, 1, title, at(page, 0.5))); // same name, same parent level
        parent = id;
    }
}

void checkCorpus() {
    struct Case final { const char* file; const char* password; std::uint32_t page; };
    const Case writable[] = {
        {"A001_simple_text.pdf", "", 0},             // no outline
        {"A002_geometry_labels.pdf", "", 2},         // page labels, mixed sizes, crop box
        {"A003_outlines_links.pdf", "", 1},          // outline and links
        {"A004_outlines_only.pdf", "", 0},           // outline without links
        {"A005_links_only.pdf", "", 0},              // links, no outline
        {"A006_unicode_semantics.pdf", "", 0},       // Arabic/Urdu outline (generated in CI)
        {"A015_encrypted_outline_rc4_40.pdf", "atlas-user", 0},
    };
    int saved = 0;
    for (const auto& entry : writable) {
        if (!QFileInfo::exists(fixture(entry.file))) {
            if (qEnvironmentVariableIsSet("CI")) throw std::runtime_error(std::string{"corpus fixture missing: "} + entry.file);
            std::cout << "Corpus: " << entry.file << " not generated here; skipped (required in CI).\n";
            continue;
        }
        Book book{entry.file, QString::fromLatin1(entry.password)};
        const auto before = qtOutline(book.pdf, book.password).size();
        growTree(book, entry.page);
        const auto outcome = book.save();
        require(outcome.ok, std::string{"Corpus save: "} + entry.file + " " + outcome.reason.toStdString());
        require(qtOutline(book.pdf, book.password).size() == before + 24, std::string{"All 24 new bookmarks: "} + entry.file);
        book.open();
        require(book.lastAdoption != BaseAdoption::needsReconcile, std::string{"Reopens without reconcile: "} + entry.file);
        require(book.save().reason == QStringLiteral("nothingToSave"), std::string{"Nothing left to save: "} + entry.file);
        ++saved;
    }
    require(saved >= 6, "the corpus ran");

    {   // A017: a real approval signature. Never written in place; a copy works and the original stays signed.
        Book book{"A017_signed_approval.pdf"};
        growTree(book, 1);
        const QByteArray original = sha(book.pdf);
        require(book.save().reason == QStringLiteral("notPermitted") && sha(book.pdf) == original,
                "A signed PDF is never written in place.");
        SafeSaveInput input;
        input.path = book.pdf;
        input.documentId = "doc";
        input.overlayRevision = book.editor->revision();
        for (const auto* node : book.editor->inOrder()) input.nodes.push_back(*node);
        input.base = book.store->load("doc").base;
        const QString copy = book.dir.filePath(QStringLiteral("signed copy.pdf"));
        const auto outcome = atlas::app::saveOutlineCopy(input, copy);
        require(outcome.ok && qtOutline(copy).size() == 2 + 24, "Save a Copy of a signed PDF: " + outcome.reason.toStdString());
        require(sha(book.pdf) == original, "The signed original is byte-for-byte unchanged.");
    }

    // A016: readable pages, malformed outline (missing page, wrong type, a cycle).
    // N5-09 regression: raw-link preflight rejects the cycle before Qt's eager
    // bookmark model is created. Always runs in the normal suite.
    {
        QTemporaryDir dir;
        const QString pdf = dir.filePath(QStringLiteral("malformed.pdf"));
        require(QFile::copy(fixture("A016_malformed_outline.pdf"), pdf), "copy A016");
        const QByteArray original = sha(pdf);
        QPdfDocument document;
        document.load(pdf);
        require(document.status() == QPdfDocument::Status::Ready && document.pageCount() == 3, "A016 pages are readable");
        QFile source{pdf};
        require(source.open(QIODevice::ReadOnly), "read malformed source");
        const auto outline = atlas::app::readOutline(document, source.readAll());
        document.close();
        const auto base = atlas::app::baseFromOutline(outline, atlas::app::fileRevision(pdf));
        std::cout << "A016: Qt PDF reads " << outline.size() << " outline entries; base "
                  << (base ? "accepted" : "refused (bookmarks read-only)") << ".\n";
        require(!base, "Cyclic outline is rejected before Qt builds its model.");
        if (base) {
            Book book{"A016_malformed_outline.pdf"};
            book.run(book.editor->create(std::nullopt, 0, "Atlas note", at(1, 0.3)));
            const auto outcome = book.save();
            if (outcome.ok) {
                require(qtOutline(book.pdf).size() >= 2, "A saved malformed outline reads back.");
            } else {
                require(sha(book.pdf) == original && !outcome.pdfChanged,
                        "A refused save leaves the malformed PDF exactly as it was: " + outcome.reason.toStdString());
            }
            std::cout << "A016 save: " << (outcome.ok ? "saved and verified" : outcome.reason.toStdString()) << ".\n";
        }
        require(sha(pdf) == original, "Reading a malformed outline never changes the file.");
    }
}

// N4 carried-forward regression: raw Qt link destinations omit inherent rotation.
void checkRotatedNavigation() {
    QFile source{fixture("A012_xyz_destinations.pdf")};
    require(source.open(QIODevice::ReadOnly), "A012 navigation fixture");
    atlas::app::DocumentTextWorker worker{std::make_shared<std::atomic<quint64>>(0)};
    QVariantList links;
    QObject::connect(&worker, &atlas::app::DocumentTextWorker::pageLinks,
                     [&](quint64, int, const QVariantList& value) { links = value; });
    worker.open(source.readAll(), {}, 1);
    worker.links(0);
    require(links.size() == 2, "Both A012 links available");
    for (const auto& value : links) {
        const auto link = value.toMap();
        const auto point = link.value("location").toPointF();
        const auto expected = link.value("page").toInt() == 1 ? QPointF{40, 50} : QPointF{250, 40};
        require((point - expected).manhattanLength() < 1.0, "Link destination includes inherent page rotation");
    }
    QTemporaryDir variants;
    for (const int angle : {0, 180, 270}) {
        const QString output = variants.filePath(QStringLiteral("rotation-%1.pdf").arg(angle));
        QProcess rotate;
        rotate.start(atlas::app::QpdfInspector::locate(),
                     {QStringLiteral("--rotate=%1:3").arg(angle), QStringLiteral("--"),
                      fixture("A012_xyz_destinations.pdf"), output});
        require(rotate.waitForFinished(10000) && rotate.exitCode() == 0, "Generate rotation variant");
        QFile variant{output};
        require(variant.open(QIODevice::ReadOnly), "Read rotation variant");
        worker.open(variant.readAll(), {}, 2);
        links.clear(); worker.links(0);
        bool found = false;
        for (const auto& value : links) {
            const auto link = value.toMap();
            if (link.value("page").toInt() != 2) continue;
            const auto expected = angle == 0 ? QPointF{40, 50} : angle == 180 ? QPointF{160, 250} : QPointF{50, 160};
            require((link.value("location").toPointF() - expected).manhattanLength() < 1.0,
                    "Destination normalized for every quarter-turn");
            found = true;
        }
        require(found, "Rotated destination link present");
    }
    Book book{"A012_xyz_destinations.pdf"};
    book.run(book.editor->create(std::nullopt, 0, "Rotated target", at(2, 0.3)));
    const auto saved = book.save();
    require(saved.ok, "Rotated bookmark saves and reopens at the same normalized point: " + saved.reason.toStdString());
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
            if ((phase == "restore" || phase == "afterRestore") && step == "afterReplace") return false;
            return true;
        });
        std::cerr << outcome.reason.toStdString();
        return 74;
    }
    if (atlas::app::QpdfInspector::locate().isEmpty()) {
        std::cerr << "Required qpdf runtime not found; safe-save tests cannot pass.\n";
        return 1;
    }
    try {
        checkRotatedNavigation();
        checkSave();
        checkProcessRecovery();
        checkHidden();
        checkFailures();
        checkProtected();
        checkCopy();
        checkCorpus();
        checkReconcileOnOpen();
        checkThroughModel();
    } catch (const std::exception& error) {
        std::cerr << "Safe save test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Safe save: write, verify, backup, replace, commit, hidden, failures and protected files passed.\n";
    return 0;
}
