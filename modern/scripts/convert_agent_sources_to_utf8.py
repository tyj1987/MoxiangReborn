"""
Phase 7.5i: convert EUC-KR / cp949 bytes in [Server]Agent sources to UTF-8.

Background: same as convert_distribute_sources_to_utf8.py — legacy [Server]Agent
sources are partially cp949 (Korean) / cp936 (Chinese) / cp1252. With MSVC's
/source-charset:utf-8 (added to [Server]Agent/CMakeLists.txt in Phase 7.5i),
cp949 lead bytes fail to decode as UTF-8 and the compiler dies with "newline in
constant" / "undeclared identifier" errors that cascade.

This script extends convert_distribute_sources_to_utf8.py in two ways:
  1. Scans both .cpp AND .h (Agent has 26 .h with cp949 byte ranges)
  2. Operates on the [Server]Agent root by default

Idempotent: re-running on a file that's already UTF-8 is a no-op.
"""
from __future__ import annotations

import argparse
import os
import sys
import tempfile
from pathlib import Path

ROOT = Path(r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Server]Agent")


def looks_like_already_utf8(raw: bytes) -> bool:
    """True if `raw` decodes as UTF-8 without raising and contains no
    substitution markers that would indicate a prior bad-decode round-trip."""
    try:
        text = raw.decode("utf-8")
    except UnicodeDecodeError:
        return False
    # U+FFFC / U+FFFD / U+FFFE / U+FFFF are the canaries for a prior
    # decode-with-replace pass. If we find any, treat the file as "needs work".
    for ch in ("\ufffc", "\ufffd", "\ufffe", "\uffff"):
        if ch in text:
            return False
    return True


def convert_one(path: Path) -> tuple[bool, str]:
    """Read path, detect cp949/UTF-8 mixed, write UTF-8 normalized. Returns
    (changed, status_message)."""
    raw = path.read_bytes()
    if looks_like_already_utf8(raw):
        return False, "already utf-8"

    # Strategy: decode as cp949 (superset that handles both ASCII and Hangul),
    # re-encode as UTF-8. cp949 is lossless for the legacy Hangul + ASCII
    # bytes in this tree; the original UTF-8 ASCII would also round-trip through
    # cp949 (cp949 is a superset of ASCII in the 0x00-0x7F range).
    try:
        text = raw.decode("cp949")
    except UnicodeDecodeError as e:
        return False, f"cp949 decode failed: {e}"

    # Verify the round-trip is sane: the only way this is wrong is if the
    # original was genuinely a different multi-byte encoding (cp936 / cp1252).
    # We sniff for cp1252-specific ranges (0x80-0x9F are reserved in cp949
    # but defined in cp1252); if those are present, prefer cp1252 fallback.
    has_cp1252_marker = any(0x80 <= b <= 0x9F for b in raw)
    if has_cp1252_marker:
        try:
            text = raw.decode("cp1252", errors="replace")
        except UnicodeDecodeError:
            pass

    new_raw = text.encode("utf-8")
    if new_raw == raw:
        return False, "no-op (cp949 decode == raw)"

    # Back up + atomic write.
    backup = path.with_suffix(path.suffix + ".pre_utf8.bak")
    if not backup.exists():
        backup.write_bytes(raw)

    fd, tmp = tempfile.mkstemp(prefix=path.name + ".", dir=str(path.parent))
    try:
        with os.fdopen(fd, "wb") as f:
            f.write(new_raw)
        os.replace(tmp, path)
    except OSError:
        if os.path.exists(tmp):
            os.unlink(tmp)
        raise

    return True, f"converted ({len(raw)} -> {len(new_raw)} bytes)"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=str(ROOT), help="Source root directory")
    parser.add_argument("--dry-run", action="store_true",
                        help="Report what would change without writing")
    parser.add_argument("--include-headers", action="store_true", default=True,
                        help="Also convert .h files (default: True)")
    parser.add_argument("--no-headers", dest="include_headers", action="store_false")
    args = parser.parse_args(argv)

    root = Path(args.root)
    patterns = ["*.cpp"]
    if args.include_headers:
        patterns.append("*.h")

    targets: list[Path] = []
    for pat in patterns:
        targets.extend(sorted(p for p in root.glob(pat) if p.is_file()))
    if not targets:
        print(f"no .cpp/.h files under {root}", file=sys.stderr)
        return 1

    print(f"=== Phase 7.5i: cp949 -> utf-8 for {len(targets)} files in {root} ===")
    if args.dry_run:
        print("(dry-run: no files will be written)")

    changed = 0
    unchanged = 0
    for p in targets:
        if args.dry_run:
            raw = p.read_bytes()
            if looks_like_already_utf8(raw):
                unchanged += 1
                print(f"  [skip]  {p.name}: already utf-8")
            else:
                changed += 1
                print(f"  [would] {p.name}: needs conversion")
            continue
        ok, msg = convert_one(p)
        if ok:
            changed += 1
            print(f"  [ok]    {p.name}: {msg}")
        else:
            unchanged += 1
            print(f"  [skip]  {p.name}: {msg}")

    print(f"\nSummary: {changed} changed, {unchanged} unchanged")
    return 0


if __name__ == "__main__":
    sys.exit(main())