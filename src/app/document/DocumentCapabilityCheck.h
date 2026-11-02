#pragma once

// N5.1 adapters for the capability check (core/document/CapabilityPolicy):
// - qpdf (first-party CLI behind a process boundary, ADR-0004) for PDF
//   security: encryption, permissions, password match, signatures, certification;
// - the file system for read-only files and folders, sharing locks and
//   cloud placeholders;
// - a revision stamp (size + modification time) to detect external changes.
// Everything here only reads; nothing writes to or next to the PDF.

#include "core/document/CapabilityPolicy.h"

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QVariantMap>

namespace atlas::app {

struct RevisionStamp final {
    qint64 size{-1};
    QDateTime modified;
    [[nodiscard]] bool valid() const { return size >= 0; }
    friend bool operator==(const RevisionStamp&, const RevisionStamp&) = default;
};

class QpdfInspector final {
public:
    // The bundled qpdf (app folder/qpdf/qpdf.exe), ATLAS_QPDF, or qpdf on PATH.
    [[nodiscard]] static QString locate();
    // Runs qpdf; the password goes through standard input, never the command line.
    [[nodiscard]] static document::PdfFacts inspect(const QString& qpdf, const QString& path, const QString& password);

    // Pure parsing, tested without qpdf.
    // encryptAcroform: `qpdf --json=2 --json-key=encrypt --json-key=acroform` output.
    // rootObject: `qpdf --json=2 --json-key=qpdf --json-object=<root>` output (may be empty).
    [[nodiscard]] static document::PdfFacts parse(const QByteArray& encryptAcroform, const QByteArray& rootObject,
                                                  int exitCode, const QByteArray& errorOutput);
    // "1 0 R" from `qpdf --json=2 --json-key=qpdf --json-object=trailer`.
    [[nodiscard]] static QString rootReference(const QByteArray& trailerJson);
};

class FileFactsProbe final {
public:
    [[nodiscard]] static document::FileFacts probe(const QString& path);
    [[nodiscard]] static RevisionStamp stamp(const QString& path);
};

struct CapabilityCheckResult final {
    document::CapabilityAssessment assessment;
    RevisionStamp stamp;
};

// Runs every probe for one document (call off the UI thread).
[[nodiscard]] CapabilityCheckResult checkDocumentCapabilities(const QString& path, const QString& password,
                                                              const RevisionStamp& baseline);

// For QML: {state, canEmbedOutline, canEmbedAnnotations, canKeepLocalResearch, reasons: [names]}.
[[nodiscard]] QVariantMap capabilityMap(const document::CapabilityAssessment& assessment);

} // namespace atlas::app
