#pragma once

#include <QString>

namespace atlas::index {

// PDF metadata often preserves the source editor filename or an undecoded PDF
// hex string. Prefer the current PDF filename when the embedded title is not a
// credible reader-facing book title.
[[nodiscard]] QString libraryDisplayTitle(
    const QString& embeddedTitle,
    const QString& fileBaseName);

// The current local filename stem is the trustworthy primary identity shown in
// the Library. Its extension is a separate protected field, while embedded
// titles remain searchable metadata rather than silent UI renames.
[[nodiscard]] QString libraryPrimaryDisplayName(
    const QString& fileStem,
    const QString& embeddedTitle);

[[nodiscard]] QString libraryDisplayAuthor(const QString& embeddedAuthor);

} // namespace atlas::index
