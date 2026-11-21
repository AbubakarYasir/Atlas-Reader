#!/usr/bin/env python3
"""Builds Atlas Reader's interface fonts from pinned Noto sources.

Atlas Naskh    = Noto Naskh Arabic + Latin from Noto Sans + bidi control glyphs
Atlas Nastaliq = Noto Nastaliq Urdu at fixed weights 400 and 600 + bidi control glyphs

Why the fonts are derived instead of bundled unmodified
-------------------------------------------------------
On Windows, Qt reads every installed font family the first time a piece of
text needs a glyph its font lacks, or asks for a weight the family does not
have. With thousands of fonts installed that takes seconds and happens before
the first frame. The interface fonts therefore carry every character the
interface shows (Latin letters, punctuation, direction marks) and both weights
the interface uses, so no lookup outside the font is needed.

The families are renamed because a PC may have the original Noto fonts
installed; two different fonts under one family name would be ambiguous.

Usage:
    python tools/fonts/build-ui-fonts.py --sources <dir> [--out assets/fonts]

<dir> must hold the pinned inputs listed in SOURCES (see assets/fonts/README.md
for where each comes from). Every input is verified by SHA-256 and the build is
deterministic: the same inputs and fontTools version give byte-identical files.

Requires fontTools (tested with 4.67.0).
"""
from __future__ import annotations

import argparse
import hashlib
import io
import sys
from pathlib import Path

from fontTools import subset
from fontTools.merge import Merger
from fontTools.ttLib import TTFont
from fontTools.ttLib.tables._g_l_y_f import Glyph
from fontTools.varLib import instancer

SOURCES = {
    "NotoNaskhArabic-Regular.ttf": "6f0a92031367b2f5a2078fe9d24f3433122b61a0bad57c423aad8f3c39aa2e6e",
    "NotoNaskhArabic-SemiBold.ttf": "76616e51f5ff1cf4d81a14df6049646911cd4d7e568903a011711c241991624c",
    "NotoNastaliqUrdu-wght.ttf": "98a4787f34eb6fde57fb9a1121a8f216301196ab0da98ea110cad581be8abbcd",
    "NotoSans-Regular.ttf": "478c558ea716033cd60c03438f628dfa75694dcf6b5f6d505a2f05fd2b4f3823",
    "NotoSans-SemiBold.ttf": "a4e91fd530ac2b4ef5367240144ff37d7d65d66cf76f2e9a2187b93c676f92d0",
}

# Fixed timestamp (2026-11-21 00:00 UTC, seconds since 1904) keeps builds reproducible.
FIXED_TIMESTAMP = 3878755200
FONT_VERSION = 1.0

# Latin and symbols taken from Noto Sans for Atlas Naskh. Characters the Arabic
# font already has (digits, space, some punctuation) keep the Arabic font's glyphs.
LATIN_RANGES = [
    (0x0020, 0x007E),  # Basic Latin
    (0x00A0, 0x00FF),  # Latin-1 Supplement
    (0x0100, 0x017F),  # Latin Extended-A
    (0x2010, 0x2027),  # dashes, quotes, bullet, ellipsis
    (0x2030, 0x2030),
    (0x2039, 0x203A),  # single guillemets (breadcrumb separator)
    (0x20AC, 0x20AC),
    (0x2190, 0x2193),  # arrows
    (0x2212, 0x2212),
]

# Direction and joining controls. They draw nothing, but the font must map them
# or the text engine goes looking for another font that does.
CONTROL_CODEPOINTS = [0x200B, 0x200C, 0x200D, 0x200E, 0x200F, 0x202A, 0x202B, 0x202C, 0x202D, 0x202E,
                      0x2060, 0x2066, 0x2067, 0x2068, 0x2069, 0xFEFF]
CONTROL_GLYPH = "atlas.control"


def load(sources: Path, name: str) -> TTFont:
    data = (sources / name).read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    if digest != SOURCES[name]:
        sys.exit(f"{name}: SHA-256 {digest} does not match the pinned {SOURCES[name]}")
    return TTFont(io.BytesIO(data))


def latin_subset(font: TTFont) -> TTFont:
    options = subset.Options()
    options.layout_features = ["kern", "mark", "mkmk", "ccmp", "locl"]
    options.name_IDs = [0, 13, 14]
    options.notdef_outline = True
    options.glyph_names = True
    options.hinting = False
    options.legacy_cmap = False
    options.symbol_cmap = False
    options.prune_unicode_ranges = True
    subsetter = subset.Subsetter(options)
    unicodes = [cp for first, last in LATIN_RANGES for cp in range(first, last + 1)]
    subsetter.populate(unicodes=unicodes)
    subsetter.subset(font)
    return roundtrip(font)


def roundtrip(font: TTFont) -> TTFont:
    buffer = io.BytesIO()
    font.save(buffer)
    buffer.seek(0)
    return TTFont(buffer)


def add_control_glyph(font: TTFont) -> None:
    missing = [cp for cp in CONTROL_CODEPOINTS if cp not in font.getBestCmap()]
    if not missing:
        return
    font.getGlyphOrder()
    if CONTROL_GLYPH not in font["glyf"].glyphs:
        font["glyf"][CONTROL_GLYPH] = Glyph()
        font["hmtx"][CONTROL_GLYPH] = (0, 0)
        order = font.getGlyphOrder()
        if CONTROL_GLYPH not in order:
            order.append(CONTROL_GLYPH)
            font.setGlyphOrder(order)
        if "HVAR" in font:
            del font["HVAR"]
    for table in font["cmap"].tables:
        if table.isUnicode():
            for cp in missing:
                if cp <= 0xFFFF or table.format in (12, 13):
                    table.cmap.setdefault(cp, CONTROL_GLYPH)
    if "GDEF" in font and font["GDEF"].table.GlyphClassDef is not None:
        font["GDEF"].table.GlyphClassDef.classDefs.pop(CONTROL_GLYPH, None)


def set_names(font: TTFont, family: str, style: str, weight: int, description: str) -> None:
    name = font["name"]
    keep = {0: None, 13: None, 14: None}
    for nid in keep:
        keep[nid] = name.getDebugName(nid)
    name.names = []
    regular = style == "Regular"
    legacy_family = family if regular else f"{family} {style}"
    full = family if regular else f"{family} {style}"
    postscript = f"{family.replace(' ', '')}-{style}"
    records = {
        0: keep[0],
        1: legacy_family,
        2: "Regular",
        3: f"{FONT_VERSION:.3f};ATLS;{postscript}",
        4: full,
        5: f"Version {FONT_VERSION:.3f}",
        6: postscript,
        10: description,
        13: keep[13],
        14: keep[14],
    }
    if not regular:
        records[16] = family
        records[17] = style
    for nid, value in sorted(records.items()):
        if value:
            name.setName(value, nid, 3, 1, 0x409)
    os2 = font["OS/2"]
    os2.usWeightClass = weight
    os2.achVendID = "ATLS"
    os2.fsSelection = (os2.fsSelection & ~0b100001) | 0b1000000  # REGULAR; not ITALIC/BOLD
    font["head"].macStyle = 0
    font["head"].fontRevision = FONT_VERSION
    font["head"].created = FIXED_TIMESTAMP
    font["head"].modified = FIXED_TIMESTAMP
    if "CFF " not in font and "post" in font:
        font["post"].isFixedPitch = 0


def save(font: TTFont, path: Path) -> None:
    font.recalcTimestamp = False
    for tag in ("DSIG", "FFTM"):
        if tag in font:
            del font[tag]
    font.save(path)
    print(f"{path.name}: {path.stat().st_size} bytes, sha256 {hashlib.sha256(path.read_bytes()).hexdigest()}")


def build_naskh(sources: Path, out: Path, style: str, weight: int, tmp: Path) -> None:
    arabic = load(sources, f"NotoNaskhArabic-{style}.ttf")
    latin = latin_subset(load(sources, f"NotoSans-{style}.ttf"))
    a_path, l_path = tmp / f"naskh-{style}.ttf", tmp / f"latin-{style}.ttf"
    arabic.recalcTimestamp = latin.recalcTimestamp = False
    arabic.save(a_path)
    latin.save(l_path)
    merged = roundtrip(Merger().merge([str(a_path), str(l_path)]))
    add_control_glyph(merged)
    set_names(merged, "Atlas Naskh", style, weight,
              "Noto Naskh Arabic 2.021 with Latin from Noto Sans 2.015 and zero-width "
              "direction-control glyphs, built for the Atlas Reader interface.")
    save(merged, out / f"AtlasNaskh-{style}.ttf")


def build_nastaliq(sources: Path, out: Path, style: str, weight: int) -> None:
    variable = load(sources, "NotoNastaliqUrdu-wght.ttf")
    static = instancer.instantiateVariableFont(variable, {"wght": weight}, inplace=False)
    static = roundtrip(static)
    add_control_glyph(static)
    set_names(static, "Atlas Nastaliq", style, weight,
              f"Noto Nastaliq Urdu at fixed weight {weight} with zero-width direction-control "
              "glyphs, built for the Atlas Reader interface.")
    save(static, out / f"AtlasNastaliq-{style}.ttf")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--sources", type=Path, required=True)
    parser.add_argument("--out", type=Path, default=Path("assets/fonts"))
    parser.add_argument("--tmp", type=Path, default=None)
    args = parser.parse_args()
    tmp = args.tmp or (args.sources / "_build")
    tmp.mkdir(parents=True, exist_ok=True)
    args.out.mkdir(parents=True, exist_ok=True)
    for style, weight in (("Regular", 400), ("SemiBold", 600)):
        build_naskh(args.sources, args.out, style, weight, tmp)
        build_nastaliq(args.sources, args.out, style, weight)


if __name__ == "__main__":
    main()
