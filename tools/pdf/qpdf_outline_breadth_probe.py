#!/usr/bin/env python3
"""Qualify qpdf full-JSON outline mutation breadth for Atlas N2 B3.

`--update-from-json` only updates existing objects. P0 bookmark editing also
requires adding/removing/reparenting outline objects, so this probe deliberately
uses qpdf's complete JSON v2 representation with inline stream data and rebuilds
the PDF through `--json-input`.

The A003 source hierarchy is transformed from:

    Chapter 1
      Section 1.1
    Chapter 2

to:

    Chapter 1        # former Section 1.1; unnested/reparented/reordered/renamed
      فوائد           # newly allocated Unicode child
    Chapter 1        # original Chapter 1, now childless

The old Chapter 2 object is removed. This single deterministic mutation covers
add, delete, reparent/move, reorder, nest, unnest, duplicate visible titles and
Unicode outline content while independent pypdf snapshots protect unrelated PDF
state.
"""

from __future__ import annotations

import argparse
import copy
import json
import re
import sys
from pathlib import Path
from typing import Any

from qpdf_structural_probe import qpdf_json, run_process, sha256_file, snapshot_pdf, write_json

OBJECT_RE = re.compile(r"^obj:(\d+) (\d+) R$")


def object_map(data: dict[str, Any]) -> dict[str, Any]:
    qpdf = data.get("qpdf")
    if not isinstance(qpdf, list) or len(qpdf) < 2 or not isinstance(qpdf[1], dict):
        raise RuntimeError("qpdf object map missing")
    return qpdf[1]


def find_outline_root(objects: dict[str, Any]) -> tuple[str, dict[str, Any]]:
    matches = []
    for name, wrapper in objects.items():
        if name == "trailer" or not isinstance(wrapper, dict):
            continue
        value = wrapper.get("value")
        if isinstance(value, dict) and value.get("/Type") == "/Outlines":
            matches.append((name, wrapper))
    if len(matches) != 1:
        raise RuntimeError(f"expected one /Outlines root; found {len(matches)}")
    return matches[0]


def find_outline_by_title(objects: dict[str, Any], title: str) -> tuple[str, dict[str, Any]]:
    target = f"u:{title}"
    matches = []
    for name, wrapper in objects.items():
        if name == "trailer" or not isinstance(wrapper, dict):
            continue
        value = wrapper.get("value")
        if isinstance(value, dict) and value.get("/Title") == target and "/Parent" in value:
            matches.append((name, wrapper))
    if len(matches) != 1:
        raise RuntimeError(f"expected one outline item {title!r}; found {len(matches)}")
    return matches[0]


def ref(name: str) -> str:
    match = OBJECT_RE.match(name)
    if match is None:
        raise RuntimeError(f"unexpected qpdf object name: {name}")
    return f"{match.group(1)} {match.group(2)} R"


def next_object_name(objects: dict[str, Any]) -> str:
    highest = 0
    for name in objects:
        match = OBJECT_RE.match(name)
        if match:
            highest = max(highest, int(match.group(1)))
    return f"obj:{highest + 1} 0 R"


def full_json_export(qpdf: Path, source: Path, output_json: Path) -> dict[str, Any]:
    result = run_process([str(qpdf), "--json-output=2", str(source), str(output_json)])
    if result["exit_code"] not in (0, 3) or not output_json.is_file():
        raise RuntimeError(f"qpdf full JSON export failed: {result}")
    return json.loads(output_json.read_text(encoding="utf-8"))


def rebuild_from_json(qpdf: Path, source_json: Path, output_pdf: Path) -> dict[str, Any]:
    return run_process([str(qpdf), "--json-input", str(source_json), str(output_pdf)])


def expected_outline() -> list[dict[str, Any]]:
    return [
        {"title": "Chapter 1", "depth": 0, "page": 1},
        {"title": "فوائد", "depth": 1, "page": 2},
        {"title": "Chapter 1", "depth": 0, "page": 0},
    ]


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
    source = (args.fixture_root / "A003_outlines_links.pdf").resolve()
    out = args.output_dir.resolve()
    out.mkdir(parents=True, exist_ok=True)

    evidence: dict[str, Any] = {
        "schema": "atlas.n2.qpdf-outline-breadth.v1",
        "qpdf_version": "12.4.1",
        "source_sha256": sha256_file(source),
        "operations": [
            "add-unicode-node",
            "delete-node",
            "reparent-existing-node",
            "reorder-siblings",
            "nest-new-node",
            "unnest-existing-node",
            "duplicate-visible-title",
        ],
        "failures": [],
    }

    try:
        full_json_path = out / "A003-full-qpdf.json"
        data = full_json_export(qpdf, source, full_json_path)
        objects = object_map(data)

        root_name, root_wrapper = find_outline_root(objects)
        chapter1_name, chapter1_wrapper = find_outline_by_title(objects, "Chapter 1")
        section_name, section_wrapper = find_outline_by_title(objects, "Section 1.1")
        chapter2_name, chapter2_wrapper = find_outline_by_title(objects, "Chapter 2")
        new_name = next_object_name(objects)

        root = root_wrapper["value"]
        chapter1 = chapter1_wrapper["value"]
        section = section_wrapper["value"]
        chapter2 = chapter2_wrapper["value"]

        new_wrapper = copy.deepcopy(chapter2_wrapper)
        new_value = new_wrapper["value"]
        new_value["/Title"] = "u:فوائد"
        new_value["/Parent"] = ref(section_name)
        new_value["/Dest"] = copy.deepcopy(chapter2["/Dest"])
        for key in ("/First", "/Last", "/Count", "/Prev", "/Next"):
            new_value.pop(key, None)

        # Root order: moved former Section first, original Chapter 1 second.
        root["/First"] = ref(section_name)
        root["/Last"] = ref(chapter1_name)
        root["/Count"] = 3

        # Existing Section is unnested/reparented/reordered and deliberately
        # renamed to duplicate the existing top-level Chapter 1 title.
        section["/Title"] = "u:Chapter 1"
        section["/Parent"] = ref(root_name)
        section["/First"] = ref(new_name)
        section["/Last"] = ref(new_name)
        section["/Count"] = 1
        section["/Next"] = ref(chapter1_name)
        section.pop("/Prev", None)

        # Existing Chapter 1 becomes the final top-level sibling and is now
        # childless because the former Section moved out from under it.
        chapter1["/Parent"] = ref(root_name)
        chapter1["/Prev"] = ref(section_name)
        for key in ("/First", "/Last", "/Count", "/Next"):
            chapter1.pop(key, None)

        # Add the new child and delete the former Chapter 2 object completely.
        objects[new_name] = new_wrapper
        objects.pop(chapter2_name)

        mutation_json = out / "A003-outline-breadth-mutated.json"
        write_json(mutation_json, data)
        output_pdf = out / "A003-outline-breadth-mutated.pdf"
        rebuild = rebuild_from_json(qpdf, mutation_json, output_pdf)
        check = run_process([str(qpdf), "--check", str(output_pdf)]) if output_pdf.exists() else {"exit_code": -1}

        before = snapshot_pdf(source)
        after = snapshot_pdf(output_pdf) if output_pdf.exists() else {}
        unrelated = compare_unrelated(before, after) if after else {}
        actual_outline = after.get("outline", []) if after else []
        expected = expected_outline()

        reopened, reopened_run = qpdf_json(qpdf, output_pdf) if output_pdf.exists() else (None, {"exit_code": -1})

        checks = {
            "full_json_export_complete": bool(data.get("qpdf")),
            "rebuild_exit_ok": rebuild["exit_code"] in (0, 3),
            "output_exists": output_pdf.exists(),
            "qpdf_check_clean": check.get("exit_code") == 0,
            "exact_expected_outline": actual_outline == expected,
            "duplicate_visible_titles_present": sum(1 for item in actual_outline if item.get("title") == "Chapter 1") == 2,
            "unicode_child_present": any(item.get("title") == "فوائد" and item.get("depth") == 1 for item in actual_outline),
            "deleted_chapter_absent": not any(item.get("title") == "Chapter 2" for item in actual_outline),
            "all_unrelated_invariants_preserved": bool(unrelated) and all(unrelated.values()),
            "qpdf_reopen_ok": reopened is not None and reopened_run.get("exit_code") in (0, 3),
        }

        evidence.update(
            {
                "source_objects": {
                    "outline_root": root_name,
                    "chapter_1": chapter1_name,
                    "section_1_1": section_name,
                    "chapter_2_deleted": chapter2_name,
                    "new_unicode_node": new_name,
                },
                "output_sha256": sha256_file(output_pdf) if output_pdf.exists() else None,
                "rebuild": rebuild,
                "qpdf_check": check,
                "expected_outline": expected,
                "actual_outline": actual_outline,
                "unrelated_invariants": unrelated,
                "checks": checks,
            }
        )
        evidence["passed"] = all(checks.values())
    except Exception as exc:
        evidence["failures"].append(f"{type(exc).__name__}: {exc}")
        evidence["passed"] = False

    if not evidence.get("passed", False) and not evidence["failures"]:
        evidence["failures"].append("outline-breadth-check-failed")

    write_json(out / "qpdf-outline-breadth.json", evidence)
    print(json.dumps({"passed": evidence["passed"], "failures": evidence["failures"]}, ensure_ascii=False))
    return 0 if evidence["passed"] else 3


if __name__ == "__main__":
    sys.exit(main())
