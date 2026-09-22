#!/usr/bin/env python3
"""Independently validate the tracked N2 synthetic PDF fixtures.

This validator deliberately does not use Qt PDF. It checks the fixture bytes and
expected structure/text with pinned pypdf test tooling so an engine probe failure
is not automatically blamed on a malformed or stale synthetic fixture.
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

    try:
        reader = PdfReader(str(fixture_path), strict=False)
    except Exception as exc:  # fixture validation must report parser failures
        failures.append(f"pypdf-open-failed:{type(exc).__name__}")
        result["passed"] = False
        return result

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

    print(json.dumps(evidence, ensure_ascii=False, indent=2))
    return 0 if passed else 2


if __name__ == "__main__":
    raise SystemExit(main())
