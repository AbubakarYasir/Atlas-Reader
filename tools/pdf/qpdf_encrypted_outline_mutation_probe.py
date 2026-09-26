#!/usr/bin/env python3
"""Qualify qpdf outline mutation while preserving an allowed encrypted PDF.

A015 is a deterministic permissive R2 fixture. The probe opens it with the user
password, changes one outline title, and requires qpdf to preserve encryption,
password identities, permission state, destination and unrelated PDF semantics.

This does not qualify restriction bypass. Restricted A009 remains non-writable
under Atlas policy and is never used as a mutation target here.
"""

from __future__ import annotations

import argparse
import copy
import json
from pathlib import Path
from typing import Any

from pypdf import PdfReader

from qpdf_structural_probe import qpdf_json, run_process, sha256_file, snapshot_pdf, write_json

USER_PASSWORD = "atlas-user"
OWNER_PASSWORD = "atlas-owner"
WRONG_PASSWORD = "atlas-wrong"
OLD_TITLE = "Encrypted Root"
NEW_TITLE = "Encrypted Root — مشفر"
EXPECTED_SOURCE_SHA256 = "f01c4422597f015eedd3c4216ba0097de3f6d90ee7f943d9a703a1ad84954694"


def find_outline_object(data: dict[str, Any], title: str) -> tuple[str, dict[str, Any]]:
    qpdf = data.get("qpdf")
    if not isinstance(qpdf, list) or len(qpdf) < 2 or not isinstance(qpdf[1], dict):
        raise RuntimeError("qpdf object map missing")
    expected = f"u:{title}"
    matches: list[tuple[str, dict[str, Any]]] = []
    for name, wrapper in qpdf[1].items():
        if name == "trailer" or not isinstance(wrapper, dict):
            continue
        value = wrapper.get("value")
        if isinstance(value, dict) and value.get("/Title") == expected and "/Parent" in value:
            matches.append((name, wrapper))
    if len(matches) != 1:
        raise RuntimeError(f"expected one outline object {title!r}; found {len(matches)}")
    return matches[0]


def decrypt_result(path: Path, password: str) -> int:
    reader = PdfReader(str(path), strict=False)
    return int(reader.decrypt(password))


def encryption_summary(path: Path, password: str) -> dict[str, Any]:
    reader = PdfReader(str(path), strict=False)
    decrypt = int(reader.decrypt(password))
    if decrypt == 0:
        raise RuntimeError(f"password unexpectedly rejected for {path.name}")
    encrypt = reader.trailer["/Encrypt"].get_object()
    return {
        "is_encrypted": reader.is_encrypted,
        "decrypt_result": decrypt,
        "V": int(encrypt.get("/V", -1)),
        "R": int(encrypt.get("/R", -1)),
        "P": int(encrypt.get("/P", 0)),
        "Length": int(encrypt.get("/Length", 40)),
    }


def outline_snapshot(path: Path, password: str) -> list[dict[str, Any]]:
    return snapshot_pdf(path, password=password).get("outline", [])


def compare_unrelated(before: dict[str, Any], after: dict[str, Any]) -> dict[str, bool]:
    keys = [
        "encrypted",
        "page_count",
        "page_labels",
        "page_text_utf8_sha256",
        "pages",
        "annotations",
        "document_info",
        "xmp_stream_sha256",
        "attachments",
    ]
    return {key: before.get(key) == after.get(key) for key in keys}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--qpdf", required=True, type=Path)
    parser.add_argument("--fixture-root", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()

    qpdf = args.qpdf.resolve()
    source = (args.fixture_root / "A015_encrypted_outline_rc4_40.pdf").resolve()
    out = args.output_dir.resolve()
    out.mkdir(parents=True, exist_ok=True)

    evidence: dict[str, Any] = {
        "schema": "atlas.n2.qpdf-encrypted-outline-mutation.v1",
        "qpdf_version": "12.4.1",
        "source_sha256": sha256_file(source),
        "public_test_credentials": True,
        "mutation_authority": "permissive R2 user password; P=-4",
        "restricted_fixture_mutated": False,
        "failures": [],
    }

    try:
        if evidence["source_sha256"] != EXPECTED_SOURCE_SHA256:
            raise RuntimeError("A015 source SHA-256 mismatch")

        source_wrong = decrypt_result(source, WRONG_PASSWORD)
        source_user = decrypt_result(source, USER_PASSWORD)
        source_owner = decrypt_result(source, OWNER_PASSWORD)
        source_encryption = encryption_summary(source, USER_PASSWORD)
        source_outline = outline_snapshot(source, USER_PASSWORD)
        if source_wrong != 0 or source_user != 1 or source_owner != 2:
            raise RuntimeError(
                f"unexpected source password identities wrong/user/owner={source_wrong}/{source_user}/{source_owner}"
            )
        if source_outline != [{"title": OLD_TITLE, "depth": 0, "page": 0}]:
            raise RuntimeError(f"unexpected source outline: {source_outline}")

        raw, raw_run = qpdf_json(qpdf, source, password=USER_PASSWORD)
        if raw is None or raw_run["exit_code"] not in (0, 3):
            raise RuntimeError("qpdf JSON read failed for A015 user password")
        object_name, wrapper = find_outline_object(raw, OLD_TITLE)
        updated = copy.deepcopy(wrapper)
        updated["value"]["/Title"] = f"u:{NEW_TITLE}"
        update_json = {"qpdf": [{"jsonversion": 2}, {object_name: updated}]}
        update_path = out / "A015-outline-update.json"
        write_json(update_path, update_json)

        output_pdf = out / "A015-encrypted-outline-mutated.pdf"
        mutation = run_process(
            [
                str(qpdf),
                f"--password={USER_PASSWORD}",
                f"--update-from-json={update_path}",
                str(source),
                str(output_pdf),
            ]
        )
        if mutation["exit_code"] not in (0, 3) or not output_pdf.is_file():
            raise RuntimeError(f"qpdf encrypted mutation failed: {mutation}")

        recheck = run_process([str(qpdf), f"--password={USER_PASSWORD}", "--check", str(output_pdf)])
        if recheck["exit_code"] != 0:
            raise RuntimeError(f"qpdf encrypted output check failed: {recheck}")

        before = snapshot_pdf(source, password=USER_PASSWORD)
        after = snapshot_pdf(output_pdf, password=USER_PASSWORD)
        unrelated = compare_unrelated(before, after)
        output_outline = after.get("outline", [])

        output_wrong = decrypt_result(output_pdf, WRONG_PASSWORD)
        output_user = decrypt_result(output_pdf, USER_PASSWORD)
        output_owner = decrypt_result(output_pdf, OWNER_PASSWORD)
        output_encryption = encryption_summary(output_pdf, USER_PASSWORD)

        user_json, user_json_run = qpdf_json(qpdf, output_pdf, password=USER_PASSWORD)
        owner_json, owner_json_run = qpdf_json(qpdf, output_pdf, password=OWNER_PASSWORD)
        user_encrypt = user_json.get("encrypt") if user_json else None
        owner_encrypt = owner_json.get("encrypt") if owner_json else None
        user_parameters = user_encrypt.get("parameters") if isinstance(user_encrypt, dict) else None
        user_capabilities = user_encrypt.get("capabilities") if isinstance(user_encrypt, dict) else None

        checks = {
            "source_password_identities": source_wrong == 0 and source_user == 1 and source_owner == 2,
            "mutation_exit_ok": mutation["exit_code"] in (0, 3),
            "qpdf_check_clean": recheck["exit_code"] == 0,
            "output_still_encrypted": bool(output_encryption["is_encrypted"]),
            "encryption_parameters_preserved": source_encryption == output_encryption,
            "output_password_identities": output_wrong == 0 and output_user == 1 and output_owner == 2,
            "exact_outline_mutation": output_outline == [{"title": NEW_TITLE, "depth": 0, "page": 0}],
            "all_unrelated_invariants_preserved": all(unrelated.values()),
            "qpdf_user_json_reopen": user_json is not None and user_json_run["exit_code"] in (0, 3),
            "qpdf_owner_json_reopen": owner_json is not None and owner_json_run["exit_code"] in (0, 3),
            "qpdf_user_password_identity": bool(isinstance(user_encrypt, dict) and user_encrypt.get("userpasswordmatched") is True and user_encrypt.get("ownerpasswordmatched") is False),
            "qpdf_owner_password_identity": bool(isinstance(owner_encrypt, dict) and owner_encrypt.get("ownerpasswordmatched") is True),
            "permissions_remain_permissive": bool(
                isinstance(user_parameters, dict)
                and int(user_parameters.get("P", 0)) == -4
                and isinstance(user_capabilities, dict)
                and all(bool(value) for value in user_capabilities.values())
            ),
        }

        evidence.update(
            {
                "source_password_results": {"wrong": source_wrong, "user": source_user, "owner": source_owner},
                "source_encryption": source_encryption,
                "source_outline": source_outline,
                "target_object": object_name,
                "new_title": NEW_TITLE,
                "mutation_exit_code": mutation["exit_code"],
                "output_sha256": sha256_file(output_pdf),
                "output_password_results": {"wrong": output_wrong, "user": output_user, "owner": output_owner},
                "output_encryption": output_encryption,
                "output_outline": output_outline,
                "unrelated_invariants": unrelated,
                "checks": checks,
            }
        )
        evidence["passed"] = all(checks.values())
    except Exception as exc:
        evidence["failures"].append(f"{type(exc).__name__}: {exc}")
        evidence["passed"] = False

    if not evidence.get("passed", False) and not evidence["failures"]:
        evidence["failures"].append("encrypted-outline-mutation-check-failed")

    write_json(out / "qpdf-encrypted-outline-mutation.json", evidence)
    print(json.dumps({"passed": evidence["passed"], "failures": evidence["failures"]}, ensure_ascii=False))
    return 0 if evidence["passed"] else 3


if __name__ == "__main__":
    raise SystemExit(main())
