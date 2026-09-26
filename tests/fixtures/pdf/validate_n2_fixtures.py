#!/usr/bin/env python3
"""Independently validate the N2 synthetic PDF fixtures.

This validator deliberately does not use Qt PDF, PDFium, or qpdf. It checks
fixture bytes and expected structure/text/security behavior with pinned pypdf
5.9.0 so an engine probe failure is not automatically blamed on malformed or
stale fixture bytes.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any

from pypdf import PdfReader

ROOT = Path(__file__).resolve().parent
MANIFEST = ROOT / "manifest.json"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def outline_entries(items: list[Any], depth: int = 0) -> list[dict[str, Any]]:
    entries: list[dict[str, Any]] = []
    for item in items:
        if isinstance(item, list):
            entries.extend(outline_entries(item, depth + 1))
            continue
        entries.append({"title": str(getattr(item, "title", item)), "depth": depth})
    return entries


def expected_outline_entries(items: list[dict[str, Any]], depth: int = 0) -> list[dict[str, Any]]:
    entries: list[dict[str, Any]] = []
    for item in items:
        entries.append({"title": str(item["title"]), "depth": depth})
        children = item.get("children", [])
        if children:
            entries.extend(expected_outline_entries(children, depth + 1))
    return entries


def resolve(value: Any) -> Any:
    getter = getattr(value, "get_object", None)
    return getter() if callable(getter) else value


def link_summary(reader: PdfReader) -> dict[str, Any]:
    internal_links = 0
    external_uris: list[str] = []

    for page in reader.pages:
        annotations = resolve(page.get("/Annots", [])) or []
        for reference in annotations:
            annotation = resolve(reference)
            if str(annotation.get("/Subtype")) != "/Link":
                continue
            if annotation.get("/Dest") is not None:
                internal_links += 1
            action = resolve(annotation.get("/A"))
            if action is not None:
                uri = action.get("/URI")
                if uri is not None:
                    external_uris.append(str(uri))

    return {
        "internal_link_count": internal_links,
        "external_uris": sorted(set(external_uris)),
    }


def signed_permissions(value: int) -> int:
    """Normalize pypdf's unsigned 32-bit permission view to PDF's signed /P."""
    value &= 0xFFFFFFFF
    return value if value < 0x80000000 else value - 0x100000000


def validate_signature_structure(reader: PdfReader, spec: dict[str, Any]) -> tuple[dict[str, Any], list[str]]:
    failures: list[str] = []
    root = resolve(reader.trailer.get("/Root"))
    acroform = resolve(root.get("/AcroForm")) if root is not None else None
    fields = resolve(acroform.get("/Fields", [])) if acroform is not None else []

    signature_fields = []
    for reference in fields or []:
        field = resolve(reference)
        if field is not None and str(field.get("/FT")) == "/Sig":
            signature_fields.append(field)

    expected_name = str(spec.get("expected_signature_field_name", ""))
    if len(signature_fields) != 1:
        failures.append(f"signature-field-count-mismatch:{len(signature_fields)}")
        field = None
    else:
        field = signature_fields[0]
        if str(field.get("/T")) != expected_name:
            failures.append("signature-field-name-mismatch")

    signature = resolve(field.get("/V")) if field is not None else None
    if signature is None:
        failures.append("signature-dictionary-missing")
    else:
        if str(signature.get("/Type")) != "/Sig":
            failures.append("signature-type-mismatch")
        if str(signature.get("/SubFilter")) != str(spec.get("expected_signature_subfilter")):
            failures.append("signature-subfilter-mismatch")
        byte_range = list(signature.get("/ByteRange", []))
        if [int(value) for value in byte_range] != [0, 0, 0, 0]:
            failures.append("signature-byte-range-sentinel-mismatch")
        if signature.get("/Contents") is None:
            failures.append("signature-contents-missing")

    perms = resolve(root.get("/Perms")) if root is not None else None
    catalog_docmdp = resolve(perms.get("/DocMDP")) if perms is not None else None
    if catalog_docmdp is None:
        failures.append("catalog-docmdp-missing")

    references = resolve(signature.get("/Reference", [])) if signature is not None else []
    docmdp_references = []
    for reference in references or []:
        item = resolve(reference)
        if item is not None and str(item.get("/TransformMethod")) == "/DocMDP":
            docmdp_references.append(item)

    if len(docmdp_references) != 1:
        failures.append(f"docmdp-reference-count-mismatch:{len(docmdp_references)}")
        transform_params = None
    else:
        transform_params = resolve(docmdp_references[0].get("/TransformParams"))
        if transform_params is None:
            failures.append("docmdp-transform-params-missing")

    actual_permission = int(transform_params.get("/P")) if transform_params is not None and transform_params.get("/P") is not None else None
    expected_permission = int(spec.get("expected_docmdp_permission"))
    if actual_permission != expected_permission:
        failures.append(f"docmdp-permission-mismatch:{actual_permission}!={expected_permission}")

    summary = {
        "signature_field_count": len(signature_fields),
        "signature_field_name": str(field.get("/T")) if field is not None else None,
        "signature_type": str(signature.get("/Type")) if signature is not None else None,
        "signature_subfilter": str(signature.get("/SubFilter")) if signature is not None else None,
        "byte_range": [int(value) for value in signature.get("/ByteRange", [])] if signature is not None else [],
        "contents_present": bool(signature is not None and signature.get("/Contents") is not None),
        "catalog_docmdp_present": catalog_docmdp is not None,
        "docmdp_reference_count": len(docmdp_references),
        "docmdp_permission": actual_permission,
        "cryptographic_validity_qualified": False,
        "fixture_cryptographically_valid": bool(spec.get("cryptographically_valid", False)),
    }
    return summary, failures


def validate_fixture(spec: dict[str, Any]) -> dict[str, Any]:
    fixture_path = ROOT / spec["file"]
    failures: list[str] = []
    result: dict[str, Any] = {
        "id": spec["id"],
        "file": spec["file"],
        "failures": failures,
    }

    if not fixture_path.is_file():
        failures.append("file-missing")
        result["passed"] = False
        return result

    actual_sha = sha256(fixture_path)
    result["sha256"] = actual_sha
    if actual_sha.lower() != str(spec["sha256"]).lower():
        failures.append("sha256-mismatch")

    if spec.get("expected_open_failure"):
        try:
            PdfReader(str(fixture_path), strict=False)
        except Exception as exc:
            result["open_failure"] = type(exc).__name__
            result["passed"] = not failures
            return result
        failures.append("expected-open-failure-missing")
        result["passed"] = False
        return result

    try:
        reader = PdfReader(str(fixture_path), strict=False)
    except Exception as exc:
        failures.append(f"pypdf-open-failed:{type(exc).__name__}")
        result["passed"] = False
        return result

    if spec.get("password_fixture"):
        result["is_encrypted"] = reader.is_encrypted
        if not reader.is_encrypted:
            failures.append("expected-encryption-missing")
        else:
            no_password_blocked = False
            try:
                _ = len(reader.pages)
            except Exception:
                no_password_blocked = True
            result["no_password_page_access_blocked"] = no_password_blocked
            if not no_password_blocked:
                failures.append("no-password-page-access-unexpectedly-succeeded")

            wrong_reader = PdfReader(str(fixture_path), strict=False)
            wrong_result = int(wrong_reader.decrypt(str(spec["test_wrong_password"])))
            result["wrong_password_result"] = wrong_result
            if wrong_result != 0:
                failures.append("wrong-password-unexpectedly-accepted")

            reader = PdfReader(str(fixture_path), strict=False)
            correct_result = int(reader.decrypt(str(spec["test_user_password"])))
            result["correct_password_result"] = correct_result
            if correct_result != 1:
                failures.append(f"user-password-result-unexpected:{correct_result}")

            owner_password = spec.get("test_owner_password")
            if owner_password is not None:
                owner_reader = PdfReader(str(fixture_path), strict=False)
                owner_result = int(owner_reader.decrypt(str(owner_password)))
                result["owner_password_result"] = owner_result
                if owner_result != 2:
                    failures.append(f"owner-password-result-unexpected:{owner_result}")

            expected_permissions = spec.get("expected_permissions_signed")
            if expected_permissions is not None and correct_result != 0:
                actual_permissions = signed_permissions(int(reader.user_access_permissions))
                result["permissions_signed"] = actual_permissions
                if actual_permissions != int(expected_permissions):
                    failures.append(
                        f"permission-integer-mismatch:{actual_permissions}!={int(expected_permissions)}"
                    )

    if spec.get("signature_structure_fixture"):
        signature_summary, signature_failures = validate_signature_structure(reader, spec)
        result["signature_structure"] = signature_summary
        failures.extend(signature_failures)

    page_count = len(reader.pages)
    result["page_count"] = page_count
    if page_count != int(spec["expected_pages"]):
        failures.append("page-count-mismatch")

    expected_labels = [str(value) for value in spec.get("expected_labels", [])]
    if expected_labels:
        actual_labels = [str(value) for value in reader.page_labels]
        result["page_labels"] = actual_labels
        if actual_labels != expected_labels:
            failures.append("page-label-mismatch")

    texts = [(page.extract_text() or "") for page in reader.pages]
    joined_text = "\n".join(texts)
    result["text_lengths"] = [len(value) for value in texts]
    result["text_utf8_sha256"] = hashlib.sha256(joined_text.encode("utf-8")).hexdigest()
    for expected in spec.get("expected_text", []):
        if str(expected) not in joined_text:
            failures.append(f"expected-text-missing:{expected}")

    expected_outline = spec.get("expected_outline")
    if expected_outline:
        actual_entries = outline_entries(reader.outline)
        expected_entries = expected_outline_entries(expected_outline)
        result["outline"] = actual_entries
        if actual_entries != expected_entries:
            failures.append("outline-hierarchy-mismatch")

    purposes = {str(value) for value in spec.get("purpose", [])}
    if "internal-link" in purposes or "external-uri-link" in purposes:
        links = link_summary(reader)
        result["links"] = links
        if "internal-link" in purposes and links["internal_link_count"] < 1:
            failures.append("internal-link-missing")
        expected_uri = spec.get("expected_external_uri")
        if expected_uri and str(expected_uri) not in links["external_uris"]:
            failures.append("external-uri-missing")

    result["passed"] = not failures
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, help="Optional JSON evidence output path")
    args = parser.parse_args()

    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    results = [validate_fixture(spec) for spec in manifest["fixtures"]]
    passed = all(result["passed"] for result in results)

    evidence = {
        "schema": "atlas.n2.fixture-validation.v1",
        "validator": "pypdf",
        "validator_version": "5.9.0",
        "manifest_schema": manifest.get("schema"),
        "fixture_count": len(results),
        "passed": passed,
        "results": results,
    }

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(evidence, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    print(json.dumps(evidence, ensure_ascii=True, indent=2))
    return 0 if passed else 2


if __name__ == "__main__":
    raise SystemExit(main())
