#!/usr/bin/env python3
"""Generate deterministic N2 production-route inventory and SPDX evidence."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


def file_hash(path: Path, algorithm: str) -> str:
    digest = hashlib.new(algorithm)
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def spdx_id(relative_path: str) -> str:
    safe = "".join(character if character.isalnum() else "-" for character in relative_path)
    return f"SPDXRef-File-{safe}"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--package-root", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--implementation-sha", required=True)
    parser.add_argument("--qpdf-archive-sha256", required=True)
    args = parser.parse_args()

    package_root = args.package_root.resolve()
    output_dir = args.output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    files = []
    for path in sorted(item for item in package_root.rglob("*") if item.is_file()):
        relative = path.relative_to(package_root).as_posix()
        files.append(
            {
                "path": relative,
                "bytes": path.stat().st_size,
                "sha1": file_hash(path, "sha1"),
                "sha256": file_hash(path, "sha256"),
            }
        )

    if not files:
        raise SystemExit("production package inventory is empty")

    qpdf_files = [item for item in files if item["path"].startswith("qpdf/")]
    qt_files = [item for item in files if not item["path"].startswith("qpdf/")]
    inventory = {
        "schemaVersion": 1,
        "checkpoint": "N2",
        "implementationSha": args.implementation_sha,
        "route": {
            "readRenderTextNavigation": "Qt PDF 6.10.3 official dynamic Windows distribution",
            "structureSecurityWrite": "qpdf 12.4.1 first-party MSVC64 CLI distribution",
            "standalonePdfiumRuntime": False,
        },
        "qpdfArchiveSha256": args.qpdf_archive_sha256,
        "totals": {
            "packageBytes": sum(item["bytes"] for item in files),
            "qtBytes": sum(item["bytes"] for item in qt_files),
            "qpdfBytes": sum(item["bytes"] for item in qpdf_files),
            "fileCount": len(files),
        },
        "files": files,
    }
    (output_dir / "runtime-inventory.json").write_text(
        json.dumps(inventory, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )

    spdx_files = [
        {
            "SPDXID": spdx_id(item["path"]),
            "fileName": f"./{item['path']}",
            "checksums": [{"algorithm": "SHA256", "checksumValue": item["sha256"]}],
            "licenseConcluded": "NOASSERTION",
            "copyrightText": "NOASSERTION",
        }
        for item in files
    ]
    relationships = [
        {"spdxElementId": "SPDXRef-DOCUMENT", "relationshipType": "DESCRIBES", "relatedSpdxElement": "SPDXRef-Package-Atlas-N2-Route"},
        {"spdxElementId": "SPDXRef-Package-Atlas-N2-Route", "relationshipType": "DEPENDS_ON", "relatedSpdxElement": "SPDXRef-Package-Qt-PDF"},
        {"spdxElementId": "SPDXRef-Package-Atlas-N2-Route", "relationshipType": "DEPENDS_ON", "relatedSpdxElement": "SPDXRef-Package-qpdf"},
    ]
    relationships.extend(
        {
            "spdxElementId": "SPDXRef-Package-Atlas-N2-Route",
            "relationshipType": "CONTAINS",
            "relatedSpdxElement": spdx_id(item["path"]),
        }
        for item in files
    )
    sbom = {
        "spdxVersion": "SPDX-2.3",
        "dataLicense": "CC0-1.0",
        "SPDXID": "SPDXRef-DOCUMENT",
        "name": f"atlas-reader-n2-production-route-{args.implementation_sha}",
        "documentNamespace": f"https://github.com/AbubakarYasir/Atlas-Reader/spdx/n2/{args.implementation_sha}",
        "creationInfo": {
            "creators": ["Tool: Atlas generate_n2_distribution_evidence.py"],
            "created": "2026-09-27T00:00:00Z",
        },
        "packages": [
            {
                "SPDXID": "SPDXRef-Package-Atlas-N2-Route",
                "name": "Atlas Reader N2 production dependency route",
                "versionInfo": args.implementation_sha,
                "downloadLocation": "NOASSERTION",
                "filesAnalyzed": True,
                "licenseConcluded": "NOASSERTION",
                "licenseDeclared": "NOASSERTION",
                "copyrightText": "NOASSERTION",
                "packageVerificationCode": {"packageVerificationCodeValue": hashlib.sha1("".join(sorted(item["sha1"] for item in files)).encode()).hexdigest()},
            },
            {
                "SPDXID": "SPDXRef-Package-Qt-PDF",
                "name": "Qt PDF",
                "versionInfo": "6.10.3",
                "downloadLocation": "https://download.qt.io/official_releases/qt/6.10/6.10.3/",
                "filesAnalyzed": False,
                "licenseConcluded": "NOASSERTION",
                "licenseDeclared": "LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only",
                "copyrightText": "NOASSERTION",
                "externalRefs": [{"referenceCategory": "PACKAGE-MANAGER", "referenceType": "purl", "referenceLocator": "pkg:generic/TheQtCompany/qtpdf@6.10.3"}],
            },
            {
                "SPDXID": "SPDXRef-Package-qpdf",
                "name": "qpdf",
                "versionInfo": "12.4.1",
                "downloadLocation": "https://github.com/qpdf/qpdf/releases/tag/v12.4.1",
                "filesAnalyzed": False,
                "licenseConcluded": "Apache-2.0",
                "licenseDeclared": "Apache-2.0",
                "copyrightText": "NOASSERTION",
                "checksums": [{"algorithm": "SHA256", "checksumValue": args.qpdf_archive_sha256}],
                "externalRefs": [{"referenceCategory": "PACKAGE-MANAGER", "referenceType": "purl", "referenceLocator": "pkg:github/qpdf/qpdf@v12.4.1"}],
            },
        ],
        "files": spdx_files,
        "relationships": relationships,
    }
    (output_dir / "atlas-n2-production-route.spdx.json").write_text(
        json.dumps(sbom, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )

    print(json.dumps(inventory["totals"], sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
