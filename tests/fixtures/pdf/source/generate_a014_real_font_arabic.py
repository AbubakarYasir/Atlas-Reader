#!/usr/bin/env python3
"""Generate Atlas N2 A014 real-font Arabic/Urdu visual fixture.

The fonts are supplied explicitly by the caller. N2 CI downloads a pinned Noto
Naskh Arabic release and a pinned Noto Nastaliq Urdu font, verifies their
SHA-256 identities, and passes both files here. No system font lookup is
permitted. Arabic and Urdu intentionally use different typefaces: rendering
Urdu with the Arabic fixture font previously produced a technically shaped but
owner-rejected visual oracle.

The generated PDF is visual-fidelity evidence. Logical Unicode extraction/search
semantics remain qualified independently by A006.
"""

from __future__ import annotations

import argparse
import hashlib
from datetime import datetime, timezone
from pathlib import Path

import arabic_reshaper
from bidi.algorithm import get_display
from fpdf import FPDF

PAGE_WIDTH = 600
PAGE_HEIGHT = 480

ARABIC_RESHAPER = arabic_reshaper.ArabicReshaper(
    configuration={
        "delete_harakat": False,
        "shift_harakat_position": True,
        "support_ligatures": True,
    }
)


def joined_rtl_arabic(text: str) -> str:
    """Return explicit contextual forms in visual order for PDF placement."""

    return get_display(ARABIC_RESHAPER.reshape(text))


def generate(arabic_font_path: Path, urdu_font_path: Path, output: Path) -> str:
    pdf = FPDF(unit="pt", format=(PAGE_WIDTH, PAGE_HEIGHT))
    pdf.set_creation_date(datetime(2026, 9, 23, 0, 0, 0, tzinfo=timezone.utc))
    pdf.set_author("Atlas Reader N2")
    pdf.set_creator("Atlas N2 deterministic real-font fixture")
    pdf.set_title("A014 readable Arabic and Urdu visual fidelity")
    pdf.set_subject("Synthetic redistributable PDF-engine qualification fixture")
    pdf.set_compression(False)
    pdf.set_auto_page_break(False)
    pdf.add_page()
    pdf.add_font("NotoNaskhArabic", fname=str(arabic_font_path))
    pdf.add_font("NotoNastaliqUrdu", fname=str(urdu_font_path))
    pdf.set_text_color(0, 0, 0)

    lines = [
        ("NotoNaskhArabic", 30, "مرحبا بالعالم", 20, "rtl", "ara"),
        ("NotoNaskhArabic", 30, "بِسْمِ اللَّهِ الرَّحْمَٰنِ الرَّحِيمِ", 105, "rtl", "ara"),
        ("NotoNastaliqUrdu", 30, "یہ اردو متن ہے", 215, "rtl", "urd"),
    ]

    for font_family, font_size, text, y, direction, language in lines:
        pdf.set_font(font_family, size=font_size)
        pdf.set_xy(30, y)
        if language == "ara":
            # Emit explicit contextual forms so the PDF fixture itself cannot
            # regress to isolated Arabic glyphs while both readers still agree.
            pdf.set_text_shaping(False)
            text = joined_rtl_arabic(text)
        elif direction is None:
            pdf.set_text_shaping(True)
        else:
            pdf.set_text_shaping(True, direction=direction, script="arab", language=language)
        pdf.cell(w=540, h=48, text=text, align="R" if direction == "rtl" else "L")

    # A006 owns logical bidi extraction/search. This visual fixture places
    # pinned Arabic glyphs and Latin core-font glyphs on one line explicitly so
    # neither script silently disappears through unsupported font fallback.
    pdf.set_text_shaping(False)
    pdf.set_font("Helvetica", size=27)
    pdf.set_xy(45, 345)
    pdf.cell(w=190, h=48, text="Atlas PDF 123", align="L")
    pdf.set_font("NotoNaskhArabic", size=27)
    pdf.set_text_shaping(False)
    pdf.set_xy(250, 345)
    pdf.cell(w=155, h=48, text=joined_rtl_arabic("العربية"), align="R")
    pdf.set_text_shaping(False)
    pdf.set_font("Helvetica", size=27)
    pdf.set_xy(440, 345)
    pdf.cell(w=115, h=48, text="English", align="L")

    output.parent.mkdir(parents=True, exist_ok=True)
    pdf.output(str(output))
    digest = hashlib.sha256(output.read_bytes()).hexdigest()
    print(f"Generated {output}")
    print(f"SHA-256: {digest}")
    return digest


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--arabic-font", type=Path, required=True)
    parser.add_argument("--urdu-font", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    arabic_font_path = args.arabic_font.resolve()
    urdu_font_path = args.urdu_font.resolve()
    if not arabic_font_path.is_file():
        raise SystemExit(f"Arabic font not found: {arabic_font_path}")
    if not urdu_font_path.is_file():
        raise SystemExit(f"Urdu font not found: {urdu_font_path}")
    generate(arabic_font_path, urdu_font_path, args.output.resolve())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
