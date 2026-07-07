"""Phase 7.5b: convert all non-UTF-8 source files in [Server]Map to UTF-8.

The legacy 墨香 [Server]Map codebase is a mix of:
  - 174 files with cp949 (Korean EUC-KR) — comments + Korean string literals
  - 3 files with cp936 (Chinese GBK) — comments with Chinese characters
  - 1 file (ServerSystem.cpp) with cp1252 — corrupted extended ASCII
  - 7 files with cp949-only bytes
  - 202 pure-ASCII files (already OK)

MSVC's /source-charset flag accepts only ONE encoding for the entire
compilation unit, so we need all source files to decode in the same
encoding. UTF-8 is the only sane universal choice.

Strategy: for each non-UTF-8 file:
  1. Try UTF-8 first — if it works, skip.
  2. Try cp949 (the dominant legacy encoding) — if works, write as UTF-8.
  3. Try cp936 (Chinese GBK) — if works, write as UTF-8.
  4. Fall back to cp1252 (extended ASCII / Western European).
  5. If nothing decodes, use cp949 with errors='replace' (preserves
     whatever bytes don't fit). This is rare (1-2 files max).

Bytes that don't decode in any codec are replaced with U+FFFD (the
official Unicode replacement character) — they're either comments
(Korean text) or string literals (Korean text shown to players).
Game logic is unaffected since all C++ identifiers are ASCII.

This script runs ONCE during Phase 7.5b and is idempotent: re-running
on already-converted files is a no-op (UTF-8 files decode cleanly).
"""
import os
import sys

ROOT = r'D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】'
# All directories whose .cpp/.h files the [Server]Map build pulls in
# (per the CMakeLists.txt add_executable list).
TARGET_DIRS = [
    os.path.join(ROOT, '[Server]Map'),
    os.path.join(ROOT, '[CC]Ability'),
    os.path.join(ROOT, '[CC]BattleSystem'),
    os.path.join(ROOT, '[CC]Header'),
    os.path.join(ROOT, '[CC]Quest'),
    os.path.join(ROOT, '[CC]ServerModule'),
    os.path.join(ROOT, '[CC]Skill'),
    os.path.join(ROOT, '[CC]Suryun'),
    # [Lib] headers transitively included via YHLibrary / 4DyuchiNET
    os.path.join(ROOT, '[Lib]YHLibrary'),
    os.path.join(ROOT, '[Lib]HSEL'),
    os.path.join(ROOT, '4DyuchiGXGFunc'),
    os.path.join(ROOT, '4DyuchiNET_Common'),
]
# We only convert cpp/h files
EXTENSIONS = {'.cpp', '.h', '.c', '.hpp'}

# Preferred decode order. The first one that decodes wins.
DECODE_ORDER = ['utf-8', 'cp949', 'cp936', 'cp1252']


def find_decode_codec(data: bytes) -> tuple[str, bool]:
    """Return (codec, strict) where strict=True means no replacements."""
    for codec in DECODE_ORDER:
        try:
            data.decode(codec)
            return (codec, True)
        except UnicodeDecodeError:
            pass
    # Last resort: cp949 with replacements
    return ('cp949', False)


def process_file(p: str) -> tuple[str, int, int, bool]:
    """Return (action, before_size, after_size, changed)."""
    with open(p, 'rb') as f:
        data = f.read()
    codec, strict = find_decode_codec(data)
    if codec == 'utf-8':
        return ('skip-utf8', len(data), len(data), False)
    text = data.decode(codec, errors='replace' if not strict else 'strict')
    new = text.encode('utf-8')
    # Avoid writing if unchanged (cp949 + errors=replace could yield
    # the same bytes in rare cases — but cp949→utf-8 always inflates)
    if new == data:
        return ('skip-equal', len(data), len(data), False)
    with open(p, 'wb') as f:
        f.write(new)
    action = f'convert-{codec}' + ('' if strict else '-lossy')
    return (action, len(data), len(new), True)


def main():
    total = 0
    changed = 0
    unchanged = 0
    lossy = 0
    report = []
    for target_dir in TARGET_DIRS:
        for dirpath, _, filenames in os.walk(target_dir):
            for fn in filenames:
                ext = os.path.splitext(fn)[1].lower()
                if ext not in EXTENSIONS:
                    continue
                p = os.path.join(dirpath, fn)
                total += 1
                action, before, after, was_changed = process_file(p)
                if was_changed:
                    changed += 1
                    if 'lossy' in action:
                        lossy += 1
                else:
                    unchanged += 1
                rel = os.path.relpath(p, ROOT)
                report.append((rel, action, before, after, was_changed))
    print(f'Total cpp/h scanned: {total}')
    print(f'  Changed:    {changed} ({lossy} lossy)')
    print(f'  Unchanged:  {unchanged}')
    print()
    # Distribution of actions
    from collections import Counter
    actions = Counter(r[1] for r in report)
    for a, n in actions.most_common():
        print(f'  {a}: {n}')
    # List lossy conversions
    lossy_files = [r for r in report if 'lossy' in r[1]]
    if lossy_files:
        print()
        print('LOSSY conversions (replacements used):')
        for rel, action, before, after, _ in lossy_files:
            print(f'  {rel}: {before} -> {after} bytes ({action})')


if __name__ == '__main__':
    main()