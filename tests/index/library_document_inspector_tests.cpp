#include "app/index/QtPdfDocumentInspector.h"

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
