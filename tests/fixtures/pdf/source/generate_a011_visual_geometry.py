#!/usr/bin/env python3
"""Generate the deterministic Atlas N2 A011 visual/geometry fixture.

A011 is deliberately emitted from explicit PDF syntax rather than ReportLab or
pypdf serialization. The latter is useful for independent validation but its
output bytes may differ across Python/runtime versions. Explicit LF-delimited
objects and a computed xref make this fixture byte-identical across platforms.

The fixture qualifies:
- raw MediaBox/CropBox/rotation geometry;
- effective visible page size after crop/rotation;
- 1x/2x semantic raster sampling without requiring identical renderer hashes;
- annotation-off vs annotation-on rendering behavior.
"""

from __future__ import annotations

import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "generated"
OUT.mkdir(parents=True, exist_ok=True)

EXPECTED_SHA256 = "b25d6b6716fbbcf095cbd68bcf57913d4e14bca3e64be19f513c35fd3ddadd29"


def stream_object(content: bytes) -> bytes:
    return b"<< /Length %d >>\nstream\n" % len(content) + content + b"endstream"


def build_pdf() -> bytes:
    page_0_content = b"""q
1 0 0 rg
20 240 40 40 re f
0 0.501961 0 rg
140 240 40 40 re f
0 0 1 rg
20 20 40 40 re f
1 1 0 rg
140 20 40 40 re f
0 0 0 rg
BT /F1 10 Tf 68 205 Td (A011 PAGE 1) Tj ET
Q
"""

    page_1_content = b"""q
1 0 0 rg
10 10 30 30 re f
1 0 1 rg
60 135 30 30 re f
0 1 1 rg
210 35 30 30 re f
0 0 0 rg
BT /F1 10 Tf 110 95 Td (A011 PAGE 2) Tj ET
Q
"""

    page_2_content = b"""q
1 0 0 rg
20 240 40 40 re f
0 0.501961 0 rg
140 240 40 40 re f
0 0 1 rg
20 20 40 40 re f
1 1 0 rg
140 20 40 40 re f
0 0 0 rg
BT /F1 10 Tf 68 145 Td (A011 PAGE 3) Tj ET
Q
"""

    # Link annotations are not guaranteed to synthesize a visible appearance
    # from /Border and /C alone. Give the test annotation an explicit normal
    # appearance so FPDF_ANNOT / Qt RenderFlag::Annotations have a concrete,
    # renderer-independent object to draw.
    annotation_appearance = b"""q
1 0 1 RG
6 w
3 3 54 44 re S
Q
"""

    objects: dict[int, bytes] = {
        1: b"<< /Type /Catalog /Pages 2 0 R >>",
        2: b"<< /Type /Pages /Kids [3 0 R 7 0 R 9 0 R] /Count 3 >>",
        3: b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 200 300] /Resources << /Font << /F1 6 0 R >> >> /Contents 4 0 R /Annots [5 0 R] >>",
        4: stream_object(page_0_content),
        5: b"<< /Type /Annot /Subtype /Link /Rect [70 125 130 175] /Border [0 0 6] /C [1 0 1] /AP << /N 11 0 R >> /A << /S /URI /URI (https://example.com/atlas-a011) >> >>",
        6: b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>",
        7: b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 300 200] /CropBox [50 25 250 175] /Resources << /Font << /F1 6 0 R >> >> /Contents 8 0 R >>",
        8: stream_object(page_1_content),
        9: b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 200 300] /Rotate 90 /Resources << /Font << /F1 6 0 R >> >> /Contents 10 0 R >>",
        10: stream_object(page_2_content),
        11: b"<< /Type /XObject /Subtype /Form /BBox [0 0 60 50] /Resources << >> /Length %d >>\nstream\n"
        % len(annotation_appearance)
        + annotation_appearance
        + b"endstream",
    }

    output = bytearray(b"%PDF-1.7\n%\xe2\xe3\xcf\xd3\n")
    offsets = [0] * 12
    for object_id in range(1, 12):
        offsets[object_id] = len(output)
        output.extend(f"{object_id} 0 obj\n".encode("ascii"))
        output.extend(objects[object_id])
        output.extend(b"\nendobj\n")

    xref_offset = len(output)
    output.extend(b"xref\n0 12\n0000000000 65535 f \n")
    for object_id in range(1, 12):
        output.extend(f"{offsets[object_id]:010d} 00000 n \n".encode("ascii"))
    output.extend(b"trailer\n<< /Size 12 /Root 1 0 R >>\n")
    output.extend(f"startxref\n{xref_offset}\n%%EOF\n".encode("ascii"))
    return bytes(output)


def main() -> None:
    output = OUT / "A011_visual_geometry.pdf"
    data = build_pdf()
    actual = hashlib.sha256(data).hexdigest()
    if actual != EXPECTED_SHA256:
        raise SystemExit(f"A011 SHA-256 mismatch: {actual} != {EXPECTED_SHA256}")

    output.write_bytes(data)
    print(f"Generated {output}")
    print(f"SHA-256: {actual}")


if __name__ == "__main__":
    main()
