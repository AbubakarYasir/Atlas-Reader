#!/usr/bin/env python3
"""Qualify qpdf visibility of PDF signature/DocMDP structures for Atlas N2.

A010 is intentionally NOT cryptographically signed. This probe qualifies only
structural detection and the pre-mutation safety policy: a document carrying
signature/certification structures must never be silently rewritten and then
represented as retaining verified integrity.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path
from typing import Any

EXPECTED_QPDF_VERSION = "12.4.1"
EXPECTED_A010_SHA256 = "95bc8daabea7d46bb70432bb65fc2fd8af3ad6e590f0fbf539e443930ce8f668"


def run(args: list[str]) -> dict[str, Any]:
    completed = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    return {
        "exit_code": completed.returncode,
        "stdout": completed.stdout.decode("utf-8", errors="replace"),
        "stderr": completed.stderr.decode("utf-8", errors="replace"),
    }


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def qpdf_json(qpdf: Path, pdf: Path) -> tuple[dict[str, Any] | None, dict[str, Any]]:
    result = run([str(qpdf), "--json=2", str(pdf)])
    parsed = None
    if result["exit_code"] in (0, 3) and result["stdout"].strip():
        try:
            parsed = json.loads(result["stdout"])
        except json.JSONDecodeError:
            parsed = None
    return parsed, result


def object_values(data: dict[str, Any]) -> list[tuple[str, dict[str, Any]]]:
    qpdf = data.get("qpdf")
    if not isinstance(qpdf, list) or len(qpdf) < 2 or not isinstance(qpdf[1], dict):
        return []
    values: list[tuple[str, dict[str, Any]]] = []
    for name, wrapper in qpdf[1].items():
        if name == "trailer" or not isinstance(wrapper, dict):
            continue
        value = wrapper.get("value")
        if isinstance(value, dict):
            values.append((name, value))
    return values


def summarize(data: dict[str, Any]) -> dict[str, Any]:
    values = object_values(data)
    signatures = []
    signature_fields = []
    docmdp_refs = []
    transform_params = []
    catalog_docmdp = []

    for name, value in values:
        if value.get("/Type") == "/Sig":
            signatures.append(
                {
                    "object": name,
                    "filter": value.get("/Filter"),
                    "subfilter": value.get("/SubFilter"),
                    "byte_range": value.get("/ByteRange"),
                    "has_contents": "/Contents" in value,
                    "has_reference": "/Reference" in value,
                }
            )
        if value.get("/FT") == "/Sig":
            signature_fields.append(
                {"object": name, "title": value.get("/T"), "has_value": "/V" in value}
            )
        if value.get("/TransformMethod") == "/DocMDP":
            docmdp_refs.append(
                {
                    "object": name,
                    "digest_method": value.get("/DigestMethod"),
                    "has_transform_params": "/TransformParams" in value,
                }
            )
        if value.get("/Type") == "/TransformParams" and "/P" in value:
            transform_params.append(
                {"object": name, "permission": value.get("/P"), "version": value.get("/V")}
            )
        if value.get("/Type") == "/Catalog" and "/Perms" in value:
            catalog_docmdp.append(
                {"object": name, "perms": value.get("/Perms"), "acroform": value.get("/AcroForm")}
            )

    return {
        "signature_fields": signature_fields,
        "signatures": signatures,
        "docmdp_references": docmdp_refs,
        "transform_params": transform_params,
        "catalogs_with_perms": catalog_docmdp,
    }


def structure_passes(summary: dict[str, Any]) -> tuple[bool, list[str]]:
    failures: list[str] = []
    fields = summary["signature_fields"]
    signatures = summary["signatures"]
    refs = summary["docmdp_references"]
    params = summary["transform_params"]
    catalogs = summary["catalogs_with_perms"]

    if len(fields) != 1 or fields[0].get("title") != "u:Atlas Certification" or not fields[0].get("has_value"):
        failures.append("signature-field-structure-mismatch")
    if len(signatures) != 1:
        failures.append("signature-dictionary-count-mismatch")
    else:
        sig = signatures[0]
        if sig.get("filter") != "/Adobe.PPKLite":
            failures.append("signature-filter-mismatch")
        if sig.get("subfilter") != "/adbe.pkcs7.detached":
            failures.append("signature-subfilter-mismatch")
        if sig.get("byte_range") != [0, 0, 0, 0]:
            failures.append("signature-byte-range-mismatch")
        if not sig.get("has_contents") or not sig.get("has_reference"):
            failures.append("signature-payload-or-reference-missing")
    if len(refs) != 1 or refs[0].get("digest_method") != "/SHA256" or not refs[0].get("has_transform_params"):
        failures.append("docmdp-reference-mismatch")
    if len(params) != 1 or params[0].get("permission") != 2 or params[0].get("version") != "/1.2":
        failures.append("docmdp-transform-params-mismatch")
    if len(catalogs) != 1:
        failures.append("catalog-docmdp-perms-missing")

    return not failures, failures


def write_json(path: Path, value: Any) -> None:
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--qpdf", required=True, type=Path)
    parser.add_argument("--fixture-root", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()

    qpdf = args.qpdf.resolve()
    fixture = (args.fixture_root / "A010_certification_structure.pdf").resolve()
    out = args.output_dir.resolve()
    out.mkdir(parents=True, exist_ok=True)

    failures: list[str] = []
    fixture_sha = sha256(fixture)
    if fixture_sha != EXPECTED_A010_SHA256:
        failures.append(f"A010-sha256-mismatch:{fixture_sha}")

    version = run([str(qpdf), "--version"])
    if version["exit_code"] != 0 or EXPECTED_QPDF_VERSION not in (version["stdout"] + version["stderr"]):
        failures.append("qpdf-version-mismatch")

    source_check = run([str(qpdf), "--check", str(fixture)])
    if source_check["exit_code"] not in (0, 3):
        failures.append(f"A010-source-check-failed:{source_check['exit_code']}")

    source_json, source_json_run = qpdf_json(qpdf, fixture)
    if source_json is None:
        failures.append("A010-source-json-unavailable")
        source_summary: dict[str, Any] = {
            "signature_fields": [], "signatures": [], "docmdp_references": [],
            "transform_params": [], "catalogs_with_perms": []
        }
    else:
        source_summary = summarize(source_json)
        ok, structural_failures = structure_passes(source_summary)
        if not ok:
            failures.extend(f"A010-source-{item}" for item in structural_failures)
        write_json(out / "qpdf-A010-certification-raw.json", source_json)

    rewritten = out / "A010-qpdf-rewrite.pdf"
    rewrite = run([str(qpdf), str(fixture), str(rewritten)])
    if rewrite["exit_code"] not in (0, 3) or not rewritten.exists():
        failures.append(f"A010-rewrite-failed:{rewrite['exit_code']}")

    rewritten_sha = sha256(rewritten) if rewritten.exists() else None
    if rewritten_sha == fixture_sha:
        failures.append("A010-rewrite-unexpectedly-byte-identical")

    rewritten_check = (
        run([str(qpdf), "--check", str(rewritten)])
        if rewritten.exists()
        else {"exit_code": -1, "stdout": "", "stderr": "missing-output"}
    )
    if rewritten_check["exit_code"] not in (0, 3):
        failures.append(f"A010-rewritten-check-failed:{rewritten_check['exit_code']}")

    rewritten_json, rewritten_json_run = (
        qpdf_json(qpdf, rewritten) if rewritten.exists() else (None, {"exit_code": -1})
    )
    rewritten_summary = summarize(rewritten_json) if rewritten_json is not None else None
    if rewritten_summary is None:
        failures.append("A010-rewritten-json-unavailable")
    else:
        ok, structural_failures = structure_passes(rewritten_summary)
        if not ok:
            failures.extend(f"A010-rewritten-{item}" for item in structural_failures)
        write_json(out / "qpdf-A010-certification-rewrite-raw.json", rewritten_json)

    evidence = {
        "schema": "atlas.n2.qpdf-signature-safety.v1",
        "qpdf_version": EXPECTED_QPDF_VERSION,
        "fixture": "A010",
        "fixture_sha256": fixture_sha,
        "cryptographic_validity_qualified": False,
        "fixture_cryptographically_valid": False,
        "qualification_scope": "signature-field-and-DocMDP-structural-detection-plus-rewrite-safety",
        "source_check_exit_code": source_check["exit_code"],
        "source_json_exit_code": source_json_run["exit_code"],
        "source_summary": source_summary,
        "rewrite_exit_code": rewrite["exit_code"],
        "rewritten_sha256": rewritten_sha,
        "rewritten_bytes_differ": rewritten_sha is not None and rewritten_sha != fixture_sha,
        "rewritten_check_exit_code": rewritten_check["exit_code"],
        "rewritten_json_exit_code": rewritten_json_run["exit_code"],
        "rewritten_summary": rewritten_summary,
        "atlas_policy": {
            "pre_mutation_signature_detection_required": True,
            "pre_mutation_docmdp_detection_required": True,
            "silent_mutation_allowed": False,
            "may_claim_signature_integrity_preserved_after_rewrite": False,
            "real_signature_validity_requires_independent_crypto_verification": True,
        },
        "failures": failures,
    }
    evidence["passed"] = not failures
    write_json(out / "qpdf-signature-certification-safety.json", evidence)
    print(json.dumps({"passed": evidence["passed"], "failures": failures}, ensure_ascii=True))
    return 0 if evidence["passed"] else 3


if __name__ == "__main__":
    raise SystemExit(main())
