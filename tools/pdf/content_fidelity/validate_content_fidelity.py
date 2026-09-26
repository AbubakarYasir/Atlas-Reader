#!/usr/bin/env python3
"""Validate N2 B1 image-only and B2 real-font Arabic/Urdu raster evidence."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

from PIL import Image, ImageChops, ImageStat

A013_EXPECTED = {
    "top_left": (255, 0, 0),
    "top_right": (0, 128, 0),
    "bottom_left": (0, 0, 255),
    "bottom_right": (255, 255, 0),
}
COLOR_TOLERANCE = 22
A014_MASK_IOU_MIN = 0.72
A014_MEAN_RGB_DIFF_MAX = 28.0


def load_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def composite_white(path: Path) -> Image.Image:
    image = Image.open(path).convert("RGBA")
    white = Image.new("RGBA", image.size, (255, 255, 255, 255))
    return Image.alpha_composite(white, image).convert("RGB")


def render_path(evidence: dict[str, Any], scale: int) -> Path:
    for render in evidence.get("renders", []):
        if int(render.get("scale", -1)) == scale:
            return Path(str(render["png"]))
    raise KeyError(f"missing render scale {scale}")


def color_close(actual: tuple[int, int, int], expected: tuple[int, int, int]) -> bool:
    return all(abs(a - b) <= COLOR_TOLERANCE for a, b in zip(actual, expected))


def validate_a013(engine: str, evidence: dict[str, Any], failures: list[str], details: dict[str, Any]) -> None:
    if not evidence.get("passed"):
        failures.append(f"{engine}:A013-probe-failed")
    if int(evidence.get("page_count", -1)) != 1:
        failures.append(f"{engine}:A013-page-count")
    if abs(float(evidence.get("width_points", -1)) - 200.0) > 0.05 or abs(float(evidence.get("height_points", -1)) - 200.0) > 0.05:
        failures.append(f"{engine}:A013-page-size")

    text_length = int(evidence.get("text_utf16_length", evidence.get("text_char_count", -1)))
    hits = int(evidence.get("search_hit_count", -1))
    if text_length != 0:
        failures.append(f"{engine}:A013-text-layer-not-empty:{text_length}")
    if hits != 0:
        failures.append(f"{engine}:A013-search-unexpected-hit:{hits}")

    engine_details: dict[str, Any] = {"text_length": text_length, "search_hits": hits, "scales": {}}
    for scale in (1, 2):
        image = composite_white(render_path(evidence, scale))
        expected_size = (200 * scale, 200 * scale)
        if image.size != expected_size:
            failures.append(f"{engine}:A013-{scale}x-size:{image.size}!={expected_size}")
        points = {
            "top_left": (0.25, 0.25),
            "top_right": (0.75, 0.25),
            "bottom_left": (0.25, 0.75),
            "bottom_right": (0.75, 0.75),
        }
        samples: dict[str, list[int]] = {}
        for name, (nx, ny) in points.items():
            x = min(image.width - 1, round(nx * (image.width - 1)))
            y = min(image.height - 1, round(ny * (image.height - 1)))
            actual = tuple(int(v) for v in image.getpixel((x, y)))
            samples[name] = list(actual)
            if not color_close(actual, A013_EXPECTED[name]):
                failures.append(f"{engine}:A013-{scale}x-{name}:{actual}!~{A013_EXPECTED[name]}")
        engine_details["scales"][str(scale)] = {"size": list(image.size), "samples": samples}
    details[engine] = engine_details


def ink_mask(image: Image.Image) -> Image.Image:
    rgb = image.convert("RGB")
    pixels = rgb.load()
    mask = Image.new("1", rgb.size, 0)
    out = mask.load()
    for y in range(rgb.height):
        for x in range(rgb.width):
            r, g, b = pixels[x, y]
            if min(r, g, b) < 225:
                out[x, y] = 1
    return mask


def mask_count(mask: Image.Image, box: tuple[int, int, int, int] | None = None) -> int:
    target = mask.crop(box) if box else mask
    return sum(1 for value in target.getdata() if value)


def mask_iou(a: Image.Image, b: Image.Image) -> float:
    intersection = ImageChops.logical_and(a, b)
    union = ImageChops.logical_or(a, b)
    union_count = mask_count(union)
    return 1.0 if union_count == 0 else mask_count(intersection) / union_count


def mean_rgb_diff(a: Image.Image, b: Image.Image) -> float:
    diff = ImageChops.difference(a.convert("RGB"), b.convert("RGB"))
    stats = ImageStat.Stat(diff)
    return sum(stats.mean[:3]) / 3.0


def validate_a014(qt: dict[str, Any], pdfium: dict[str, Any], failures: list[str]) -> dict[str, Any]:
    details: dict[str, Any] = {"scales": {}}
    for engine, evidence in (("qt-pdf", qt), ("pdfium", pdfium)):
        if not evidence.get("passed"):
            failures.append(f"{engine}:A014-probe-failed")
        if int(evidence.get("page_count", -1)) != 1:
            failures.append(f"{engine}:A014-page-count")
        if abs(float(evidence.get("width_points", -1)) - 600.0) > 0.1 or abs(float(evidence.get("height_points", -1)) - 480.0) > 0.1:
            failures.append(f"{engine}:A014-page-size")

    for scale in (1, 2):
        qt_img = composite_white(render_path(qt, scale))
        pdfium_img = composite_white(render_path(pdfium, scale))
        expected_size = (600 * scale, 480 * scale)
        if qt_img.size != expected_size:
            failures.append(f"qt-pdf:A014-{scale}x-size:{qt_img.size}!={expected_size}")
        if pdfium_img.size != expected_size:
            failures.append(f"pdfium:A014-{scale}x-size:{pdfium_img.size}!={expected_size}")
        if qt_img.size != pdfium_img.size:
            failures.append(f"A014-{scale}x-cross-engine-size")
            continue

        qt_mask = ink_mask(qt_img)
        pdfium_mask = ink_mask(pdfium_img)
        iou = mask_iou(qt_mask, pdfium_mask)
        rgb_diff = mean_rgb_diff(qt_img, pdfium_img)
        if iou < A014_MASK_IOU_MIN:
            failures.append(f"A014-{scale}x-mask-iou:{iou:.4f}<{A014_MASK_IOU_MIN}")
        if rgb_diff > A014_MEAN_RGB_DIFF_MAX:
            failures.append(f"A014-{scale}x-mean-rgb-diff:{rgb_diff:.3f}>{A014_MEAN_RGB_DIFF_MAX}")

        bands = [(12, 90), (95, 190), (200, 315), (335, 420)]
        band_details: list[dict[str, Any]] = []
        for index, (top, bottom) in enumerate(bands):
            box = (0, top * scale, expected_size[0], bottom * scale)
            qcount = mask_count(qt_mask, box)
            pcount = mask_count(pdfium_mask, box)
            minimum = 80 * scale
            if qcount < minimum:
                failures.append(f"qt-pdf:A014-{scale}x-band-{index}-ink:{qcount}<{minimum}")
            if pcount < minimum:
                failures.append(f"pdfium:A014-{scale}x-band-{index}-ink:{pcount}<{minimum}")
            band_details.append({"band": index, "qt_ink": qcount, "pdfium_ink": pcount})

        details["scales"][str(scale)] = {
            "size": list(expected_size),
            "mask_iou": iou,
            "mean_rgb_difference": rgb_diff,
            "bands": band_details,
        }
    return details


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--qt-a013", type=Path, required=True)
    parser.add_argument("--pdfium-a013", type=Path, required=True)
    parser.add_argument("--qt-a014", type=Path, required=True)
    parser.add_argument("--pdfium-a014", type=Path, required=True)
    parser.add_argument("--a014-sha256", required=True)
    parser.add_argument("--noto-naskh-archive-sha256", required=True)
    parser.add_argument("--noto-naskh-font-sha256", required=True)
    parser.add_argument("--noto-nastaliq-font-sha256", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    qt_a013 = load_json(args.qt_a013)
    pdfium_a013 = load_json(args.pdfium_a013)
    qt_a014 = load_json(args.qt_a014)
    pdfium_a014 = load_json(args.pdfium_a014)
    failures: list[str] = []
    a013_details: dict[str, Any] = {}
    validate_a013("qt-pdf", qt_a013, failures, a013_details)
    validate_a013("pdfium", pdfium_a013, failures, a013_details)
    a014_details = validate_a014(qt_a014, pdfium_a014, failures)

    evidence = {
        "schema": "atlas.n2.content-fidelity-validation.v2",
        "a013": {
            "sha256": "f6b1173e34bbd7dab6ad5d91c3000190e1de9f6c156536f06d7f48812bd40021",
            "purpose": "image-only-read-render-no-text-layer",
            "details": a013_details,
        },
        "a014": {
            "sha256": args.a014_sha256,
            "purpose": "readable-real-font-Arabic-Urdu-mixed-raster-fidelity",
            "noto_naskh_arabic_release": "2.021",
            "noto_naskh_archive_sha256": args.noto_naskh_archive_sha256,
            "noto_naskh_regular_ttf_sha256": args.noto_naskh_font_sha256,
            "noto_nastaliq_urdu_ttf_sha256": args.noto_nastaliq_font_sha256,
            "generator": "Qt 6.10.3 QPdfWriter/QPainter full text layout",
            "details": a014_details,
        },
        "limits": [
            "A013 proves raster reading and safe absence of a text layer; OCR remains out of scope",
            "A014 is visual real-font evidence; A006 remains the logical Unicode extraction/search oracle",
            "cross-engine visual comparison uses bounded mask/color similarity and does not require byte-identical PNGs",
            "automated cross-engine similarity cannot certify human readability; explicit owner visual acceptance remains mandatory",
        ],
        "failures": failures,
        "passed": not failures,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(evidence, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"passed": evidence["passed"], "failures": failures}, ensure_ascii=True))
    return 0 if evidence["passed"] else 2


if __name__ == "__main__":
    raise SystemExit(main())
