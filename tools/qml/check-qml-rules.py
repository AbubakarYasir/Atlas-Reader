"""QML rules that Qt only reports at run time, as warnings nobody sees.

1. `X.Accessible.announce(...)`: X must be an Item. On a Popup, Dialog or Menu
   Qt prints "Accessible attached property must be attached to an object
   deriving from Item" and the announcement is dropped, so screen readers hear
   nothing (N5: "Bookmarks saved", save errors).
2. A TextField or TextInput that is shown and hidden with `visible:` must give
   focus up when it hides (`focus = false`); a hidden field that keeps focus
   swallows Delete, Ctrl+Z and the arrow keys (N5: bookmark rename field,
   title-bar search).

    python tools/qml/check-qml-rules.py
"""
from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2] / "qml"
NON_ITEMS = {"Popup", "Dialog", "Menu", "ToolTip", "Timer", "Connections", "QtObject", "Shortcut"}


def type_of_id(text: str, ident: str) -> str | None:
    match = re.search(rf"\bid:\s*{re.escape(ident)}\b", text)
    if not match:
        return None
    depth = 0
    for index in range(match.start() - 1, -1, -1):
        char = text[index]
        if char == "}":
            depth += 1
        elif char == "{":
            if depth == 0:
                head = re.search(r"([A-Za-z_][\w.]*)\s*$", text[:index])
                return head.group(1).split(".")[-1] if head else None
            depth -= 1
    return None


def block_after(text: str, start: int) -> str:
    depth = 0
    for index in range(start, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start:index + 1]
    return text[start:]


def main() -> int:
    problems: list[str] = []
    for path in sorted(ROOT.rglob("*.qml")):
        text = path.read_text(encoding="utf-8")
        for call in re.finditer(r"\b(\w+)\.Accessible\.announce\(", text):
            kind = type_of_id(text, call.group(1))
            if kind in NON_ITEMS:
                line = text.count("\n", 0, call.start()) + 1
                problems.append(f"{path.name}:{line}: Accessible.announce on {kind} '{call.group(1)}' is dropped; call it on an Item")
        for field in re.finditer(r"\b(TextField|TextInput|AtlasTextField)\s*\{", text):
            block = block_after(text, field.end() - 1)
            own = block[1:-1]
            while re.search(r"\{[^{}]*\}", own):          # drop child objects and handlers
                own = re.sub(r"\{[^{}]*\}", "", own)
            if not re.search(r"^\s*visible:\s*(?!true\b)", own, re.M):
                continue
            ident = re.search(r"\bid:\s*(\w+)", own)
            name = ident.group(1) if ident else "?"
            if "focus = false" in block or re.search(rf"\b{name}\.focus\s*=\s*false", text):
                continue
            line = text.count("\n", 0, field.start()) + 1
            problems.append(f"{path.name}:{line}: text field '{name}' hides with visible: but never gives focus up (focus = false)")
    for problem in problems:
        print("PROBLEM " + problem)
    print(f"QML rules: {'FAIL' if problems else 'PASS'} ({len(list(ROOT.rglob('*.qml')))} files)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
