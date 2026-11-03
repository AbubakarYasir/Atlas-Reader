#!/usr/bin/env python3
"""Interface string governance (ADR-0006).

Fails when:
  * a key is missing or empty in any of en / ar / ur,
  * a translation's %1..%n placeholders differ from English,
  * a QML file uses an I18n key that does not exist,
  * a QML view contains a hard-coded user-facing string literal.
"""
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
STRINGS = ROOT / "qml" / "i18n" / "Strings.js"
LANGUAGES = ("en", "ar", "ur")
# Keys built at runtime from a prefix plus a known suffix set.
DYNAMIC = {
    "availability.": ["available", "locked", "offline", "missing", "unreadable",
                      "unsupported", "readOnly", "unavailable"],
    "view.colors.": ["0", "1", "2", "3"],
    "bookmarks.state.": ["embedded", "local_only", "pending_embed", "local_override", "locally_hidden", "conflict"],
    "bookmarks.readOnly.": ["notInLibrary", "ambiguous", "outlineUnsupported", "storageError"],
    "bookmarks.refused.": ["readOnly", "atTop", "atBottom", "atTopLevel", "nothingToIndentInto", "unhideFirst",
                           "resolveFirst", "containsSourceNodes", "cycle", "tooDeep", "tooManyNodes", "badTitle",
                           "historyStale", "other"],
    "skins.color.": ["blue", "midnight", "sky", "teal", "plum", "graphite"],
    "capability.state.": ["restricted", "readOnly", "locked", "signed", "changed", "unknown", "writable"],
    "capability.reason.": ["missing", "offline", "cloudPlaceholder", "passwordRequired", "unsupportedSecurity",
                           "unreadable", "malformed", "notInspected", "outlineNotPermitted",
                           "annotationsNotPermitted", "fileReadOnly", "folderReadOnly", "sharingLocked",
                           "signatures", "certified", "changedExternally"],
}
# Literal text allowed in QML views (symbols, language names shown in their own script).
ALLOWED_LITERALS = {"·", "English", "العربية", "اردو", "%", "/", "…"}


def load_tables():
    source = STRINGS.read_text(encoding="utf-8")
    body = source[source.index("var tables =") + len("var tables ="):].strip()
    body = body.rstrip(";").strip()
    body = re.sub(r"^\s*//.*$", "", body, flags=re.M)
    body = re.sub(r"(\{|,)\s*(en|ar|ur)\s*:", r'\1 "\2":', body)
    body = re.sub(r",(\s*[}\]])", r"\1", body)
    return json.loads(body)


def placeholders(text):
    return sorted(set(re.findall(r"%\d", text)))


def main():
    errors = []
    tables = load_tables()
    english = tables.get("en", {})
    for language in LANGUAGES:
        table = tables.get(language)
        if table is None:
            errors.append(f"missing language table: {language}")
            continue
        for key, value in english.items():
            if key not in table:
                errors.append(f"[{language}] missing key: {key}")
            elif not str(table[key]).strip():
                errors.append(f"[{language}] empty value: {key}")
            elif placeholders(table[key]) != placeholders(value):
                errors.append(f"[{language}] placeholder mismatch in {key}: "
                              f"{placeholders(table[key])} vs {placeholders(value)}")
        for key in table:
            if key not in english:
                errors.append(f"[{language}] key not in English source: {key}")

    used = set()
    for qml in sorted((ROOT / "qml").glob("*.qml")):
        text = qml.read_text(encoding="utf-8")
        for key in re.findall(r'I18n\.(?:t|n)\("([^"]+)"', text):
            if key.endswith("."):
                for suffix in DYNAMIC.get(key, []):
                    used.add(key + suffix)
                if key not in DYNAMIC:
                    errors.append(f"{qml.name}: dynamic key prefix without a known suffix list: {key}")
            else:
                used.add(key)
        for number, line in enumerate(text.splitlines(), 1):
            match = re.search(r'\b(text|placeholderText|title|ToolTip\.text|Accessible\.(?:name|description))\s*:\s*"([^"]*)"', line)
            if match and match.group(2).strip() and match.group(2).strip() not in ALLOWED_LITERALS:
                literal = match.group(2).strip()
                if re.fullmatch(r"((Ctrl|Alt|Shift)\+)*(F\d{1,2}|Esc|[A-Z0-9=+\-]|Page (Up|Down)|Home|End)", literal):
                    continue  # keyboard shortcut labels are identical in every language
                if re.search(r"[A-Za-z؀-ۿ]", literal):
                    errors.append(f"{qml.name}:{number}: hard-coded text \"{match.group(2)}\"; use I18n.t()")
    for key in sorted(used):
        if key not in english:
            errors.append(f"QML uses unknown key: {key}")

    if errors:
        print("\n".join(errors))
        print(f"Interface strings FAILED with {len(errors)} error(s).")
        return 1
    print(f"Interface strings PASS: {len(english)} keys × {len(LANGUAGES)} languages, "
          f"{len(used)} keys used in QML.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
