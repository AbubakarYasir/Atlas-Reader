"""A018: synthetic self-signed PDF, CC0; no real identity or private key on disk.

Generation only: pypdf 6.12.2, cryptography 46.0.5 (existing developer tools).
Verification uses independent OpenSSL CMS and needs only Python's standard library.
The certificate is intentionally not publicly trusted; verify mathematical integrity,
never identity trust. Regeneration uses a fresh ephemeral key and changes the hash.
See https://cryptography.io/en/latest/hazmat/primitives/asymmetric/serialization/
"""
import argparse
from datetime import datetime, timezone
from io import BytesIO
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PDF = ROOT / "generated/A018_self_signed.pdf"


def generate():
    from cryptography import x509
    from cryptography.hazmat.primitives import hashes, serialization
    from cryptography.hazmat.primitives.asymmetric import rsa
    from cryptography.hazmat.primitives.serialization import pkcs7
    from cryptography.x509.oid import NameOID
    from pypdf import PdfWriter
    from pypdf.generic import (ArrayObject, ByteStringObject, DictionaryObject,
                              NameObject, NumberObject, TextStringObject)

    if PDF.exists():
        raise RuntimeError("Preserve the existing fixture; choose a new ID to regenerate.")
    writer = PdfWriter(clone_from=ROOT / "generated/A001_simple_text.pdf")
    sig = DictionaryObject({NameObject('/Type'): NameObject('/Sig'),
        NameObject('/Filter'): NameObject('/Adobe.PPKLite'),
        NameObject('/SubFilter'): NameObject('/adbe.pkcs7.detached'),
        NameObject('/ByteRange'): ArrayObject([NumberObject(9999999999)] * 4),
        NameObject('/Contents'): ByteStringObject(bytes(8192)),
        NameObject('/Name'): TextStringObject('Atlas synthetic test signer')})
    field = writer._add_object(DictionaryObject({NameObject('/FT'): NameObject('/Sig'),
        NameObject('/Type'): NameObject('/Annot'), NameObject('/Subtype'): NameObject('/Widget'),
        NameObject('/T'): TextStringObject('AtlasFixtureSignature'),
        NameObject('/Rect'): ArrayObject([NumberObject(0)] * 4),
        NameObject('/V'): writer._add_object(sig), NameObject('/P'): writer.pages[0].indirect_reference}))
    writer.pages[0][NameObject('/Annots')] = ArrayObject([field])
    writer._root_object[NameObject('/AcroForm')] = writer._add_object(DictionaryObject({
        NameObject('/Fields'): ArrayObject([field]), NameObject('/SigFlags'): NumberObject(3)}))
    stream = BytesIO()
    writer.write(stream)
    data = stream.getvalue()
    content = re.search(rb'/Contents\s*(<0{16384}>)', data)
    start, end = content.span(1)
    placeholder = b'[ 9999999999 9999999999 9999999999 9999999999 ]'
    actual = f'[ 0 {start} {end} {len(data)-end} ]'.encode().ljust(len(placeholder))
    assert placeholder in data
    data = data.replace(placeholder, actual, 1)
    key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
    name = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, 'Atlas fixture - not a trusted identity')])
    cert = (x509.CertificateBuilder().subject_name(name).issuer_name(name).public_key(key.public_key())
        .serial_number(16).not_valid_before(datetime(2026, 1, 1, tzinfo=timezone.utc))
        .not_valid_after(datetime(2036, 1, 1, tzinfo=timezone.utc)).sign(key, hashes.SHA256()))
    signature = (pkcs7.PKCS7SignatureBuilder().set_data(data[:start] + data[end:])
        .add_signer(cert, key, hashes.SHA256()).sign(serialization.Encoding.DER,
             [pkcs7.PKCS7Options.DetachedSignature, pkcs7.PKCS7Options.Binary]))
    assert len(signature) <= 8192
    PDF.write_bytes(data[:start + 1] + signature.hex().encode().ljust(16384, b'0') + data[end - 1:])


def verify(openssl, pdf=PDF):
    data = pdf.read_bytes()
    match = re.search(rb'/ByteRange\s*\[\s*(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s*\]', data)
    a, b, c, d = map(int, match.groups())
    assert a == 0 and c + d == len(data) and data[b:b+1] == b'<' and data[c-1:c] == b'>'
    signature = bytes.fromhex(data[b+1:c-1].decode())
    signed = data[:b] + data[c:]
    with tempfile.TemporaryDirectory(prefix='atlas-signature-') as directory:
        root = Path(directory)
        (root/'signature.der').write_bytes(signature)
        (root/'content.bin').write_bytes(signed)
        command = [openssl, 'cms', '-verify', '-binary', '-inform', 'DER', '-noverify',
                   '-in', str(root/'signature.der'), '-content', str(root/'content.bin'),
                   '-out', str(root/'verified.bin')]
        valid = subprocess.run(command, capture_output=True, check=False)
        assert valid.returncode == 0, valid.stderr.decode(errors='replace')
        (root/'content.bin').write_bytes(signed + b'tampered')
        invalid = subprocess.run(command, capture_output=True, check=False)
        assert invalid.returncode != 0, 'The verifier must reject modified content.'
    print(f'{pdf.name}: detached CMS integrity valid; tamper rejected; identity trust intentionally not asserted.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--generate', action='store_true')
    parser.add_argument('--openssl', required=True)
    args = parser.parse_args()
    if args.generate:
        generate()
    verify(args.openssl)
    verify(args.openssl, ROOT / "generated/A017_signed_approval.pdf")
