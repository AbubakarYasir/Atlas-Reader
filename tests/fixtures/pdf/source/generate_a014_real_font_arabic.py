#!/usr/bin/env python3
"""Generate Atlas N2 A014 real-font Arabic/Urdu visual fixture.

The font is supplied explicitly by the caller. N2 CI downloads Amiri 1.003 from
its official release, verifies the release archive SHA-256, and passes the
located Amiri-Regular.ttf here. No system font lookup is permitted.

The generated PDF is visual-fidelity evidence. Logical Unicode extraction/search
semantics remain qualified independently by A006.
"""

from __future__ import annotations

import argparse
import hashlib
from datetime import datetime, timezone
from pathlib import Path

from fpdf import FPDF

PAGE_WIDTH = 600
PAGE_HEIGHT = 400


def generate(font_path: Path, output: Path) -> str:
    pdf = FPDF(unit="pt", format=(PAGE_WIDTH, PAGE_HEIGHT))
    pdf.set_creation_date(datetime(2026, 9, 23, 0, 0, 0, tzinfo=timezone.utc))
    pdf.set_author("Atlas Reader N2")
    pdf.set_creator("Atlas N2 deterministic real-font fixture")
    pdf.set_title("A014 real-font Arabic Urdu visual fidelity")
    pdf.set_subject("Synthetic redistributable PDF-engine qualification fixture")
    pdf.set_compression(False)
    pdf.set_auto_page_break(False)
    pdf.add_page()
    pdf.add_font("Amiri", fname=str(font_path))
    pdf.set_font("Amiri", size=30)
    pdf.set_text_color(0, 0, 0)
    pdf.set_text_shaping(True)

    lines = [
        ("مرحبا بالعالم", 28, "rtl", "ara"),
        ("بِسْمِ اللَّهِ الرَّحْمَٰنِ الرَّحِيمِ", 112, "rtl", "ara"),
        ("یہ اردو متن ہے", 196, "rtl", "urd"),
        ("Atlas PDF 123 — العربية English", 280, None, None),
    ]

    for text, y, direction, language in lines:
        pdf.set_xy(30, y)
        if direction is None:
            pdf.set_text_shaping(True)
        else:
            pdf.set_text_shaping(True, direction=direction, script="arab", language=language)
        pdf.cell(w=540, h=48, text=text, align="R" if direction == "rtl" else "L")

    output.parent.mkdir(parents=True, exist_ok=True)
    pdf.output(str(output))
    digest = hashlib.sha256(output.read_bytes()).hexdigest()
    print(f"Generated {output}")
    print(f"SHA-256: {digest}")
    return digest


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--font", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    font_path = args.font.resolve()
    if not font_path.is_file():
        raise SystemExit(f"Font not found: {font_path}")
    generate(font_path, args.output.resolve())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
