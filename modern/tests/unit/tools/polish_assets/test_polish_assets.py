#!/usr/bin/env python3
"""test_polish_assets.py - unit tests for modern/tools/polish_assets.py."""

from __future__ import annotations

import os
import struct
import sys
import tempfile
import unittest
from pathlib import Path

_THIS = Path(__file__).resolve().parent
# test is at modern/tests/unit/tools/polish_assets/test_*.py
# tools dir is at modern/tools/
_TOOLS = _THIS.parents[3] / "tools"
if str(_TOOLS) not in sys.path:
    sys.path.insert(0, str(_TOOLS))

import polish_assets  # noqa: E402


def _dds_check(path: Path):
    with path.open("rb") as f:
        magic = f.read(4)
        assert magic == b"DDS ", f"bad magic in {path}: {magic!r}"
        hdr = f.read(124)
        height, width = struct.unpack("<II", hdr[8:16])
        # ddspf.dwBitCount is at offset 84 (after dwSize, dwFlags, dwFourCC)
        bits = struct.unpack("<I", hdr[84:88])[0]
        # We write 32-bit BGRA8 internally (loadDDS round-trips on it);
        # callers pass 24-bit RGB triples which we expand to 4-channel BGRA.
        assert bits == 32, f"expected 32-bit BGRA storage, got {bits}"
        pixel_size = width * height * 4
        data = f.read(pixel_size)
        assert len(data) == pixel_size, f"pixel data short: {len(data)}"
        return width, height, data


class WriteDds24Tests(unittest.TestCase):

    def test_writes_valid_dds_header(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            # 16x8 RGB24 = 384 bytes input -> 16x8 BGRA8 = 512 bytes output
            pixels = b"\x20\x40\x60" * (16 * 8)
            polish_assets.write_dds_24(tdp / "test.dds", 16, 8, pixels)
            w, h, data = _dds_check(tdp / "test.dds")
            self.assertEqual((w, h), (16, 8))
            # Each RGB triple (0x20,0x40,0x60) becomes BGRA (0x60,0x40,0x20,0xFF)
            expected = b"\x60\x40\x20\xff" * (16 * 8)
            self.assertEqual(data, expected)

    def test_rejects_wrong_buffer_size(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            with self.assertRaises(ValueError):
                polish_assets.write_dds_24(tdp / "bad.dds", 4, 4, b"\x00" * 3)


class WriteTifRgbTests(unittest.TestCase):

    def test_writes_minimal_tif(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pixels = b"\x10\x20\x30" * (8 * 4)
            polish_assets.write_tif_rgb(tdp / "test.tif", 8, 4, pixels)
            with (tdp / "test.tif").open("rb") as f:
                header = f.read(8)
                self.assertEqual(header[:2], b"II")
                self.assertEqual(header[2:4], b"\x2a\x00")
                # pixel data should match
                f.seek(-8 * 4 * 3, 2)
                tail = f.read()
                self.assertEqual(tail, pixels)


class LoadingGeneratorTests(unittest.TestCase):

    def test_generates_expected_files(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            n = polish_assets.generate_loading(tdp)
            self.assertEqual(n, 65 + 32 + 3)
            self.assertTrue((tdp / "maploadingimage000.dds").is_file())
            self.assertTrue((tdp / "maploadingimage064.dds").is_file())
            self.assertTrue((tdp / "LoadingTip01.dds").is_file())
            self.assertTrue((tdp / "LoadingTip32.dds").is_file())
            self.assertTrue((tdp / "now_loading01.TIF").is_file())
            self.assertTrue((tdp / "now_loading03.TIF").is_file())
            # 65 maploading dds should have unique hashes (different palettes)
            # We use 7 palette buckets, so we expect 7 unique hashes — but
            # at least 2 (proving the per-map palette selection works
            # instead of every map collapsing to a single shared file).
            hashes = set()
            for i in range(65):
                d = (tdp / f"maploadingimage{i:03d}.dds").read_bytes()
                hashes.add(hash(d))
            self.assertGreaterEqual(len(hashes), 2,
                "all 65 maps collapsed to 1 image")


class MinimapGeneratorTests(unittest.TestCase):

    def test_generates_mini_files(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            n = polish_assets.generate_minimap(tdp, force=True)
            self.assertEqual(n, 65 * 2)
            self.assertTrue((tdp / "mini_0.dds").is_file())
            self.assertTrue((tdp / "mini_0_ful.dds").is_file())
            self.assertTrue((tdp / "mini_64.dds").is_file())
            self.assertTrue((tdp / "mini_64_ful.dds").is_file())


class ParseMiniFilenameTests(unittest.TestCase):
    """_parse_mini_filename maps legacy minimap DDS stems to map ids."""

    CASES = [
        ("mini_0",       0),
        ("mini_0_ful",   0),
        ("mini_01",      1),
        ("mini_01_ful",  1),
        ("mini_64",      64),
        ("mini_64_ful",  64),
        ("mini_001",     1),
    ]
    NEGATIVE = ["", "mini_", "mini_abc", "mini_1_xyz", "loading_0"]

    def test_known_stems(self):
        for stem, expected in self.CASES:
            self.assertEqual(
                polish_assets._parse_mini_filename(stem), expected,
                f"expected {stem!r} -> {expected}")

    def test_case_insensitive_prefix(self):
        self.assertEqual(polish_assets._parse_mini_filename("MINI_5"), 5)
        self.assertEqual(polish_assets._parse_mini_filename("Mini_5_ful"), 5)

    def test_negative_stems_return_none(self):
        for stem in self.NEGATIVE:
            self.assertIsNone(
                polish_assets._parse_mini_filename(stem),
                f"{stem!r} should be rejected")


class ScanExistingMinimapsTests(unittest.TestCase):
    """scan_existing_minimaps collects the set of map ids already on disk."""

    def test_empty_dir_returns_empty_set(self):
        with tempfile.TemporaryDirectory() as td:
            self.assertEqual(
                polish_assets.scan_existing_minimaps(Path(td)), set())

    def test_missing_dir_returns_empty_set(self):
        # scan must not raise when the directory does not yet exist.
        self.assertEqual(
            polish_assets.scan_existing_minimaps(Path("/no/such/dir/xyz")), set())

    def test_collects_both_dds_and_ful_variants(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            polish_assets.write_dds_24(tdp / "mini_5.dds", 4, 4, b"\x20" * 48)
            polish_assets.write_dds_24(tdp / "mini_42_ful.dds", 4, 4, b"\x20" * 48)
            polish_assets.write_dds_24(tdp / "mini_7_ful.dds", 4, 4, b"\x20" * 48)
            self.assertEqual(
                polish_assets.scan_existing_minimaps(tdp), {5, 42, 7})

    def test_ignores_non_minimap_dds(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            polish_assets.write_dds_24(tdp / "login.dds", 4, 4, b"\x20" * 48)
            polish_assets.write_dds_24(tdp / "mini_0.dds", 4, 4, b"\x20" * 48)
            polish_assets.write_dds_24(tdp / "loading.dds", 4, 4, b"\x20" * 48)
            self.assertEqual(
                polish_assets.scan_existing_minimaps(tdp), {0})


class FoldMinimapTests(unittest.TestCase):
    """generate_minimap must NOT clobber real shipped minimap textures."""

    def test_fold_mode_skips_existing(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            # Seed two real minimap DDS files (mimicking shipped assets).
            sentinel_a = b"\xAA" * 196736
            sentinel_b = b"\xBB" * 196736
            (tdp / "mini_5.dds").write_bytes(sentinel_a)
            (tdp / "mini_42_ful.dds").write_bytes(sentinel_b)
            n = polish_assets.generate_minimap(tdp, force=False)
            # 2 maps skipped -> 63 maps * 2 files = 126 new DDS written
            self.assertEqual(n, 63 * 2)
            # Real shipped assets must remain byte-identical (no clobber).
            self.assertEqual((tdp / "mini_5.dds").read_bytes(), sentinel_a)
            self.assertEqual((tdp / "mini_42_ful.dds").read_bytes(), sentinel_b)
            # Fold is per-map-id: a seeded mini_5.dds skips the entire
            # map-id 5 -- so neither mini_5_ful.dds nor the seeded file
            # is touched.  Likewise for map-id 42.
            self.assertFalse((tdp / "mini_5_ful.dds").is_file(),
                "fold must skip per-map-id, not per-file")
            self.assertFalse((tdp / "mini_42.dds").is_file(),
                "fold must skip per-map-id, not per-file")

    def test_force_mode_rewrites_existing(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            sentinel = b"\xCC" * 196736
            (tdp / "mini_5.dds").write_bytes(sentinel)
            n = polish_assets.generate_minimap(tdp, force=True)
            self.assertEqual(n, 65 * 2)
            # The sentinel should have been overwritten by the new DDS.
            self.assertNotEqual((tdp / "mini_5.dds").read_bytes(), sentinel)

    def test_fold_on_empty_dir_writes_all(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            n = polish_assets.generate_minimap(tdp, force=False)
            self.assertEqual(n, 65 * 2)
            self.assertTrue((tdp / "mini_0.dds").is_file())
            self.assertTrue((tdp / "mini_64_ful.dds").is_file())


class UIBackgroundGeneratorTests(unittest.TestCase):

    def test_generates_4_backgrounds(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            n = polish_assets.generate_ui_bg(tdp)
            self.assertEqual(n, 4)
            self.assertTrue((tdp / "login.dds").is_file())
            self.assertTrue((tdp / "KeySetting1.dds").is_file())
            self.assertTrue((tdp / "KeySetting2.dds").is_file())
            self.assertTrue((tdp / "Titanlogo_sub.dds").is_file())
            # Each background should be visually distinct
            contents = {
                f: (tdp / f).read_bytes() for f in
                ["login.dds", "KeySetting1.dds", "KeySetting2.dds", "Titanlogo_sub.dds"]
            }
            unique = set(contents.values())
            self.assertEqual(len(unique), 4, "all 4 backgrounds must be unique")


if __name__ == "__main__":
    unittest.main(verbosity=2)
