#include "core/bookmarks/LocationEnvelope.h"

#include "core/bookmarks/Utf8.h"

#include <array>
#include <charconv>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <system_error>

namespace atlas::bookmarks {
namespace {

constexpr std::string_view envelopeName = "atlas-location/";
constexpr std::size_t maxEnvelopeBytes = 8 * 1024;
constexpr std::size_t maxLabelBytes = 1024;
constexpr std::size_t maxExtensions = 32;
constexpr double maxZoom = 64.0; // 6400 %, the largest zoom the reader offers

struct KindInfo final {
    DestinationKind kind;
    std::string_view name;
    bool left;
    bool top;
    bool rect; // right and bottom, all four required
};

constexpr std::array<KindInfo, 8> kinds{{
    {DestinationKind::xyz, "xyz", true, true, false},
    {DestinationKind::fit, "fit", false, false, false},
    {DestinationKind::fitH, "fith", false, true, false},
    {DestinationKind::fitV, "fitv", true, false, false},
    {DestinationKind::fitR, "fitr", true, true, true},
    {DestinationKind::fitB, "fitb", false, false, false},
    {DestinationKind::fitBH, "fitbh", false, true, false},
    {DestinationKind::fitBV, "fitbv", true, false, false},
}};

[[nodiscard]] const KindInfo& infoFor(DestinationKind kind) {
    for (const auto& info : kinds) {
        if (info.kind == kind) return info;
    }
    throw std::invalid_argument("Unknown destination kind.");
}

[[nodiscard]] const KindInfo* infoFor(std::string_view name) {
    for (const auto& info : kinds) {
        if (info.name == name) return &info;
    }
    return nullptr;
}

[[nodiscard]] bool isKeyChar(char c) {
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
}

[[nodiscard]] bool isValidKey(std::string_view key) {
    if (key.empty() || key.size() > 64) return false;
    for (const char c : key) {
        if (!isKeyChar(c)) return false;
    }
    return true;
}

[[nodiscard]] std::string percentEncode(std::string_view value) {
    constexpr std::string_view hex = "0123456789ABCDEF";
    std::string out;
    out.reserve(value.size());
    for (const char raw : value) {
        const auto c = static_cast<unsigned char>(raw);
        if (c < 0x20 || c == 0x7F || c == '%' || c == ';' || c == '=') {
            out.push_back('%');
            out.push_back(hex[c >> 4]);
            out.push_back(hex[c & 0x0F]);
        } else {
            out.push_back(raw);
        }
    }
    return out;
}

[[nodiscard]] int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

[[nodiscard]] std::optional<std::string> percentDecode(std::string_view value) {
    std::string out;
    out.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] != '%') {
            out.push_back(value[i]);
            continue;
        }
        if (i + 2 >= value.size()) return std::nullopt;
        const int high = hexValue(value[i + 1]);
        const int low = hexValue(value[i + 2]);
        if (high < 0 || low < 0) return std::nullopt;
        out.push_back(static_cast<char>((high << 4) | low));
        i += 2;
    }
    if (!isValidUtf8(out)) return std::nullopt;
    return out;
}

[[nodiscard]] std::string formatNumber(double value) {
    std::array<char, 64> buffer{};
    const auto [end, error] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    if (error != std::errc{}) throw std::invalid_argument("A destination number could not be written.");
    return {buffer.data(), end};
}

[[nodiscard]] std::optional<double> parseNumber(std::string_view text) {
    if (text.empty() || text.size() > 40) return std::nullopt;
    double value{};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || !std::isfinite(value)) return std::nullopt;
    return value;
}

[[nodiscard]] bool inUnitRange(const std::optional<double>& value) {
    return !value || (std::isfinite(*value) && *value >= 0.0 && *value <= 1.0);
}

} // namespace

std::string_view destinationKindName(DestinationKind kind) {
    return infoFor(kind).name;
}

std::string validate(const PdfDestination& d) {
    const auto& info = infoFor(d.kind);
    if (d.pageLabel && (d.pageLabel->size() > maxLabelBytes || !isValidUtf8(*d.pageLabel))) return "invalidLabel";
    if ((d.left && !info.left) || (d.top && !info.top) || ((d.right || d.bottom) && !info.rect)) {
        return "unexpectedCoordinate";
    }
    if (d.zoom && d.kind != DestinationKind::xyz) return "unexpectedZoom";
    if (!inUnitRange(d.left) || !inUnitRange(d.top) || !inUnitRange(d.right) || !inUnitRange(d.bottom)) {
        return "coordinateOutOfRange";
    }
    if (info.rect) {
        if (!d.left || !d.top || !d.right || !d.bottom) return "missingCoordinate";
        if (!(*d.left < *d.right) || !(*d.top < *d.bottom)) return "emptyRectangle";
    }
    if (d.zoom && !(std::isfinite(*d.zoom) && *d.zoom > 0.0 && *d.zoom <= maxZoom)) return "zoomOutOfRange";
    return {};
}

std::string encodeLocation(const LocationEnvelope& envelope) {
    if (envelope.scheme != LocationScheme::pdf) throw std::invalid_argument("Unknown location scheme.");
    const auto& d = envelope.pdf;
    if (const auto problem = validate(d); !problem.empty()) {
        throw std::invalid_argument("Invalid bookmark destination: " + problem);
    }
    if (envelope.extensions.size() > maxExtensions) throw std::invalid_argument("Too many location extensions.");

    std::string out{envelopeName};
    out += std::to_string(locationEnvelopeVersion);
    out += ";scheme=pdf;page=";
    out += std::to_string(d.pageIndex);
    if (d.pageLabel) {
        out += ";label=";
        out += percentEncode(*d.pageLabel);
    }
    out += ";kind=";
    out += infoFor(d.kind).name;
    const auto number = [&out](std::string_view key, const std::optional<double>& value) {
        if (!value) return;
        out += ';';
        out += key;
        out += '=';
        out += formatNumber(*value);
    };
    number("left", d.left);
    number("top", d.top);
    number("right", d.right);
    number("bottom", d.bottom);
    number("zoom", d.zoom);
    for (const auto& [key, value] : envelope.extensions) {
        if (!isValidKey(key) || !isValidUtf8(value)) throw std::invalid_argument("Invalid location extension.");
        out += ';';
        out += key;
        out += '=';
        out += percentEncode(value);
    }
    if (out.size() > maxEnvelopeBytes) throw std::invalid_argument("A bookmark location is too large.");
    return out;
}

LocationDecodeResult decodeLocation(std::string_view text) {
    LocationDecodeResult result;
    if (text.size() > maxEnvelopeBytes || !text.starts_with(envelopeName)) return result;

    // Split on ';' (values are percent-encoded, so ';' only separates fields).
    std::vector<std::string_view> fields;
    std::size_t start = 0;
    while (start <= text.size()) {
        const auto end = text.find(';', start);
        fields.push_back(text.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start));
        if (end == std::string_view::npos) break;
        start = end + 1;
    }

    const auto versionText = fields.front().substr(envelopeName.size());
    int major{};
    {
        const auto [end, error] = std::from_chars(versionText.data(), versionText.data() + versionText.size(), major);
        if (error != std::errc{} || end != versionText.data() + versionText.size() || major < 1 || versionText.size() > 4) {
            return result;
        }
    }
    if (major > locationEnvelopeVersion) {
        result.status = LocationDecodeStatus::unsupportedVersion;
        return result;
    }

    LocationEnvelope envelope;
    bool sawScheme = false;
    bool sawPage = false;
    bool sawKind = false;
    std::vector<std::string_view> seen;
    for (std::size_t i = 1; i < fields.size(); ++i) {
        const auto field = fields[i];
        const auto equals = field.find('=');
        if (equals == std::string_view::npos) return result;
        const auto key = field.substr(0, equals);
        const auto raw = field.substr(equals + 1);
        if (!isValidKey(key)) return result;
        for (const auto earlier : seen) {
            if (earlier == key) return result; // duplicate keys are ambiguous
        }
        seen.push_back(key);

        if (key == "scheme") {
            if (raw != "pdf") {
                result.status = LocationDecodeStatus::unsupportedScheme;
                return result;
            }
            sawScheme = true;
        } else if (key == "page") {
            std::uint32_t page{};
            const auto [end, error] = std::from_chars(raw.data(), raw.data() + raw.size(), page);
            if (raw.empty() || error != std::errc{} || end != raw.data() + raw.size()) return result;
            envelope.pdf.pageIndex = page;
            sawPage = true;
        } else if (key == "label") {
            auto label = percentDecode(raw);
            if (!label) return result;
            envelope.pdf.pageLabel = std::move(*label);
        } else if (key == "kind") {
            const auto* info = infoFor(raw);
            if (info == nullptr) return result;
            envelope.pdf.kind = info->kind;
            sawKind = true;
        } else if (key == "left" || key == "top" || key == "right" || key == "bottom" || key == "zoom") {
            const auto value = parseNumber(raw);
            if (!value) return result;
            if (key == "left") envelope.pdf.left = value;
            else if (key == "top") envelope.pdf.top = value;
            else if (key == "right") envelope.pdf.right = value;
            else if (key == "bottom") envelope.pdf.bottom = value;
            else envelope.pdf.zoom = value;
        } else {
            auto value = percentDecode(raw);
            if (!value || envelope.extensions.size() >= maxExtensions) return result;
            envelope.extensions.emplace_back(std::string{key}, std::move(*value));
        }
    }
    if (!sawScheme || !sawPage || !sawKind || !validate(envelope.pdf).empty()) return result;

    result.status = LocationDecodeStatus::ok;
    result.envelope = std::move(envelope);
    return result;
}

} // namespace atlas::bookmarks
