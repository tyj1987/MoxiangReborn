#!/usr/bin/env python3
"""Fail when project planning or workspace hygiene invariants regress."""

from __future__ import annotations

import re
import sys
import argparse
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ROADMAP = ROOT / "ROADMAP.md"
SOURCE_ROOTS = (ROOT / "modern" / "src", ROOT / "modern" / "tools")
ROOT_ARTIFACT_PATTERNS = ("*.log", "*.obj", "*.db", "*.db-shm", "*.db-wal", "test_*.txt", "scratch_*.py")
FORBIDDEN_ROADMAP_TERMS = ("Session 20", "状态刷新：", "cumulative ~", "tests PASS (was")
TEMP_SOURCE_MARKERS = ("TEMP diag", "mesh-probe", "mesh-dump", "removed before commit")


def markdown_heading_errors(path: Path) -> list[str]:
    errors: list[str] = []
    seen: set[tuple[int, str]] = set()
    for line_no, line in enumerate(path.read_text(encoding="utf-8-sig").splitlines(), 1):
        match = re.match(r"^(#{1,6})\s+(.+?)\s*$", line)
        if not match:
            continue
        key = (len(match.group(1)), match.group(2))
        if key in seen:
            errors.append(f"{path.relative_to(ROOT)}:{line_no}: duplicate heading {match.group(2)!r}")
        seen.add(key)
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--ignore-root-artifacts",
        action="store_true",
        help="validate document/source rules while a reviewed cleanup manifest is still pending",
    )
    args = parser.parse_args()
    errors = markdown_heading_errors(ROADMAP)
    roadmap_text = ROADMAP.read_text(encoding="utf-8-sig")
    for term in FORBIDDEN_ROADMAP_TERMS:
        if term in roadmap_text:
            errors.append(f"ROADMAP.md: session/history text is forbidden: {term!r}")

    for source_root in SOURCE_ROOTS:
        for path in source_root.rglob("*"):
            if not path.is_file() or path.suffix.lower() not in {".cpp", ".hpp", ".h", ".cmake", ".txt"}:
                continue
            text = path.read_text(encoding="utf-8", errors="ignore")
            for marker in TEMP_SOURCE_MARKERS:
                if marker in text:
                    errors.append(f"{path.relative_to(ROOT)}: temporary marker {marker!r}")

    if not args.ignore_root_artifacts:
        artifacts: set[Path] = set()
        for pattern in ROOT_ARTIFACT_PATTERNS:
            artifacts.update(path for path in ROOT.glob(pattern) if path.is_file())
        for path in sorted(artifacts):
            errors.append(f"{path.relative_to(ROOT)}: root runtime/scratch artifact")

    if errors:
        print("Project governance check FAILED:")
        for error in errors:
            print(f"  - {error}")
        return 1
    print("Project governance check PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
