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


def _make_pak(entries: list[tuple[str, bytes]], out_path: Path,
              *, header_count: int | None = None) -> int:
    """Build a minimal valid .pak file.  Returns the header file_item_num.

    header_count overrides dwFileItemNum.  The entry bytes that follow are
    still written in order; pass 0 to reproduce archives that pack entries
    through EOF with a zero count in the header.
    """
    count = len(entries) if header_count is None else header_count
    with out_path.open("wb") as f:
        # 92-byte PACK_FILE_HEADER
        f.write(struct.pack("<III", 1, count, 0))  # version, n_items, flag
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
            # NOTE: the legacy 4DyuchiFileStorage writer does NOT
            # insert a 4-byte pad between entries; the cursor advances
            # by exactly 32 + name_len + 1 + real_size, with no
            # trailing alignment.  Earlier fixtures added a pad here
            # which self-consistently passed but did not match what
            # real .pak files actually contain.
    return count


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


class HeaderCountEofTests(unittest.TestCase):
    """dwFileItemNum == 0 means entries run until EOF.

    Synthetic only.  A few KB, no original client resources.
    """

    def test_zero_header_count_reads_three_entries(self):
        entries = [
            ("Map/zone_a.bmhm", b"zone-a:" + b"A" * 1200),
            ("Map/zone_b.bmhm", b"zone-b:" + b"B" * 1300),
            ("Char/npc_c.chx", b"npc-c:" + b"C" * 1400),
        ]
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = tdp / "zero_count.pak"
            self.assertEqual(_make_pak(entries, pak, header_count=0), 0)
            self.assertLess(pak.stat().st_size, 16 * 1024)
            _header, _version, n_items, _flag = unpack_pak.parse_pak(str(pak))
            self.assertEqual(n_items, 0)

            raw = pak.read_bytes()
            walked = list(unpack_pak.walk_entries(str(pak), 0))
            self.assertEqual(len(walked), 3)
            for entry, (name, payload) in zip(walked, entries):
                self.assertEqual(entry.name, name)
                got = raw[entry.data_offset:entry.data_offset + entry.real_size]
                self.assertEqual(got, payload)
                self.assertEqual(entry.real_size, len(payload))

            named_out = tdp / "named"
            stats = unpack_pak.pak_extract(
                str(pak), str(named_out),
                dry_run=False, write_manifest=False, verify=False)
            self.assertEqual(stats.file_item_num, 0)
            self.assertEqual(stats.parsed, 3)
            self.assertEqual(stats.written, 3)
            for name, payload in entries:
                self.assertEqual((named_out / name).read_bytes(), payload)

            legacy_out = tdp / "legacy"
            rc = unpack_pak.unpack_legacy(str(pak), str(legacy_out))
            self.assertEqual(rc, 0)
            written = sorted(p.name for p in legacy_out.iterdir())
            self.assertEqual(
                written,
                [f"{i:05d}_{len(payload)}.bin" for i, (_name, payload) in enumerate(entries)],
            )
            for i, (_name, payload) in enumerate(entries):
                blob = legacy_out / f"{i:05d}_{len(payload)}.bin"
                self.assertEqual(blob.read_bytes(), payload)

    def test_nonzero_header_count_still_stops_at_declared_count(self):
        entries = [
            ("Map/one.bmhm", b"one-payload"),
            ("Map/two.bmhm", b"two-payload-ok"),
            ("Map/three.bmhm", b"three-must-stay-unread"),
        ]
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = tdp / "counted.pak"
            self.assertEqual(_make_pak(entries, pak, header_count=2), 2)
            _header, _version, n_items, _flag = unpack_pak.parse_pak(str(pak))
            self.assertEqual(n_items, 2)

            walked = list(unpack_pak.walk_entries(str(pak), n_items))
            self.assertEqual([e.name for e in walked],
                             ["Map/one.bmhm", "Map/two.bmhm"])
            raw = pak.read_bytes()
            self.assertEqual(
                raw[walked[0].data_offset:walked[0].data_offset + walked[0].real_size],
                b"one-payload")
            self.assertEqual(
                raw[walked[1].data_offset:walked[1].data_offset + walked[1].real_size],
                b"two-payload-ok")

            out = tdp / "named"
            stats = unpack_pak.pak_extract(
                str(pak), str(out),
                dry_run=False, write_manifest=False, verify=False)
            self.assertEqual(stats.file_item_num, 2)
            self.assertEqual(stats.parsed, 2)
            self.assertEqual(stats.written, 2)
            self.assertEqual((out / "Map" / "one.bmhm").read_bytes(), b"one-payload")
            self.assertEqual((out / "Map" / "two.bmhm").read_bytes(), b"two-payload-ok")
            self.assertFalse((out / "Map" / "three.bmhm").exists())

            legacy_out = tdp / "legacy"
            rc = unpack_pak.unpack_legacy(str(pak), str(legacy_out))
            self.assertEqual(rc, 0)
            self.assertEqual(
                sorted(p.name for p in legacy_out.iterdir()),
                ["00000_11.bin", "00001_14.bin"],
            )
            self.assertEqual((legacy_out / "00000_11.bin").read_bytes(), b"one-payload")
            self.assertEqual((legacy_out / "00001_14.bin").read_bytes(), b"two-payload-ok")

    def test_zero_header_count_truncated_last_entry_errors(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = tdp / "truncated.pak"
            _make_pak([("Map/ok.bmhm", b"complete-entry")], pak, header_count=0)
            name = b"Map/cut.bmhm"
            claimed_size = 64
            with pak.open("ab") as f:
                f.write(struct.pack(
                    "<IIIIIIII",
                    32 + len(name) + 1 + claimed_size,
                    claimed_size, len(name),
                    0, 0, 0, 0, 0))
                f.write(name)
                f.write(b"\x00")
                f.write(b"X" * 10)  # 10 of the 64 declared payload bytes

            walked = unpack_pak.walk_entries(str(pak), 0)
            first = next(walked)
            self.assertEqual(first.name, "Map/ok.bmhm")
            self.assertEqual(
                pak.read_bytes()[first.data_offset:first.data_offset + first.real_size],
                b"complete-entry")
            with self.assertRaisesRegex(ValueError, r"truncated pak entry 1 .*EOF"):
                next(walked)

            with self.assertRaisesRegex(ValueError, r"truncated pak entry 1 .*EOF"):
                unpack_pak.pak_extract(
                    str(pak), str(tdp / "named"),
                    dry_run=False, write_manifest=False, verify=False)
            with self.assertRaisesRegex(ValueError, r"truncated pak entry 1 .*EOF"):
                unpack_pak.unpack_legacy(str(pak), str(tdp / "legacy"))


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
