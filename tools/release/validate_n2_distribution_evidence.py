#!/usr/bin/env python3
"""Fail closed when the selected N2 package/compliance evidence is incomplete."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


EXPECTED_QPDF_FILES = {
    "qpdf/qpdf.exe": (19456, "57c003e868fb66cd343fd5afb91be8c2277f56434eea8635762499731bf9f60d"),
    "qpdf/qpdf30.dll": (7438848, "36fe5b2023f244e5785d96b8372dba26b75ea51b63adf4d4f1e66ad0aa1c8a61"),
    "qpdf/concrt140.dll": (374200, "54716f0738af891f283d213b5c8d11b25896bb8ee3097d301eae718560cf974e"),
    "qpdf/msvcp140.dll": (643512, "7c26614e1d733892c2deac7e245ce115504b1d80592dd0a01b08e3e5a55f89ca"),
    "qpdf/msvcp140_1.dll": (35768, "206c931bf90fdad8816de3b5e2ef80b2bcaa9406c89ecc05fe6fddffe251e982"),
    "qpdf/msvcp140_2.dll": (274872, "d50d7883f20d1dc6191768d3746f52dd9cac89c346ffaed5be1f110c2f34a838"),
    "qpdf/msvcp140_atomic_wait.dll": (57792, "3d0cbfaa1bf3eecf5a3f4491d2960ee803cb994f30292c6adc4a07c498f60e2b"),
    "qpdf/msvcp140_codecvt_ids.dll": (31160, "8a65c7596ef2e6938731f5a1058e7e40145b6d97967cc649231a076b9a608d78"),
    "qpdf/vcruntime140.dll": (178616, "d1f4225df2cd877dbf130d5668a021dce3f94118455ff5ec952061c30afc9ce7"),
    "qpdf/vcruntime140_1.dll": (50112, "a7146c08f89fe5b04541ab507cdb59ff7b44534d4ba3c668a426c6450a03434e"),
}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", type=Path, required=True)
    args = parser.parse_args()
    root = args.evidence_dir
    inventory = json.loads((root / "runtime-inventory.json").read_text(encoding="utf-8"))
    sbom = json.loads((root / "atlas-n2-production-route.spdx.json").read_text(encoding="utf-8"))
    files = {item["path"]: (item["bytes"], item["sha256"]) for item in inventory["files"]}

    for path, expected in EXPECTED_QPDF_FILES.items():
        if files.get(path) != expected:
            raise SystemExit(f"qpdf runtime identity mismatch for {path}: {files.get(path)} != {expected}")
    if inventory["totals"]["qpdfBytes"] != 9104336:
        raise SystemExit(f"unexpected qpdf runtime footprint: {inventory['totals']['qpdfBytes']}")
    if inventory["route"]["standalonePdfiumRuntime"] is not False:
        raise SystemExit("standalone PDFium must not be a selected N2 runtime")
    if any("pdfium.dll" in path.lower() for path in files):
        raise SystemExit("standalone pdfium.dll leaked into selected production package")
    if not any(path.lower().endswith("qt6pdf.dll") for path in files):
        raise SystemExit("Qt6Pdf.dll is missing from selected production package")
    if not (root / "qt-upstream-sbom").is_dir() or not any((root / "qt-upstream-sbom").iterdir()):
        raise SystemExit("official installed Qt SPDX SBOM directory is missing or empty")
    if not (root / "licenses" / "qt-LICENSES").is_dir() or not any((root / "licenses" / "qt-LICENSES").iterdir()):
        raise SystemExit("official installed Qt license directory is missing or empty")
    required_compliance = [
        root / "licenses" / "qpdf-LICENSE.txt",
        root / "licenses" / "qpdf-NOTICE.md",
        root / "licenses" / "qpdf-binary-license.html",
    ]
    for path in required_compliance:
        if not path.is_file() or path.stat().st_size == 0:
            raise SystemExit(f"required compliance file missing: {path}")
    if sbom.get("spdxVersion") != "SPDX-2.3" or not sbom.get("files"):
        raise SystemExit("Atlas SPDX inventory is incomplete")
    print(json.dumps(inventory["totals"], sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
