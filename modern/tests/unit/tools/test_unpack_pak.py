#!/usr/bin/env python3
"""
test_unpack_pak.py - unit tests for modern/tools/unpack_pak.py
(legacy {i:05d}_{real}.bin mode + pak_extract --named mode).

The tests are self-contained: they synthesize a minimal .pak in a
temp directory and exercise the parser end-to-end.  No real PlayDH
data is required to run them, so the test is always green on a clean
checkout.

Run via ctest (registered as pak_extract_smoke in tests/unit/tools/CMakeLists.txt)
or directly:

    python -m unittest modern/tests/unit/tools/test_unpack_pak.py
"""

from __future__ import annotations

import hashlib
import json
import os
import struct
import sys
import tempfile
import unittest
from pathlib import Path

# Make the tool importable regardless of cwd.
_THIS_DIR = Path(__file__).resolve().parent
_TOOLS_DIR = _THIS_DIR.parents[2] / "tools"
if str(_TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(_TOOLS_DIR))

import unpack_pak  # noqa: E402  (sys.path modified above)


def _make_pak(entries: list[tuple[str, bytes]], out_path: Path) -> int:
    """Build a minimal valid .pak file.  Returns file_item_num."""
    with out_path.open("wb") as f:
        # 92-byte PACK_FILE_HEADER
        f.write(struct.pack("<III", 1, len(entries), 0))  # version, n_items, flag
        f.write(b"\x00" * (92 - 12))
        for name, data in entries:
            name_b = name.encode("latin-1")
            name_len = len(name_b)
            real_size = len(data)
            total_size = 32 + name_len + 1 + real_size
            f.write(struct.pack("<IIIIIIII",
                                total_size, real_size, name_len,
                                0, 0, 0, 0, 0))  # 32 bytes
            f.write(name_b)
            f.write(b"\x00")
            f.write(data)
            pad = (-f.tell()) & 3
            if pad:
                f.write(b"\x00" * pad)
    return len(entries)


def _make_dummy_hfl(face_size: float = 512.0) -> bytes:
    """Build a 32-byte HFL_DESC-shaped prefix that hfl_header_ok accepts.

    Layout matches the Python hfl_header_ok() sniff (32 bytes total):
      u32 version
      f32 left, top, right, bottom
      f32 face_size
      u32 faces_per_object_axis
      u32 object_count_x
    """
    return struct.pack(
        "<IfffffII",
        1,            # version
        0.0, 0.0,     # left, top
        5000.0, 5000.0,  # right, bottom
        face_size,    # face_size
        1,            # faces_per_object_axis
        100,          # object_count_x
    )


class UnpackPakLegacyModeTests(unittest.TestCase):
    """The original {i:05d}_{real}.bin convention must keep working."""

    def test_legacy_writes_indexed_bins(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = tdp / "tiny.pak"
            _make_pak([
                ("Map/Map0.bmhm", b"alpha"),
                ("Map/Map1.bmhm", b"beta-beta"),
            ], pak)
            out = tdp / "out"
            rc = unpack_pak.unpack_legacy(str(pak), str(out))
            self.assertEqual(rc, 0)
            written = sorted(p.name for p in out.iterdir())
            self.assertEqual(written, ["00000_5.bin", "00001_9.bin"])
            self.assertEqual((out / "00000_5.bin").read_bytes(), b"alpha")
            self.assertEqual((out / "00001_9.bin").read_bytes(), b"beta-beta")


class PakExtractNamedModeTests(unittest.TestCase):
    """The new --named mode (pak_extract) must produce real names + manifest."""

    def test_named_mode_writes_by_canonical_name(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = tdp / "tiny.pak"
            _make_pak([
                ("Map\\Map0.bmhm", b"alpha"),
                ("Character\\man.chx", b"beta-beta"),
            ], pak)
            out = tdp / "out"
            stats = unpack_pak.pak_extract(
                str(pak), str(out),
                dry_run=False, write_manifest=True, verify=False)
            self.assertEqual(stats.written, 2)
            # Legacy '\\' separator must be normalized to '/'
            self.assertTrue((out / "Map" / "Map0.bmhm").is_file())
            self.assertTrue((out / "Character" / "man.chx").is_file())
            self.assertEqual((out / "Map" / "Map0.bmhm").read_bytes(), b"alpha")
            self.assertEqual((out / "Character" / "man.chx").read_bytes(),
                             b"beta-beta")
            # manifest.json must exist and contain both entries
            manifest = json.loads((out / "manifest.json").read_text(encoding="utf-8"))
            self.assertEqual(manifest["stats"]["written"], 2)
            names = [e["name"] for e in manifest["entries"]]
            self.assertIn("Map/Map0.bmhm", names)
            self.assertIn("Character/man.chx", names)
            sha = next(e for e in manifest["entries"]
                       if e["name"] == "Map/Map0.bmhm")["sha256"]
            self.assertEqual(sha, hashlib.sha256(b"alpha").hexdigest())

    def test_dry_run_does_not_create_files(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = tdp / "tiny.pak"
            _make_pak([("a.bin", b"x")], pak)
            out = tdp / "out"
            stats = unpack_pak.pak_extract(
                str(pak), str(out),
                dry_run=True, write_manifest=False, verify=False)
            self.assertEqual(stats.written, 1)
            self.assertFalse(out.exists(),
                             "dry-run must not create the output dir")

    def test_filter_only_writes_matching(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = tdp / "tiny.pak"
            _make_pak([
                ("Map/Map0.bmhm", b"a"),
                ("Character/man.chx", b"b"),
                ("Map/Map1.bmhm", b"c"),
            ], pak)
            out = tdp / "out"
            stats = unpack_pak.pak_extract(
                str(pak), str(out),
                dry_run=False, write_manifest=False, verify=False,
                filter_pattern=r"^Map/")
            self.assertEqual(stats.written, 2)
            self.assertEqual(stats.skipped_filter, 1)
            self.assertTrue((out / "Map" / "Map0.bmhm").is_file())
            self.assertTrue((out / "Map" / "Map1.bmhm").is_file())
            self.assertFalse((out / "Character").exists())

    def test_hfl_header_ok_accepts_valid_and_rejects_garbage(self):
        valid = _make_dummy_hfl(face_size=512.0)
        self.assertTrue(unpack_pak.hfl_header_ok(valid))
        # face_size out of plausible range -> reject
        bad_size = _make_dummy_hfl(face_size=1.0)
        self.assertFalse(unpack_pak.hfl_header_ok(bad_size))
        # too short -> reject
        self.assertFalse(unpack_pak.hfl_header_ok(valid[:16]))
        # all zeros -> reject
        self.assertFalse(unpack_pak.hfl_header_ok(b"\x00" * 32))

    def test_hfl_entries_with_bad_header_are_skipped(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = tdp / "tiny.pak"
            _make_pak([
                ("Map/Map0.hfl", _make_dummy_hfl(512.0)),
                ("Map/Map1.hfl", b"\x00" * 32),  # not a valid HFL
                ("Map/Map2.hfl", _make_dummy_hfl(256.0)),
            ], pak)
            out = tdp / "out"
            stats = unpack_pak.pak_extract(
                str(pak), str(out),
                dry_run=False, write_manifest=True, verify=True)
            self.assertEqual(stats.written, 2)
            self.assertEqual(stats.skipped_hfl_bad, 1)
            self.assertTrue((out / "Map" / "Map0.hfl").is_file())
            self.assertTrue((out / "Map" / "Map2.hfl").is_file())
            self.assertFalse((out / "Map" / "Map1.hfl").exists())


class ParsePakHeaderTests(unittest.TestCase):
    def test_rejects_truncated_header(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            bad = tdp / "bad.pak"
            bad.write_bytes(b"short")
            with self.assertRaises(ValueError):
                unpack_pak.parse_pak(str(bad))

    def test_rejects_unknown_version(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            bad = tdp / "bad.pak"
            with bad.open("wb") as f:
                f.write(struct.pack("<III", 0xdeadbeef, 0, 0))
                f.write(b"\x00" * (92 - 12))
            with self.assertRaises(ValueError):
                unpack_pak.parse_pak(str(bad))


if __name__ == "__main__":
    unittest.main(verbosity=2)
