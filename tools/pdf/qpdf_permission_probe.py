#!/usr/bin/env python3
"""Focused qpdf permission and owner/user password qualification for Atlas N2.

The probe uses deterministic public test credentials only. It reads qpdf JSON v2
rather than depending on human-formatted ``--show-encryption`` text for strict
capability assertions.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

from qpdf_structural_probe import qpdf_json, run_process, sha256_file, write_json

EXPECTED_QPDF_VERSION = "12.4.1"
USER_PASSWORD = "atlas-user"
OWNER_PASSWORD = "atlas-owner"
WRONG_PASSWORD = "atlas-wrong"


def require(condition: bool, failures: list[str], label: str) -> None:
    if not condition:
        failures.append(label)


def encryption_summary(data: dict[str, Any] | None) -> dict[str, Any] | None:
    if not isinstance(data, dict):
        return None
    encrypt = data.get("encrypt")
    if not isinstance(encrypt, dict):
        return None
    return {
        "encrypted": encrypt.get("encrypted"),
        "userpasswordmatched": encrypt.get("userpasswordmatched"),
        "ownerpasswordmatched": encrypt.get("ownerpasswordmatched"),
        "parameters": encrypt.get("parameters"),
        "capabilities": encrypt.get("capabilities"),
    }


def probe_fixture(
    qpdf: Path,
    pdf: Path,
    fixture_id: str,
    expected_p: int,
    expect_restricted: bool,
    failures: list[str],
) -> dict[str, Any]:
    is_encrypted = run_process([str(qpdf), "--is-encrypted", str(pdf)])
    requires_none = run_process([str(qpdf), "--requires-password", str(pdf)])
    requires_wrong = run_process(
        [str(qpdf), f"--password={WRONG_PASSWORD}", "--requires-password", str(pdf)]
    )
    requires_user = run_process(
        [str(qpdf), f"--password={USER_PASSWORD}", "--requires-password", str(pdf)]
    )
    requires_owner = run_process(
        [str(qpdf), f"--password={OWNER_PASSWORD}", "--requires-password", str(pdf)]
    )

    user_json, user_run = qpdf_json(qpdf, pdf, USER_PASSWORD)
    owner_json, owner_run = qpdf_json(qpdf, pdf, OWNER_PASSWORD)
    user_summary = encryption_summary(user_json)
    owner_summary = encryption_summary(owner_json)
    show = run_process([str(qpdf), "--show-encryption", str(pdf)])

    prefix = fixture_id
    require(is_encrypted["exit_code"] == 0, failures, f"{prefix}-is-encrypted-exit:{is_encrypted['exit_code']}!=0")
    require(requires_none["exit_code"] == 0, failures, f"{prefix}-requires-none-exit:{requires_none['exit_code']}!=0")
    require(requires_wrong["exit_code"] == 0, failures, f"{prefix}-requires-wrong-exit:{requires_wrong['exit_code']}!=0")
    require(requires_user["exit_code"] == 3, failures, f"{prefix}-requires-user-exit:{requires_user['exit_code']}!=3")
    require(requires_owner["exit_code"] == 3, failures, f"{prefix}-requires-owner-exit:{requires_owner['exit_code']}!=3")
    require(user_run["exit_code"] in (0, 3) and user_summary is not None, failures, f"{prefix}-user-json-unavailable")
    require(owner_run["exit_code"] in (0, 3) and owner_summary is not None, failures, f"{prefix}-owner-json-unavailable")

    if user_summary is not None:
        params = user_summary.get("parameters") or {}
        caps = user_summary.get("capabilities") or {}
        require(user_summary.get("encrypted") is True, failures, f"{prefix}-user-json-not-encrypted")
        require(user_summary.get("userpasswordmatched") is True, failures, f"{prefix}-user-password-not-matched")
        require(params.get("P") == expected_p, failures, f"{prefix}-P:{params.get('P')}!={expected_p}")
        require(params.get("R") == 2, failures, f"{prefix}-R:{params.get('R')}!=2")
        if expect_restricted:
            # These four are the direct Revision-2 permission classes represented
            # by bits 3-6 of /P. Higher-revision sub-capabilities are recorded but
            # not made strict assumptions in this R2 fixture.
            expected_false = ("extract", "printlow", "modify", "modifyannotations")
            for capability in expected_false:
                require(caps.get(capability) is False, failures, f"{prefix}-{capability}-not-restricted:{caps.get(capability)}")

    if owner_summary is not None:
        params = owner_summary.get("parameters") or {}
        require(owner_summary.get("encrypted") is True, failures, f"{prefix}-owner-json-not-encrypted")
        require(owner_summary.get("ownerpasswordmatched") is True, failures, f"{prefix}-owner-password-not-matched")
        require(params.get("P") == expected_p, failures, f"{prefix}-owner-P:{params.get('P')}!={expected_p}")
        require(params.get("R") == 2, failures, f"{prefix}-owner-R:{params.get('R')}!=2")

    return {
        "fixture_id": fixture_id,
        "sha256": sha256_file(pdf),
        "expected_permissions_signed": expected_p,
        "expect_restricted": expect_restricted,
        "exit_contract": {
            "is_encrypted": is_encrypted["exit_code"],
            "requires_password_none": requires_none["exit_code"],
            "requires_password_wrong": requires_wrong["exit_code"],
            "requires_password_user": requires_user["exit_code"],
            "requires_password_owner": requires_owner["exit_code"],
        },
        "user_password_json_exit_code": user_run["exit_code"],
        "user_password_encrypt": user_summary,
        "owner_password_json_exit_code": owner_run["exit_code"],
        "owner_password_encrypt": owner_summary,
        "show_encryption": {
            "exit_code": show["exit_code"],
            "stdout": show["stdout"],
            "stderr": show["stderr"],
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--qpdf", required=True, type=Path)
    parser.add_argument("--fixture-root", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()

    qpdf = args.qpdf.resolve()
    root = args.fixture_root.resolve()
    out = args.output_dir.resolve()
    out.mkdir(parents=True, exist_ok=True)

    failures: list[str] = []
    version = run_process([str(qpdf), "--version"])
    version_text = (version["stdout"] + version["stderr"]).strip()
    require(version["exit_code"] == 0 and EXPECTED_QPDF_VERSION in version_text, failures, "qpdf-version-mismatch")

    cases = {
        "A008": probe_fixture(
            qpdf,
            root / "A008_password_rc4_40.pdf",
            "A008",
            expected_p=-4,
            expect_restricted=False,
            failures=failures,
        ),
        "A009": probe_fixture(
            qpdf,
            root / "A009_restricted_permissions_rc4_40.pdf",
            "A009",
            expected_p=-64,
            expect_restricted=True,
            failures=failures,
        ),
    }

    evidence = {
        "schema": "atlas.n2.qpdf-permission-security.v1",
        "qpdf_version": EXPECTED_QPDF_VERSION,
        "public_test_credentials": {
            "user_password_supplied": True,
            "owner_password_supplied": True,
            "wrong_password_supplied": True,
            "password_values_emitted": False,
        },
        "cases": cases,
        "failures": failures,
        "passed": not failures,
    }
    write_json(out / "qpdf-permission-security.json", evidence)
    print(json.dumps({"passed": evidence["passed"], "failures": failures}, ensure_ascii=False))
    return 0 if evidence["passed"] else 3


if __name__ == "__main__":
    sys.exit(main())
