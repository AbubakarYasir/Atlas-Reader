#!/usr/bin/env python3
"""Checks an Atlas accessibility dump (atlas_reader --show a11y:<surface>@<file>).

Fails when a control a screen reader can reach has no name, or when the
Bookmarks tree does not expose its rows as named tree items with level,
open/closed state and page in their description (Narrator reads these).

usage: check_accessibility.py <dump.jsonl> [--expect-tree] [--expect-focus Role[,Role]]
"""
from __future__ import annotations

import json
import sys

NAMED_ROLES = {"Button", "CheckBox", "RadioButton", "EditableText", "ComboBox", "Slider",
               "SpinBox", "PageTab", "MenuItem", "ButtonMenu", "ButtonDropDown", "Link", "TreeItem"}


def main() -> int:
    path = sys.argv[1]
    lines = [json.loads(line) for line in open(path, encoding="utf-8") if line.strip()]
    nodes = [n for n in lines if not n.get("focusReport")]
    focus = next((n for n in lines if n.get("focusReport")), None)
    problems: list[str] = []
    for node in nodes:
        if node["disabled"]:
            continue
        if node["role"] in NAMED_ROLES and not node["name"].strip() and not node["description"].strip():
            problems.append(f"unnamed {node['role']} ({node['class']}) at depth {node['depth']}")
    if "--expect-tree" in sys.argv:
        trees = [n for n in nodes if n["role"] == "Tree"]
        items = [n for n in nodes if n["role"] == "TreeItem"]
        if not trees or not any(t["name"].strip() for t in trees):
            problems.append("no named Tree (Bookmarks panel)")
        if len(items) < 2:
            problems.append(f"only {len(items)} tree items")
        for item in items:
            d = item["description"]
            if not d:
                problems.append(f"tree item {item['name']!r} has no description")
        for item in items:
            if item.get("expandable") and item.get("expanded") == item.get("collapsed"):
                problems.append(f"tree item {item['name']!r} is expandable but reports no open/closed state")
        if items and not any(item.get("expandable") for item in items):
            problems.append("no tree item reports a native expandable state")
        if focus is not None and focus.get("role") != "TreeItem":
            problems.append(f"focused bookmarks list reports {focus.get('role')} {focus.get('name')!r} as focused, not the current row")
        if items and not any(item["selected"] for item in items):
            problems.append("no tree item reports the current (selected) row")
    roles: dict[str, int] = {}
    for node in nodes:
        roles[node["role"]] = roles.get(node["role"], 0) + 1
    if focus is not None and focus.get("role") == "Dialog" and not focus.get("name", "").strip():
        problems.append("focus is on an unnamed dialog (Narrator would say only 'dialog')")
    if focus is not None and focus.get("role") in (None, "NoRole"):
        problems.append(f"nothing a screen reader can announce has focus (item {focus.get('item')})")
    expect = None
    if "--expect-focus" in sys.argv:
        expect = sys.argv[sys.argv.index("--expect-focus") + 1].split(",")
    if expect and (focus is None or focus.get("role") not in expect):
        problems.append(f"focus is {focus.get('role') if focus else None} {focus.get('name', '') if focus else ''!r}; expected {'/'.join(expect)}")
    if focus is not None:
        print(f"  focus: {focus.get('role')} {focus.get('name', '')!r} (item {focus.get('item')} in {focus.get('itemParent')})")
    print(f"{path}: {len(nodes)} accessible nodes; " + ", ".join(f"{k} {v}" for k, v in sorted(roles.items())))
    for problem in problems:
        print("  PROBLEM:", problem)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
