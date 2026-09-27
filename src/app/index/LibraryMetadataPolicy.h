#pragma once

#include <QString>

namespace atlas::index {

// PDF metadata often preserves the source editor filename or an undecoded PDF
// hex string. Prefer the current PDF filename when the embedded title is not a
// credible reader-facing book title.
[[nodiscard]] QString libraryDisplayTitle(
    const QString& embeddedTitle,
    const QString& fileBaseName);

[[nodiscard]] QString libraryDisplayAuthor(const QString& embeddedAuthor);

} // namespace atlas::index
