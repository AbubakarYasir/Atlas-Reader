#!/usr/bin/env python3
"""Every character the interface shows must be in the font that draws it.

On Windows a single character missing from a text's font makes Qt read every
installed font family before it can draw the text: seconds on a PC with
thousands of fonts, and it happens before the first frame (N1-03,
src/app/fonts/InterfaceFonts.h). This check keeps that from coming back.

  * Arabic interface  -> assets/fonts/AtlasNaskh-{Regular,SemiBold}.ttf
  * Urdu interface    -> assets/fonts/AtlasNastaliq-{Regular,SemiBold}.ttf
  * English interface -> the Windows interface font (Segoe UI), when present
    on this machine or given with --system-font; skipped otherwise.

Checked text: every string in qml/i18n/Strings.js, every string literal in
qml/*.qml, printable ASCII and the direction marks I18n.qml inserts.

    python tools/fonts/check-ui-coverage.py [--system-font FILE ...]
"""
from __future__ import annotations

import argparse
import importlib.util
import pathlib
import re
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
FONTS = ROOT / "assets" / "fonts"
DIRECTION_MARKS = {0x200E, 0x200F, 0x202A, 0x202B, 0x202C}
ASCII = set(range(0x20, 0x7F))
WINDOWS_FONTS = [pathlib.Path(r"C:\Windows\Fonts\segoeui.ttf"), pathlib.Path(r"C:\Windows\Fonts\seguisb.ttf")]


def cmap_codepoints(path: pathlib.Path) -> set[int]:
    """Code points mapped by a TrueType/OpenType font (cmap formats 4 and 12)."""
    data = path.read_bytes()
    num_tables = struct.unpack_from(">H", data, 4)[0]
    cmap_offset = None
    for index in range(num_tables):
        tag, _, offset, _ = struct.unpack_from(">4sLLL", data, 12 + 16 * index)
        if tag == b"cmap":
            cmap_offset = offset
    if cmap_offset is None:
        raise ValueError(f"{path}: no cmap table")
    covered: set[int] = set()
    count = struct.unpack_from(">H", data, cmap_offset + 2)[0]
    for index in range(count):
        platform, encoding, sub = struct.unpack_from(">HHL", data, cmap_offset + 4 + 8 * index)
        if not (platform == 0 or (platform == 3 and encoding in (1, 10))):
            continue
        base = cmap_offset + sub
        fmt = struct.unpack_from(">H", data, base)[0]
        if fmt == 4:
            seg_count = struct.unpack_from(">H", data, base + 6)[0] // 2
            ends = struct.unpack_from(f">{seg_count}H", data, base + 14)
            starts = struct.unpack_from(f">{seg_count}H", data, base + 16 + 2 * seg_count)
            deltas = struct.unpack_from(f">{seg_count}h", data, base + 16 + 4 * seg_count)
            range_base = base + 16 + 6 * seg_count
            offsets = struct.unpack_from(f">{seg_count}H", data, range_base)
            for seg in range(seg_count):
                for code in range(starts[seg], ends[seg] + 1):
                    if code == 0xFFFF:
                        continue
                    if offsets[seg] == 0:
                        glyph = (code + deltas[seg]) & 0xFFFF
                    else:
                        position = range_base + 2 * seg + offsets[seg] + 2 * (code - starts[seg])
                        glyph = struct.unpack_from(">H", data, position)[0]
                        if glyph:
                            glyph = (glyph + deltas[seg]) & 0xFFFF
                    if glyph:
                        covered.add(code)
        elif fmt == 12:
            groups = struct.unpack_from(">L", data, base + 12)[0]
            for group in range(groups):
                start, end, glyph = struct.unpack_from(">LLL", data, base + 16 + 12 * group)
                if glyph or end > start:
                    covered.update(range(start, end + 1))
    return covered


def load_tables() -> dict[str, dict[str, str]]:
    spec = importlib.util.spec_from_file_location("check_strings", ROOT / "tools" / "i18n" / "check-strings.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.load_tables()


def qml_literal_characters() -> set[int]:
    found: set[int] = set()
    for path in sorted((ROOT / "qml").glob("*.qml")):
        text = path.read_text(encoding="utf-8")
        text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
        for line in text.splitlines():
            line = re.sub(r'(?<![:"\'])//.*$', "", line)
            for literal in re.findall(r'"((?:[^"\\]|\\.)*)"', line):
                literal = re.sub(r"\\u([0-9A-Fa-f]{4})", lambda m: chr(int(m.group(1), 16)), literal)
                found.update(ord(ch) for ch in literal if ord(ch) > 0x7E)
    return found


def is_arabic_script(code: int) -> bool:
    return (0x0600 <= code <= 0x06FF or 0x0750 <= code <= 0x077F or 0x08A0 <= code <= 0x08FF
            or 0xFB50 <= code <= 0xFDFF or 0xFE70 <= code <= 0xFEFF or code in (0x200C, 0x200D))


def describe(codes: set[int]) -> str:
    return " ".join(f"U+{code:04X}" for code in sorted(codes))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--system-font", type=pathlib.Path, action="append", default=[])
    args = parser.parse_args()

    tables = load_tables()
    chars = {lang: {ord(ch) for value in table.values() for ch in value if ord(ch) >= 0x20}
             for lang, table in tables.items()}
    literals = qml_literal_characters()
    latin_literals = {code for code in literals if not is_arabic_script(code)}
    arabic_literals = literals - latin_literals
    common = ASCII | DIRECTION_MARKS | latin_literals

    errors: list[str] = []
    bundled = {
        "ar": ["AtlasNaskh-Regular.ttf", "AtlasNaskh-SemiBold.ttf"],
        "ur": ["AtlasNastaliq-Regular.ttf", "AtlasNastaliq-SemiBold.ttf"],
    }
    covered_by_bundled: set[int] = set()
    for lang, files in bundled.items():
        # English strings too: a key missing from a translation falls back to English.
        needed = chars[lang] | chars["en"] | common
        for name in files:
            covered = cmap_codepoints(FONTS / name)
            covered_by_bundled |= covered
            missing = needed - covered
            if missing:
                errors.append(f"{name} lacks characters the {lang} interface shows: {describe(missing)}")
    stray = arabic_literals - covered_by_bundled
    if stray:
        errors.append(f"QML literals not in any bundled font: {describe(stray)}")

    system_fonts = args.system_font or [path for path in WINDOWS_FONTS if path.exists()]
    if system_fonts:
        needed = chars["en"] | common
        for path in system_fonts:
            missing = needed - cmap_codepoints(path)
            if missing:
                errors.append(f"{path.name} lacks characters the English interface shows: {describe(missing)}")
    else:
        print("English interface: no Windows interface font on this machine; not checked here.")

    if errors:
        print("\n".join(errors))
        return 1
    checked = ", ".join(name for files in bundled.values() for name in files)
    print(f"Interface text is covered by its fonts ({checked}"
          + (", " + ", ".join(path.name for path in system_fonts) if system_fonts else "") + ").")
    return 0


if __name__ == "__main__":
    sys.exit(main())
