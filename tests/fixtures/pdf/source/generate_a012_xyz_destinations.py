#!/usr/bin/env python3
"""Generate deterministic A012 explicit /XYZ destination fixture for Atlas N2."""

from __future__ import annotations

import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "generated"
OUT.mkdir(parents=True, exist_ok=True)
EXPECTED_SHA256 = "0c710f3f6e4457a44b8d2dbaa42a476a5c5b429764a0a7569002b66222ef069d"


def stream_object(content: bytes) -> bytes:
    return b"<< /Length %d >>\nstream\n" % len(content) + content + b"endstream"


def build_pdf() -> bytes:
    source = b"BT /F1 12 Tf 30 260 Td (A012 SOURCE) Tj ET\n"
    normal = (
        b"BT /F1 12 Tf 30 260 Td (A012 NORMAL TARGET) Tj ET\n"
        b"1 0 0 rg 35 245 10 10 re f\n"
    )
    rotated = (
        b"BT /F1 12 Tf 30 260 Td (A012 ROTATED TARGET) Tj ET\n"
        b"0 0 1 rg 35 245 10 10 re f\n"
    )

    objects: dict[int, bytes] = {
        1: b"<< /Type /Catalog /Pages 2 0 R >>",
        2: b"<< /Type /Pages /Kids [3 0 R 7 0 R 9 0 R] /Count 3 >>",
        3: b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 200 300] /Resources << /Font << /F1 6 0 R >> >> /Contents 4 0 R /Annots [5 0 R 11 0 R] >>",
        4: stream_object(source),
        5: b"<< /Type /Annot /Subtype /Link /Rect [20 200 100 220] /Border [0 0 1] /Dest [7 0 R /XYZ 40 250 null] >>",
        6: b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>",
        7: b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 200 300] /Resources << /Font << /F1 6 0 R >> >> /Contents 8 0 R >>",
        8: stream_object(normal),
        9: b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 200 300] /Rotate 90 /Resources << /Font << /F1 6 0 R >> >> /Contents 10 0 R >>",
        10: stream_object(rotated),
        11: b"<< /Type /Annot /Subtype /Link /Rect [20 160 100 180] /Border [0 0 1] /Dest [9 0 R /XYZ 40 250 null] >>",
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
    data = build_pdf()
    actual = hashlib.sha256(data).hexdigest()
    if actual != EXPECTED_SHA256:
        raise SystemExit(f"A012 SHA-256 mismatch: {actual} != {EXPECTED_SHA256}")
    output = OUT / "A012_xyz_destinations.pdf"
    output.write_bytes(data)
    print(f"Generated {output}")
    print(f"SHA-256: {actual}")


if __name__ == "__main__":
    main()
