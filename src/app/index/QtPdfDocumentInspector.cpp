#include "app/index/QtPdfDocumentInspector.h"

#include "app/index/LibraryMetadataPolicy.h"

#include <QDateTime>
#include <QFileInfo>
#include <QPdfDocument>
#include <QString>

#include <stdexcept>

namespace atlas::index {
namespace {

[[nodiscard]] QString toQString(const std::filesystem::path& path) {
#ifdef _WIN32
    return QString::fromStdWString(path.wstring());
#else
    const auto utf8 = path.u8string();
    return QString::fromUtf8(reinterpret_cast<const char*>(utf8.data()), static_cast<qsizetype>(utf8.size()));
#endif
}

[[nodiscard]] std::string toUtf8(const QString& value) {
    const auto utf8 = value.toUtf8();
    return {utf8.constData(), static_cast<std::size_t>(utf8.size())};
}

[[nodiscard]] document::Availability availabilityFor(QPdfDocument::Error error) {
    switch (error) {
    case QPdfDocument::Error::None: return document::Availability::available;
    case QPdfDocument::Error::FileNotFound: return document::Availability::missing;
    case QPdfDocument::Error::IncorrectPassword: return document::Availability::locked;
    case QPdfDocument::Error::UnsupportedSecurityScheme: return document::Availability::unsupported;
    case QPdfDocument::Error::Unknown:
    case QPdfDocument::Error::DataNotYetAvailable:
    case QPdfDocument::Error::InvalidFileFormat: return document::Availability::unreadable;
    }
    return document::Availability::unreadable;
}

} // namespace

LibraryDocumentInspection QtPdfDocumentInspector::inspect(const std::filesystem::path& source) const {
    if (source.empty()) throw std::invalid_argument("A PDF source path is required for inspection.");

    LibraryDocumentInspection result;
    result.source = source;
    const QString sourcePath = toQString(source);
    const QFileInfo fileInfo{sourcePath};
    if (fileInfo.exists() && fileInfo.isFile()) {
        result.fileSizeBytes = static_cast<std::uintmax_t>(fileInfo.size());
        result.modifiedUtcMs = fileInfo.lastModified().toUTC().toMSecsSinceEpoch();
    }

    QPdfDocument document;
    const auto error = document.load(sourcePath);
    result.availability = availabilityFor(error);
    if (error == QPdfDocument::Error::None) {
        result.pageCount = document.pageCount();
        result.title = toUtf8(libraryDisplayTitle(
            document.metaData(QPdfDocument::MetaDataField::Title).toString(),
            fileInfo.completeBaseName()));
        result.author = toUtf8(libraryDisplayAuthor(
            document.metaData(QPdfDocument::MetaDataField::Author).toString()));
    }
    if (result.title.empty()) result.title = toUtf8(fileInfo.completeBaseName());
    return result;
}

} // namespace atlas::index
