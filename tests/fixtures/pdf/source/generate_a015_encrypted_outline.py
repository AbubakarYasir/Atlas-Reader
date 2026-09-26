#!/usr/bin/env python3
"""Generate deterministic N2 A015 encrypted writable-outline fixture.

A015 uses Standard Security Handler V1/R2 RC4-40 solely for compact deterministic
qualification of encrypted rewrite preservation. Its public test credentials are:

  user:  atlas-user
  owner: atlas-owner

Permissions are P=-4 (the permissive R2 state already qualified by A008). The
fixture contains one encrypted outline title and no signed/certified structure.
RC4-40 is weak legacy test data and is never a production crypto recommendation.
"""

from __future__ import annotations

import hashlib
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "generated"
OUTPUT = OUT / "A015_encrypted_outline_rc4_40.pdf"
EXPECTED_SHA256 = "f01c4422597f015eedd3c4216ba0097de3f6d90ee7f943d9a703a1ad84954694"

USER_PASSWORD = "atlas-user"
OWNER_PASSWORD = "atlas-owner"
PERMISSIONS = -4

PASSWORD_PADDING = bytes.fromhex(
    "28 BF 4E 5E 4E 75 8A 41 64 00 4E 56 FF FA 01 08 "
    "2E 2E 00 B6 D0 68 3E 80 2F 0C A9 FE 64 53 69 7A"
)


def padded_password(value: str) -> bytes:
    return (value.encode("latin-1") + PASSWORD_PADDING)[:32]


def rc4(key: bytes, data: bytes) -> bytes:
    state = list(range(256))
    j = 0
    for i in range(256):
        j = (j + state[i] + key[i % len(key)]) % 256
        state[i], state[j] = state[j], state[i]
    output = bytearray()
    i = 0
    j = 0
    for value in data:
        i = (i + 1) % 256
        j = (j + state[i]) % 256
        state[i], state[j] = state[j], state[i]
        output.append(value ^ state[(state[i] + state[j]) % 256])
    return bytes(output)


def object_key(document_key: bytes, object_number: int, generation: int = 0) -> bytes:
    suffix = object_number.to_bytes(3, "little") + generation.to_bytes(2, "little")
    return hashlib.md5(document_key + suffix).digest()[: min(len(document_key) + 5, 16)]


def build_pdf() -> bytes:
    file_id = hashlib.md5(b"Atlas N2 A015 encrypted outline fixture").digest()

    owner_key = hashlib.md5(padded_password(OWNER_PASSWORD)).digest()[:5]
    owner_entry = rc4(owner_key, padded_password(USER_PASSWORD))

    digest = hashlib.md5()
    digest.update(padded_password(USER_PASSWORD))
    digest.update(owner_entry)
    digest.update(struct.pack("<i", PERMISSIONS))
    digest.update(file_id)
    document_key = digest.digest()[:5]
    user_entry = rc4(document_key, PASSWORD_PADDING)

    encrypted_title = rc4(object_key(document_key, 5), b"Encrypted Root")

    objects: dict[int, bytes] = {
        1: b"<< /Type /Catalog /Pages 2 0 R /Outlines 4 0 R /PageMode /UseOutlines >>",
        2: b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        3: b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Resources << >> >>",
        4: b"<< /Type /Outlines /First 5 0 R /Last 5 0 R /Count 1 >>",
        5: (
            b"<< /Title <"
            + encrypted_title.hex().upper().encode("ascii")
            + b"> /Parent 4 0 R /Dest [3 0 R /XYZ 0 792 null] >>"
        ),
        6: (
            f"<< /Filter /Standard /V 1 /R 2 "
            f"/O <{owner_entry.hex().upper()}> "
            f"/U <{user_entry.hex().upper()}> /P {PERMISSIONS} >>"
        ).encode("ascii"),
    }

    output = bytearray(b"%PDF-1.4\n%\xE2\xE3\xCF\xD3\n")
    offsets: dict[int, int] = {}
    for object_number in range(1, 7):
        offsets[object_number] = len(output)
        output.extend(f"{object_number} 0 obj\n".encode("ascii"))
        output.extend(objects[object_number])
        output.extend(b"\nendobj\n")

    startxref = len(output)
    output.extend(b"xref\n0 7\n0000000000 65535 f \n")
    for object_number in range(1, 7):
        output.extend(f"{offsets[object_number]:010d} 00000 n \n".encode("ascii"))

    id_hex = file_id.hex().upper()
    output.extend(
        (
            "trailer\n"
            f"<< /Size 7 /Root 1 0 R /Encrypt 6 0 R /ID [<{id_hex}><{id_hex}>] >>\n"
            f"startxref\n{startxref}\n%%EOF\n"
        ).encode("ascii")
    )
    return bytes(output)


def main() -> None:
    data = build_pdf()
    actual = hashlib.sha256(data).hexdigest()
    if actual != EXPECTED_SHA256:
        raise SystemExit(f"A015 SHA-256 drift: {actual} != {EXPECTED_SHA256}")
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_bytes(data)
    print(f"Generated {OUTPUT}")
    print(f"SHA-256: {actual}")


if __name__ == "__main__":
    main()
