#!/usr/bin/env python3
"""Phase 12.x: Scan entire 墨香【源码】/ tree for active g_Console.LOG(
(big L, 2-arg style — the C-1 MLOG rename target). Skip line comments
// and block comments /* */. Excludes g_Console.Log (small L, 3-arg style).
"""
import os
import re

ROOT = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】"
# Strict match: g_Console.LOG( — no alpha char immediately before (no Log, no MLOG)
PATTERN = re.compile(r"(?<![A-Za-z])g_Console\.LOG\(")

active_sites = []
for dirpath, _, filenames in os.walk(ROOT):
    for fn in filenames:
        if not fn.endswith(".cpp"):
            continue
        path = os.path.join(dirpath, fn)
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            lines = f.readlines()
        in_block = False
        for i, line in enumerate(lines, 1):
            if re.match(r"^\s*/\*", line):
                in_block = True
            if in_block:
                if re.search(r"\*/", line):
                    in_block = False
                continue
            stripped = line.lstrip()
            if stripped.startswith("//"):
                continue
            if PATTERN.search(line):
                rel = os.path.relpath(path, ROOT)
                active_sites.append((rel, i, line.rstrip()))

# Group by file
from collections import defaultdict
by_file = defaultdict(list)
for rel, line, _ in active_sites:
    by_file[rel].append(line)

print(f"Total files with active g_Console.LOG( calls: {len(by_file)}")
print(f"Total active sites: {len(active_sites)}")
print()
for rel, lines in sorted(by_file.items(), key=lambda x: -len(x[1])):
    print(f"  {rel}: {len(lines)} sites")
    for ln in lines[:3]:
        print(f"    line {ln}")
    if len(lines) > 3:
        print(f"    ... and {len(lines) - 3} more")
