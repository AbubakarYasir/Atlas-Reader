#pragma once

// N5.2 versioned location envelope (C06): where a bookmark points, stored in a
// form that survives restart, zoom, layout and rotation changes and leaves room
// for later formats (ebook and comic locators after 2.0). A destination is not
// just a page number. Pure C++; no engine, Qt or OS types.
//
// Text form (stored in SQLite, later carried inside .atlas-bookmarks.json):
//
//   atlas-location/1;scheme=pdf;page=3;label=iv;kind=xyz;left=0.125;top=0.5;zoom=1.5
//
// - The first field names the envelope and its major version. A reader rejects
//   a newer major version as "unsupported" and the caller keeps the raw text
//   unchanged, so nothing is lost when an older build opens a newer profile.
// - Every other field is key=value. Keys are lower-case ASCII; values are
//   percent-encoded UTF-8 (`%`, `;`, `=` and control bytes are escaped).
// - Unknown keys are kept in `extensions` and written back in the same order,
//   so a minor-version addition round-trips through an older build.
//
// PDF coordinates are in normalized Atlas page space: fractions of the page as
// displayed at view rotation 0 (after the page's own /Rotate and crop box),
// origin top-left, 0 ≤ x, y ≤ 1. Adapters convert engine coordinates.

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace atlas::bookmarks {

inline constexpr int locationEnvelopeVersion = 1;

enum class LocationScheme : std::uint8_t {
    pdf,
};

// PDF destination kinds (ISO 32000 12.3.2.2). FitB* behave like Fit* on the
// bounding box; they are kept distinct for faithful round trips.
enum class DestinationKind : std::uint8_t {
    xyz,
    fit,
    fitH,
    fitV,
    fitR,
    fitB,
    fitBH,
    fitBV,
};

struct PdfDestination final {
    std::uint32_t pageIndex{};        // zero-based; the stable page identity inside one revision
    std::optional<std::string> pageLabel; // printed label at capture time, informational
    DestinationKind kind{DestinationKind::xyz};
    std::optional<double> left;   // xyz, fitV, fitR, fitBV
    std::optional<double> top;    // xyz, fitH, fitR, fitBH
    std::optional<double> right;  // fitR
    std::optional<double> bottom; // fitR
    std::optional<double> zoom;   // xyz only; absent = keep the current zoom
};

struct LocationEnvelope final {
    LocationScheme scheme{LocationScheme::pdf};
    PdfDestination pdf;
    // Unknown keys from a newer minor version, in their original order.
    std::vector<std::pair<std::string, std::string>> extensions;
};

enum class LocationDecodeStatus : std::uint8_t {
    ok,
    malformed,          // not an envelope, or a field is invalid
    unsupportedVersion, // a newer major version: keep the raw text, do not rewrite it
    unsupportedScheme,  // a scheme this build does not know: keep the raw text
};

struct LocationDecodeResult final {
    LocationDecodeStatus status{LocationDecodeStatus::malformed};
    std::optional<LocationEnvelope> envelope; // set only when status == ok
};

// Throws std::invalid_argument if the destination is invalid (see validate).
[[nodiscard]] std::string encodeLocation(const LocationEnvelope& envelope);
[[nodiscard]] LocationDecodeResult decodeLocation(std::string_view text);

// Empty when valid; otherwise a short machine-readable reason
// ("coordinateOutOfRange", "missingCoordinate", "zoomOutOfRange", …).
[[nodiscard]] std::string validate(const PdfDestination& destination);

[[nodiscard]] std::string_view destinationKindName(DestinationKind kind);

} // namespace atlas::bookmarks
