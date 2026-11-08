#!/usr/bin/env python3
"""A016: readable pages, malformed outline (N5 corpus, docs/QUALITY.md).

Three readable text pages. The outline has one valid item, one item whose
destination points to an object that does not exist, one whose /Dest is the
wrong type, and an item whose /Next points back to the first item (a cycle).
Readers must still open the pages; Atlas must never crash on it, never claim a
bookmark it cannot place, and never write over the file because of it.
Deterministic with the pinned generator dependencies (requirements.txt).
"""

from __future__ import annotations

from pathlib import Path

from pypdf import PdfReader, PdfWriter
from pypdf.generic import ArrayObject, DictionaryObject, NameObject, NumberObject, TextStringObject
from reportlab.lib.pagesizes import letter
from reportlab.pdfgen import canvas

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "generated"
OUT.mkdir(parents=True, exist_ok=True)


def main() -> None:
    base = OUT / "_A016_base.pdf"
    c = canvas.Canvas(str(base), pagesize=letter, invariant=1)
    c.setTitle("Atlas N5 Malformed Outline Fixture")
    c.setAuthor("Atlas Reader Test Corpus")
    for index in range(3):
        c.setFont("Helvetica", 16)
        c.drawString(72, 720, f"Atlas N5 Fixture A016 page {index + 1}")
        c.setFont("Helvetica", 11)
        c.drawString(72, 696, "Readable page; the document outline is deliberately malformed.")
        c.showPage()
    c.save()

    writer = PdfWriter(clone_from=PdfReader(str(base)))
    pages = [page.indirect_reference for page in writer.pages]
    root = DictionaryObject({NameObject("/Type"): NameObject("/Outlines")})
    root_ref = writer._add_object(root)

    def item(title: str, dest) -> tuple[DictionaryObject, object]:
        entry = DictionaryObject({NameObject("/Title"): TextStringObject(title), NameObject("/Parent"): root_ref})
        if dest is not None:
            entry[NameObject("/Dest")] = dest
        return entry, writer._add_object(entry)

    valid, valid_ref = item("Valid chapter", ArrayObject([pages[0], NameObject("/Fit")]))
    missing, missing_ref = item("Missing page", ArrayObject([NumberObject(99), NameObject("/Fit")]))
    wrong, wrong_ref = item("Wrong destination type", NumberObject(7))
    loop, loop_ref = item("Loops back", ArrayObject([pages[2], NameObject("/Fit")]))
    chain = [(valid, valid_ref), (missing, missing_ref), (wrong, wrong_ref), (loop, loop_ref)]
    for (entry, _), (_, next_ref) in zip(chain, chain[1:]):
        entry[NameObject("/Next")] = next_ref
    for (entry, _), (_, prev_ref) in zip(chain[1:], chain):
        entry[NameObject("/Prev")] = prev_ref
    loop[NameObject("/Next")] = valid_ref  # cycle
    root[NameObject("/First")] = valid_ref
    root[NameObject("/Last")] = loop_ref
    root[NameObject("/Count")] = NumberObject(4)
    writer._root_object[NameObject("/Outlines")] = root_ref

    target = OUT / "A016_malformed_outline.pdf"
    with target.open("wb") as handle:
        writer.write(handle)
    base.unlink()


if __name__ == "__main__":
    main()
