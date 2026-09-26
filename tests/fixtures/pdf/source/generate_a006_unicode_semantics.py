#!/usr/bin/env python3
"""Generate A006, the deterministic N2 logical-Unicode semantics fixture.

This fixture intentionally uses a synthetic Type3 font with a ToUnicode CMap.
It tests text extraction/search/outline Unicode semantics without depending on
font shaping, system fonts, or redistributable font binaries. Its glyphs are
simple rectangles and MUST NOT be used as Arabic/Urdu visual-fidelity evidence.

Pinned generator dependency: pypdf==5.9.0.
"""

from __future__ import annotations

import hashlib
from pathlib import Path

from pypdf import PdfWriter
from pypdf.generic import (
    ArrayObject,
    DecodedStreamObject,
    DictionaryObject,
    FloatObject,
    NameObject,
    NumberObject,
)

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "generated" / "A006_unicode_semantics.pdf"
EXPECTED_SHA256 = "efe3aa5f9538a8a18af71e4517c9f1e10980db394107c0e16141b03ec7058f0b"

RUNS: list[list[tuple[str, str]]] = [
    [("مرحبا بالعالم", "rtl")],
    [("بِسْمِ اللَّهِ الرَّحْمَٰنِ الرَّحِيمِ", "rtl")],
    [("Atlas PDF 123", "ltr"), ("مرحبا بالعالم", "rtl")],
    [("یہ اردو متن ہے", "rtl")],
]


def stream(writer: PdfWriter, data: bytes):
    obj = DecodedStreamObject()
    obj.set_data(data)
    return writer._add_object(obj)  # pypdf low-level fixture construction


def build() -> bytes:
    writer = PdfWriter()

    codepoints: list[str] = []
    for page_runs in RUNS:
        for text, _direction in page_runs:
            for char in text:
                if char not in codepoints:
                    codepoints.append(char)

    code_for = {char: index + 1 for index, char in enumerate(codepoints)}

    char_procs = DictionaryObject()
    widths = []
    differences = [NumberObject(1)]

    for char, code in code_for.items():
        glyph_name = NameObject(f"/g{code}")
        glyph = DecodedStreamObject()
        if char == " ":
            glyph.set_data(b"300 0 d0\n")
            width = 300
        else:
            glyph.set_data(b"600 0 0 0 500 700 d1\n50 50 400 600 re f\n")
            width = 600
        char_procs[glyph_name] = writer._add_object(glyph)
        differences.append(glyph_name)
        widths.append(NumberObject(width))

    cmap_lines = [
        "/CIDInit /ProcSet findresource begin",
        "12 dict begin",
        "begincmap",
        "/CIDSystemInfo << /Registry (Atlas) /Ordering (Unicode) /Supplement 0 >> def",
        "/CMapName /AtlasUnicode def",
        "/CMapType 2 def",
        "1 begincodespacerange",
        "<01> <FF>",
        "endcodespacerange",
        f"{len(code_for)} beginbfchar",
    ]
    for char, code in code_for.items():
        cmap_lines.append(f"<{code:02X}> <{char.encode('utf-16-be').hex().upper()}>")
    cmap_lines += [
        "endbfchar",
        "endcmap",
        "CMapName currentdict /CMap defineresource pop",
        "end",
        "end",
    ]
    to_unicode = stream(writer, ("\n".join(cmap_lines) + "\n").encode("ascii"))

    font = DictionaryObject(
        {
            NameObject("/Type"): NameObject("/Font"),
            NameObject("/Subtype"): NameObject("/Type3"),
            NameObject("/FontBBox"): ArrayObject(
                [NumberObject(0), NumberObject(0), NumberObject(600), NumberObject(700)]
            ),
            NameObject("/FontMatrix"): ArrayObject(
                [
                    FloatObject(0.001),
                    FloatObject(0),
                    FloatObject(0),
                    FloatObject(0.001),
                    FloatObject(0),
                    FloatObject(0),
                ]
            ),
            NameObject("/CharProcs"): char_procs,
            NameObject("/Encoding"): DictionaryObject(
                {
                    NameObject("/Type"): NameObject("/Encoding"),
                    NameObject("/Differences"): ArrayObject(differences),
                }
            ),
            NameObject("/FirstChar"): NumberObject(1),
            NameObject("/LastChar"): NumberObject(len(code_for)),
            NameObject("/Widths"): ArrayObject(widths),
            NameObject("/Resources"): DictionaryObject(),
            NameObject("/ToUnicode"): to_unicode,
        }
    )
    font_ref = writer._add_object(font)

    for page_runs in RUNS:
        page = writer.add_blank_page(width=612, height=792)
        page[NameObject("/Resources")] = DictionaryObject(
            {NameObject("/Font"): DictionaryObject({NameObject("/F1"): font_ref})}
        )

        commands: list[str] = []
        y = 720
        for text, direction in page_runs:
            chars = list(text)
            if direction == "rtl":
                # The synthetic content stream is stored in visual order. Map
                # one PDF code to one Unicode scalar so bidi extraction can
                # restore logical order without moving combining marks ahead
                # of their base letters.
                chars.reverse()
            encoded = bytes(code_for[char] for char in chars)
            commands.append(
                f"BT /F1 14 Tf 72 {y} Td <{encoded.hex().upper()}> Tj ET"
            )
            y -= 40

        page[NameObject("/Contents")] = stream(
            writer, ("\n".join(commands) + "\n").encode("ascii")
        )

    arabic_parent = writer.add_outline_item("العربية", page_number=0)
    writer.add_outline_item("مُشَكَّل", page_number=1, parent=arabic_parent)
    writer.add_outline_item("Mixed العربية English", page_number=2)
    writer.add_outline_item("اردو", page_number=3)

    writer.add_metadata(
        {
            "/Title": "Atlas N2 Unicode Semantics Fixture",
            "/Author": "Atlas Reader Test Corpus",
            "/Subject": (
                "Logical Unicode extraction/search fixture; glyphs are synthetic, "
                "not shaping fidelity"
            ),
        }
    )

    from io import BytesIO

    buffer = BytesIO()
    writer.write(buffer)
    return buffer.getvalue()


def main() -> None:
    data = build()
    actual = hashlib.sha256(data).hexdigest()
    if actual != EXPECTED_SHA256:
        raise SystemExit(f"A006 SHA-256 drift: {actual} != {EXPECTED_SHA256}")

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_bytes(data)
    print(f"Generated {OUT}")
    print(f"SHA-256: {actual}")


if __name__ == "__main__":
    main()
