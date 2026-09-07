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
            n = polish_assets.generate_minimap(tdp)
            self.assertEqual(n, 65 * 2)
            self.assertTrue((tdp / "mini_0.dds").is_file())
            self.assertTrue((tdp / "mini_0_ful.dds").is_file())
            self.assertTrue((tdp / "mini_64.dds").is_file())
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
