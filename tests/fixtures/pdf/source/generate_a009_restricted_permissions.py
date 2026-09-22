#!/usr/bin/env python3
"""Generate deterministic N2 restricted-permission encryption fixture A009.

A009 uses the legacy PDF Standard Security Handler revision 2 (40-bit RC4)
only because it is tiny and deterministic enough for permission/password-state
qualification. The weak cipher is test data, not an Atlas production
recommendation.

Public test credentials:
  user:  atlas-user
  owner: atlas-owner

The permission integer is -64 (0xFFFFFFC0): all revision-2 permission bits
(print, modify, copy/extract, annotations) are cleared.
"""

from __future__ import annotations

import hashlib
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "generated"
A009_PATH = OUT / "A009_restricted_permissions_rc4_40.pdf"
A009_EXPECTED_SHA256 = "0db18b77d4dc8f43ee728fd22eb897f1072f630444a41fef3a8fff12b83400b2"

USER_PASSWORD = "atlas-user"
OWNER_PASSWORD = "atlas-owner"
PERMISSIONS = -64

PASSWORD_PADDING = bytes.fromhex(
    "28 BF 4E 5E 4E 75 8A 41 64 00 4E 56 FF FA 01 08 "
    "2E 2E 00 B6 D0 68 3E 80 2F 0C A9 FE 64 53 69 7A"
)


def padded_password(value: str) -> bytes:
    raw = value.encode("latin-1")
    return (raw + PASSWORD_PADDING)[:32]


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


def build_a009() -> bytes:
    file_id = hashlib.md5(b"Atlas N2 A009 restricted permissions fixture").digest()

    owner_key = hashlib.md5(padded_password(OWNER_PASSWORD)).digest()[:5]
    owner_entry = rc4(owner_key, padded_password(USER_PASSWORD))

    digest = hashlib.md5()
    digest.update(padded_password(USER_PASSWORD))
    digest.update(owner_entry)
    digest.update(struct.pack("<i", PERMISSIONS))
    digest.update(file_id)
    document_key = digest.digest()[:5]
    user_entry = rc4(document_key, PASSWORD_PADDING)

    objects: dict[int, bytes] = {
        1: b"<< /Type /Catalog /Pages 2 0 R >>",
        2: b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        3: b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] >>",
        4: (
            f"<< /Filter /Standard /V 1 /R 2 "
            f"/O <{owner_entry.hex().upper()}> "
            f"/U <{user_entry.hex().upper()}> /P {PERMISSIONS} >>"
        ).encode("ascii"),
    }

    output = bytearray(b"%PDF-1.4\n%\xE2\xE3\xCF\xD3\n")
    offsets: dict[int, int] = {0: 0}
    for object_number in range(1, 5):
        offsets[object_number] = len(output)
        output.extend(f"{object_number} 0 obj\n".encode("ascii"))
        output.extend(objects[object_number])
        output.extend(b"\nendobj\n")

    startxref = len(output)
    output.extend(b"xref\n0 5\n0000000000 65535 f \n")
    for object_number in range(1, 5):
        output.extend(f"{offsets[object_number]:010d} 00000 n \n".encode("ascii"))

    id_hex = file_id.hex().upper()
    output.extend(
        (
            "trailer\n"
            f"<< /Size 5 /Root 1 0 R /Encrypt 4 0 R /ID [<{id_hex}><{id_hex}>] >>\n"
            f"startxref\n{startxref}\n%%EOF\n"
        ).encode("ascii")
    )
    return bytes(output)


def main() -> None:
    data = build_a009()
    actual = hashlib.sha256(data).hexdigest()
    if actual != A009_EXPECTED_SHA256:
        raise SystemExit(
            f"{A009_PATH.name} SHA-256 drift: {actual} != {A009_EXPECTED_SHA256}"
        )
    A009_PATH.parent.mkdir(parents=True, exist_ok=True)
    A009_PATH.write_bytes(data)
    print(f"Generated {A009_PATH}")
    print(f"SHA-256: {actual}")
    print(f"Permissions P: {PERMISSIONS}")


if __name__ == "__main__":
    main()
