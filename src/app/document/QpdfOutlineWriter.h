#pragma once

// N5.4 safe save: reading and rewriting a PDF's outline with the qpdf CLI
// (ADR-0004, ADR-0009). Reading uses `qpdf --json=2`; writing builds an
// `--update-from-json` document with a fresh set of outline items (existing
// items keep every key but the tree links), so nothing else in the file is
// touched. Verification compares the written file with the source and the
// plan through qpdf's own reading of the new file.
//
// Nothing here replaces the source: it writes only to the output path given.
// The safe-save pipeline (temporary file, backup, replace, reopen) is
// OutlineSafeSave.

#include "core/bookmarks/OutlineSavePlan.h"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include <optional>

namespace atlas::app {

struct PdfOutlineItem final {
    QString key;        // "0", "0/3", … (same scheme as the base snapshot)
    QString parentKey;  // empty at the top level
    QString title;      // as qpdf decodes it
    QString objectRef;  // "12 0 R" in this file
    QJsonObject dict;   // the item's dictionary, qpdf JSON v2 form
    int destPage{-1};   // 0-based, -1 when none or unresolved
    bool open{false};
};

struct PdfSourceOutline final {
    bool ok{false};
    QString error;                         // stable reason when !ok
    QVector<PdfOutlineItem> items;         // depth first
    QStringList pageRefs;                  // page index → "N 0 R"
    QVector<bookmarks::PageGeometry> pages;
    int maxObjectId{};
    QString rootRef;
    QJsonObject rootDict;
    QJsonObject outlinesDict;              // the old /Outlines dictionary (other keys are kept)
    QJsonObject preservation;              // everything a save must not change (refs normalized)
};

class QpdfOutlineWriter final {
public:
    // Runs `qpdf --json=2` on the file (password through standard input).
    [[nodiscard]] static PdfSourceOutline read(const QString& qpdf, const QString& path, const QString& password);
    // Pure: parses that output. Tested without qpdf.
    [[nodiscard]] static PdfSourceOutline parse(const QByteArray& json);

    // Pure: the `--update-from-json` document for this plan. Every base item in
    // the plan must match the source item at the same place with the same
    // title; otherwise std::nullopt with reason "outlineMismatch".
    [[nodiscard]] static std::optional<QByteArray> buildUpdate(const PdfSourceOutline& source,
                                                              const bookmarks::BaseSnapshot& base,
                                                              const bookmarks::SavePlan& plan, QString* reason);

    // Runs `qpdf --update-from-json`; then `qpdf --check` on the output.
    // Returns an empty string on success, else a stable reason; `detail` gets
    // qpdf's own message for logs and tests (never shown as is).
    [[nodiscard]] static QString write(const QString& qpdf, const QString& source, const QString& password,
                                       const QByteArray& update, const QString& output, QString* detail = nullptr);

    // Pure: the written file (`after`) holds exactly the plan's outline and
    // everything else equals `before`. Empty on success, else a reason:
    // "preservation" or "outlineDiffers".
    [[nodiscard]] static QString verify(const PdfSourceOutline& before, const PdfSourceOutline& after,
                                        const bookmarks::SavePlan& plan);
};

} // namespace atlas::app
