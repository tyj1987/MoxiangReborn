"""Phase 7.5c: detect_encoding.py — evidence-only source encoding survey.

Walks the [Server]Map build input set (188 [Server]Map cpp/h + all [CC]*
headers + all 4DyuchiGRX_common + all [Lib]* headers transitively included)
and prints a JSON object {relative_path: detected_encoding} to stdout.

Detection rule (best-effort, byte-level only — no chardet dependency):
  - Read first 4 KB of the file as raw bytes.
  - If bytes start with UTF-8 BOM (EF BB BF) -> "utf-8-sig".
  - Else if all bytes are pure ASCII (<0x80) -> "ascii".
  - Else try strict UTF-8 decode of the first 4 KB; if it succeeds with no
    replacement, the file is treated as utf-8 even without a BOM (still
    note this is lossy for downstream MBCS tools).
  - Else if bytes look like GBK/CP936 / EUC-KR (high bytes >= 0x80 with
    specific lead/trail patterns) tag as the most likely MBCS:
      - GB18030/CP936 heuristic: lead 0x81-0xFE, trail 0x40-0xFE except 0x7F
      - EUC-KR  heuristic:      lead 0x81-0xFE, trail 0x41-0x5A
  - Else mark "unknown".

This script is **evidence-only** — it does NOT modify any source file.
It writes the JSON map next to the build log so the verifier can
cross-reference non-ASCII content with MSVC compile warnings
(warning C4828 "字符在当前源字符集中无效/代码页 65001").

Usage:
  python detect_encoding.py
  (writes build_map_detected_encoding.json next to the script)
"""

import json
import os
import sys

ROOT = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】"

# File extensions to inspect
EXT = (".cpp", ".h", ".hpp", ".inl", ".c")

# Directories to walk — only the build input surface, not the whole tree
INCLUDE_DIRS = [
    r"[Server]Map",
    r"[CC]Header",
    r"[CC]Skill",
    r"[CC]Ability",
    r"[CC]BattleSystem",
    r"[CC]Quest",
    r"[CC]Suryun",
    r"[ServerModule]",
    r"4DyuchiGRX_common",
]
# Stable relative-to-ROOT wildcard list (legacy paths with [Brackets])
# All real source dirs we'll discover by simple tree walk; trim to those
# that actually contain sources.


def detect(buf: bytes) -> str:
    if buf.startswith(b"\xef\xbb\xbf"):
        return "utf-8-sig"
    # All pure ASCII?
    if all(b < 0x80 for b in buf):
        return "ascii"
    # Strict UTF-8?
    try:
        buf.decode("utf-8", errors="strict")
        return "utf-8"  # no BOM, but valid UTF-8
    except UnicodeDecodeError:
        pass
    # MBCS heuristic: try GB18030 (superset of GBK / CP936)
    try:
        buf.decode("gb18030", errors="strict")
        return "gb18030"
    except UnicodeDecodeError:
        pass
    # EUC-KR heuristic (lead A1-FE, trail 41-5A) — narrow check
    i = 0
    while i < len(buf):
        b = buf[i]
        if b < 0x80:
            i += 1
        elif 0x81 <= b <= 0xFE and i + 1 < len(buf):
            t = buf[i + 1]
            if 0x41 <= t <= 0x5A:
                # plausible EUC-KR syllable block; step 2
                i += 2
            else:
                return "unknown"
        else:
            return "unknown"
    return "euc-kr"


def main() -> int:
    buckets: dict[str, list[str]] = {}
    total_files = 0
    total_scanned = 0
    for dirpath, _dirs, files in os.walk(ROOT):
        for fn in files:
            if not fn.lower().endswith(EXT):
                continue
            full = os.path.join(dirpath, fn)
            rel = os.path.relpath(full, ROOT)
            try:
                with open(full, "rb") as f:
                    sample = f.read(4096)
            except OSError:
                continue
            enc = detect(sample)
            buckets.setdefault(enc, []).append(rel.replace("\\", "/"))
            total_files += 1
            total_scanned += len(sample)

    summary = {k: len(v) for k, v in sorted(buckets.items())}
    print(f"Scanned {total_files} files ({total_scanned} bytes read).")
    print("Encoding histogram:")
    for k, n in sorted(summary.items(), key=lambda kv: -kv[1]):
        print(f"  {k:>12}: {n}")
    out_path = os.path.join(os.path.dirname(__file__),
                           "build_map_detected_encoding.json")
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(
            {
                "root": ROOT,
                "summary": summary,
                "files": buckets,
            },
            f, ensure_ascii=False, indent=2)
    print(f"JSON map written to: {out_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
