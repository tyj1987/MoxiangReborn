#!/usr/bin/env python3
"""test_audit_minimap.py - unit tests for modern/tools/audit_minimap.py."""

from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

_THIS = Path(__file__).resolve().parent
_TOOLS = _THIS.parents[3] / "tools"
if str(_TOOLS) not in sys.path:
    sys.path.insert(0, str(_TOOLS))

import audit_minimap  # noqa: E402


def _write_mini(d: Path, mid: int, *, plain: bool = True, ful: bool = True, payload: bytes = b"DDS ") -> None:
    if plain:
        (d / f"mini_{mid}.dds").write_bytes(payload)
    if ful:
        (d / f"mini_{mid}_ful.dds").write_bytes(payload)


class ParseMiniFilenameTests(unittest.TestCase):
    """audit_minimap's _parse_mini_filename must agree with polish_assets."""

    def test_matches_known_forms(self):
        for stem, expected in [
            ("mini_0", 0),
            ("mini_0_ful", 0),
            ("mini_01", 1),
            ("mini_01_ful", 1),
            ("MINI_5", 5),
        ]:
            self.assertEqual(audit_minimap._parse_mini_filename(stem), expected)

    def test_rejects_non_mini(self):
        for stem in ["", "mini_", "loading_0", "Map0"]:
            self.assertIsNone(audit_minimap._parse_mini_filename(stem))


class ScanMinimapDdsTests(unittest.TestCase):

    def test_empty_dir(self):
        with tempfile.TemporaryDirectory() as td:
            self.assertEqual(audit_minimap.scan_minimap_dds(Path(td)), {})

    def test_missing_dir(self):
        self.assertEqual(audit_minimap.scan_minimap_dds(Path("/no/such/dir/zzz")), {})

    def test_collects_plain_and_ful(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            _write_mini(tdp, 5)
            _write_mini(tdp, 42, plain=False)
            _write_mini(tdp, 7, ful=False)
            result = audit_minimap.scan_minimap_dds(tdp)
            self.assertEqual(result[5], {"plain": True, "ful": True})
            self.assertEqual(result[42], {"plain": False, "ful": True})
            self.assertEqual(result[7], {"plain": True, "ful": False})

    def test_ignores_non_minimap_dds(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            (tdp / "login.dds").write_bytes(b"DDS ")
            (tdp / "Map0.dds").write_bytes(b"DDS ")
            _write_mini(tdp, 0)
            result = audit_minimap.scan_minimap_dds(tdp)
            self.assertIn(0, result)
            self.assertNotIn(-1, result)
            self.assertEqual(len(result), 1)


class ScanMinimapBinsTests(unittest.TestCase):

    def test_returns_minimap_files(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            (tdp / "Minimap1.bin").write_bytes(b"x" * 62)
            (tdp / "Minimap10.bin").write_bytes(b"x" * 62)
            (tdp / "image_minimap_path.bin").write_bytes(b"x" * 860)
            (tdp / "loading.dds").write_bytes(b"x")
            files = audit_minimap.scan_minimap_bins(tdp)
            self.assertEqual(len(files), 2)
            for f in files:
                self.assertTrue(f.name.startswith("Minimap"))
                self.assertTrue(f.name.endswith(".bin"))


class ScanCentralPathIndexTests(unittest.TestCase):

    def test_missing_returns_not_exists(self):
        with tempfile.TemporaryDirectory() as td:
            r = audit_minimap.scan_central_path_index(Path(td))
            self.assertFalse(r["exists"])

    def test_present_reports_size_and_magic(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            (tdp / "Image").mkdir()
            data = b"\x7a\xcf\x31\x01" + b"\x00" * 856
            (tdp / "Image" / "image_minimap_path.bin").write_bytes(data)
            r = audit_minimap.scan_central_path_index(tdp)
            self.assertTrue(r["exists"])
            self.assertEqual(r["size"], 860)
            self.assertEqual(r["magic"], "0x0131cf7a")


class CheckFoldConsistencyTests(unittest.TestCase):

    def test_zero_missing_when_dds_complete(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            mm = tdp / "Image" / "MiniMap"
            mm.mkdir(parents=True)
            for mid in [0, 1, 2, 3, 4]:
                _write_mini(mm, mid)
            r = audit_minimap._check_fold_consistency(tdp, [0, 1, 2, 3, 4])
            self.assertEqual(r["known"], 5)
            self.assertEqual(r["existing"], 5)
            self.assertEqual(r["missing"], 0)
            self.assertEqual(r["to_synthesize"], 0)

    def test_reports_missing_per_map_id(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            mm = tdp / "Image" / "MiniMap"
            mm.mkdir(parents=True)
            _write_mini(mm, 0)
            # maps 1, 2, 3 are missing
            r = audit_minimap._check_fold_consistency(tdp, [0, 1, 2, 3])
            self.assertEqual(r["known"], 4)
            self.assertEqual(r["existing"], 1)
            self.assertEqual(r["missing"], 3)
            self.assertEqual(r["to_synthesize"], 6)


class ReportFormatTests(unittest.TestCase):

    def test_markdown_report_contains_headline_counts(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            mm = tdp / "Image" / "MiniMap"
            mm.mkdir(parents=True)
            for mid in [0, 1, 2]:
                _write_mini(mm, mid)
            report = audit_minimap.build_report(tdp, [0, 1, 2, 3], check_fold=True)
            self.assertIn("# PlayDH minimap audit", report)
            self.assertIn("real `mini_<N>.dds`: 3", report)
            self.assertIn("known map ids: 4", report)
            self.assertIn("missing (would synthesize): 1", report)
            # 3 of 4 present -> INCOMPLETE, not PASS.
            self.assertIn("INCOMPLETE:", report)

    def test_json_output_is_valid(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            mm = tdp / "Image" / "MiniMap"
            mm.mkdir(parents=True)
            _write_mini(mm, 0)
            data = json.loads(audit_minimap.build_json(tdp, [0, 1], check_fold=True))
            self.assertEqual(data["known_map_ids_count"], 2)
            self.assertEqual(data["real_dds_count"], 1)
            self.assertEqual(data["fold_dry_run"]["missing"], 1)

    def test_full_pass_verdict(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            mm = tdp / "Image" / "MiniMap"
            mm.mkdir(parents=True)
            for mid in [0, 1, 2]:
                _write_mini(mm, mid)
            report = audit_minimap.build_report(tdp, [0, 1, 2], check_fold=False)
            self.assertIn("PASS:", report)


if __name__ == "__main__":
    unittest.main(verbosity=2)
