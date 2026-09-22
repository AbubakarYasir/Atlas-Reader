#!/usr/bin/env python3
"""Generate deterministic A010 certification-structure fixture.

A010 deliberately contains the PDF structures used to represent a signature
field and DocMDP certification policy, but its /Contents bytes are synthetic and
its /ByteRange is a sentinel. It is therefore NOT a cryptographically valid
signed PDF. The fixture exists only to qualify structural detection and Atlas's
pre-mutation safety interlock.
"""

from __future__ import annotations

import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "generated" / "A010_certification_structure.pdf"
EXPECTED_SHA256 = "95bc8daabea7d46bb70432bb65fc2fd8af3ad6e590f0fbf539e443930ce8f668"


def build_pdf() -> bytes:
    objects: dict[int, bytes] = {
        1: b"<< /Type /Catalog /Pages 2 0 R /AcroForm 5 0 R /Perms << /DocMDP 7 0 R >> >>",
        2: b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        3: b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Resources << >> /Contents 4 0 R /Annots [6 0 R] >>",
        4: b"<< /Length 0 >>\nstream\n\nendstream",
        5: b"<< /Fields [6 0 R] /SigFlags 3 >>",
        6: b"<< /Type /Annot /Subtype /Widget /FT /Sig /T (Atlas Certification) /Rect [0 0 0 0] /V 7 0 R /P 3 0 R /F 132 >>",
        8: b"<< /Type /SigRef /TransformMethod /DocMDP /DigestMethod /SHA256 /TransformParams 9 0 R >>",
        9: b"<< /Type /TransformParams /P 2 /V /1.2 >>",
    }
    contents = b"00" * 128
    objects[7] = (
        b"<< /Type /Sig /Filter /Adobe.PPKLite /SubFilter /adbe.pkcs7.detached "
        b"/ByteRange [0 0 0 0] /Contents <" + contents + b"> "
        b"/Reason (Atlas synthetic certification structure; not cryptographically valid) "
        b"/M (D:20260922210000Z) /Reference [8 0 R] >>"
    )

    output = bytearray(b"%PDF-1.7\n%ATLS\n")
    offsets: dict[int, int] = {0: 0}
    for number in range(1, 10):
        offsets[number] = len(output)
        output.extend(f"{number} 0 obj\n".encode("ascii"))
        output.extend(objects[number])
        output.extend(b"\nendobj\n")

    startxref = len(output)
    output.extend(b"xref\n0 10\n0000000000 65535 f \n")
    for number in range(1, 10):
        output.extend(f"{offsets[number]:010d} 00000 n \n".encode("ascii"))
    output.extend(
        (
            "trailer\n<< /Size 10 /Root 1 0 R >>\n"
            f"startxref\n{startxref}\n%%EOF\n"
        ).encode("ascii")
    )
    return bytes(output)


def main() -> None:
    data = build_pdf()
    actual = hashlib.sha256(data).hexdigest()
    if actual != EXPECTED_SHA256:
        raise SystemExit(f"A010 SHA-256 drift: {actual} != {EXPECTED_SHA256}")
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_bytes(data)
    print(f"Generated {OUT}")
    print(f"SHA-256: {actual}")


if __name__ == "__main__":
    main()
