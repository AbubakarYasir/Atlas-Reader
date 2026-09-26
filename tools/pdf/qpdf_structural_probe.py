#!/usr/bin/env python3
"""Atlas N2 qpdf structural/security/preservation qualification probe.

This is engineering evidence only. It intentionally drives the exact pinned qpdf
CLI distribution rather than linking qpdf into Atlas product targets.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path
from typing import Any

from pypdf import PdfReader
from pypdf.generic import ArrayObject, IndirectObject

EXPECTED_QPDF_VERSION = "12.4.1"


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def run_process(args: list[str]) -> dict[str, Any]:
    completed = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    return {
        "exit_code": completed.returncode,
        "stdout": completed.stdout.decode("utf-8", errors="replace"),
        "stderr": completed.stderr.decode("utf-8", errors="replace"),
    }


def qpdf_json(qpdf: Path, pdf: Path, password: str | None = None) -> tuple[dict[str, Any] | None, dict[str, Any]]:
    args = [str(qpdf)]
    if password is not None:
        args.append(f"--password={password}")
    args.extend(["--json=2", str(pdf)])
    result = run_process(args)
    parsed = None
    if result["exit_code"] in (0, 3) and result["stdout"].strip():
        try:
            parsed = json.loads(result["stdout"])
        except json.JSONDecodeError:
            parsed = None
    return parsed, result


def collect_outline_titles(value: Any) -> list[str]:
    titles: list[str] = []

    def walk(node: Any) -> None:
        if isinstance(node, dict):
            title = node.get("title")
            if isinstance(title, str):
                titles.append(title)
            for child in node.values():
                walk(child)
        elif isinstance(node, list):
            for child in node:
                walk(child)

    walk(value)
    return titles


def compact_qpdf_json(data: dict[str, Any]) -> dict[str, Any]:
    pages = data.get("pages")
    outlines = data.get("outlines")
    return {
        "top_level_keys": sorted(data.keys()),
        "page_count": len(pages) if isinstance(pages, list) else None,
        "outline_titles_depth_first": collect_outline_titles(outlines),
        "pagelabels": data.get("pagelabels"),
        "encrypt": data.get("encrypt"),
    }


def flatten_pypdf_outline(reader: PdfReader) -> list[dict[str, Any]]:
    result: list[dict[str, Any]] = []

    def walk(items: Any, depth: int) -> None:
        if not isinstance(items, list):
            return
        last_was_destination = False
        for item in items:
            if isinstance(item, list):
                walk(item, depth + 1 if last_was_destination else depth)
                last_was_destination = False
                continue
            title = getattr(item, "title", None)
            if title is None and isinstance(item, dict):
                title = item.get("/Title")
            try:
                page = reader.get_destination_page_number(item)
            except Exception:  # pypdf destination helper may reject malformed/non-destination items
                page = None
            result.append({"title": str(title) if title is not None else None, "depth": depth, "page": page})
            last_was_destination = True

    try:
        walk(reader.outline, 0)
    except Exception:
        return []
    return result


def page_ref_map(reader: PdfReader) -> dict[tuple[int, int], int]:
    refs: dict[tuple[int, int], int] = {}
    for index, page in enumerate(reader.pages):
        ref = page.indirect_reference
        if ref is not None:
            refs[(ref.idnum, ref.generation)] = index
    return refs


def canonical_destination(value: Any, refs: dict[tuple[int, int], int]) -> Any:
    if isinstance(value, ArrayObject) and value:
        first = value[0]
        page = None
        if isinstance(first, IndirectObject):
            page = refs.get((first.idnum, first.generation))
        tail = [str(x) for x in value[1:]]
        return {"page": page, "tail": tail}
    if value is None:
        return None
    return str(value)


def canonical_annotations(reader: PdfReader) -> list[list[dict[str, Any]]]:
    refs = page_ref_map(reader)
    pages: list[list[dict[str, Any]]] = []
    for page in reader.pages:
        items: list[dict[str, Any]] = []
        for ref in page.get("/Annots", []) or []:
            try:
                annot = ref.get_object()
            except Exception:
                continue
            action = annot.get("/A")
            try:
                action = action.get_object() if action is not None else None
            except Exception:
                pass
            rect = annot.get("/Rect")
            rect_values = None
            if rect is not None:
                try:
                    rect_values = [float(x) for x in rect]
                except Exception:
                    rect_values = [str(x) for x in rect]
            items.append(
                {
                    "subtype": str(annot.get("/Subtype")) if annot.get("/Subtype") is not None else None,
                    "rect": rect_values,
                    "uri": str(action.get("/URI")) if isinstance(action, dict) and action.get("/URI") is not None else None,
                    "action": str(action.get("/S")) if isinstance(action, dict) and action.get("/S") is not None else None,
                    "dest": canonical_destination(annot.get("/Dest"), refs),
                }
            )
        pages.append(items)
    return pages


def metadata_stream_hash(reader: PdfReader) -> str | None:
    try:
        root = reader.trailer["/Root"]
        metadata = root.get("/Metadata")
        if metadata is None:
            return None
        return sha256_bytes(metadata.get_object().get_data())
    except Exception:
        return None


def attachment_summary(reader: PdfReader) -> dict[str, list[str]]:
    try:
        attachments = reader.attachments
    except Exception:
        return {}
    result: dict[str, list[str]] = {}
    try:
        for name in sorted(attachments.keys()):
            blobs = attachments[name]
            if isinstance(blobs, bytes):
                blobs = [blobs]
            result[str(name)] = [sha256_bytes(bytes(blob)) for blob in blobs]
    except Exception:
        return {}
    return result


def page_content_hash(page: Any) -> str:
    try:
        contents = page.get_contents()
        if contents is None:
            return sha256_bytes(b"")
        return sha256_bytes(contents.get_data())
    except Exception:
        return "unavailable"


def snapshot_pdf(path: Path, password: str | None = None) -> dict[str, Any]:
    reader = PdfReader(str(path), password=password)
    page_summaries = []
    text_hashes = []
    for page in reader.pages:
        text = page.extract_text() or ""
        text_hashes.append(sha256_bytes(text.encode("utf-8")))
        page_summaries.append(
            {
                "media_box": [float(v) for v in page.mediabox],
                "crop_box": [float(v) for v in page.cropbox],
                "rotation": int(page.get("/Rotate", 0) or 0),
                "decoded_content_sha256": page_content_hash(page),
            }
        )

    metadata = {}
    if reader.metadata is not None:
        metadata = {str(k): str(v) for k, v in reader.metadata.items()}

    return {
        "encrypted": bool(reader.is_encrypted),
        "page_count": len(reader.pages),
        "page_labels": [str(v) for v in reader.page_labels],
        "page_text_utf8_sha256": text_hashes,
        "pages": page_summaries,
        "outline": flatten_pypdf_outline(reader),
        "annotations": canonical_annotations(reader),
        "document_info": metadata,
        "xmp_stream_sha256": metadata_stream_hash(reader),
        "attachments": attachment_summary(reader),
    }


def compare_snapshots(before: dict[str, Any], after: dict[str, Any]) -> dict[str, Any]:
    keys = [
        "encrypted",
        "page_count",
        "page_labels",
        "page_text_utf8_sha256",
        "pages",
        "outline",
        "annotations",
        "document_info",
        "xmp_stream_sha256",
        "attachments",
    ]
    checks = {key: before.get(key) == after.get(key) for key in keys}
    return {"checks": checks, "passed": all(checks.values())}


def write_json(path: Path, value: Any) -> None:
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


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

    failures: list[str] = []
    evidence: dict[str, Any] = {
        "schema": "atlas.n2.qpdf-structural-probe.v1",
        "expected_qpdf_version": EXPECTED_QPDF_VERSION,
        "qpdf_executable_name": qpdf.name,
        "structural": {},
        "security": {},
        "preservation": {},
        "failures": failures,
    }

    version = run_process([str(qpdf), "--version"])
    version_text = (version["stdout"] + version["stderr"]).strip()
    evidence["version"] = {"exit_code": version["exit_code"], "reported": version_text}
    if version["exit_code"] != 0 or EXPECTED_QPDF_VERSION not in version_text:
        failures.append("qpdf-version-mismatch")

    fixtures = {
        "A003": fixture_root / "A003_outlines_links.pdf",
        "A006": fixture_root / "A006_unicode_semantics.pdf",
        "A007": fixture_root / "A007_malformed_truncated.pdf",
        "A008": fixture_root / "A008_password_rc4_40.pdf",
    }
    for fixture_id, path in fixtures.items():
        evidence.setdefault("fixture_sha256", {})[fixture_id] = sha256_file(path)

    expected_structural = {
        "A003": {"pages": 3, "titles": ["Chapter 1", "Section 1.1", "Chapter 2"]},
        "A006": {"pages": 4, "titles": ["العربية", "مُشَكَّل", "Mixed العربية English", "اردو"]},
    }

    for fixture_id in ("A003", "A006"):
        path = fixtures[fixture_id]
        check = run_process([str(qpdf), "--check", str(path)])
        data, json_run = qpdf_json(qpdf, path)
        raw_path = out / f"qpdf-{fixture_id}-raw.json"
        if data is not None:
            write_json(raw_path, data)
        summary = compact_qpdf_json(data) if data is not None else None
        evidence["structural"][fixture_id] = {
            "check_exit_code": check["exit_code"],
            "check_stdout_sha256": sha256_bytes(check["stdout"].encode("utf-8")),
            "check_stderr_sha256": sha256_bytes(check["stderr"].encode("utf-8")),
            "json_exit_code": json_run["exit_code"],
            "json_summary": summary,
        }
        if check["exit_code"] != 0:
            failures.append(f"{fixture_id}-qpdf-check-not-clean:{check['exit_code']}")
        if data is None or summary is None:
            failures.append(f"{fixture_id}-qpdf-json-unavailable")
            continue
        if summary["page_count"] != expected_structural[fixture_id]["pages"]:
            failures.append(f"{fixture_id}-page-count-mismatch:{summary['page_count']}")
        if summary["outline_titles_depth_first"] != expected_structural[fixture_id]["titles"]:
            failures.append(f"{fixture_id}-outline-title-mismatch")

    malformed = run_process([str(qpdf), "--check", str(fixtures["A007"])])
    evidence["security"]["A007_malformed"] = {
        "check_exit_code": malformed["exit_code"],
        "stdout": malformed["stdout"],
        "stderr": malformed["stderr"],
    }
    if malformed["exit_code"] == 0:
        failures.append("A007-malformed-silently-clean")

    a003_encrypted = run_process([str(qpdf), "--is-encrypted", str(fixtures["A003"])])
    a008_encrypted = run_process([str(qpdf), "--is-encrypted", str(fixtures["A008"])])
    a008_requires_none = run_process([str(qpdf), "--requires-password", str(fixtures["A008"])])
    a008_requires_wrong = run_process(
        [str(qpdf), "--password=atlas-wrong", "--requires-password", str(fixtures["A008"])]
    )
    a008_requires_correct = run_process(
        [str(qpdf), "--password=atlas-user", "--requires-password", str(fixtures["A008"])]
    )
    show_encryption = run_process([str(qpdf), "--show-encryption", str(fixtures["A008"])])

    evidence["security"]["encryption_exit_contract"] = {
        "A003_is_encrypted": a003_encrypted["exit_code"],
        "A008_is_encrypted": a008_encrypted["exit_code"],
        "A008_requires_password_none": a008_requires_none["exit_code"],
        "A008_requires_password_wrong": a008_requires_wrong["exit_code"],
        "A008_requires_password_correct": a008_requires_correct["exit_code"],
    }
    evidence["security"]["A008_show_encryption"] = {
        "exit_code": show_encryption["exit_code"],
        "stdout": show_encryption["stdout"],
        "stderr": show_encryption["stderr"],
    }

    expected_codes = {
        "A003_is_encrypted": (a003_encrypted["exit_code"], 2),
        "A008_is_encrypted": (a008_encrypted["exit_code"], 0),
        "A008_requires_password_none": (a008_requires_none["exit_code"], 0),
        "A008_requires_password_wrong": (a008_requires_wrong["exit_code"], 0),
        "A008_requires_password_correct": (a008_requires_correct["exit_code"], 3),
    }
    for name, (actual, expected) in expected_codes.items():
        if actual != expected:
            failures.append(f"{name}-exit-mismatch:{actual}!={expected}")
    if show_encryption["exit_code"] not in (0, 3) or not show_encryption["stdout"].strip():
        failures.append("A008-show-encryption-unavailable")

    for fixture_id in ("A003", "A006"):
        source = fixtures[fixture_id]
        rewritten = out / f"{fixture_id}-qpdf-rewrite.pdf"
        before = snapshot_pdf(source)
        rewrite = run_process([str(qpdf), str(source), str(rewritten)])
        recheck = run_process([str(qpdf), "--check", str(rewritten)]) if rewritten.exists() else {"exit_code": -1, "stdout": "", "stderr": "missing-output"}
        after = snapshot_pdf(rewritten) if rewritten.exists() else {}
        comparison = compare_snapshots(before, after) if after else {"checks": {}, "passed": False}
        evidence["preservation"][fixture_id] = {
            "rewrite_exit_code": rewrite["exit_code"],
            "recheck_exit_code": recheck["exit_code"],
            "source_sha256": sha256_file(source),
            "output_sha256": sha256_file(rewritten) if rewritten.exists() else None,
            "before": before,
            "after": after,
            "comparison": comparison,
        }
        if rewrite["exit_code"] not in (0, 3) or not rewritten.exists():
            failures.append(f"{fixture_id}-rewrite-failed:{rewrite['exit_code']}")
        if recheck["exit_code"] != 0:
            failures.append(f"{fixture_id}-rewritten-check-not-clean:{recheck['exit_code']}")
        if not comparison["passed"]:
            failed_keys = [k for k, ok in comparison["checks"].items() if not ok]
            failures.append(f"{fixture_id}-preservation-mismatch:{','.join(failed_keys)}")

        if rewritten.exists():
            rewritten_json, rewritten_json_run = qpdf_json(qpdf, rewritten)
            if rewritten_json is not None:
                write_json(out / f"qpdf-{fixture_id}-rewrite-raw.json", rewritten_json)
            evidence["preservation"][fixture_id]["rewritten_qpdf_json_exit_code"] = rewritten_json_run["exit_code"]
            evidence["preservation"][fixture_id]["rewritten_qpdf_json_summary"] = (
                compact_qpdf_json(rewritten_json) if rewritten_json is not None else None
            )

    evidence["passed"] = not failures
    write_json(out / "qpdf-structural-security-preservation.json", evidence)
    print(json.dumps({"passed": evidence["passed"], "failures": failures}, ensure_ascii=False))
    return 0 if evidence["passed"] else 3


if __name__ == "__main__":
    sys.exit(main())
