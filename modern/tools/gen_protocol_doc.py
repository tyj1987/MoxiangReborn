#!/usr/bin/env python3
"""Phase 12.x: Generate simplified Moxian Protocol documentation.

Phase 11.2 was planned to "auto-generate protocol documentation" from
Protocol.h. MoxianProtocolDoc C++ tool exists at
modern/tools/MoxianProtocolDoc/ but has STATUS_STACK_BUFFER_OVERRUN
crash on the real Protocol.h (92KB / 124 categories / 3458 protocols)
when calling generateMarkdown(). The --summary mode works fine.

This script uses the working --summary output and adds:
  - File metadata (date, parser version)
  - Per-category info extracted from MP_CATEGORY enum
  - Per-protocol-enum counts (top-level)
  - Pointers to legacy source locations

Output: docs/MoxianProtocolDoc.md (canonical reference)

Does NOT touch the C++ tool — that's a separate bug for someone to
investigate (likely the inner `for (const auto& proto : protocols_)`
match loop has pathological complexity for the full protocol set).
"""
import os
import re
import subprocess
import sys
from datetime import datetime

ROOT = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）"
PROTO_H = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[CC]Header\Protocol.h"
DOC_OUT = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\docs\MoxianProtocolDoc.md"
EXE = r"D:\Moxian\modern\build\tools\MoxianProtocolDoc\Release\MoxianProtocolDoc.exe"


def run_summary():
    """Run MoxianProtocolDoc --summary, parse counts."""
    r = subprocess.run([EXE, PROTO_H, "--summary"], capture_output=True, text=True, shell=False)
    if r.returncode != 0:
        raise SystemExit(f"MoxianProtocolDoc --summary failed rc={r.returncode}: {r.stderr}")
    out = r.stdout
    m_cat = re.search(r"Categories:\s*(\d+)", out)
    m_enum = re.search(r"Protocol Enums:\s*(\d+)", out)
    m_total = re.search(r"Total Protocols:\s*(\d+)", out)
    return {
        "categories": int(m_cat.group(1)) if m_cat else 0,
        "protocol_enums": int(m_enum.group(1)) if m_enum else 0,
        "total_protocols": int(m_total.group(1)) if m_total else 0,
    }


def extract_mp_category():
    """Extract MP_CATEGORY enum entries from Protocol.h (no regex parser)."""
    # Protocol.h is EUC-KR / cp949 encoded (legacy 2003-era Korean source)
    with open(PROTO_H, "rb") as f:
        raw = f.read()
    # Use 'ignore' for unrepresentable chars (Korean comments contain some
    # CJK ideographs that don't render in cp949); replaced with empty.
    text = raw.decode("cp949", errors="ignore")
    start = text.find("enum MP_CATEGORY")
    end = text.find("}", start)
    if start < 0 or end < 0:
        return []
    body = text[start:end]
    entries = []
    for line in body.split("\n"):
        m = re.match(r"^\s*(MP_\w+)\s*(?:=\s*(\d+))?\s*(?:,)?\s*(?://\s*(.*))?$", line)
        if not m:
            continue
        name = m.group(1)
        if name in ("MP_CATEGORY", "MP_MAX"):
            continue
        value = m.group(2)
        comment = m.group(3) or ""
        # Strip non-ASCII junk from comment (cp949 round-trip artifacts)
        comment = re.sub(r"[^\x20-\x7e\u00a0-\u00ff]", "?", comment).strip()
        entries.append((name, int(value) if value else None, comment))
    return entries


def extract_protocol_enum_names():
    """Extract MP_PROTOCOL_* enum NAMES only (not all 3458 values)."""
    with open(PROTO_H, "rb") as f:
        raw = f.read()
    text = raw.decode("cp949", errors="replace")
    names = []
    for m in re.finditer(r"enum\s+(MP_PROTOCOL_\w+)\s*\{", text):
        names.append(m.group(1))
    return names


def main():
    if not os.path.isfile(EXE):
        raise SystemExit(f"MoxianProtocolDoc.exe not found at {EXE}; build it first")
    if not os.path.isfile(PROTO_H):
        raise SystemExit(f"Protocol.h not found at {PROTO_H}")

    summary = run_summary()
    cat_entries = extract_mp_category()
    enum_names = extract_protocol_enum_names()



    today = datetime.now().strftime("%Y-%m-%d")
    lines = []
    lines.append("# Moxian Protocol Documentation")
    lines.append("")
    lines.append(f"> Auto-generated from `Protocol.h` on {today}")
    lines.append("")
    lines.append("**Generation tool**: `modern/tools/MoxianProtocolDoc/` (C++) + `modern/tools/gen_protocol_doc.py` (this Python wrapper)")
    lines.append("")
    lines.append("**Tool status**: MoxianProtocolDoc `--summary` works; `--output <md>` crashes with STATUS_STACK_BUFFER_OVERRUN on the full 92KB Protocol.h. This doc uses `--summary` + Python-side enum extraction as a stable interim format.")
    lines.append("")
    lines.append("---")
    lines.append("")
    lines.append("## Summary")
    lines.append("")
    lines.append(f"- **MP_CATEGORY entries (per C++ parser)**: {summary['categories']} (parser counts 124 by including comment references; the canonical enum body has {len(cat_entries)} real entries — see table below)")
    lines.append(f"- **MP_PROTOCOL_* enums (per C++ parser)**: {summary['protocol_enums']}")
    lines.append(f"- **Total protocol values (all enums combined)**: {summary['total_protocols']}")
    lines.append(f"- **Real category enum entries (Python-verified)**: {len(cat_entries)}")
    lines.append(f"- **Protocol enum names (Python-verified)**: {len(enum_names)}")
    lines.append("")
    lines.append("---")
    lines.append("")
    lines.append("## MP_CATEGORY enum")
    lines.append("")
    lines.append("| # | Name | Value | Description |")
    lines.append("|---|------|-------|-------------|")
    for i, (name, value, comment) in enumerate(cat_entries, 1):
        v = value if value is not None else "—"
        c = comment if comment else "—"
        lines.append(f"| {i} | `{name}` | {v} | {c} |")
    lines.append("")
    lines.append("---")
    lines.append("")
    lines.append("## MP_PROTOCOL_* enums (top-level)")
    lines.append("")
    lines.append("| # | Enum name |")
    lines.append("|---|-----------|")
    for i, name in enumerate(enum_names, 1):
        lines.append(f"| {i} | `{name}` |")
    lines.append("")
    lines.append("---")
    lines.append("")
    lines.append("## Per-protocol-enum value tables")
    lines.append("")
    lines.append(f"The full per-enum value breakdown (3,458 individual `MP_*` constants) is generated by MoxianProtocolDoc but is not embedded here due to the STATUS_STACK_BUFFER_OVERRUN crash in the C++ tool's `generateMarkdown()` path.")
    lines.append("")
    lines.append("To regenerate one enum at a time, you can manually split Protocol.h and call:")
    lines.append("")
    lines.append("```bash")
    lines.append(f"{EXE} <single-enum.h> --output <output.md>")
    lines.append("```")
    lines.append("")
    lines.append("Or fix the C++ tool's generateMarkdown() bug (likely an O(N×M) nested loop that explodes for 124×64) and re-run the full generation.")
    lines.append("")
    lines.append("---")
    lines.append("")
    lines.append("## Legacy source references")
    lines.append("")
    lines.append(f"- Master definition: `墨香【源码】\\[CC]Header\\Protocol.h` ({os.path.getsize(PROTO_H):,} bytes)")
    lines.append("- Network struct definitions: `墨香【源码】\\[CC]Header\\CommonStruct.h`")
    lines.append("- Client-side message handlers: `墨香【源码】\\[Client]MH\\MHNetworkMsgParser.cpp`")
    lines.append("- Server-side message handlers: `墨香【源码】\\[Server]Agent\\AgentNetworkMsgParser.cpp` + `墨香【源码】\\[Server]Map\\MapNetworkMsgParser.cpp` + `墨香【源码】\\[Server]Distribute\\DistributeNetworkMsgParser.cpp`")
    lines.append("")

    with open(DOC_OUT, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    print(f"Generated: {DOC_OUT} ({os.path.getsize(DOC_OUT):,} bytes)")


if __name__ == "__main__":
    main()
