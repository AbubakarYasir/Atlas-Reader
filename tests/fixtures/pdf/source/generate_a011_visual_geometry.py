#!/usr/bin/env python3
"""Generate the deterministic Atlas N2 A011 visual/geometry fixture.

A011 is deliberately simple vector content. It is designed to qualify:
- raw MediaBox/CropBox/rotation geometry;
- effective visible page size after crop/rotation;
- 1x/2x raster sampling without requiring byte-identical renderer output;
- annotation-off vs annotation-on rendering behavior.

Generator dependencies are pinned by source/requirements.txt.
"""

from __future__ import annotations

import hashlib
from pathlib import Path

from pypdf import PdfReader, PdfWriter
from reportlab.lib import colors
from reportlab.pdfgen import canvas

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "generated"
OUT.mkdir(parents=True, exist_ok=True)

EXPECTED_SHA256 = "c4862d88c867dd893c742d21e30f922338c771c3caaf2e7e1a2ea4bc8e4de81d"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> None:
    temporary = OUT / "_A011_visual_geometry_base.pdf"
    output = OUT / "A011_visual_geometry.pdf"

    c = canvas.Canvas(str(temporary), pagesize=(200, 300), invariant=1)
    c.setTitle("Atlas N2 Visual Geometry Fixture")
    c.setAuthor("Atlas Reader Test Corpus")

    # Page 0: full 200 x 300 MediaBox/CropBox. Four large solid corner
    # rectangles provide stable interior color samples. The link annotation has
    # a thick magenta border and no page-content border, so annotation-on/off
    # rendering can be detected without engine-specific pixel hashes.
    c.setPageSize((200, 300))
    c.setFillColor(colors.red)
    c.rect(20, 240, 40, 40, stroke=0, fill=1)
    c.setFillColor(colors.green)
    c.rect(140, 240, 40, 40, stroke=0, fill=1)
    c.setFillColor(colors.blue)
    c.rect(20, 20, 40, 40, stroke=0, fill=1)
    c.setFillColor(colors.yellow)
    c.rect(140, 20, 40, 40, stroke=0, fill=1)
    c.setFillColor(colors.black)
    c.setFont("Helvetica", 10)
    c.drawString(68, 205, "A011 PAGE 1")
    c.linkURL(
        "https://example.com/atlas-a011",
        (70, 125, 130, 175),
        relative=0,
        thickness=6,
        color=colors.magenta,
    )
    c.showPage()

    # Page 1: 300 x 200 MediaBox, later cropped to [50,25,250,175]. The red
    # block is intentionally outside the final crop. Magenta/cyan blocks are
    # safely inside the crop for stable visible-region samples.
    c.setPageSize((300, 200))
    c.setFillColor(colors.red)
    c.rect(10, 10, 30, 30, stroke=0, fill=1)
    c.setFillColor(colors.magenta)
    c.rect(60, 135, 30, 30, stroke=0, fill=1)
    c.setFillColor(colors.cyan)
    c.rect(210, 35, 30, 30, stroke=0, fill=1)
    c.setFillColor(colors.black)
    c.setFont("Helvetica", 10)
    c.drawString(110, 95, "A011 PAGE 2")
    c.showPage()

    # Page 2: same quadrant anchors as page 0, later rotated 90 degrees.
    c.setPageSize((200, 300))
    c.setFillColor(colors.red)
    c.rect(20, 240, 40, 40, stroke=0, fill=1)
    c.setFillColor(colors.green)
    c.rect(140, 240, 40, 40, stroke=0, fill=1)
    c.setFillColor(colors.blue)
    c.rect(20, 20, 40, 40, stroke=0, fill=1)
    c.setFillColor(colors.yellow)
    c.rect(140, 20, 40, 40, stroke=0, fill=1)
    c.setFillColor(colors.black)
    c.setFont("Helvetica", 10)
    c.drawString(68, 145, "A011 PAGE 3")
    c.showPage()
    c.save()

    reader = PdfReader(str(temporary))
    writer = PdfWriter()
    for page in reader.pages:
        writer.add_page(page)

    writer.pages[1].cropbox.lower_left = (50, 25)
    writer.pages[1].cropbox.upper_right = (250, 175)
    writer.pages[2].rotate(90)

    with output.open("wb") as stream:
        writer.write(stream)

    temporary.unlink()

    actual = sha256(output)
    if actual != EXPECTED_SHA256:
        raise SystemExit(f"A011 SHA-256 mismatch: {actual} != {EXPECTED_SHA256}")

    print(f"Generated {output}")
    print(f"SHA-256: {actual}")


if __name__ == "__main__":
    main()
