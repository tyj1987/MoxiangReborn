#!/usr/bin/env python3
"""
integration_minimap_audit.py — cross-tool consistency ctest.

Verifies that the two minimap audit paths agree on the same numbers
for the same PlayDH tree:

  modern/tools/polish_assets.py --generate-minimap --audit
      (CLI output line:  [minimap-audit] dir=... existing=X missing=Y known=Z)

  modern/tools/audit_minimap.py --check-fold --json
      (JSON field:        fold_dry_run.existing / .missing / .known)

The two tools take slightly different code paths
(polish_assets::scan_existing_minimaps is the production scanner,
audit_minimap::scan_minimap_dds is the report scanner) so this ctest
locks down that they observe the same ground truth.  A future
optimization or refactor that drifts one without the other will trip
this gate.

Strategy
--------
Build a synthetic PlayDH tree in a temp dir with N real minimap DDS
files (N from 0 to 5), then:

  1. Run polish_assets with the temp dir as --out + --generate-minimap
     --audit (dry-run mode) and parse the audit line from stdout.
  2. Run audit_minimap with the same temp dir + --check-fold --json and
     parse the JSON.
  3. Assert existing / known match exactly, and missing = known - existing.
  4. Then run polish_assets for real (no --audit) to actually write the
     placeholders, re-run audit_minimap, and assert missing == 0 and
     existing == known.

This is a system-level ctest -- it lives in tests/unit/tools (not
tests/unit) because it shells out to the two scripts as subprocesses,
just like mxh_pak_extract_tests and friends.
"""

from __future__ import annotations

import json
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

_THIS = Path(__file__).resolve().parent
# _THIS is modern/tests/unit/tools/ -- its parents are:
#   [0] modern/tests/unit/tools/  (== _THIS itself, never used)
#   [1] modern/tests/unit/
#   [2] modern/tests/
#   [3] modern/
#   [4] C:\moxiang  (or whatever the repo root is)
# The two tools live at <repo>/modern/tools/, so we anchor on the
# directory that contains both _THIS and the tools/ dir: parents[3].
_REPO = _THIS.parents[3]  # modern/

# Detect the Python interpreter the same way the CMakeLists wrappers do.
# Fall back to whatever `python` resolves to in PATH if the explicit
# vcpkg toolchain isn't visible from the import.
import shutil
_PYTHON = sys.executable or shutil.which("python") or "python"
_POLISH = _REPO / "modern" / "tools" / "polish_assets.py"
_AUDIT = _REPO / "modern" / "tools" / "audit_minimap.py"


def _write_mini(d: Path, mid: int) -> None:
    (d / f"mini_{mid}.dds").write_bytes(b"DDS " + b"\x00" * 124)
    (d / f"mini_{mid}_ful.dds").write_bytes(b"DDS " + b"\x00" * 124)


def _run(cmd: list[str]) -> str:
    """Run a subprocess and return its stdout (utf-8 decoded)."""
    p = subprocess.run(cmd, capture_output=True, text=True, cwd=str(_REPO))
    return p.stdout


def _parse_polish_audit(stdout: str) -> dict[str, int]:
    """Pull existing/missing/known from polish_assets --audit stdout."""
    for line in stdout.splitlines():
        if line.startswith("[minimap-audit]"):
            m = re.search(r"existing=(\d+)\s+missing=(\d+)\s+known=(\d+)", line)
            if m:
                return {"existing": int(m.group(1)),
                        "missing":  int(m.group(2)),
                        "known":    int(m.group(3))}
    raise AssertionError(f"polish_assets --audit line not found in:\n{stdout}")


def _parse_audit_minimap_json(stdout: str) -> dict[str, int]:
    payload = json.loads(stdout)
    fdr = payload["fold_dry_run"]
    return {"existing": fdr["existing"],
            "missing":  fdr["missing"],
            "known":    fdr["known"]}


class CrossToolConsistencyTests(unittest.TestCase):
    """polish_assets --audit and audit_minimap --check-fold must agree."""

    def test_empty_playdh_both_report_zero_existing(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td) / "PlayDH"
            (tdp / "Image" / "MiniMap").mkdir(parents=True)
            a = _parse_polish_audit(_run(
                [_PYTHON, str(_POLISH), "--out", str(tdp),
                 "--generate-minimap", "--audit"]))
            b = _parse_audit_minimap_json(_run(
                [_PYTHON, str(_AUDIT), "--playdh", str(tdp),
                 "--check-fold", "--json"]))
            self.assertEqual(a, b)
            self.assertEqual(a["existing"], 0)
            self.assertGreater(a["known"], 0)
            self.assertEqual(a["missing"], a["known"])

    def test_partial_coverage_both_agree(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td) / "PlayDH"
            mm = tdp / "Image" / "MiniMap"
            mm.mkdir(parents=True)
            # Seed 3 real minimap DDS into a 65-map universe.
            for mid in (0, 1, 2):
                _write_mini(mm, mid)
            a = _parse_polish_audit(_run(
                [_PYTHON, str(_POLISH), "--out", str(tdp),
                 "--generate-minimap", "--audit"]))
            b = _parse_audit_minimap_json(_run(
                [_PYTHON, str(_AUDIT), "--playdh", str(tdp),
                 "--check-fold", "--json"]))
            self.assertEqual(a, b)
            self.assertEqual(a["existing"], 3)
            self.assertEqual(a["missing"], a["known"] - 3)
            self.assertGreater(a["missing"], 0)

    def test_full_coverage_both_report_zero_missing(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td) / "PlayDH"
            mm = tdp / "Image" / "MiniMap"
            mm.mkdir(parents=True)
            # Seed every map id that polish_assets --generate-minimap
            # actually targets (0..64 -> 65 maps).  polish_assets slices
            # _known_map_ids()[:65] internally, so 0..65 (66 maps) is
            # one too many -- the test compares to that 65.
            for mid in range(65):
                _write_mini(mm, mid)
            a = _parse_polish_audit(_run(
                [_PYTHON, str(_POLISH), "--out", str(tdp),
                 "--generate-minimap", "--audit"]))
            b = _parse_audit_minimap_json(_run(
                [_PYTHON, str(_AUDIT), "--playdh", str(tdp),
                 "--check-fold", "--json"]))
            self.assertEqual(a, b)
            self.assertEqual(a["existing"], a["known"])
            self.assertEqual(a["missing"], 0)


class FoldModeFillsMissingTests(unittest.TestCase):
    """polish_assets --generate-minimap (no --audit, no --dry-run)
    must actually write the missing placeholders so a follow-up
    audit returns missing=0."""

    def test_fold_mode_writes_only_missing(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td) / "PlayDH"
            mm = tdp / "Image" / "MiniMap"
            mm.mkdir(parents=True)
            for mid in (0, 5, 10):
                _write_mini(mm, mid)
            # First audit: 3 of N exist.
            a0 = _parse_polish_audit(_run(
                [_PYTHON, str(_POLISH), "--out", str(tdp),
                 "--generate-minimap", "--audit"]))
            self.assertEqual(a0["existing"], 3)
            self.assertEqual(a0["missing"], a0["known"] - 3)
            # Real run: synthesizes only the missing placeholders.
            _run([_PYTHON, str(_POLISH), "--out", str(tdp),
                  "--generate-minimap"])
            # Follow-up audit: 0 missing.
            a1 = _parse_polish_audit(_run(
                [_PYTHON, str(_POLISH), "--out", str(tdp),
                 "--generate-minimap", "--audit"]))
            self.assertEqual(a1["existing"], a1["known"])
            self.assertEqual(a1["missing"], 0)
            # Pre-seeded files must be byte-identical (fold, not clobber).
            for mid in (0, 5, 10):
                self.assertEqual((mm / f"mini_{mid}.dds").stat().st_size, 128,
                    f"pre-seeded mini_{mid}.dds was clobbered")
                self.assertEqual((mm / f"mini_{mid}_ful.dds").stat().st_size, 128,
                    f"pre-seeded mini_{mid}_ful.dds was clobbered")


if __name__ == "__main__":
    unittest.main(verbosity=2)
