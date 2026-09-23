#!/usr/bin/env python3
"""Validate Atlas N2 normalized PDF coordinate evidence."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

ATLAS_PAGE_SPACE = "effective-visible-page; origin=top-left; x-right; y-down; units=points"
LINK_TOLERANCE = 1.25
SEARCH_EDGE_TOLERANCE = 4.0
DESTINATION_TOLERANCE = 1.0

EXPECTED_VISIBLE = {0: (200.0, 300.0), 1: (200.0, 150.0), 2: (300.0, 200.0)}
EXPECTED_A003 = {
    "internal": {"x": 72.0, "y": 112.0, "width": 178.0, "height": 20.0},
    "external": {"x": 72.0, "y": 172.0, "width": 208.0, "height": 20.0},
}
EXPECTED_A012 = {
    1: {"x": 40.0, "y": 50.0},
    2: {"x": 250.0, "y": 40.0},
}


def close(a: float, b: float, tolerance: float) -> bool:
    return abs(float(a) - float(b)) <= tolerance


def rect_close(actual: dict[str, Any], expected: dict[str, Any], tolerance: float) -> bool:
    return all(close(actual[key], expected[key], tolerance) for key in ("x", "y", "width", "height"))


def union_rects(rects: list[dict[str, Any]]) -> dict[str, float] | None:
    if not rects:
        return None
    left = min(float(r["x"]) for r in rects)
    top = min(float(r["y"]) for r in rects)
    right = max(float(r["x"]) + float(r["width"]) for r in rects)
    bottom = max(float(r["y"]) + float(r["height"]) for r in rects)
    return {"x": left, "y": top, "width": right - left, "height": bottom - top}


def find_link(links: list[dict[str, Any]], kind: str) -> list[dict[str, Any]]:
    if kind == "internal":
        return [item for item in links if int(item.get("destination_page", -1)) == 2 and not str(item.get("url", ""))]
    return [item for item in links if str(item.get("url", "")) == "https://example.com/atlas-n2-fixture"]


def validate_links(engine: str, evidence: dict[str, Any], failures: list[str], details: dict[str, Any]) -> None:
    links = evidence.get("a003_links", [])
    engine_details: dict[str, Any] = {}
    for kind, expected in EXPECTED_A003.items():
        candidates = find_link(links, kind)
        matching = [item for item in candidates if rect_close(item.get("atlas_rect", {}), expected, LINK_TOLERANCE)]
        engine_details[kind] = {
            "candidate_count": len(candidates), "matching_count": len(matching),
            "expected": expected, "actual": [item.get("atlas_rect") for item in candidates],
        }
        if not matching:
            failures.append(f"{engine}:a003-{kind}-normalized-rect-mismatch")
    details[engine] = engine_details


def by_page(entries: list[dict[str, Any]], key: str = "page") -> dict[int, dict[str, Any]]:
    return {int(item.get(key, -1)): item for item in entries}


def validate_destinations(qt: dict[str, Any], pdfium: dict[str, Any], failures: list[str]) -> list[dict[str, Any]]:
    q = by_page(qt.get("a012_destinations", []), "destination_page")
    p = by_page(pdfium.get("a012_destinations", []), "destination_page")
    comparisons: list[dict[str, Any]] = []
    for page, expected in EXPECTED_A012.items():
        q_item = q.get(page)
        p_item = p.get(page)
        if q_item is None or p_item is None:
            failures.append(f"a012-page-{page}:destination-evidence-missing")
            continue
        q_point = q_item.get("atlas_destination", {})
        p_point = p_item.get("atlas_destination", {})
        row = {"destination_page": page, "expected": expected, "qt": q_point, "pdfium": p_point}
        comparisons.append(row)
        for engine, point in (("qt-pdf", q_point), ("pdfium", p_point)):
            if not close(point.get("x", -999), expected["x"], DESTINATION_TOLERANCE) or not close(point.get("y", -999), expected["y"], DESTINATION_TOLERANCE):
                failures.append(f"{engine}:a012-page-{page}:destination-point-mismatch")
    return comparisons


def validate_search(qt: dict[str, Any], pdfium: dict[str, Any], failures: list[str]) -> list[dict[str, Any]]:
    qt_pages = by_page(qt.get("a011_search", []))
    pdfium_pages = by_page(pdfium.get("a011_search", []))
    comparisons: list[dict[str, Any]] = []
    for page in range(3):
        q = qt_pages.get(page); p = pdfium_pages.get(page)
        if q is None or p is None:
            failures.append(f"a011-page-{page}:search-evidence-missing"); continue
        expected_width, expected_height = EXPECTED_VISIBLE[page]
        for engine, item in (("qt-pdf", q), ("pdfium", p)):
            width = float(item.get("visible_width_points", -1)); height = float(item.get("visible_height_points", -1))
            if not close(width, expected_width, 0.05) or not close(height, expected_height, 0.05):
                failures.append(f"{engine}:a011-page-{page}:visible-size-mismatch")
        q_rect = union_rects(q.get("atlas_rects", [])); p_rect = union_rects(p.get("atlas_rects", []))
        if q_rect is None or p_rect is None:
            failures.append(f"a011-page-{page}:normalized-search-rect-empty"); continue
        deltas = {key: abs(q_rect[key] - p_rect[key]) for key in ("x", "y", "width", "height")}
        comparisons.append({"page": page, "query": q.get("query"), "qt_union": q_rect, "pdfium_union": p_rect, "absolute_deltas": deltas, "edge_tolerance_points": SEARCH_EDGE_TOLERANCE})
        if any(value > SEARCH_EDGE_TOLERANCE for value in deltas.values()):
            failures.append(f"a011-page-{page}:cross-engine-search-geometry-mismatch")
        for engine, rect in (("qt-pdf", q_rect), ("pdfium", p_rect)):
            if rect["x"] < -0.1 or rect["y"] < -0.1 or rect["x"] + rect["width"] > expected_width + 0.1 or rect["y"] + rect["height"] > expected_height + 0.1:
                failures.append(f"{engine}:a011-page-{page}:normalized-search-rect-out-of-bounds")
    return comparisons


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--qt", type=Path, required=True); parser.add_argument("--pdfium", type=Path, required=True); parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    qt = json.loads(args.qt.read_text(encoding="utf-8-sig")); pdfium = json.loads(args.pdfium.read_text(encoding="utf-8-sig")); failures: list[str] = []
    for engine, evidence in (("qt-pdf", qt), ("pdfium", pdfium)):
        if not evidence.get("passed"): failures.append(f"{engine}:probe-not-passed")
        if evidence.get("atlas_page_space") != ATLAS_PAGE_SPACE: failures.append(f"{engine}:page-space-contract-mismatch")
    link_details: dict[str, Any] = {}
    validate_links("qt-pdf", qt, failures, link_details); validate_links("pdfium", pdfium, failures, link_details)
    destination_comparisons = validate_destinations(qt, pdfium, failures)
    search_comparisons = validate_search(qt, pdfium, failures)
    result = {
        "schema": "atlas.n2.coordinate-normalization-validation.v2", "atlas_page_space": ATLAS_PAGE_SPACE,
        "link_tolerance_points": LINK_TOLERANCE, "destination_tolerance_points": DESTINATION_TOLERANCE, "search_edge_tolerance_points": SEARCH_EDGE_TOLERANCE,
        "checks": ["a003-internal-link-source-rectangle", "a003-external-link-source-rectangle", "a012-xyz-normal-destination", "a012-xyz-rotated-destination", "a011-search-geometry-normal-page", "a011-search-geometry-cropped-page", "a011-search-geometry-rotated-page", "normalized-rectangles-in-effective-visible-bounds"],
        "link_details": link_details, "destination_comparisons": destination_comparisons, "search_comparisons": search_comparisons,
        "limitations": ["search rectangle comparison allows bounded metric differences between renderer text engines", "current explicit destination corpus covers /XYZ with X/Y present and null zoom on normal and 90-degree rotated pages"],
        "failures": failures, "passed": not failures,
    }
    text = json.dumps(result, ensure_ascii=False, indent=2) + "\n"; print(text, end="")
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True); args.output.write_text(text, encoding="utf-8")
    return 0 if not failures else 2


if __name__ == "__main__": raise SystemExit(main())
