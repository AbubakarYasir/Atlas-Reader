#include "app/index/QtPdfDocumentInspector.h"
#include "app/index/LibraryMetadataPolicy.h"

#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Expected readable, Unicode, malformed and password-protected PDF paths.\n";
        return 64;
    }
    try {
        require(atlas::index::libraryDisplayTitle(QStringLiteral("1.docx"), QStringLiteral("الكتاب"))
                == QStringLiteral("الكتاب"),
            "A source-editor filename must not replace the current PDF filename in the Library.");
        require(atlas::index::libraryDisplayTitle(
                    QStringLiteral("<4D6963726F736F667420576F7264>"), QStringLiteral("الحجاب الدرع الواقي"))
                == QStringLiteral("الحجاب الدرع الواقي"),
            "An undecoded PDF hex title must fall back to the current Unicode PDF filename.");
        require(atlas::index::libraryDisplayTitle(
                    QStringLiteral("G:\\books\\legacy-title.inp"), QStringLiteral("فتاوى"))
                == QStringLiteral("فتاوى"),
            "An embedded source path must not be shown as the reader-facing book title.");
        require(atlas::index::libraryDisplayTitle(QStringLiteral("عنوان عربي صحيح"), QStringLiteral("fallback"))
                == QStringLiteral("عنوان عربي صحيح"),
            "A credible Unicode title must remain unchanged.");
        require(atlas::index::libraryPrimaryDisplayName(
                    QStringLiteral("الاحتساب على الشذوذ"), QStringLiteral("<(PDF) الاحتساب على الشذوذ>"))
                == QStringLiteral("الاحتساب على الشذوذ"),
            "The current filename stem must be the Library card's primary name.");
        require(atlas::index::libraryPrimaryDisplayName(
                    QStringLiteral("الفرائد اللؤلؤية في القواعد النحوية"), QStringLiteral("Binde.pdf"))
                == QStringLiteral("الفرائد اللؤلؤية في القواعد النحوية"),
            "A plausible-looking embedded filename must not replace the real filename stem in the UI.");
        require(atlas::index::libraryPrimaryDisplayName(
                    QStringLiteral("الاحتساب على الأطفال"), QStringLiteral("TIFF.pdf"))
                == QStringLiteral("الاحتساب على الأطفال"),
            "A conversion-tool title must not replace the current filename stem in the UI.");
        require(atlas::index::libraryDisplayAuthor(QStringLiteral("<4D6963726F736F6674>")).isEmpty(),
            "Undecoded author metadata must not be exposed in the Library.");
        require(atlas::index::libraryDisplayAuthor(QStringLiteral("Unknown author")).isEmpty(),
            "A generic unknown-author placeholder must not be exposed in the Library.");

        atlas::index::QtPdfDocumentInspector inspector;
        const auto readable = inspector.inspect(argv[1]);
        require(readable.availability == atlas::document::Availability::available
                && readable.pageCount == 1
                && readable.fileSizeBytes.has_value()
                && readable.modifiedUtcMs.has_value()
                && !readable.title.empty(),
            "A readable PDF must expose bounded library metadata and revision signals.");

        const auto unicode = inspector.inspect(argv[2]);
        require(unicode.availability == atlas::document::Availability::available
                && unicode.pageCount.has_value(),
            "A Unicode/Arabic PDF path must remain inspectable.");

        const auto malformed = inspector.inspect(argv[3]);
        require(malformed.availability == atlas::document::Availability::unreadable
                && !malformed.pageCount.has_value(),
            "A malformed PDF must be explicit and must not pretend to have readable pages.");

        const auto locked = inspector.inspect(argv[4]);
        require(locked.availability == atlas::document::Availability::locked
                && !locked.pageCount.has_value(),
            "A password-protected PDF must be classified as locked without bypassing its password.");

        const auto missing = inspector.inspect(std::filesystem::path{argv[1]}.parent_path() / "missing.pdf");
        require(missing.availability == atlas::document::Availability::missing,
            "A disappeared PDF must be classified as missing.");
    } catch (const std::exception& error) {
        std::cerr << "Library document inspector test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Library PDF readable, Unicode, malformed, locked and missing inspection checks passed.\n";
}
