#!/usr/bin/env python3
"""Regenerate Atlas N2 synthetic core PDF fixtures deterministically.

This script is development/test tooling only. ReportLab invariant mode fixes
metadata/IDs so repeated runs with the pinned generator dependencies produce the
same fixture bytes. It intentionally uses standard Helvetica for the first
engine-neutral fixtures and does not embed external font files. Arabic/Unicode
fixtures are added separately with explicit redistributable font/source
provenance.
"""

from __future__ import annotations

from pathlib import Path

from pypdf import PdfReader, PdfWriter
from pypdf.constants import PageLabelStyle
from reportlab.lib.pagesizes import A4, letter
from reportlab.pdfgen import canvas

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "generated"
OUT.mkdir(parents=True, exist_ok=True)


def make_a001() -> None:
    path = OUT / "A001_simple_text.pdf"
    c = canvas.Canvas(str(path), pagesize=letter, invariant=1)
    c.setTitle("Atlas N2 Simple Text Fixture")
    c.setAuthor("Atlas Reader Test Corpus")
    c.setSubject("N2 simple text extraction/render/open fixture")
    c.setFont("Helvetica", 18)
    c.drawString(72, 720, "Atlas N2 Fixture A001")
    c.setFont("Helvetica", 12)
    c.drawString(72, 690, "Simple searchable English text.")
    c.drawString(72, 672, "Page index 0; expected display label 1.")
    c.showPage()
    c.save()


def make_a002() -> None:
    temporary = OUT / "_A002_base.pdf"
    c = canvas.Canvas(str(temporary), pagesize=letter, invariant=1)
    c.setTitle("Atlas N2 Geometry Fixture")

    for index, page_size in enumerate([letter, A4, (400, 600)]):
        c.setPageSize(page_size)
        c.setFont("Helvetica", 14)
        c.drawString(40, page_size[1] - 60, f"Atlas N2 Fixture A002 page {index + 1}")
        c.setFont("Helvetica", 10)
        c.drawString(
            40,
            page_size[1] - 80,
            f"Size points: {page_size[0]:.2f} x {page_size[1]:.2f}",
        )
        c.showPage()
    c.save()

    reader = PdfReader(str(temporary))
    writer = PdfWriter()
    for page in reader.pages:
        writer.add_page(page)

    writer.pages[1].rotate(90)
    writer.pages[2].cropbox.lower_left = (20, 20)
    writer.pages[2].cropbox.upper_right = (380, 580)
    writer.set_page_label(0, 1, style=PageLabelStyle.LOWERCASE_ROMAN)
    writer.set_page_label(2, 2, style=PageLabelStyle.DECIMAL, start=1)

    with (OUT / "A002_geometry_labels.pdf").open("wb") as stream:
        writer.write(stream)

    temporary.unlink()


def make_a003() -> None:
    path = OUT / "A003_outlines_links.pdf"
    c = canvas.Canvas(str(path), pagesize=letter, invariant=1)
    c.setTitle("Atlas N2 Outlines and Links Fixture")

    for index in range(3):
        destination = f"p{index + 1}"
        c.bookmarkPage(destination)
        if index == 0:
            c.addOutlineEntry("Chapter 1", destination, level=0, closed=False)
        elif index == 1:
            c.addOutlineEntry("Section 1.1", destination, level=1, closed=False)
        else:
            c.addOutlineEntry("Chapter 2", destination, level=0, closed=False)

        c.setFont("Helvetica", 16)
        c.drawString(72, 720, f"Atlas N2 Fixture A003 page {index + 1}")
        c.setFont("Helvetica", 11)

        if index == 0:
            c.drawString(72, 690, "Internal destination link to page 3:")
            c.linkRect("", "p3", (72, 660, 250, 680), relative=0, thickness=1)
            c.drawString(76, 664, "Go to Chapter 2")
            c.drawString(72, 630, "External URI link:")
            c.linkURL(
                "https://example.com/atlas-n2-fixture",
                (72, 600, 280, 620),
                relative=0,
            )
            c.drawString(76, 604, "https://example.com/atlas-n2-fixture")

        c.showPage()

    c.save()


def main() -> None:
    make_a001()
    make_a002()
    make_a003()
    print(f"Generated deterministic fixtures in {OUT}")
    print("Verify SHA-256 against manifest.json after intentional changes.")


if __name__ == "__main__":
    main()
