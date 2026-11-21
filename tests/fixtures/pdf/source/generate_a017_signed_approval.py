#!/usr/bin/env python3
"""A017: a cryptographically valid approval signature (N5 corpus).

A two-page PDF with a two-item outline, signed (approval, not certification)
with a throwaway self-signed RSA-2048 test certificate generated on each run
("CN=Atlas Test Signer, O=Atlas Reader Test Corpus"). The signature covers the
whole file; any change to the signed bytes breaks it, which is why Atlas must
never change this file in place and offers Save a Copy instead.

NOT deterministic: the key, certificate and signing time are new on each run.
The committed bytes are the fixture; the manifest records their SHA-256 and the
test checks the signature with pyHanko (tools: pyhanko 0.29, cryptography).
The private key is discarded; no secret is stored anywhere.
"""

from __future__ import annotations

import datetime
from pathlib import Path

from cryptography import x509
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.x509.oid import NameOID
from pyhanko.pdf_utils.incremental_writer import IncrementalPdfFileWriter
from pyhanko.sign import signers
from pyhanko.sign.fields import SigFieldSpec, append_signature_field
from pypdf import PdfReader, PdfWriter
from reportlab.lib.pagesizes import letter
from reportlab.pdfgen import canvas
from asn1crypto import keys as asn1_keys, x509 as asn1_x509
from cryptography.hazmat.primitives import serialization

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "generated"


def main() -> None:
    base = OUT / "_A017_base.pdf"
    c = canvas.Canvas(str(base), pagesize=letter, invariant=1)
    c.setTitle("Atlas N5 Signed Approval Fixture")
    c.setAuthor("Atlas Reader Test Corpus")
    for index in range(2):
        c.setFont("Helvetica", 16)
        c.drawString(72, 720, f"Atlas N5 Fixture A017 page {index + 1}")
        c.setFont("Helvetica", 11)
        c.drawString(72, 696, "Signed with a throwaway test certificate. Do not modify in place.")
        c.showPage()
    c.save()
    writer = PdfWriter(clone_from=PdfReader(str(base)))
    writer.add_outline_item("Signed chapter one", 0)
    writer.add_outline_item("Signed chapter two", 1)
    outlined = OUT / "_A017_outlined.pdf"
    with outlined.open("wb") as handle:
        writer.write(handle)
    base.unlink()

    key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
    name = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, "Atlas Test Signer"),
                      x509.NameAttribute(NameOID.ORGANIZATION_NAME, "Atlas Reader Test Corpus")])
    now = datetime.datetime.now(datetime.timezone.utc)
    cert = (x509.CertificateBuilder().subject_name(name).issuer_name(name).public_key(key.public_key())
            .serial_number(x509.random_serial_number()).not_valid_before(now - datetime.timedelta(days=1))
            .not_valid_after(now + datetime.timedelta(days=3650))
            .add_extension(x509.KeyUsage(True, True, False, False, False, False, False, False, False), critical=True)
            .sign(key, hashes.SHA256()))
    signer = signers.SimpleSigner(
        signing_cert=asn1_x509.Certificate.load(cert.public_bytes(serialization.Encoding.DER)),
        signing_key=asn1_keys.PrivateKeyInfo.load(key.private_bytes(
            serialization.Encoding.DER, serialization.PrivateFormat.PKCS8, serialization.NoEncryption())),
        cert_registry=None)
    with outlined.open("rb") as source:
        pdf = IncrementalPdfFileWriter(source)
        append_signature_field(pdf, SigFieldSpec(sig_field_name="AtlasTestSignature", on_page=0, box=(72, 600, 300, 650)))
        out = signers.sign_pdf(pdf, signers.PdfSignatureMetadata(field_name="AtlasTestSignature", reason="Atlas N5 test fixture"),
                               signer=signer)
        (OUT / "A017_signed_approval.pdf").write_bytes(out.getvalue())
        (OUT / "A017_test_signer.pem").write_bytes(cert.public_bytes(serialization.Encoding.PEM))
    outlined.unlink()


if __name__ == "__main__":
    main()
