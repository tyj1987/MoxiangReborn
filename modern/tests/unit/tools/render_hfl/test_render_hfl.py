#!/usr/bin/env python3
"""test_render_hfl.py - unit tests for modern/tools/render_hfl.py."""

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

import render_hfl  # noqa: E402


def _make_synth_hfl(width: int = 64, height: int = 64) -> bytes:
    """Build a minimal valid HFL on disk: 4 + 108 + 4*w*h bytes."""
    import struct
    data = bytearray()
    data += struct.pack("<I", 1)  # version
    desc = bytearray(108)
    # 5 floats at offset 0..20: left, top, right, bottom, face_size
    struct.pack_into("<fffff", desc, 0, 0.0, 0.0, 100.0, 100.0, 1.0)
    # 3 u32 at offset 20: fpoa, ocx, ocz
    struct.pack_into("<III", desc, 20, 1, 1, 1)
    desc[32] = 1  # dlc
    desc[33] = 0  # blend
    struct.pack_into("<I", desc, 36, 0)  # idx_l0
    struct.pack_into("<I", desc, 44, 0)  # textureCount
    struct.pack_into("<II", desc, 52, width, height)  # height_count_x/z
    struct.pack_into("<ff", desc, 60, 100.0, 100.0)  # width/height
    data += desc
    # heights: width*height floats
    for y in range(height):
        for x in range(width):
            data += struct.pack("<f", float(x + y))
    return bytes(data)


class ParseHflTests(unittest.TestCase):

    def test_parses_synth_hfl(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            p = tdp / "synth.hfl"
            p.write_bytes(_make_synth_hfl(8, 8))
            hcx, hcz, heights, w, h = render_hfl.parse_hfl(p)
            self.assertEqual((hcx, hcz), (8, 8))
            self.assertEqual(len(heights), 64)
            # height[0] = 0+0 = 0, height[7] = 7+0 = 7
            self.assertAlmostEqual(heights[0], 0.0)
            self.assertAlmostEqual(heights[7], 7.0)
            # height[63] = 7+7 = 14
            self.assertAlmostEqual(heights[63], 14.0)

    def test_rejects_truncated(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            p = tdp / "short.hfl"
            p.write_bytes(b"\x01\x00\x00\x00")
            with self.assertRaises(ValueError):
                render_hfl.parse_hfl(p)

    def test_rejects_zero_grid(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            p = tdp / "zero.hfl"
            data = bytearray(4 + 108)
            struct.pack_into("<I", data, 0, 1)  # version
            # leave hcx/hcz = 0
            p.write_bytes(bytes(data))
            with self.assertRaises(ValueError):
                render_hfl.parse_hfl(p)


class RenderHflTests(unittest.TestCase):

    def test_renders_to_bmp(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            in_p = tdp / "synth.hfl"
            in_p.write_bytes(_make_synth_hfl(16, 16))
            out_p = tdp / "out.bmp"
            w, h = render_hfl.render(16, 16, list(range(256)), 100.0, 100.0, 4, out_p)
            self.assertEqual((w, h), (4, 4))
            data = out_p.read_bytes()
            self.assertEqual(data[:2], b"BM")
            # BMP header: 14 bytes file header + 40 bytes DIB
            file_size = struct.unpack_from("<I", data, 2)[0]
            self.assertEqual(file_size, len(data))


if __name__ == "__main__":
    unittest.main(verbosity=2)
