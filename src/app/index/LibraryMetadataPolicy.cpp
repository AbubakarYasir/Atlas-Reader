#include "app/index/LibraryMetadataPolicy.h"

#include <algorithm>

#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>

namespace atlas::index {
namespace {

[[nodiscard]] QString normalizedMetadata(const QString& value) {
    return value.trimmed().simplified();
}

[[nodiscard]] bool containsControlOrReplacement(const QString& value) {
    return std::any_of(value.cbegin(), value.cend(), [](QChar character) {
        return character == QChar::ReplacementCharacter
            || character.category() == QChar::Other_Control;
    });
}

[[nodiscard]] bool looksLikeRawPdfHexString(const QString& value) {
    static const QRegularExpression rawHex{QStringLiteral(R"(^<\s*[0-9A-Fa-f]+\s*>$)")};
    const auto match = rawHex.match(value);
    if (!match.hasMatch()) return false;
    QString digits = value.mid(1, value.size() - 2);
    digits.remove(QRegularExpression{QStringLiteral(R"(\s)")});
    return digits.size() >= 4 && digits.size() % 2 == 0;
}

[[nodiscard]] bool looksLikePath(const QString& value) {
    static const QRegularExpression drivePath{QStringLiteral(R"(^[A-Za-z]:[\\/])")};
    return drivePath.match(value).hasMatch()
        || value.startsWith(QStringLiteral("\\\\"))
        || value.startsWith(QStringLiteral("//"))
        || value.startsWith(QStringLiteral("file:/"), Qt::CaseInsensitive);
}

[[nodiscard]] bool looksLikeSourceDocumentName(const QString& value) {
    static const QSet<QString> sourceSuffixes{
        QStringLiteral("doc"), QStringLiteral("docx"), QStringLiteral("odt"),
        QStringLiteral("rtf"), QStringLiteral("wps"), QStringLiteral("inp"),
    };
    const QFileInfo candidate{value};
    return sourceSuffixes.contains(candidate.suffix().toLower());
}

[[nodiscard]] bool unusableMetadata(const QString& value) {
    return value.isEmpty() || value.size() > 240
        || containsControlOrReplacement(value)
        || looksLikeRawPdfHexString(value)
        || looksLikePath(value);
}

} // namespace

QString libraryDisplayTitle(const QString& embeddedTitle, const QString& fileBaseName) {
    const QString title = normalizedMetadata(embeddedTitle);
    const QString fallback = normalizedMetadata(fileBaseName);
    if (unusableMetadata(title) || looksLikeSourceDocumentName(title)
        || title.compare(QStringLiteral("Microsoft Word"), Qt::CaseInsensitive) == 0
        || title.startsWith(QStringLiteral("Microsoft Word -"), Qt::CaseInsensitive)) {
        return fallback;
    }
    return title;
}

QString libraryDisplayAuthor(const QString& embeddedAuthor) {
    const QString author = normalizedMetadata(embeddedAuthor);
    return unusableMetadata(author) ? QString{} : author;
}

} // namespace atlas::index
