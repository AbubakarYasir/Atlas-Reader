#!/usr/bin/env python3
"""Controlled qpdf outline-title mutation qualification for Atlas N2.

The probe intentionally changes one existing outline object's /Title while
retaining every other key in that object. It then proves that hierarchy,
destination and unrelated document invariants are preserved.
"""

from __future__ import annotations

import argparse
import copy
import json
import subprocess
import sys
from pathlib import Path
from typing import Any

from qpdf_structural_probe import (
    compact_qpdf_json,
    compare_snapshots,
    qpdf_json,
    run_process,
    sha256_file,
    snapshot_pdf,
    write_json,
)


def find_outline_object(data: dict[str, Any], title: str) -> tuple[str, dict[str, Any]]:
    qpdf = data.get("qpdf")
    if not isinstance(qpdf, list) or len(qpdf) < 2 or not isinstance(qpdf[1], dict):
        raise RuntimeError("qpdf object map missing from JSON v2 output")

    expected = f"u:{title}"
    matches: list[tuple[str, dict[str, Any]]] = []
    for object_name, wrapper in qpdf[1].items():
        if object_name == "trailer" or not isinstance(wrapper, dict):
            continue
        value = wrapper.get("value")
        if isinstance(value, dict) and value.get("/Title") == expected:
            # Require this to be an outline node rather than Document Info.
            if "/Parent" in value and ("/Dest" in value or "/A" in value):
                matches.append((object_name, wrapper))

    if len(matches) != 1:
        raise RuntimeError(f"expected exactly one outline object with title {title!r}; found {len(matches)}")
    return matches[0]


def outline_without_titles(snapshot: dict[str, Any]) -> list[dict[str, Any]]:
    result: list[dict[str, Any]] = []
    for item in snapshot.get("outline", []):
        result.append({"depth": item.get("depth"), "page": item.get("page")})
    return result


def compare_expected_mutation(before: dict[str, Any], after: dict[str, Any], old_title: str, new_title: str) -> dict[str, Any]:
    non_outline_keys = [
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
    checks: dict[str, bool] = {key: before.get(key) == after.get(key) for key in non_outline_keys}

    before_outline = copy.deepcopy(before.get("outline", []))
    expected_outline = copy.deepcopy(before_outline)
    changed = 0
    for item in expected_outline:
        if item.get("title") == old_title:
            item["title"] = new_title
            changed += 1

    checks["exactly_one_expected_outline_title_changed"] = changed == 1 and after.get("outline") == expected_outline
    checks["outline_hierarchy_and_destinations_unchanged"] = outline_without_titles(before) == outline_without_titles(after)

    return {
        "checks": checks,
        "expected_outline": expected_outline,
        "actual_outline": after.get("outline", []),
        "passed": all(checks.values()),
    }


def mutate_one(
    qpdf: Path,
    source: Path,
    output_dir: Path,
    fixture_id: str,
    old_title: str,
    new_title: str,
) -> dict[str, Any]:
    raw, raw_run = qpdf_json(qpdf, source)
    if raw is None or raw_run["exit_code"] not in (0, 3):
        raise RuntimeError(f"{fixture_id}: unable to obtain qpdf JSON")

    object_name, wrapper = find_outline_object(raw, old_title)
    original_wrapper = copy.deepcopy(wrapper)
    updated_wrapper = copy.deepcopy(wrapper)
    updated_wrapper["value"]["/Title"] = f"u:{new_title}"

    update_json = {
        "qpdf": [
            {"jsonversion": 2},
            {object_name: updated_wrapper},
        ]
    }
    update_path = output_dir / f"{fixture_id}-outline-update.json"
    write_json(update_path, update_json)

    output_pdf = output_dir / f"{fixture_id}-outline-mutated.pdf"
    mutation = run_process(
        [
            str(qpdf),
            f"--update-from-json={update_path}",
            str(source),
            str(output_pdf),
        ]
    )

    recheck = (
        run_process([str(qpdf), "--check", str(output_pdf)])
        if output_pdf.exists()
        else {"exit_code": -1, "stdout": "", "stderr": "missing-output"}
    )

    before = snapshot_pdf(source)
    after = snapshot_pdf(output_pdf) if output_pdf.exists() else {}
    comparison = (
        compare_expected_mutation(before, after, old_title, new_title)
        if after
        else {"checks": {}, "passed": False}
    )

    mutated_raw, mutated_raw_run = (
        qpdf_json(qpdf, output_pdf) if output_pdf.exists() else (None, {"exit_code": -1})
    )
    if mutated_raw is not None:
        write_json(output_dir / f"qpdf-{fixture_id}-outline-mutated-raw.json", mutated_raw)

    # Verify at raw qpdf object level that the target object changed only /Title.
    raw_object_check = False
    mutated_object = None
    if mutated_raw is not None:
        try:
            mutated_object_name, mutated_wrapper = find_outline_object(mutated_raw, new_title)
            mutated_object = copy.deepcopy(mutated_wrapper)
            expected_wrapper = copy.deepcopy(original_wrapper)
            expected_wrapper["value"]["/Title"] = f"u:{new_title}"
            raw_object_check = mutated_object_name == object_name and mutated_wrapper == expected_wrapper
        except Exception:
            raw_object_check = False

    result = {
        "fixture_id": fixture_id,
        "old_title": old_title,
        "new_title": new_title,
        "target_object": object_name,
        "source_sha256": sha256_file(source),
        "output_sha256": sha256_file(output_pdf) if output_pdf.exists() else None,
        "mutation_exit_code": mutation["exit_code"],
        "mutation_stdout": mutation["stdout"],
        "mutation_stderr": mutation["stderr"],
        "recheck_exit_code": recheck["exit_code"],
        "update_json": update_json,
        "comparison": comparison,
        "raw_target_object_exact_except_title": raw_object_check,
        "mutated_qpdf_json_exit_code": mutated_raw_run["exit_code"],
        "mutated_qpdf_json_summary": compact_qpdf_json(mutated_raw) if mutated_raw is not None else None,
    }
    result["passed"] = (
        mutation["exit_code"] in (0, 3)
        and output_pdf.exists()
        and recheck["exit_code"] == 0
        and comparison["passed"]
        and raw_object_check
        and mutated_raw is not None
    )
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--qpdf", required=True, type=Path)
    parser.add_argument("--fixture-root", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()

    qpdf = args.qpdf.resolve()
    fixture_root = args.fixture_root.resolve()
    out = args.output_dir.resolve()
    out.mkdir(parents=True, exist_ok=True)

    cases = [
        (
            "A003",
            fixture_root / "A003_outlines_links.pdf",
            "Chapter 2",
            "Atlas Controlled Outline",
        ),
        (
            "A006",
            fixture_root / "A006_unicode_semantics.pdf",
            "اردو",
            "اردو — فوائد",
        ),
    ]

    evidence: dict[str, Any] = {
        "schema": "atlas.n2.qpdf-outline-mutation.v1",
        "qpdf_version": "12.4.1",
        "cases": {},
        "failures": [],
    }

    for fixture_id, source, old_title, new_title in cases:
        try:
            result = mutate_one(qpdf, source, out, fixture_id, old_title, new_title)
        except Exception as exc:
            result = {"passed": False, "exception": f"{type(exc).__name__}: {exc}"}
        evidence["cases"][fixture_id] = result
        if not result.get("passed", False):
            evidence["failures"].append(f"{fixture_id}-controlled-outline-mutation-failed")

    evidence["passed"] = not evidence["failures"]
    write_json(out / "qpdf-controlled-outline-mutation.json", evidence)
    print(json.dumps({"passed": evidence["passed"], "failures": evidence["failures"]}, ensure_ascii=False))
    return 0 if evidence["passed"] else 3


if __name__ == "__main__":
    sys.exit(main())
