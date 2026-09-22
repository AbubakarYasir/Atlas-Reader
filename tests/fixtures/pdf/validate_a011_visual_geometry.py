#!/usr/bin/env python3
"""Independent raw-geometry validator for A011 using pinned pypdf only."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path

from pypdf import PdfReader

ROOT = Path(__file__).resolve().parent
PDF = ROOT / "generated" / "A011_visual_geometry.pdf"
EXPECTED_SHA256 = "ebf82d49391c4df27f50afc2eb785aa828effbdeb85039ec2bb4c26d1896f824"

EXPECTED = [
    {
        "media_box": [0.0, 0.0, 200.0, 300.0],
        "crop_box": [0.0, 0.0, 200.0, 300.0],
        "rotation": 0,
        "visible_points": [200.0, 300.0],
    },
    {
        "media_box": [0.0, 0.0, 300.0, 200.0],
        "crop_box": [50.0, 25.0, 250.0, 175.0],
        "rotation": 0,
        "visible_points": [200.0, 150.0],
    },
    {
        "media_box": [0.0, 0.0, 200.0, 300.0],
        "crop_box": [0.0, 0.0, 200.0, 300.0],
        "rotation": 90,
        "visible_points": [300.0, 200.0],
    },
]


def box_values(box) -> list[float]:
    return [float(box.left), float(box.bottom), float(box.right), float(box.top)]


def main() -> int:
    failures: list[str] = []
    actual_sha = hashlib.sha256(PDF.read_bytes()).hexdigest()
    if actual_sha != EXPECTED_SHA256:
        failures.append(f"sha256:{actual_sha}!={EXPECTED_SHA256}")

    reader = PdfReader(str(PDF), strict=False)
    if len(reader.pages) != len(EXPECTED):
        failures.append(f"page-count:{len(reader.pages)}!={len(EXPECTED)}")

    pages: list[dict[str, object]] = []
    for index, expected in enumerate(EXPECTED):
        page = reader.pages[index]
        media = box_values(page.mediabox)
        crop = box_values(page.cropbox)
        rotation = int(page.get("/Rotate", 0) or 0) % 360
        crop_width = crop[2] - crop[0]
        crop_height = crop[3] - crop[1]
        visible = [crop_height, crop_width] if rotation in (90, 270) else [crop_width, crop_height]

        if media != expected["media_box"]:
            failures.append(f"page-{index}-media:{media}!={expected['media_box']}")
        if crop != expected["crop_box"]:
            failures.append(f"page-{index}-crop:{crop}!={expected['crop_box']}")
        if rotation != expected["rotation"]:
            failures.append(f"page-{index}-rotation:{rotation}!={expected['rotation']}")
        if visible != expected["visible_points"]:
            failures.append(f"page-{index}-visible:{visible}!={expected['visible_points']}")

        pages.append(
            {
                "index": index,
                "media_box": media,
                "crop_box": crop,
                "rotation": rotation,
                "visible_points": visible,
            }
        )

    # Page 0 carries the annotation-fidelity sentinel.
    annots = reader.pages[0].get("/Annots", []) or []
    if len(annots) != 1:
        failures.append(f"annotation-count:{len(annots)}!=1")
    else:
        annot = annots[0].get_object()
        if str(annot.get("/Subtype")) != "/Link":
            failures.append("annotation-subtype-not-link")
        rect = [float(v) for v in annot.get("/Rect", [])]
        if rect != [70.0, 125.0, 130.0, 175.0]:
            failures.append(f"annotation-rect:{rect}")
        action = annot.get("/A")
        action = action.get_object() if hasattr(action, "get_object") else action
        if not action or str(action.get("/URI")) != "https://example.com/atlas-a011":
            failures.append("annotation-uri-mismatch")

    evidence = {
        "schema": "atlas.n2.a011-independent-geometry.v1",
        "validator": "pypdf",
        "validator_version": "5.9.0",
        "sha256": actual_sha,
        "pages": pages,
        "annotation_count_page_0": len(annots),
        "failures": failures,
        "passed": not failures,
    }
    print(json.dumps(evidence, indent=2))
    return 0 if not failures else 2


if __name__ == "__main__":
    raise SystemExit(main())
