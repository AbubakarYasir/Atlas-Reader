#!/usr/bin/env python3
"""Generate deterministic Atlas N2 A013 image-only PDF fixture."""

from __future__ import annotations

import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "generated"
OUT.mkdir(parents=True, exist_ok=True)
EXPECTED_SHA256 = "f6b1173e34bbd7dab6ad5d91c3000190e1de9f6c156536f06d7f48812bd40021"


def stream_object(content: bytes) -> bytes:
    return b"<< /Length %d >>\nstream\n" % len(content) + content + b"\nendstream"


def build_pdf() -> bytes:
    width = height = 16
    pixels = bytearray()
    for y in range(height):
        for x in range(width):
            if y < 8 and x < 8:
                rgb = (255, 0, 0)
            elif y < 8:
                rgb = (0, 128, 0)
            elif x < 8:
                rgb = (0, 0, 255)
            else:
                rgb = (255, 255, 0)
            pixels.extend(rgb)

    page_content = b"q\n200 0 0 200 0 0 cm\n/Im1 Do\nQ\n"
    objects: dict[int, bytes] = {
        1: b"<< /Type /Catalog /Pages 2 0 R >>",
        2: b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        3: b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 200 200] /Resources << /XObject << /Im1 5 0 R >> >> /Contents 4 0 R >>",
        4: stream_object(page_content),
        5: (
            b"<< /Type /XObject /Subtype /Image /Width 16 /Height 16 /ColorSpace /DeviceRGB /BitsPerComponent 8 /Length %d >>\nstream\n"
            % len(pixels)
        )
        + bytes(pixels)
        + b"\nendstream",
    }

    output = bytearray(b"%PDF-1.7\n%\xe2\xe3\xcf\xd3\n")
    offsets = [0] * 6
    for object_id in range(1, 6):
        offsets[object_id] = len(output)
        output.extend(f"{object_id} 0 obj\n".encode("ascii"))
        output.extend(objects[object_id])
        output.extend(b"\nendobj\n")

    xref = len(output)
    output.extend(b"xref\n0 6\n0000000000 65535 f \n")
    for object_id in range(1, 6):
        output.extend(f"{offsets[object_id]:010d} 00000 n \n".encode("ascii"))
    output.extend(b"trailer\n<< /Size 6 /Root 1 0 R >>\n")
    output.extend(f"startxref\n{xref}\n%%EOF\n".encode("ascii"))
    return bytes(output)


def main() -> None:
    output = OUT / "A013_image_only.pdf"
    data = build_pdf()
    actual = hashlib.sha256(data).hexdigest()
    if actual != EXPECTED_SHA256:
        raise SystemExit(f"A013 SHA-256 mismatch: {actual} != {EXPECTED_SHA256}")
    output.write_bytes(data)
    print(f"Generated {output}")
    print(f"SHA-256: {actual}")


if __name__ == "__main__":
    main()
