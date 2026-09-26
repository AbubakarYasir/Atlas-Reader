#!/usr/bin/env python3
"""Validate A011 Qt PDF/PDFium raster evidence semantically."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

EXPECTED_VISIBLE = {
    0: (200.0, 300.0),
    1: (200.0, 150.0),
    2: (300.0, 200.0),
}

EXPECTED_SAMPLES = {
    0: {
        "top_left_red": (255, 0, 0, 255),
        "top_right_green": (0, 128, 0, 255),
        "bottom_left_blue": (0, 0, 255, 255),
        "bottom_right_yellow": (255, 255, 0, 255),
    },
    1: {
        "crop_magenta": (255, 0, 255, 255),
        "crop_cyan": (0, 255, 255, 255),
    },
    2: {
        "rotated_top_left_blue": (0, 0, 255, 255),
        "rotated_top_right_red": (255, 0, 0, 255),
        "rotated_bottom_left_yellow": (255, 255, 0, 255),
        "rotated_bottom_right_green": (0, 128, 0, 255),
    },
}

# Native blank-page behavior differs by API. QPdfDocument::render() leaves
# untouched pixels transparent. The PDFium qualification probe deliberately
# pre-fills its caller-owned bitmap opaque white before rendering. Both are
# valid as long as Atlas treats background/compositing as an adapter policy.
EXPECTED_BLANK = {
    "qt-pdf": (0, 0, 0, 0),
    "pdfium": (255, 255, 255, 255),
}

TOLERANCE = 24


def rgba(sample: dict[str, Any]) -> tuple[int, int, int, int]:
    return tuple(int(sample[channel]) for channel in ("r", "g", "b", "a"))


def close_color(actual: tuple[int, ...], expected: tuple[int, ...]) -> bool:
    return all(abs(a - b) <= TOLERANCE for a, b in zip(actual, expected))


def find_render(page: dict[str, Any], scale: int, annotations: bool) -> dict[str, Any] | None:
    for render in page.get("renders", []):
        if int(render.get("scale", -1)) == scale and bool(render.get("annotations")) == annotations:
            return render
    return None


def validate_engine(evidence: dict[str, Any]) -> list[str]:
    failures: list[str] = []
    engine = str(evidence.get("engine", "unknown"))
    if engine not in EXPECTED_BLANK:
        failures.append(f"{engine}:unknown-background-policy")
        return failures
    blank = EXPECTED_BLANK[engine]

    if not evidence.get("passed"):
        failures.append(f"{engine}:probe-not-passed")

    pages = evidence.get("pages", [])
    if len(pages) != 3:
        failures.append(f"{engine}:page-count:{len(pages)}")
        return failures

    for page_index, page in enumerate(pages):
        expected_width, expected_height = EXPECTED_VISIBLE[page_index]
        width = float(page.get("visible_width_points", -1))
        height = float(page.get("visible_height_points", -1))
        if abs(width - expected_width) > 0.05 or abs(height - expected_height) > 0.05:
            failures.append(
                f"{engine}:page-{page_index}-visible:{width}x{height}!={expected_width}x{expected_height}"
            )

        for scale in (1, 2):
            render = find_render(page, scale, False)
            if render is None:
                failures.append(f"{engine}:page-{page_index}-scale-{scale}-render-missing")
                continue

            expected_px_width = int(round(expected_width * scale))
            expected_px_height = int(round(expected_height * scale))
            if int(render.get("width", -1)) != expected_px_width or int(render.get("height", -1)) != expected_px_height:
                failures.append(
                    f"{engine}:page-{page_index}-scale-{scale}-pixels:"
                    f"{render.get('width')}x{render.get('height')}!={expected_px_width}x{expected_px_height}"
                )

            samples = render.get("samples", {})
            for name, expected in EXPECTED_SAMPLES[page_index].items():
                if name not in samples:
                    failures.append(f"{engine}:page-{page_index}-scale-{scale}-sample-missing:{name}")
                    continue
                actual = rgba(samples[name])
                if not close_color(actual, expected):
                    failures.append(
                        f"{engine}:page-{page_index}-scale-{scale}-{name}:{actual}!~{expected}"
                    )

            if page_index == 1:
                for name in ("crop_top_left_white", "crop_bottom_right_white"):
                    if name not in samples:
                        failures.append(f"{engine}:page-1-scale-{scale}-sample-missing:{name}")
                        continue
                    actual = rgba(samples[name])
                    if not close_color(actual, blank):
                        failures.append(
                            f"{engine}:page-1-scale-{scale}-{name}:{actual}!~background{blank}"
                        )

        if page_index == 0:
            for scale in (1, 2):
                without = find_render(page, scale, False)
                with_annotations = find_render(page, scale, True)
                if without is None or with_annotations is None:
                    failures.append(f"{engine}:annotation-render-missing-scale-{scale}")
                    continue
                plain = rgba(without["samples"]["annotation_probe"])
                annotated = rgba(with_annotations["samples"]["annotation_probe"])
                if not close_color(plain, blank):
                    failures.append(f"{engine}:annotation-off-background-scale-{scale}:{plain}!~{blank}")
                if not close_color(annotated, (255, 0, 255, 255)):
                    failures.append(f"{engine}:annotation-on-not-magenta-scale-{scale}:{annotated}")

    return failures


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--qt", type=Path, required=True)
    parser.add_argument("--pdfium", type=Path, required=True)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    qt = json.loads(args.qt.read_text(encoding="utf-8-sig"))
    pdfium = json.loads(args.pdfium.read_text(encoding="utf-8-sig"))
    failures = validate_engine(qt) + validate_engine(pdfium)

    result = {
        "schema": "atlas.n2.a011-fidelity-validation.v1",
        "fixture_sha256": "b25d6b6716fbbcf095cbd68bcf57913d4e14bca3e64be19f513c35fd3ddadd29",
        "engines": ["qt-pdf", "pdfium"],
        "scales": [1, 2],
        "color_tolerance": TOLERANCE,
        "blank_background_policy": {
            "qt-pdf": "native transparent untouched pixels",
            "pdfium": "caller-owned bitmap prefilled opaque white by qualification probe",
        },
        "checks": [
            "effective-visible-geometry",
            "cropbox-visibility",
            "inherent-rotation-orientation",
            "solid-color-landmarks-1x-2x",
            "native-background-behavior",
            "annotation-off-on-explicit-appearance-sentinel",
        ],
        "rotation_evidence": "raw /Rotate is independently validated by pypdf; each engine must then expose the rotated effective size and render the rotated semantic landmarks at 1x and 2x",
        "failures": failures,
        "passed": not failures,
    }

    text = json.dumps(result, indent=2) + "\n"
    print(text, end="")
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text, encoding="utf-8")
    return 0 if not failures else 2


if __name__ == "__main__":
    raise SystemExit(main())
