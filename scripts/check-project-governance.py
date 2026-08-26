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
HUMAN_RUNNER_FORBIDDEN = (
    "--auto-login", "--auto-create", "--password", "--username",
    "SendKeys", "mouse_event", "keybd_event", "taskkill",
    "Stop-Process -Name", "Get-Process -Name",
)
SENSITIVE_LOG_MARKERS = ("auth_key=%", "dist_auth_key=%", "password=%")
FORBIDDEN_PATH_MARKERS = (
    "c:\\moxiang", "d:\\[sworking]", "source-recovery",
    "modern/scratch", "modern\\scratch",
)


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


def human_runner_errors() -> list[str]:
    path = ROOT / "scripts" / "run-human-acceptance.ps1"
    if not path.is_file():
        return ["scripts/run-human-acceptance.ps1: tracked human runner is missing"]
    text = path.read_text(encoding="utf-8-sig", errors="replace")
    errors: list[str] = []
    for marker in HUMAN_RUNNER_FORBIDDEN:
        if marker.lower() in text.lower():
            errors.append(f"scripts/run-human-acceptance.ps1: forbidden automation/secret marker {marker!r}")
    if "Stop-OwnedProcess" not in text or "Test-ExactProcess" not in text:
        errors.append("scripts/run-human-acceptance.ps1: exact owned-process shutdown guards are required")
    if "runId" not in text or "evidence" not in text:
        errors.append("scripts/run-human-acceptance.ps1: unique run/evidence output is required")
    return errors


def duplicate_ui_header_errors() -> list[str]:
    """Reject divergent public/private UI header mirrors.

    The build includes both roots.  A class declaration that differs between
    them creates an ODR/layout hazard even when each translation unit compiles.
    Comments, BOMs and whitespace are intentionally ignored so documentation
    edits do not create false positives.
    """
    public_root = ROOT / "modern" / "include" / "mxh" / "ui"
    private_root = ROOT / "modern" / "src" / "ui"

    def normalized(path: Path) -> str:
        text = path.read_text(encoding="utf-8-sig", errors="replace")
        text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
        text = re.sub(r"//[^\n]*", "", text)
        return "".join(text.split())

    errors: list[str] = []
    for private in sorted(private_root.glob("*.hpp")):
        public = public_root / private.name
        if public.is_file() and normalized(private) != normalized(public):
            errors.append(
                f"{private.relative_to(ROOT)}: public/private UI header mirror diverges"
            )
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
    errors.extend(human_runner_errors())
    errors.extend(duplicate_ui_header_errors())
    roadmap_text = ROADMAP.read_text(encoding="utf-8-sig")
    for term in FORBIDDEN_ROADMAP_TERMS:
        if term in roadmap_text:
            errors.append(f"ROADMAP.md: session/history text is forbidden: {term!r}")

    for source_root in SOURCE_ROOTS:
        for path in source_root.rglob("*"):
            if not path.is_file() or path.suffix.lower() not in {
                ".cpp", ".hpp", ".h", ".cmake", ".txt", ".py", ".ps1"
            }:
                continue
            text = path.read_text(encoding="utf-8", errors="ignore")
            for marker in SENSITIVE_LOG_MARKERS:
                if marker.lower() in text.lower():
                    errors.append(f"{path.relative_to(ROOT)}: sensitive credential log marker {marker!r}")
            for marker in TEMP_SOURCE_MARKERS:
                if marker in text:
                    errors.append(f"{path.relative_to(ROOT)}: temporary marker {marker!r}")
            for marker in FORBIDDEN_PATH_MARKERS:
                if marker in text.lower():
                    errors.append(f"{path.relative_to(ROOT)}: repository/scratch path dependency {marker!r}")

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
