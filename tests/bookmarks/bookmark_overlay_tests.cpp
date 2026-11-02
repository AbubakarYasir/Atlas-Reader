// N5.2: location envelope, storage-state transitions and tree rules (pure C++).

#include "core/bookmarks/BookmarkOverlay.h"
#include "core/bookmarks/LocationEnvelope.h"
#include "core/bookmarks/Utf8.h"

#include <array>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace atlas::bookmarks;

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

LocationEnvelope pdfAt(std::uint32_t page, DestinationKind kind) {
    LocationEnvelope envelope;
    envelope.pdf.pageIndex = page;
    envelope.pdf.kind = kind;
    return envelope;
}

void checkUtf8() {
    require(isValidUtf8(""), "Empty text is valid UTF-8.");
    require(isValidUtf8("Atlas"), "ASCII is valid UTF-8.");
    require(isValidUtf8("مقدمة الكتاب"), "Arabic is valid UTF-8.");
    require(isValidUtf8("اُردُو کی کتاب"), "Urdu with harakat is valid UTF-8.");
    require(isValidUtf8("\xF0\x9F\x93\x9A"), "A four-byte emoji is valid UTF-8.");
    require(isValidUtf8(std::string_view{"a\0b", 3}), "U+0000 is valid UTF-8 and kept as given.");
    require(!isValidUtf8("\xC0\xAF"), "Overlong forms are refused.");
    require(!isValidUtf8("\xED\xA0\x80"), "Surrogates are refused.");
    require(!isValidUtf8("\xF4\x90\x80\x80"), "Code points above U+10FFFF are refused.");
    require(!isValidUtf8("\xE2\x82"), "Truncated sequences are refused.");
    require(!isValidUtf8("\xFF"), "Invalid lead bytes are refused.");
}

void checkEnvelopeRoundTrips() {
    auto xyz = pdfAt(41, DestinationKind::xyz);
    xyz.pdf.pageLabel = "xii;=%";
    xyz.pdf.left = 0.125;
    xyz.pdf.top = 0.5;
    xyz.pdf.zoom = 1.5;
    const auto text = encodeLocation(xyz);
    require(text == "atlas-location/1;scheme=pdf;page=41;label=xii%3B%3D%25;kind=xyz;left=0.125;top=0.5;zoom=1.5",
            "The XYZ envelope must use the documented text form.");
    const auto decoded = decodeLocation(text);
    require(decoded.status == LocationDecodeStatus::ok && decoded.envelope, "The XYZ envelope must decode.");
    require(decoded.envelope->pdf.pageIndex == 41 && decoded.envelope->pdf.pageLabel == "xii;=%" &&
                decoded.envelope->pdf.left == 0.125 && decoded.envelope->pdf.top == 0.5 &&
                decoded.envelope->pdf.zoom == 1.5 && !decoded.envelope->pdf.right,
            "Every XYZ field must survive the round trip.");
    require(encodeLocation(*decoded.envelope) == text, "Re-encoding must give the same text.");

    // A value that is not exact in binary still round-trips exactly.
    auto precise = pdfAt(0, DestinationKind::fitH);
    precise.pdf.top = 0.1 + 0.2;
    const auto preciseBack = decodeLocation(encodeLocation(precise));
    require(preciseBack.envelope && preciseBack.envelope->pdf.top == 0.1 + 0.2, "Numbers must round-trip exactly.");

    auto arabicLabel = pdfAt(3, DestinationKind::fit);
    arabicLabel.pdf.pageLabel = "صفحة ٣ / ب";
    const auto arabicBack = decodeLocation(encodeLocation(arabicLabel));
    require(arabicBack.envelope && arabicBack.envelope->pdf.pageLabel == "صفحة ٣ / ب",
            "Arabic page labels must be kept byte for byte.");

    auto rect = pdfAt(7, DestinationKind::fitR);
    rect.pdf.left = 0.1;
    rect.pdf.top = 0.2;
    rect.pdf.right = 0.9;
    rect.pdf.bottom = 0.8;
    const auto rectBack = decodeLocation(encodeLocation(rect));
    require(rectBack.envelope && rectBack.envelope->pdf.kind == DestinationKind::fitR &&
                rectBack.envelope->pdf.bottom == 0.8,
            "FitR must keep its rectangle.");

    for (const auto kind : {DestinationKind::fit, DestinationKind::fitB, DestinationKind::fitH, DestinationKind::fitBH,
                            DestinationKind::fitV, DestinationKind::fitBV, DestinationKind::xyz}) {
        const auto back = decodeLocation(encodeLocation(pdfAt(2, kind)));
        require(back.envelope && back.envelope->pdf.kind == kind, "Every destination kind must round-trip.");
    }

    // A newer minor version's extra keys survive an older build untouched.
    const std::string future =
        "atlas-location/1;scheme=pdf;page=2;kind=fit;anchor-id=n%3B42;note=نص";
    const auto withExtensions = decodeLocation(future);
    require(withExtensions.envelope && withExtensions.envelope->extensions.size() == 2 &&
                withExtensions.envelope->extensions[0].second == "n;42",
            "Unknown keys must be kept as extensions.");
    require(encodeLocation(*withExtensions.envelope) == future, "Extensions must be written back in order.");
}

void checkEnvelopeRejections() {
    require(decodeLocation("atlas-location/2;scheme=pdf;page=1;kind=fit").status ==
                LocationDecodeStatus::unsupportedVersion,
            "A newer major version is reported, not guessed.");
    require(decodeLocation("atlas-location/1;scheme=epub;cfi=x").status == LocationDecodeStatus::unsupportedScheme,
            "An unknown scheme is reported, not guessed.");
    const std::array<std::string_view, 16> malformed{
        "",
        "page=3",
        "atlas-location/0;scheme=pdf;page=1;kind=fit",
        "atlas-location/x;scheme=pdf;page=1;kind=fit",
        "atlas-location/1;scheme=pdf;kind=fit",
        "atlas-location/1;scheme=pdf;page=1",
        "atlas-location/1;page=1;kind=fit",
        "atlas-location/1;scheme=pdf;page=-1;kind=fit",
        "atlas-location/1;scheme=pdf;page=1;page=2;kind=fit",
        "atlas-location/1;scheme=pdf;page=1;kind=fit;left=0.5",
        "atlas-location/1;scheme=pdf;page=1;kind=xyz;left=1.5",
        "atlas-location/1;scheme=pdf;page=1;kind=xyz;left=nan",
        "atlas-location/1;scheme=pdf;page=1;kind=xyz;zoom=0",
        "atlas-location/1;scheme=pdf;page=1;kind=fitr;left=0.5;top=0.1;right=0.5;bottom=0.9",
        "atlas-location/1;scheme=pdf;page=1;kind=fit;label=%ZZ",
        "atlas-location/1;scheme=pdf;page=1;kind=fit;",
    };
    for (const auto text : malformed) {
        require(decodeLocation(text).status == LocationDecodeStatus::malformed,
                std::string{"Malformed envelope accepted: "} + std::string{text});
    }
    require(decodeLocation(std::string(9000, 'a')).status == LocationDecodeStatus::malformed,
            "Oversized envelopes are refused.");

    bool threw = false;
    try {
        auto bad = pdfAt(1, DestinationKind::xyz);
        bad.pdf.zoom = 100.0;
        (void)encodeLocation(bad);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    require(threw, "Encoding an invalid destination must fail rather than store it.");
}

void checkTransitions() {
    using S = StorageState;
    using E = StorageEvent;
    const std::array<S, 6> states{S::embedded, S::localOnly, S::pendingEmbed, S::localOverride, S::locallyHidden,
                                  S::conflict};
    const std::array<E, 11> events{E::editLocally, E::requestEmbed, E::cancelEmbed, E::hide, E::unhideUnchanged,
                                   E::unhideChanged, E::revertToSource, E::detectConflict, E::keepLocal,
                                   E::takeSource, E::embedVerified};
    // Expected table, written independently of the implementation.
    const std::map<std::pair<S, E>, S> expected{
        {{S::embedded, E::editLocally}, S::localOverride},
        {{S::localOverride, E::editLocally}, S::localOverride},
        {{S::localOnly, E::editLocally}, S::localOnly},
        {{S::pendingEmbed, E::editLocally}, S::pendingEmbed},
        {{S::localOnly, E::requestEmbed}, S::pendingEmbed},
        {{S::pendingEmbed, E::cancelEmbed}, S::localOnly},
        {{S::embedded, E::hide}, S::locallyHidden},
        {{S::localOverride, E::hide}, S::locallyHidden},
        {{S::locallyHidden, E::unhideUnchanged}, S::embedded},
        {{S::locallyHidden, E::unhideChanged}, S::localOverride},
        {{S::localOverride, E::revertToSource}, S::embedded},
        {{S::embedded, E::detectConflict}, S::conflict},
        {{S::localOnly, E::detectConflict}, S::conflict},
        {{S::pendingEmbed, E::detectConflict}, S::conflict},
        {{S::localOverride, E::detectConflict}, S::conflict},
        {{S::locallyHidden, E::detectConflict}, S::conflict},
        {{S::conflict, E::keepLocal}, S::localOnly},
        {{S::conflict, E::takeSource}, S::embedded},
        {{S::localOnly, E::embedVerified}, S::embedded},
        {{S::pendingEmbed, E::embedVerified}, S::embedded},
        {{S::localOverride, E::embedVerified}, S::embedded},
        {{S::embedded, E::embedVerified}, S::embedded},
    };
    EmbedProof proof{true, true, true, true, "size=10;mtime=20"};
    std::size_t allowed = 0;
    for (const auto state : states) {
        for (const auto event : events) {
            const auto result = transition(state, event, &proof);
            const auto found = expected.find({state, event});
            if (found == expected.end()) {
                require(!result.allowed && result.next == state,
                        "A transition missing from the contract must be refused: " + std::string{stateName(state)});
            } else {
                require(result.allowed && result.next == found->second,
                        "A contract transition must be allowed: " + std::string{stateName(state)});
                ++allowed;
            }
        }
    }
    require(allowed == expected.size(), "Every contract transition must be exercised.");

    // Nothing becomes Embedded on intent: every proof step is required.
    require(!transition(S::pendingEmbed, E::embedVerified).allowed, "Embedding without proof is refused.");
    for (int missing = 0; missing < 5; ++missing) {
        EmbedProof partial{missing != 0, missing != 1, missing != 2, missing != 3,
                           missing == 4 ? std::string{} : std::string{"rev"}};
        require(!transition(S::pendingEmbed, E::embedVerified, &partial).allowed,
                "Embedding with an incomplete proof is refused.");
    }
    require(!isOrdinaryStateChange(S::pendingEmbed, S::embedded) && !isOrdinaryStateChange(S::localOnly, S::embedded),
            "Local work never reaches Embedded by an ordinary edit.");
    require(isOrdinaryStateChange(S::embedded, S::localOverride) && isOrdinaryStateChange(S::conflict, S::localOnly),
            "Ordinary edits follow the table.");
    require(!isOrdinaryStateChange(S::locallyHidden, S::localOnly), "Hidden source nodes cannot turn local.");

    for (const auto state : states) {
        require(parseStateName(stateName(state)) == state, "State names must round-trip.");
    }
    require(!parseStateName("Embedded"), "State names are exact.");
}

BookmarkNode localNode(std::string id, std::optional<std::string> parent, std::uint32_t position,
                       std::string title = "Title") {
    BookmarkNode node;
    node.id = std::move(id);
    node.documentId = "doc";
    node.parentId = std::move(parent);
    node.position = position;
    node.title = std::move(title);
    return node;
}

void checkTreeRules() {
    std::vector<BookmarkNode> tree{localNode("a", std::nullopt, 0), localNode("b", std::nullopt, 1),
                                   localNode("c", "a", 0), localNode("d", "a", 1)};
    require(validateTree(tree).empty(), "A well-formed tree is valid.");
    require(validateTree({}).empty(), "An empty overlay is valid.");

    auto duplicate = tree;
    duplicate.push_back(localNode("a", "b", 0));
    require(validateTree(duplicate) == "duplicateId", "Duplicate IDs are refused.");

    auto orphan = tree;
    orphan.push_back(localNode("e", "missing", 0));
    require(validateTree(orphan) == "missingParent", "Missing parents are refused.");

    auto gap = tree;
    gap[1].position = 2;
    require(validateTree(gap) == "badPosition", "Sibling positions must be contiguous.");
    auto clash = tree;
    clash[3].position = 0;
    require(validateTree(clash) == "badPosition", "Sibling positions must be unique.");

    std::vector<BookmarkNode> cycle{localNode("x", "y", 0), localNode("y", "x", 0)};
    require(validateTree(cycle) == "cycle", "Cycles are refused.");

    auto mixed = tree;
    mixed[2].documentId = "other";
    require(validateTree(mixed) == "mixedDocuments", "One overlay holds one document.");

    std::vector<BookmarkNode> deep;
    for (std::size_t i = 0; i < OverlayLimits::maxDepth; ++i) {
        deep.push_back(localNode("n" + std::to_string(i),
                                 i == 0 ? std::nullopt : std::optional<std::string>{"n" + std::to_string(i - 1)}, 0));
    }
    require(validateTree(deep).empty(), "The full supported depth is valid.");
    deep.push_back(localNode("too-deep", "n" + std::to_string(OverlayLimits::maxDepth - 1), 0));
    require(validateTree(deep) == "tooDeep", "Deeper trees are refused.");

    auto badTitle = localNode("t", std::nullopt, 0, "\xC0\xAF");
    require(validateNode(badTitle) == "badTitle", "Invalid UTF-8 titles are refused.");
    auto spaced = localNode("t", std::nullopt, 0, "  عنوان  ");
    require(validateNode(spaced).empty(), "Leading and trailing spaces are user content, kept as given.");
    auto badId = localNode("has space", std::nullopt, 0);
    require(validateNode(badId) == "badId", "IDs are printable ASCII.");
    auto embeddedWithoutBase = localNode("e", std::nullopt, 0);
    embeddedWithoutBase.state = StorageState::embedded;
    require(validateNode(embeddedWithoutBase) == "missingBaseKey", "Embedded nodes must name their source node.");
    auto localWithBase = localNode("l", std::nullopt, 0);
    localWithBase.baseKey = "0";
    require(validateNode(localWithBase) == "unexpectedBaseKey", "Local nodes have no source node.");
    auto conflictWithoutInfo = localNode("c", std::nullopt, 0);
    conflictWithoutInfo.state = StorageState::conflict;
    require(validateNode(conflictWithoutInfo) == "conflictMismatch", "A conflict records what it was.");
    auto newerLocation = localNode("n", std::nullopt, 0);
    newerLocation.location = "atlas-location/7;scheme=pdf;page=1";
    require(validateNode(newerLocation).empty(), "A newer location envelope is kept, not refused.");
    newerLocation.location = "page 7";
    require(validateNode(newerLocation) == "malformedLocation", "Garbage locations are refused.");
    auto badTag = localNode("t", std::nullopt, 0);
    badTag.tags = {"ok", ""};
    require(validateNode(badTag) == "badTag", "Empty tags are refused.");
}

void checkBase() {
    BaseSnapshot base;
    base.sourceRevision = "size=100;mtime=5";
    base.nodes = {{"0", std::nullopt, 0, "الفصل الأول", encodeLocation(pdfAt(0, DestinationKind::fit))},
                  {"0/0", "0", 0, "Section 1.1", encodeLocation(pdfAt(1, DestinationKind::fit))},
                  {"1", std::nullopt, 1, "الفصل الأول", {}}};
    require(validateBase(base).empty(), "A well-formed base is valid.");

    const auto embedded = embeddedNodesFromBase("doc", base);
    require(embedded.size() == 3, "One Embedded node per base node.");
    std::set<std::string> ids;
    for (const auto& node : embedded) {
        require(node.state == StorageState::embedded && node.baseKey && node.documentId == "doc",
                "Seeded nodes are Embedded and linked to their source.");
        ids.insert(node.id);
    }
    require(ids.size() == 3, "Seeded nodes get distinct IDs.");
    require(validateTree(embedded).empty() && validateAgainstBase(embedded, base).empty(),
            "Seeded nodes match the base exactly.");

    auto renamed = embedded;
    renamed[1].title = "Renamed";
    require(validateAgainstBase(renamed, base) == "embeddedDiffersFromSource",
            "A changed node may not stay Embedded.");
    renamed[1].state = StorageState::localOverride;
    require(validateAgainstBase(renamed, base).empty(), "A changed source node is a LocalOverride.");

    auto moved = embedded;
    moved[1].parentId = moved[2].id; // under the other chapter
    require(validateAgainstBase(moved, base) == "embeddedDiffersFromSource", "A moved node may not stay Embedded.");

    auto twice = embedded;
    twice[2].baseKey = "0";
    require(validateAgainstBase(twice, base) == "baseClaimedTwice", "A source node has one overlay node.");

    auto gone = embedded;
    gone[0].baseKey = "9";
    require(validateAgainstBase(gone, base) == "unknownBaseKey", "Embedded nodes point at real source nodes.");
    require(validateAgainstBase(embedded, std::nullopt) == "unknownBaseKey", "Embedded nodes need a base.");

    BaseSnapshot noRevision = base;
    noRevision.sourceRevision.clear();
    require(validateBase(noRevision) == "missingSourceRevision", "A base records its revision.");

    std::set<std::string> many;
    for (int i = 0; i < 2000; ++i) many.insert(newBookmarkId());
    require(many.size() == 2000, "Generated IDs do not repeat.");
    require(newBookmarkId().size() == 32, "IDs are 32 hex digits.");
}

} // namespace

int main() {
    try {
        checkUtf8();
        checkEnvelopeRoundTrips();
        checkEnvelopeRejections();
        checkTransitions();
        checkTreeRules();
        checkBase();
    } catch (const std::exception& error) {
        std::cerr << "Bookmark overlay test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Location envelope, storage-state transitions, tree and base rules passed.\n";
    return 0;
}
