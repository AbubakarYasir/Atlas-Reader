#!/usr/bin/env python3
"""Generate deterministic N2 malformed/password security fixtures.

A007 is intentionally truncated after a catalog object so independent/read-engine
parsers must classify it as invalid/malformed rather than silently accepting it.

A008 is a tiny blank one-page PDF using the legacy PDF Standard Security Handler
revision 2 (40-bit RC4). It exists only to qualify password-required / wrong-
password / correct-password control flow in candidate readers. RC4-40 is weak
legacy cryptography and MUST NOT be treated as an Atlas production recommendation.

The fixture passwords are public test data:
  user:  atlas-user
  owner: atlas-owner
"""

from __future__ import annotations

import hashlib
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "generated"

A007_PATH = OUT / "A007_malformed_truncated.pdf"
A008_PATH = OUT / "A008_password_rc4_40.pdf"

A007_EXPECTED_SHA256 = "d6e2fb7962bb233083c110593f6e6968b067c155c713ebb9418c19493e48d79f"
A008_EXPECTED_SHA256 = "62a6b4e332b8296250b9f8d1076e7c5d717a01c54a7471418bc771f0e2d4a08e"

USER_PASSWORD = "atlas-user"
OWNER_PASSWORD = "atlas-owner"
PERMISSIONS = -4

PASSWORD_PADDING = bytes.fromhex(
    "28 BF 4E 5E 4E 75 8A 41 64 00 4E 56 FF FA 01 08 "
    "2E 2E 00 B6 D0 68 3E 80 2F 0C A9 FE 64 53 69 7A"
)


def padded_password(value: str) -> bytes:
    raw = value.encode("latin-1")
    return (raw + PASSWORD_PADDING)[:32]


def rc4(key: bytes, data: bytes) -> bytes:
    """Small deterministic RC4 implementation used only by the legacy test PDF."""
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
        key_byte = state[(state[i] + state[j]) % 256]
        output.append(value ^ key_byte)
    return bytes(output)


def build_a007() -> bytes:
    return (
        b"%PDF-1.7\n"
        b"% Atlas N2 A007 intentionally truncated malformed fixture\n"
        b"1 0 obj\n"
        b"<< /Type /Catalog /Pages 2 0 R >>\n"
        b"endobj\n"
    )


def build_a008() -> bytes:
    # Fixed file ID makes the Standard Security Handler derivation deterministic.
    file_id = hashlib.md5(b"Atlas N2 A008 encrypted fixture").digest()

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


def write_checked(path: Path, data: bytes, expected_sha256: str) -> None:
    actual = hashlib.sha256(data).hexdigest()
    if actual != expected_sha256:
        raise SystemExit(f"{path.name} SHA-256 drift: {actual} != {expected_sha256}")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    print(f"Generated {path}")
    print(f"SHA-256: {actual}")


def main() -> None:
    write_checked(A007_PATH, build_a007(), A007_EXPECTED_SHA256)
    write_checked(A008_PATH, build_a008(), A008_EXPECTED_SHA256)


if __name__ == "__main__":
    main()
