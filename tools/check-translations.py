#!/usr/bin/env python3
"""Every string the sources mark for translation (tr, QT_TR_NOOP, QCoreApplication::translate) has a
Russian entry in the translation table (the i18n.cpp under the source folder), and every entry there
is still used. Shared by the Qt projects Sotto and PkgScope.

    tools/check-translations.py [src]
"""
import pathlib
import re
import sys

src = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else pathlib.Path(__file__).parent.parent / "src")

LITERAL = re.compile(r'\s*"((?:[^"\\]|\\.)*)"')
CALL = re.compile(r'\b(?:tr|QT_TR_NOOP)\(|\btranslate\(\s*"(?:[^"\\]|\\.)*"\s*,')


def unescape(s):
    return re.sub(r'\\(.)', lambda m: {"n": "\n", "t": "\t"}.get(m.group(1), m.group(1)), s)


def literals_at(text, pos):
    """The string made of the adjacent literals starting at pos, or None."""
    parts = []
    while True:
        m = LITERAL.match(text, pos)
        if not m:
            break
        parts.append(unescape(m.group(1)))
        pos = m.end()
    return "".join(parts) if parts else None


used = {}
tables = [p for p in src.rglob("*.cpp") if p.name.lower() == "i18n.cpp"]
if len(tables) != 1:
    sys.exit(f"expected one i18n.cpp under {src}, found {len(tables)}")

for path in sorted(src.rglob("*.cpp")):
    if path == tables[0]:
        continue
    text = path.read_text(encoding="utf-8")
    for m in CALL.finditer(text):
        s = literals_at(text, m.end())
        if s:
            used.setdefault(s, f"{path.relative_to(src)}:{text.count(chr(10), 0, m.start()) + 1}")

table = tables[0].read_text(encoding="utf-8")
body = table[table.index("table{"):table.index("};", table.index("table{"))]
entries = {}
STR = r'(?:QStringLiteral\(\s*)?"((?:[^"\\]|\\.)*)"\s*\)?'
for m in re.finditer(r'\{\s*' + STR + r'\s*,\s*' + STR + r'\s*\}', body):
    key = unescape(m.group(1))
    if key in entries:
        print(f"duplicate entry: {key!r}")
        sys.exit(1)
    entries[key] = unescape(m.group(2))

missing = sorted(set(used) - set(entries))
unused = sorted(set(entries) - set(used))
for s in missing:
    print(f"no Russian translation ({used[s]}): {s!r}")
for s in unused:
    print(f"translation no longer used: {s!r}")
print(f"{len(used)} strings, {len(missing)} missing, {len(unused)} unused")
sys.exit(1 if missing or unused else 0)
