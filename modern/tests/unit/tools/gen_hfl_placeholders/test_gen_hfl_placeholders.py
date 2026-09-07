#!/usr/bin/env python3
"""test_gen_hfl_placeholders.py - unit tests for modern/tools/gen_hfl_placeholders.py."""

from __future__ import annotations

import struct
import sys
import tempfile
import unittest
from pathlib import Path

_THIS = Path(__file__).resolve().parent
_TOOLS = _THIS.parents[3] / "tools"
if str(_TOOLS) not in sys.path:
    sys.path.insert(0, str(_TOOLS))

import gen_hfl_placeholders  # noqa: E402


def _make_minimal_hfl_bytes(hcx: int = 4, hcz: int = 4, tex_count: int = 0) -> bytes:
    """Build a minimal but valid HFL file in memory.

    Layout: 4 byte version + 108 byte desc + hcx*hcz floats +
    4 byte stored texture count + tex_count * 200 byte texture
    entries (each: 1 byte index + 199 byte padded name).

    All offsets below are absolute byte offsets inside `desc` (which
    itself starts at byte 4 after the version u32).  They match
    modern/src/hfl_height_field.cpp::parse_hfl exactly.
    """
    version = 1
    desc = bytearray(108)
    # detail_level_count (d+32 == byte 36)
    struct.pack_into("<I", desc, 32, 1)
    # textureCount (d+44 == byte 48)
    struct.pack_into("<I", desc, 44, tex_count)
    # height_count_x / height_count_z (d+52/d+56 == bytes 56, 60)
    struct.pack_into("<II", desc, 52, hcx, hcz)
    # width / height (d+60/d+64 == bytes 64, 68)
    struct.pack_into("<ff", desc, 60, 1000.0, 1000.0)

    heights = struct.pack(f"<{hcx * hcz}f", *([1.0] * (hcx * hcz)))
    stored = struct.pack("<I", tex_count)
    entries = b""
    for i in range(tex_count):
        name = f"tile_{i}.tga".encode("ascii")
        entries += struct.pack("<B", i) + name + b"\x00" * (200 - 1 - len(name))
    return struct.pack("<I", version) + bytes(desc) + heights + stored + entries


class ParseHflHeaderTests(unittest.TestCase):

    def test_parses_minimal_template(self):
        data = _make_minimal_hfl_bytes(hcx=4, hcz=4, tex_count=0)
        version, desc, hcx, hcz, w, h, tex, heights = gen_hfl_placeholders.parse_hfl_header(data)
        self.assertEqual(version, 1)
        self.assertEqual((hcx, hcz), (4, 4))
        self.assertEqual(tex, 0)
        self.assertEqual(len(heights), 16)
        self.assertAlmostEqual(w, 1000.0)
        self.assertAlmostEqual(h, 1000.0)

    def test_parses_with_textures(self):
        data = _make_minimal_hfl_bytes(hcx=2, hcz=2, tex_count=3)
        version, desc, hcx, hcz, w, h, tex, heights = gen_hfl_placeholders.parse_hfl_header(data)
        self.assertEqual(tex, 3)
        self.assertEqual(len(heights), 4)

    def test_rejects_truncated(self):
        with self.assertRaises(ValueError):
            gen_hfl_placeholders.parse_hfl_header(b"\x01\x00\x00\x00")

    def test_rejects_zero_version(self):
        data = _make_minimal_hfl_bytes()
        # Corrupt version
        bad = b"\x00\x00\x00\x00" + data[4:]
        with self.assertRaises(ValueError):
            gen_hfl_placeholders.parse_hfl_header(bad)

    def test_rejects_zero_grid(self):
        # Build a header that has height_count_x = 0
        data = _make_minimal_hfl_bytes()
        bad = bytearray(data)
        # Clear height_count_x (desc offset 52, since d=4 and the desc
        # slice starts at byte 4 of the file).
        struct.pack_into("<I", bad, 4 + 52, 0)
        with self.assertRaises(ValueError):
            gen_hfl_placeholders.parse_hfl_header(bytes(bad))


class SynthesizeHeightsTests(unittest.TestCase):

    def test_deterministic_per_map_id(self):
        a = gen_hfl_placeholders.synthesize_heights(42, 4, 4)
        b = gen_hfl_placeholders.synthesize_heights(42, 4, 4)
        self.assertEqual(a, b)

    def test_different_ids_produce_different_grids(self):
        a = gen_hfl_placeholders.synthesize_heights(1, 8, 8)
        b = gen_hfl_placeholders.synthesize_heights(2, 8, 8)
        self.assertNotEqual(a, b)

    def test_grid_size(self):
        a = gen_hfl_placeholders.synthesize_heights(7, 16, 16)
        self.assertEqual(len(a), 16 * 16)


class SynthesizePlaceholderTests(unittest.TestCase):

    def test_round_trip_through_parser(self):
        template = _make_minimal_hfl_bytes(hcx=8, hcz=8, tex_count=0)
        out = gen_hfl_placeholders.synthesize_placeholder(template, 42)
        v, d, hcx, hcz, w, h, tex, heights = gen_hfl_placeholders.parse_hfl_header(out)
        self.assertEqual(v, 1)
        # New grid is the placeholder default
        self.assertEqual(hcx, gen_hfl_placeholders.PLACEHOLDER_GRID)
        self.assertEqual(hcz, gen_hfl_placeholders.PLACEHOLDER_GRID)
        self.assertEqual(len(heights), hcx * hcz)
        # Texture count must round-trip exactly so parse_hfl's
        # "invalid HFL texture table" check still passes.
        self.assertEqual(tex, 0)

    def test_keeps_texture_table_verbatim(self):
        template = _make_minimal_hfl_bytes(hcx=4, hcz=4, tex_count=2)
        out = gen_hfl_placeholders.synthesize_placeholder(template, 5)
        v, d, hcx, hcz, w, h, tex, heights = gen_hfl_placeholders.parse_hfl_header(out)
        self.assertEqual(tex, 2)
        # Stored texture count and the entry bytes must be present after
        # the height grid (so the trailing texture table survived).
        cursor = 4 + 108 + hcx * hcz * 4
        self.assertGreater(len(out), cursor + 4)
        (stored,) = struct.unpack_from("<I", out, cursor)
        self.assertEqual(stored, 2)

    def test_width_height_depend_on_map_id(self):
        template = _make_minimal_hfl_bytes()
        a = gen_hfl_placeholders.synthesize_placeholder(template, 1)
        b = gen_hfl_placeholders.synthesize_placeholder(template, 33)
        # Width/height live in desc at offsets 60, 64 (4 bytes apart).
        wa, ha = struct.unpack_from("<ff", a, 4 + 60)
        wb, hb = struct.unpack_from("<ff", b, 4 + 60)
        self.assertNotEqual(wa, wb)
        self.assertNotEqual(ha, hb)
        # Each map id gets a positive world extent.
        self.assertGreater(wa, 0.0)
        self.assertGreater(wb, 0.0)


class DiscoverMapsTests(unittest.TestCase):

    def test_discovers_bmhm_files(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            (tdp / "Map0.bmhm").write_bytes(b"x" * 100)
            (tdp / "Map12.bmhm").write_bytes(b"x" * 100)
            (tdp / "Map99.bmhm").write_bytes(b"x" * 100)
            (tdp / "10.hfl").write_bytes(b"x" * 10)  # HFL, not bmhm
            (tdp / "Map0.hfl").write_bytes(b"x" * 10)  # also HFL
            ids = gen_hfl_placeholders.discover_maps(tdp)
            self.assertEqual(ids, {0, 12, 99})

    def test_case_insensitive_bmhm(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            (tdp / "map5.bmhm").write_bytes(b"x")
            ids = gen_hfl_placeholders.discover_maps(tdp)
            self.assertIn(5, ids)

    def test_empty_dir(self):
        with tempfile.TemporaryDirectory() as td:
            self.assertEqual(gen_hfl_placeholders.discover_maps(Path(td)), set())


class MainCLITests(unittest.TestCase):

    def test_dry_run_lists_todo_without_writing(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            template = _make_minimal_hfl_bytes(hcx=2, hcz=2, tex_count=0)
            (tdp / "10.hfl").write_bytes(template)
            (tdp / "Map0.bmhm").write_bytes(b"x")
            (tdp / "Map1.bmhm").write_bytes(b"x")
            (tdp / "10.bmhm").write_bytes(b"x")
            rc = gen_hfl_placeholders.main_with_args([
                "--src", str(tdp / "10.hfl"),
                "--out", str(tdp),
                "--dry-run",
            ])
            self.assertEqual(rc, 0)
            # Dry run must not create 0.hfl or 1.hfl
            self.assertFalse((tdp / "0.hfl").exists())
            self.assertFalse((tdp / "1.hfl").exists())

    def test_real_run_writes_only_missing(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            template = _make_minimal_hfl_bytes(hcx=2, hcz=2, tex_count=0)
            (tdp / "10.hfl").write_bytes(template)
            (tdp / "Map0.bmhm").write_bytes(b"x")
            (tdp / "Map1.bmhm").write_bytes(b"x")
            # Pre-existing 0.hfl (mimics a real shipped map) must be skipped.
            (tdp / "0.hfl").write_bytes(b"SENTINEL")
            rc = gen_hfl_placeholders.main_with_args([
                "--src", str(tdp / "10.hfl"),
                "--out", str(tdp),
            ])
            self.assertEqual(rc, 0)
            self.assertTrue((tdp / "1.hfl").exists())
            # Pre-existing 0.hfl must remain byte-identical.
            self.assertEqual((tdp / "0.hfl").read_bytes(), b"SENTINEL")

    def test_map_ids_override_auto_discovery(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            template = _make_minimal_hfl_bytes(hcx=2, hcz=2, tex_count=0)
            (tdp / "10.hfl").write_bytes(template)
            # No .bmhm files at all -- map-ids should still drive the run.
            rc = gen_hfl_placeholders.main_with_args([
                "--src", str(tdp / "10.hfl"),
                "--out", str(tdp),
                "--map-ids", "5,7",
            ])
            self.assertEqual(rc, 0)
            self.assertTrue((tdp / "5.hfl").exists())
            self.assertTrue((tdp / "7.hfl").exists())


if __name__ == "__main__":
    unittest.main(verbosity=2)
