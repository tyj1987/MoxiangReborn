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

import contextlib
import hashlib
import io
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
    """Build a worktree-style .pak: 92-byte header, entries from offset 92.

    dwFileItemNum at offset 4 is the entry count and must be non-zero.
    The original 96-byte header is built by _write_original_pak, not here.
    """
    count = len(entries) if header_count is None else header_count
    if count == 0:
        raise ValueError("worktree fixture requires a non-zero header count")
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


def _write_original_pak(out_path: Path, entries: list[tuple[str, bytes]],
                        *, count: int | None = None, chop: int = 0,
                        trailer: bytes = b"") -> bytes:
    """Write an original-style pak from the on-disk layout, not the parser.

    96-byte header: version=1 at 0, n_items=0 at offset 4, entry count
    uint32 at offset 92.  Entries start at 96.  Each entry is
    total, real, name_len, data_off (uint32) + 16 zero bytes, then the
    GBK filename, a NUL, then the payload.  total = 32 + name_len + 1 +
    len(payload).  The next entry is immediately cur + total; no pad.
    """
    body = bytearray()
    cursor = 96
    for name, data in entries:
        name_b = name.encode("gbk")
        name_len = len(name_b)
        real = len(data)
        total = 32 + name_len + 1 + real
        body += struct.pack("<IIII", total, real, name_len, cursor)
        body += b"\x00" * 16
        body += name_b
        body += b"\x00"
        body += data
        cursor += total
    declared = len(entries) if count is None else count
    header = bytearray(96)
    struct.pack_into("<III", header, 0, 1, 0, 0)
    struct.pack_into("<I", header, 92, declared)
    blob = bytes(header) + bytes(body) + trailer
    if chop:
        if chop >= len(blob):
            raise ValueError("chop removes the whole fixture")
        blob = blob[:-chop]
    out_path.write_bytes(blob)
    return blob


class PakHeaderVariantTests(unittest.TestCase):
    """One synthetic pak for each header variant.  No original resources."""

    def test_original_style_count_at_offset_92_reads_three_entries(self):
        entries = [
            ("water.tif", b"water-pixels:" + b"W" * 400),
            ("水面.tif", b"surface:" + b"S" * 500),
            ("Map/zone_c.bmhm", b"zone-c:" + b"Z" * 600),
        ]
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = tdp / "original.pak"
            raw = _write_original_pak(pak, entries)
            self.assertLess(len(raw), 16 * 1024)
            self.assertEqual(struct.unpack_from("<I", raw, 4)[0], 0)
            self.assertEqual(struct.unpack_from("<I", raw, 92)[0], 3)
            gbk_name = "水面.tif".encode("gbk")
            self.assertEqual(gbk_name, bytes.fromhex("cbaec3e62e746966"))
            self.assertIn(gbk_name, raw)
            self.assertNotIn("水面.tif".encode("utf-8"), raw)

            totals = []
            cursor = 96
            for name, payload in entries:
                total, real, name_len, data_off = struct.unpack_from("<IIII", raw, cursor)
                name_b = name.encode("gbk")
                self.assertEqual(total, 32 + len(name_b) + 1 + len(payload))
                self.assertEqual(real, len(payload))
                self.assertEqual(name_len, len(name_b))
                self.assertEqual(data_off, cursor)
                self.assertEqual(raw[cursor + 16:cursor + 32], b"\x00" * 16)
                name_at = cursor + 32
                self.assertEqual(raw[name_at:name_at + name_len], name_b)
                self.assertEqual(raw[name_at + name_len], 0)
                data_at = name_at + name_len + 1
                self.assertEqual(raw[data_at:data_at + real], payload)
                totals.append(total)
                cursor += total
            self.assertEqual(cursor, len(raw))
            # Entry 1 begins on the byte after entry 0.  No alignment gap.
            self.assertEqual(struct.unpack_from("<I", raw, 96 + totals[0])[0], totals[1])

            _header, _version, n_items, _flag = unpack_pak.parse_pak(str(pak))
            self.assertEqual(n_items, 0)
            walked = list(unpack_pak.walk_entries(str(pak), 0))
            self.assertEqual(len(walked), 3)
            self.assertEqual([e.name for e in walked], [name for name, _ in entries])
            self.assertEqual(walked[0].entry_offset, 96)
            self.assertEqual(walked[1].entry_offset, 96 + totals[0])
            self.assertEqual(walked[2].entry_offset, 96 + totals[0] + totals[1])
            for entry, (_name, payload) in zip(walked, entries):
                got = raw[entry.data_offset:entry.data_offset + entry.real_size]
                self.assertEqual(got, payload)

            named_out = tdp / "named"
            stats = unpack_pak.pak_extract(
                str(pak), str(named_out),
                dry_run=False, write_manifest=False, verify=False)
            self.assertEqual(stats.file_item_num, 3)
            self.assertEqual(stats.parsed, 3)
            self.assertEqual(stats.written, 3)
            for name, payload in entries:
                self.assertEqual((named_out / name).read_bytes(), payload)

            legacy_out = tdp / "legacy"
            rc = unpack_pak.unpack_legacy(str(pak), str(legacy_out))
            self.assertEqual(rc, 0)
            for i, (_name, payload) in enumerate(entries):
                blob = legacy_out / f"{i:05d}_{len(payload)}.bin"
                self.assertEqual(blob.read_bytes(), payload)

    def test_worktree_style_nonzero_count_starts_at_92(self):
        entries = [
            ("Map/one.bmhm", b"one-payload"),
            ("Map/two.bmhm", b"two-payload-ok"),
            ("Map/three.bmhm", b"three-must-stay-unread"),
        ]
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = tdp / "worktree.pak"
            self.assertEqual(_make_pak(entries, pak, header_count=2), 2)
            raw = pak.read_bytes()
            self.assertEqual(struct.unpack_from("<I", raw, 0)[0], 1)
            self.assertEqual(struct.unpack_from("<I", raw, 4)[0], 2)
            # Offset 92 is entry 0's total, not an original-style count.
            name_b = b"Map/one.bmhm"
            entry0_total = 32 + len(name_b) + 1 + len(b"one-payload")
            self.assertEqual(struct.unpack_from("<I", raw, 92)[0], entry0_total)
            self.assertNotEqual(entry0_total, 2)
            self.assertEqual(raw[92 + 32:92 + 32 + len(name_b)], name_b)

            _header, _version, n_items, _flag = unpack_pak.parse_pak(str(pak))
            self.assertEqual(n_items, 2)
            walked = list(unpack_pak.walk_entries(str(pak), n_items))
            self.assertEqual([e.name for e in walked],
                             ["Map/one.bmhm", "Map/two.bmhm"])
            self.assertEqual(walked[0].entry_offset, 92)
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

    def test_original_style_early_eof_errors(self):
        entries = [
            ("Map/ok.bmhm", b"complete-entry"),
            ("Map/cut.bmhm", b"X" * 64),
        ]
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = tdp / "early_eof.pak"
            _write_original_pak(pak, entries, chop=20)
            raw = pak.read_bytes()
            self.assertEqual(struct.unpack_from("<I", raw, 4)[0], 0)
            self.assertEqual(struct.unpack_from("<I", raw, 92)[0], 2)

            walked = unpack_pak.walk_entries(str(pak), 0)
            first = next(walked)
            self.assertEqual(first.name, "Map/ok.bmhm")
            self.assertEqual(first.entry_offset, 96)
            self.assertEqual(
                raw[first.data_offset:first.data_offset + first.real_size],
                b"complete-entry")
            with self.assertRaisesRegex(ValueError, r"EOF arrived early"):
                next(walked)

            with self.assertRaisesRegex(ValueError, r"EOF arrived early"):
                unpack_pak.pak_extract(
                    str(pak), str(tdp / "named"),
                    dry_run=False, write_manifest=False, verify=False)
            with self.assertRaisesRegex(ValueError, r"EOF arrived early"):
                unpack_pak.unpack_legacy(str(pak), str(tdp / "legacy"))

    def test_original_style_trailing_bytes_error(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = tdp / "trailing.pak"
            _write_original_pak(
                pak,
                [("water.tif", b"pixels")],
                trailer=b"\x99\x99\x99",
            )
            with self.assertRaisesRegex(
                    ValueError, r"byte\(s\) remain after the declared count"):
                list(unpack_pak.walk_entries(str(pak), 0))
            with self.assertRaisesRegex(ValueError, r"remain after the declared count"):
                unpack_pak.pak_extract(
                    str(pak), str(tdp / "named"),
                    dry_run=False, write_manifest=False, verify=False)


def _run_cli(argv: list[str]) -> tuple[int, str]:
    """Invoke unpack_pak.main with argv.  Returns (rc, stdout)."""
    buf = io.StringIO()
    old = sys.argv
    try:
        sys.argv = ["unpack_pak.py", *argv]
        with contextlib.redirect_stdout(buf):
            rc = unpack_pak.main()
    finally:
        sys.argv = old
    return rc, buf.getvalue()


class DryRunTests(unittest.TestCase):
    """--dry-run enumerates both modes and both headers, and writes nothing."""

    def _assert_dry_run_creates_nothing(self, pak: Path, extra: list[str],
                                        expect_in_stdout: str) -> None:
        out = pak.parent / "out"
        before = sorted(p.name for p in pak.parent.iterdir())
        rc, text = _run_cli([str(pak), "--dry-run", "--out", str(out), *extra])
        self.assertEqual(rc, 0, text)
        self.assertIn(expect_in_stdout, text)
        self.assertIn("dry-run", text)
        self.assertFalse(out.exists(), text)
        self.assertEqual(sorted(p.name for p in pak.parent.iterdir()), before)

    def test_legacy_worktree_92_byte_header(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = tdp / "worktree.pak"
            _make_pak([
                ("Map/one.bmhm", b"one-payload"),
                ("Map/two.bmhm", b"two-payload"),
            ], pak)
            self._assert_dry_run_creates_nothing(pak, [], "Map/one.bmhm")

    def test_legacy_original_96_byte_header(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = tdp / "original.pak"
            _write_original_pak(pak, [
                ("water.tif", b"pixels"),
                ("水面.tif", b"surface"),
            ])
            self._assert_dry_run_creates_nothing(pak, [], "water.tif")

    def test_named_worktree_92_byte_header(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = tdp / "worktree.pak"
            _make_pak([
                ("Map/one.bmhm", b"one-payload"),
                ("Map/two.bmhm", b"two-payload"),
            ], pak)
            self._assert_dry_run_creates_nothing(
                pak, ["--named", "--manifest"], "Map/one.bmhm")

    def test_named_original_96_byte_header(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = tdp / "original.pak"
            _write_original_pak(pak, [
                ("water.tif", b"pixels"),
                ("水面.tif", b"surface"),
            ])
            self._assert_dry_run_creates_nothing(
                pak, ["--named", "--manifest"], "水面.tif")


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
