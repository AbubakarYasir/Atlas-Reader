// N5.4 safe save: rewriting a PDF outline with qpdf (ADR-0009). Uses the N2
// fixtures and the qpdf found like the app finds it (ATLAS_QPDF, bundled, PATH).
// Every write goes to a temporary folder; fixtures are never changed.

#include "app/document/DocumentCapabilityCheck.h"
#include "app/document/QpdfOutlineWriter.h"
#include "core/bookmarks/BookmarkEditor.h"
#include "core/bookmarks/LocationEnvelope.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QGuiApplication>
#include <QPdfBookmarkModel>
#include <QPdfDocument>
#include <QTemporaryDir>

#include <cmath>
#include <functional>
#include <unordered_map>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace atlas::bookmarks;
using atlas::app::PdfSourceOutline;
using atlas::app::QpdfOutlineWriter;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

QString fixture(const char* name) { return QStringLiteral(ATLAS_FIXTURE_DIR "/") + QString::fromLatin1(name); }

QByteArray sha(const QString& path) {
    QFile file{path};
    require(file.open(QIODevice::ReadOnly), "Cannot read " + path.toStdString());
    return QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256);
}

std::string at(std::uint32_t page, double left, double top) {
    LocationEnvelope envelope;
    envelope.pdf.pageIndex = page;
    envelope.pdf.kind = DestinationKind::xyz;
    envelope.pdf.left = left;
    envelope.pdf.top = top;
    return encodeLocation(envelope);
}

// The base snapshot Atlas would have read from this PDF (titles and tree).
BaseSnapshot baseOf(const PdfSourceOutline& source) {
    BaseSnapshot base;
    base.sourceRevision = "test";
    std::unordered_map<std::string, std::uint32_t> positions;
    for (const auto& item : source.items) {
        BaseNode node;
        node.key = item.key.toStdString();
        if (!item.parentKey.isEmpty()) node.parentKey = item.parentKey.toStdString();
        node.position = positions[item.parentKey.toStdString()]++;
        node.title = item.title.toStdString();
        if (item.destPage >= 0) {
            LocationEnvelope envelope;
            envelope.pdf.pageIndex = static_cast<std::uint32_t>(item.destPage);
            envelope.pdf.kind = DestinationKind::fit;
            node.location = encodeLocation(envelope);
        }
        base.nodes.push_back(node);
    }
    return base;
}

struct Saved final {
    PdfSourceOutline before;
    PdfSourceOutline after;
    SavePlan plan;
    QString verdict;
};

Saved save(const QString& qpdf, const QString& source, const QString& output, const QString& password,
           const std::function<void(BookmarkEditor&, const BaseSnapshot&)>& edit, SaveOptions options = {}) {
    Saved saved;
    saved.before = QpdfOutlineWriter::read(qpdf, source, password);
    require(saved.before.ok, "qpdf reads the source: " + saved.before.error.toStdString());
    const BaseSnapshot base = baseOf(saved.before);
    BookmarkEditor editor{"doc", embeddedNodesFromBase("doc", base), base, 1};
    edit(editor, base);
    saved.plan = planOutlineSave(editor.inOrder(), base, options);
    require(saved.plan.refusal.empty(), "Plan refused: " + saved.plan.refusal);
    QString reason;
    const auto update = QpdfOutlineWriter::buildUpdate(saved.before, base, saved.plan, &reason);
    require(update.has_value(), "Update refused: " + reason.toStdString());
    QString detail;
    const QString written = QpdfOutlineWriter::write(qpdf, source, password, *update, output, &detail);
    require(written.isEmpty(), "qpdf write failed: " + written.toStdString() + " — " + detail.toStdString());
    saved.after = QpdfOutlineWriter::read(qpdf, output, password);
    saved.verdict = QpdfOutlineWriter::verify(saved.before, saved.after, saved.plan);
    return saved;
}

void run(BookmarkEditor& editor, EditResult result) {
    require(result.ok, "command refused: " + result.reason);
    editor.confirm(editor.revision() + 1);
}

std::string idOf(BookmarkEditor& editor, std::string_view key) {
    for (const auto* node : editor.inOrder()) {
        if (node->baseKey && *node->baseKey == key) return node->id;
    }
    throw std::runtime_error("no node for key");
}

// What Qt PDF (the engine Atlas reads with) sees: "title@page" depth first.
QStringList qtOutline(const QString& path, const QString& password = {}) {
    QPdfDocument document;
    if (!password.isEmpty()) document.setPassword(password);
    document.load(path);
    require(document.status() == QPdfDocument::Status::Ready, "Qt PDF opens " + path.toStdString());
    QPdfBookmarkModel model;
    model.setDocument(&document);
    QStringList out;
    std::function<void(const QModelIndex&, int)> walk = [&](const QModelIndex& parent, int depth) {
        for (int row = 0; row < model.rowCount(parent); ++row) {
            const QModelIndex index = model.index(row, 0, parent);
            out << QString(depth * 2, u' ') + model.data(index, static_cast<int>(QPdfBookmarkModel::Role::Title)).toString() +
                       u'@' + QString::number(model.data(index, static_cast<int>(QPdfBookmarkModel::Role::Page)).toInt());
            walk(index, depth + 1);
        }
    };
    walk({}, 0);
    return out;
}

void checkEditsRoundTrip(const QString& qpdf, const QTemporaryDir& dir) {
    const QString source = fixture("A003_outlines_links.pdf");
    const QByteArray original = sha(source);
    const QString output = dir.filePath(QStringLiteral("a003-saved.pdf"));
    const auto saved = save(qpdf, source, output, {}, [](BookmarkEditor& editor, const BaseSnapshot&) {
        run(editor, editor.rename(idOf(editor, "1"), "الفصل الثاني — مراجَع"));
        const auto folder = editor.create(std::nullopt, 0, "Atlas: ملاحظات", "");
        run(editor, folder);
        run(editor, editor.create(folder.nodeId, 0, "Note on page 2", at(1, 0.1, 0.25)));
        run(editor, editor.move(idOf(editor, "0/0"), std::nullopt, 9)); // Section 1.1 to the end, top level
    });
    require(saved.verdict.isEmpty(), "Verification passed: " + saved.verdict.toStdString());
    require(saved.plan.added == 2 && saved.plan.changed >= 2, "Plan counts.");
    require(sha(source) == original, "The source is never changed.");
    const QStringList qt = qtOutline(output);
    const QStringList expected{QStringLiteral("Atlas: ملاحظات@-1"), QStringLiteral("  Note on page 2@1"),
                               QStringLiteral("Chapter 1@0"), QStringLiteral("الفصل الثاني — مراجَع@2"),
                               QStringLiteral("Section 1.1@1")};
    require(qt == expected, "Qt PDF reads the written outline: " + qt.join(u" | ").toStdString());
}

void checkGeometry(const QString& qpdf, const QTemporaryDir& dir) {
    // A002: page 2 is rotated 90°, page 3 has a crop box inside its media box.
    const QString output = dir.filePath(QStringLiteral("a002-saved.pdf"));
    const auto saved = save(qpdf, fixture("A002_geometry_labels.pdf"), output, {},
                            [](BookmarkEditor& editor, const BaseSnapshot&) {
                                run(editor, editor.create(std::nullopt, 0, "upright", at(0, 0.25, 0.1)));
                                run(editor, editor.create(std::nullopt, 1, "rotated", at(1, 0.25, 0.1)));
                                run(editor, editor.create(std::nullopt, 2, "cropped", at(2, 0.0, 0.0)));
                            });
    require(saved.verdict.isEmpty(), "Verification passed: " + saved.verdict.toStdString());
    const auto dest = [&](int i) { return saved.after.items[i].dict.value(QStringLiteral("/Dest")).toArray(); };
    require(std::abs(dest(0).at(2).toDouble() - 153) < 0.01 && std::abs(dest(0).at(3).toDouble() - 712.8) < 0.01,
            "Upright page: left/top from the top-left corner.");
    // Rotated 90°: displayed (0.25, 0.1) → user x = 0.1·W, y = 0.25·H.
    require(std::abs(dest(1).at(2).toDouble() - 59.528) < 0.01 && std::abs(dest(1).at(3).toDouble() - 210.472) < 0.01,
            "Rotated page converted through /Rotate.");
    require(dest(2).at(2).toDouble() == 20 && dest(2).at(3).toDouble() == 580, "Cropped page: the crop box's corner.");
    require(qtOutline(output).size() == 3, "Qt PDF reads three bookmarks.");
}

void checkEncrypted(const QString& qpdf, const QTemporaryDir& dir) {
    const QString output = dir.filePath(QStringLiteral("a015-saved.pdf"));
    const auto saved = save(qpdf, fixture("A015_encrypted_outline_rc4_40.pdf"), output, QStringLiteral("atlas-user"),
                            [](BookmarkEditor& editor, const BaseSnapshot&) {
                                run(editor, editor.rename(idOf(editor, "0"), "جذر مشفّر"));
                            });
    require(saved.verdict.isEmpty(), "Encrypted save verified (encryption kept): " + saved.verdict.toStdString());
    require(saved.after.preservation.value(QStringLiteral("encrypt")).toObject().contains(QStringLiteral("/Filter")),
            "The written file is still encrypted.");
    require(qtOutline(output, QStringLiteral("atlas-user")) == QStringList{QStringLiteral("جذر مشفّر@0")},
            "Qt PDF opens it with the same password and reads the new title.");
    const PdfSourceOutline owner = QpdfOutlineWriter::read(qpdf, output, QStringLiteral("atlas-owner"));
    require(owner.ok, "The owner password still works.");
}

void checkRefusals(const QString& qpdf, const QTemporaryDir& dir) {
    const auto source = QpdfOutlineWriter::read(qpdf, fixture("A003_outlines_links.pdf"), {});
    BaseSnapshot base = baseOf(source);
    BookmarkEditor editor{"doc", embeddedNodesFromBase("doc", base), base, 1};
    const auto plan = planOutlineSave(editor.inOrder(), base);
    base.nodes[0].title = "Not what the PDF says";
    QString reason;
    require(!QpdfOutlineWriter::buildUpdate(source, base, plan, &reason) && reason == QStringLiteral("outlineMismatch"),
            "A base that does not match the PDF is refused.");
    // A different file "written" in place of the save fails verification.
    const auto other = QpdfOutlineWriter::read(qpdf, fixture("A004_outlines_only.pdf"), {});
    require(!QpdfOutlineWriter::verify(source, other, plan).isEmpty(), "A wrong output never verifies.");
    // The page tree tampered with: preservation catches it.
    PdfSourceOutline tampered = source;
    QJsonObject preservation = tampered.preservation;
    preservation.insert(QStringLiteral("info"), QJsonObject{{QStringLiteral("/Title"), QStringLiteral("u:x")}});
    tampered.preservation = preservation;
    require(QpdfOutlineWriter::verify(source, tampered, plan) == QStringLiteral("preservation"),
            "A change outside the outline fails preservation.");
    (void)dir;
}

} // namespace

int main(int argc, char** argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app{argc, argv};
    const QString qpdf = atlas::app::QpdfInspector::locate();
    if (qpdf.isEmpty()) {
        std::cout << "qpdf not found; outline writer tests skipped.\n";
        return 0;
    }
    try {
        QTemporaryDir dir;
        require(dir.isValid(), "A temporary folder is required.");
        checkEditsRoundTrip(qpdf, dir);
        checkGeometry(qpdf, dir);
        checkEncrypted(qpdf, dir);
        checkRefusals(qpdf, dir);
    } catch (const std::exception& error) {
        std::cerr << "Outline writer test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "qpdf outline writer: edits, geometry, encryption and refusals passed.\n";
    return 0;
}
