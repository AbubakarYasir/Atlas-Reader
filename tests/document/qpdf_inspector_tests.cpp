// N5.1: qpdf security facts and the full capability check on the N2 fixtures.
// Parsing is tested on recorded qpdf JSON; with qpdf available (bundled,
// ATLAS_QPDF or PATH) the real fixtures are inspected too. Source files must
// never change.
#include "app/document/DocumentCapabilityCheck.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include <iostream>
#include <stdexcept>
#include <string>

using namespace atlas;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

QByteArray sha256(const QString& path) {
    QFile file{path};
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256);
}

const char* kRestricted = R"({"version":2,"parameters":{},"acroform":{"fields":[],"hasacroform":false},
 "encrypt":{"capabilities":{"accessibility":false,"extract":false,"modify":false,"modifyannotations":true,
 "modifyassembly":false,"modifyforms":false,"modifyother":false,"printhigh":false,"printlow":true},
 "encrypted":true,"ownerpasswordmatched":false,"userpasswordmatched":true}})";

const char* kSigned = R"({"version":2,"acroform":{"fields":[{"fieldtype":"/Sig","value":"7 0 R"},
 {"fieldtype":"/Sig","value":null}],"hasacroform":true},"encrypt":{"encrypted":false}})";

const char* kRoot = R"({"version":2,"qpdf":[{"jsonversion":2},{"obj:1 0 R":{"value":{"/Type":"/Catalog",
 "/Perms":{"/DocMDP":"9 0 R"}}}}]})";

const char* kTrailer = R"({"version":2,"qpdf":[{"jsonversion":2},{"trailer":{"value":{"/Root":"1 0 R","/Size":10}}}]})";

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app{argc, argv};
    using document::Encryption;
    try {
        // ---- parsing
        auto facts = app::QpdfInspector::parse(kRestricted, {}, 0, {});
        require(facts.inspected && facts.encryption == Encryption::openedWithUserPassword, "User password detected.");
        require(!facts.permissions.modifyContents && facts.permissions.modifyAnnotations && !facts.permissions.extract &&
                    facts.permissions.print,
                "Permission flags are read.");
        facts = app::QpdfInspector::parse(kSigned, kRoot, 0, {});
        require(facts.hasSignatures && facts.certified, "Signed fields and DocMDP certification are found.");
        facts = app::QpdfInspector::parse(R"({"acroform":{"fields":[{"fieldtype":"/Sig","value":null}]},"encrypt":{}})", {},
                                          0, {});
        require(!facts.hasSignatures && !facts.certified, "An empty signature field is not a signature.");
        facts = app::QpdfInspector::parse({}, {}, 2, "qpdf: file.pdf: invalid password\n");
        require(facts.encryption == Encryption::openPasswordRequired, "A wrong password is reported as such.");
        facts = app::QpdfInspector::parse({}, {}, 2, "qpdf: file.pdf: not a PDF file\n");
        require(!facts.readable, "Unreadable files are reported.");
        facts = app::QpdfInspector::parse(R"({"encrypt":{"encrypted":false},"acroform":{"fields":[]}})", {}, 3,
                                          "WARNING: xref repaired");
        require(facts.inspected && facts.malformed, "Warnings mean repaired (malformed) structure.");
        require(app::QpdfInspector::rootReference(kTrailer) == QStringLiteral("1 0 R"), "The catalog is found.");
        require(!app::QpdfInspector::parse({}, {}, 0, {}).inspected, "No output means not inspected.");

        // ---- real qpdf on the fixtures (skipped when qpdf is not installed)
        const QString qpdf = app::QpdfInspector::locate();
        const QString fixtures = QStringLiteral(ATLAS_FIXTURE_DIR);
        if (qpdf.isEmpty()) {
            std::cout << "qpdf not found: fixture checks skipped\n";
        } else {
            const QString normal = fixtures + QStringLiteral("/A001_simple_text.pdf");
            const QString restricted = fixtures + QStringLiteral("/A009_restricted_permissions_rc4_40.pdf");
            const QString certified = fixtures + QStringLiteral("/A010_certification_structure.pdf");
            const QByteArray before[3] = {sha256(normal), sha256(restricted), sha256(certified)};

            auto pdf = app::QpdfInspector::inspect(qpdf, normal, {});
            require(pdf.inspected && pdf.readable && pdf.encryption == Encryption::none && !pdf.hasSignatures,
                    "A001 is a plain readable PDF.");

            pdf = app::QpdfInspector::inspect(qpdf, restricted, {});
            require(pdf.encryption == Encryption::openPasswordRequired, "A009 needs its password.");
            pdf = app::QpdfInspector::inspect(qpdf, restricted, QStringLiteral("atlas-wrong"));
            require(pdf.encryption == Encryption::openPasswordRequired, "A wrong password never opens A009.");
            pdf = app::QpdfInspector::inspect(qpdf, restricted, QStringLiteral("atlas-user"));
            require(pdf.encryption == Encryption::openedWithUserPassword && !pdf.permissions.modifyContents &&
                        !pdf.permissions.modifyAnnotations,
                    "A009 opened with the user password is restricted.");
            pdf = app::QpdfInspector::inspect(qpdf, restricted, QStringLiteral("atlas-owner"));
            require(pdf.encryption == Encryption::openedWithOwnerPassword, "The owner password is recognized.");

            pdf = app::QpdfInspector::inspect(qpdf, certified, {});
            require(pdf.certified, "A010's certification structure is detected.");

            // Full check, including file facts and an external change.
            const auto first = app::checkDocumentCapabilities(restricted, QStringLiteral("atlas-user"), {});
            require(!first.assessment.capabilities.canEmbedOutline && first.assessment.canKeepLocalResearch &&
                        first.stamp.valid(),
                    "Restricted PDFs stay local.");
            QTemporaryDir dir;
            const QString copy = dir.filePath(QStringLiteral("كتاب copy.pdf"));
            require(QFile::copy(normal, copy), "copy fixture");
            QFile::setPermissions(copy, QFile::ReadOwner | QFile::WriteOwner);
            const auto writable = app::checkDocumentCapabilities(copy, {}, {});
            require(writable.assessment.capabilities.canEmbedOutline, "A writable copy with a Unicode name can embed.");
            {
                QFile f{copy};
                require(f.open(QIODevice::Append), "append");
                f.write("\n% changed outside Atlas\n");
            }
            const auto changed = app::checkDocumentCapabilities(copy, {}, writable.stamp);
            require(changed.assessment.capabilities.mutation == document::MutationSafety::externalConflict,
                    "A change made outside Atlas blocks writes.");
            QFile::setPermissions(copy, QFile::ReadOwner);
            if (!QFileInfo{copy}.isWritable()) { // an administrator account may ignore the flag
                const auto readOnly = app::checkDocumentCapabilities(copy, {}, {});
                require(!readOnly.assessment.capabilities.canEmbedOutline, "A read-only copy never embeds.");
            }
            QFile::setPermissions(copy, QFile::ReadOwner | QFile::WriteOwner);

            require(sha256(normal) == before[0] && sha256(restricted) == before[1] && sha256(certified) == before[2],
                    "The capability check never changes a PDF.");
        }
        const auto missing = app::checkDocumentCapabilities(QStringLiteral("/no/such/book.pdf"), {}, {});
        require(missing.assessment.capabilities.availability == document::Availability::missing &&
                    missing.assessment.canKeepLocalResearch,
                "Missing books keep their research.");
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "qpdf inspector tests passed\n";
    return 0;
}
