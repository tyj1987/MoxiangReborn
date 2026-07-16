#!/usr/bin/env python3
"""Cross-check MODERNIZATION_PLAN.md file references vs reality.

Phase 12.x helper: scan the plan for file paths under modern/, then
check each path on disk. Reports missing files (plan claims done but
file gone) and found files.

Three regex passes:
  1. Backticked paths: `modern/...`
  2. Backticked Windows paths: `modern\\...`
  3. Bare modern/... paths in free text

By design, this is conservative — it only flags files that DON'T
exist on disk. Directory references (with trailing `/`) and
braced-glob references (e.g. `{cObject,cWindow}.hpp`) are intentionally
filtered out as they are not literal file paths.

Use: `python check_modernization_plan_refs.py`
Exit 0 if all referenced files exist; 1 if any are missing.
"""
import os
import re
import sys

ROOT = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）"
PLAN = os.path.join(ROOT, "MODERNIZATION_PLAN.md")
MODERN = os.path.join(ROOT, "modern")

# File extensions we care about for the plan check
FILE_EXTS = ("cpp", "hpp", "h", "py", "md", "cmake", "txt", "json", "yml", "yaml", "exe", "log")


def main():
    if not os.path.isfile(PLAN):
        print(f"ERROR: {PLAN} not found", file=sys.stderr)
        return 1

    with open(PLAN, "r", encoding="utf-8", errors="replace") as f:
        text = f.read()

    # Pattern 1: backticked modern/... paths
    p1 = re.findall(r"`(modern/[^`\s]+)`", text)
    # Pattern 2: backticked modern\\... paths
    p2 = re.findall(r"`(modern\\\\[^`\s]+)`", text)
    # Pattern 3: bare modern/... paths in free text
    p3 = re.findall(r"(modern/[A-Za-z0-9_./\\-]+\.(?:" + "|".join(FILE_EXTS) + r"))", text)

    all_paths = set(p1) | set(p2) | set(p3)
    # Filter: skip non-file refs (braced globs, trailing slashes, glob wildcards, etc.)
    clean = set()
    for p in all_paths:
        # Only consider file paths that look like modern/...
        # (exclude bare project name references like `MoxianCompat`)
        # Pattern 1 (p1) is always modern/... so safe.
        # Pattern 3 (p3) requires the regex to have matched a file extension
        # so it always has modern/ prefix. Only pattern 2 needs the check.
        if "{" in p or "}" in p:
            continue
        if p.endswith("/") or p.endswith("\\"):
            continue
        if "*" in p or "?" in p:
            continue
        # Pattern 2 (backticked modern\\...) already starts with modern\\
        # but defensively ensure all paths start with modern/
        if not (p.startswith("modern/") or p.startswith("modern\\")):
            continue
        clean.add(p)

    print(f"Plan file: {PLAN}")
    print(f"Total modern/ file references: {len(clean)}")
    print()

    missing = []
    found = []
    for p in sorted(clean):
        p_norm = p.replace("\\", "/")
        full = os.path.join(ROOT, p_norm)
        if os.path.isfile(full):
            found.append(p)
        else:
            missing.append(p)

    print(f"  Found on disk: {len(found)}")
    print(f"  Missing on disk: {len(missing)}")
    print()
    if missing:
        print("MISSING FILES (plan claims done but file is gone):")
        for p in missing:
            print(f"  - {p}")
        return 1
    print("All plan-referenced files exist on disk.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
