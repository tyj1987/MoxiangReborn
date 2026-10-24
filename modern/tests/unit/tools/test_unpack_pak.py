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
import shutil
import struct
import sys
import tempfile
import unittest
import uuid
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
            pak = _src_pak(tdp, "tiny.pak")
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
            pak = _src_pak(tdp, "tiny.pak")
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
            pak = _src_pak(tdp, "tiny.pak")
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
            pak = _src_pak(tdp, "tiny.pak")
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
            pak = _src_pak(tdp, "tiny.pak")
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
            pak = _src_pak(tdp, "original.pak")
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
            pak = _src_pak(tdp, "worktree.pak")
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
            pak = _src_pak(tdp, "early_eof.pak")
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
            pak = _src_pak(tdp, "trailing.pak")
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


def _src_pak(root: Path, name: str) -> Path:
    """Put the pak in its own directory so a sibling out dir is legal."""
    src = root / "src"
    src.mkdir(exist_ok=True)
    return src / name


def _run_cli(argv: list[str]) -> tuple[int, str, str]:
    """Invoke unpack_pak.main with argv.  Returns (rc, stdout, stderr)."""
    out_buf = io.StringIO()
    err_buf = io.StringIO()
    old = sys.argv
    try:
        sys.argv = ["unpack_pak.py", *argv]
        with contextlib.redirect_stdout(out_buf), contextlib.redirect_stderr(err_buf):
            rc = unpack_pak.main()
    finally:
        sys.argv = old
    return rc, out_buf.getvalue(), err_buf.getvalue()


def _tree(root: Path) -> list[str]:
    """Relative paths under root, including symlink targets."""
    if not root.exists():
        return []
    items = []
    for path in root.rglob("*"):
        rel = path.relative_to(root).as_posix()
        if path.is_symlink():
            rel += " -> " + os.readlink(path)
        items.append(rel)
    return sorted(items)


class DryRunTests(unittest.TestCase):
    """--dry-run enumerates both modes and both headers, and writes nothing."""

    def _assert_dry_run_creates_nothing(self, pak: Path, extra: list[str],
                                        expect_in_stdout: str) -> None:
        out = pak.parent.parent / "out"
        before = sorted(p.name for p in pak.parent.iterdir())
        rc, text, err = _run_cli([str(pak), "--dry-run", "--out", str(out), *extra])
        self.assertEqual(rc, 0, text + err)
        self.assertIn(expect_in_stdout, text)
        self.assertIn("dry-run", text)
        self.assertFalse(out.exists(), text + err)
        self.assertEqual(sorted(p.name for p in pak.parent.iterdir()), before)
        self.assertFalse(any(p.is_dir() and p != pak.parent
                             for p in pak.parent.parent.iterdir()),
                         "dry-run created a directory")

    def test_legacy_worktree_92_byte_header(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = _src_pak(tdp, "worktree.pak")
            _make_pak([
                ("Map/one.bmhm", b"one-payload"),
                ("Map/two.bmhm", b"two-payload"),
            ], pak)
            self._assert_dry_run_creates_nothing(pak, [], "Map/one.bmhm")

    def test_legacy_original_96_byte_header(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = _src_pak(tdp, "original.pak")
            _write_original_pak(pak, [
                ("water.tif", b"pixels"),
                ("水面.tif", b"surface"),
            ])
            self._assert_dry_run_creates_nothing(pak, [], "water.tif")

    def test_named_worktree_92_byte_header(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = _src_pak(tdp, "worktree.pak")
            _make_pak([
                ("Map/one.bmhm", b"one-payload"),
                ("Map/two.bmhm", b"two-payload"),
            ], pak)
            self._assert_dry_run_creates_nothing(
                pak, ["--named", "--manifest"], "Map/one.bmhm")

    def test_named_original_96_byte_header(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = _src_pak(tdp, "original.pak")
            _write_original_pak(pak, [
                ("water.tif", b"pixels"),
                ("水面.tif", b"surface"),
            ])
            self._assert_dry_run_creates_nothing(
                pak, ["--named", "--manifest"], "水面.tif")


class ReadOnlyOutputTests(unittest.TestCase):
    """Original resource directories are read-only.

    Resolved output may not be the pak directory or anything under it.
    The default directory is the system temp dir (then the workspace).
    """

    def _worktree(self, root: Path, name: str = "tiny.pak") -> Path:
        pak = _src_pak(root, name)
        _make_pak([("Map/one.bmhm", b"one-payload")], pak)
        return pak

    def _assert_cli_refuses(self, pak: Path, out: str, extra: list[str] | None = None) -> None:
        before = _tree(pak.resolve().parent)
        argv = [str(pak), "--out", out]
        if extra:
            argv.extend(extra)
        rc, text, err = _run_cli(argv)
        self.assertEqual(rc, 1, text + err)
        self.assertIn("refusing", err)
        self.assertIn("read-only", err)
        self.assertNotIn("Traceback", err)
        self.assertNotIn("Traceback", text)
        self.assertEqual(_tree(pak.resolve().parent), before)

    def test_output_under_pak_dir_errors_and_writes_nothing(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = self._worktree(tdp)
            pak_dir = pak.parent
            cases = {
                "directory itself": str(pak_dir),
                "subdirectory": str(pak_dir / "extracted"),
                "dotdot back into pak dir": str(pak_dir / "nested" / ".." / "unpacked"),
                "positional-equivalent flag": str(pak_dir / "bins"),
            }
            for label, out in cases.items():
                with self.subTest(label=label):
                    self._assert_cli_refuses(pak, out)
            with self.subTest(label="named"):
                self._assert_cli_refuses(pak, str(pak_dir / "named"), ["--named", "--manifest"])
            with self.subTest(label="dry-run does not bypass the guard"):
                self._assert_cli_refuses(pak, str(pak_dir / "dry"), ["--dry-run"])
            with self.subTest(label="positional out"):
                before = _tree(pak_dir)
                rc, text, err = _run_cli([str(pak), str(pak_dir / "positional")])
                self.assertEqual(rc, 1, text + err)
                self.assertIn("read-only", err)
                self.assertEqual(_tree(pak_dir), before)
                self.assertFalse((pak_dir / "positional").exists())

    def test_relative_and_dot_outputs_from_inside_pak_dir_are_refused(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = self._worktree(tdp)
            old = os.getcwd()
            try:
                os.chdir(pak.parent)
                before = _tree(pak.parent)
                for out in ("extracted", ".", os.path.join("nested", "deeper")):
                    rc, text, err = _run_cli([pak.name, "--out", out])
                    self.assertEqual(rc, 1, text + err)
                    self.assertIn("read-only", err)
                    self.assertEqual(_tree(pak.parent), before)
                os.chdir(tdp)
                before = _tree(pak.parent)
                rc, text, err = _run_cli([
                    str(Path("src") / pak.name),
                    "--out",
                    str(Path("src") / "out"),
                ])
                self.assertEqual(rc, 1, text + err)
                self.assertIn("read-only", err)
                self.assertEqual(_tree(pak.parent), before)
                self.assertFalse((pak.parent / "out").exists())
            finally:
                os.chdir(old)

    def test_api_refuses_before_creating_directories(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = self._worktree(tdp)
            nested = pak.parent / "nested"
            before = _tree(pak.parent)
            for dry_run in (False, True):
                with self.assertRaises(unpack_pak.PakOutputError):
                    unpack_pak.unpack_legacy(str(pak), str(nested), dry_run=dry_run)
                with self.assertRaises(unpack_pak.PakOutputError):
                    unpack_pak.pak_extract(
                        str(pak), str(pak.parent / "named"),
                        dry_run=dry_run, write_manifest=True, verify=False)
            self.assertEqual(_tree(pak.parent), before)
            self.assertFalse(nested.exists())

    def test_resolved_symlink_into_pak_dir_is_refused(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            real_dir = tdp / "resources"
            real_dir.mkdir()
            pak = real_dir / "Map.pak"
            _make_pak([("Map/one.bmhm", b"one-payload")], pak)
            outside = tdp / "outside"
            outside.mkdir()
            trap = outside / "trap"
            try:
                trap.symlink_to(real_dir, target_is_directory=True)
                alias = tdp / "alias"
                alias.symlink_to(Path("resources"), target_is_directory=True)
            except OSError as exc:
                self.skipTest(f"symlinks unavailable: {exc}")
            before = _tree(real_dir)
            self._assert_cli_refuses(pak, str(trap))
            self._assert_cli_refuses(pak, str(trap / "child"))
            self._assert_cli_refuses(alias / "Map.pak", str(real_dir / "unpacked"))
            self.assertEqual(_tree(real_dir), before)
            # The symlink's parent is not the real resource directory.
            rc, text, err = _run_cli([str(alias / "Map.pak"), "--out", str(tdp / "safe")])
            self.assertEqual(rc, 0, text + err)
            self.assertEqual((tdp / "safe" / "00000_11.bin").read_bytes(), b"one-payload")
            self.assertEqual(_tree(real_dir), before)

    def test_output_elsewhere_writes(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = self._worktree(tdp)
            out = tdp / "elsewhere"
            rc, text, err = _run_cli([str(pak), "--out", str(out)])
            self.assertEqual(rc, 0, text + err)
            self.assertEqual((out / "00000_11.bin").read_bytes(), b"one-payload")
            self.assertEqual(_tree(pak.parent), [pak.name])

            named = tdp / "named"
            original = _src_pak(tdp, "original.pak")
            _write_original_pak(original, [
                ("water.tif", b"pixels"),
                ("水面.tif", b"surface"),
            ])
            rc, text, err = _run_cli([
                str(original), "--named", "--manifest", "--out", str(named),
            ])
            self.assertEqual(rc, 0, text + err)
            self.assertEqual((named / "water.tif").read_bytes(), b"pixels")
            self.assertEqual((named / "水面.tif").read_bytes(), b"surface")
            self.assertTrue((named / "manifest.json").is_file())
            self.assertEqual(sorted(p.name for p in original.parent.iterdir()),
                             sorted([pak.name, original.name]))

    def test_parent_of_pak_dir_is_allowed(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = self._worktree(tdp)
            old = os.getcwd()
            try:
                os.chdir(pak.parent)
                before = _tree(pak.parent)
                rc, text, err = _run_cli([pak.name, "--out", ".."])
                self.assertEqual(rc, 0, text + err)
                self.assertEqual((tdp / "00000_11.bin").read_bytes(), b"one-payload")
                self.assertEqual(_tree(pak.parent), before)
            finally:
                os.chdir(old)

    def test_sibling_directory_prefix_is_not_inside(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak_dir = tdp / "client"
            pak_dir.mkdir()
            pak = pak_dir / "a.pak"
            _make_pak([("a.bin", b"x")], pak)
            out = tdp / "client2" / "out"
            self.assertFalse(unpack_pak.output_is_inside_pak_dir(str(pak), str(out)))
            self.assertFalse(unpack_pak.output_is_inside_pak_dir(
                str(pak), str(out), case_insensitive=True))
            rc, text, err = _run_cli([str(pak), "--out", str(out)])
            self.assertEqual(rc, 0, text + err)
            self.assertEqual((out / "00000_1.bin").read_bytes(), b"x")
            self.assertEqual(_tree(pak_dir), [pak.name])

    def test_dry_run_never_writes(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = self._worktree(tdp)
            out = tdp / "out"
            before_root = _tree(tdp)
            rc, text, err = _run_cli([str(pak), "--dry-run", "--out", str(out)])
            self.assertEqual(rc, 0, text + err)
            self.assertIn("dry-run", text)
            self.assertIn("no files written", text)
            self.assertFalse(out.exists())
            self.assertEqual(_tree(tdp), before_root)

            original = _src_pak(tdp, "original.pak")
            _write_original_pak(original, [("water.tif", b"pixels")])
            named = tdp / "named"
            before = _tree(tdp)
            rc, text, err = _run_cli([
                str(original), "--dry-run", "--named", "--manifest", "--out", str(named),
            ])
            self.assertEqual(rc, 0, text + err)
            self.assertIn("dry-run", text)
            self.assertFalse(named.exists())
            self.assertEqual(_tree(tdp), before)

    def test_default_output_is_temp_not_beside_pak(self):
        unique = uuid.uuid4().hex
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = self._worktree(tdp, f"map-{unique}.pak")
            dest = Path(tempfile.gettempdir()) / "moxiang-unpack" / pak.stem
            beside = pak.parent / f"{pak.stem}_unpacked"
            workspace = Path.cwd() / "moxiang-unpack" / pak.stem
            old = os.getcwd()
            try:
                os.chdir(pak.parent)
                before = _tree(pak.parent)
                rc, text, err = _run_cli([pak.name])
                self.assertEqual(rc, 0, text + err)
                self.assertEqual((dest / "00000_11.bin").read_bytes(), b"one-payload")
                self.assertIn(str(dest), text)
                self.assertFalse(beside.exists())
                self.assertFalse(workspace.exists())
                self.assertEqual(_tree(pak.parent), before)
            finally:
                os.chdir(old)
                shutil.rmtree(dest, ignore_errors=True)
                parent = dest.parent
                if parent.is_dir() and not any(parent.iterdir()):
                    parent.rmdir()

    def test_default_uses_workspace_when_temp_is_inside_pak_dir(self):
        unique = uuid.uuid4().hex
        pak = Path(tempfile.gettempdir()) / f"mxh-ro-{unique}.pak"
        scratch = Path(tempfile.mkdtemp(prefix=f"mxh-ro-ws-{unique}-", dir="/workspace"))
        old = os.getcwd()
        temp_candidate = Path(tempfile.gettempdir()) / "moxiang-unpack" / pak.stem
        try:
            _make_pak([("a.bin", b"zz")], pak)
            os.chdir(scratch)
            chosen = unpack_pak.default_unpack_dir(str(pak))
            self.assertEqual(chosen, scratch / "moxiang-unpack" / pak.stem)
            self.assertTrue(unpack_pak.output_is_inside_pak_dir(
                str(pak), str(temp_candidate)))
            rc, text, err = _run_cli([str(pak)])
            self.assertEqual(rc, 0, text + err)
            self.assertEqual((chosen / "00000_2.bin").read_bytes(), b"zz")
            self.assertFalse(temp_candidate.exists())
            self.assertFalse((pak.parent / f"{pak.stem}_unpacked").exists())
        finally:
            os.chdir(old)
            pak.unlink(missing_ok=True)
            shutil.rmtree(scratch, ignore_errors=True)
            if temp_candidate.exists():
                shutil.rmtree(temp_candidate, ignore_errors=True)

    def test_default_errors_when_temp_and_workspace_are_inside_pak_dir(self):
        unique = uuid.uuid4().hex
        pak = Path(tempfile.gettempdir()) / f"mxh-ro-both-{unique}.pak"
        old = os.getcwd()
        temp_candidate = Path(tempfile.gettempdir()) / "moxiang-unpack" / pak.stem
        try:
            _make_pak([("a.bin", b"zz")], pak)
            os.chdir(pak.parent)
            with self.assertRaises(unpack_pak.PakOutputError) as raised:
                unpack_pak.default_unpack_dir(str(pak))
            self.assertIn("refusing", str(raised.exception))
            rc, text, err = _run_cli([pak.name])
            self.assertEqual(rc, 1, text + err)
            self.assertIn("refusing", err)
            self.assertIn("read-only", err)
            self.assertFalse(temp_candidate.exists())
            self.assertFalse((pak.parent / f"{pak.stem}_unpacked").exists())
            self.assertTrue(pak.is_file())
        finally:
            os.chdir(old)
            pak.unlink(missing_ok=True)

    def test_casefold_comparison_and_prefix(self):
        sep = os.sep
        parent = f"{sep}data{sep}client"
        inside = f"{sep}data{sep}Client{sep}out"
        same = f"{sep}data{sep}CLIENT"
        sibling = f"{sep}data{sep}client2{sep}out"
        self.assertTrue(unpack_pak._same_or_under(
            parent, inside, case_insensitive=True))
        self.assertTrue(unpack_pak._same_or_under(
            parent, same, case_insensitive=True))
        self.assertFalse(unpack_pak._same_or_under(
            parent, inside, case_insensitive=False))
        self.assertFalse(unpack_pak._same_or_under(
            parent, same, case_insensitive=False))
        self.assertFalse(unpack_pak._same_or_under(
            parent, sibling, case_insensitive=True))
        self.assertFalse(unpack_pak._same_or_under(
            parent, sibling, case_insensitive=False))
        self.assertFalse(unpack_pak._same_or_under(
            parent, "client", case_insensitive=True))

        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = self._worktree(tdp)
            resolved = str(pak.parent.resolve())
            folded = resolved.swapcase()
            self.assertNotEqual(folded, resolved)
            nested = folded + f"{sep}Nested"
            self.assertTrue(unpack_pak.output_is_inside_pak_dir(
                str(pak), nested, case_insensitive=True))
            self.assertTrue(unpack_pak.output_is_inside_pak_dir(
                str(pak), folded, case_insensitive=True))
            self.assertFalse(unpack_pak.output_is_inside_pak_dir(
                str(pak), nested, case_insensitive=False))
            self.assertFalse(unpack_pak.output_is_inside_pak_dir(
                str(pak), str(pak.parent.resolve()) + "2",
                case_insensitive=True))

    @unittest.skipUnless(os.name == "nt", "case-insensitive CLI paths require Windows")
    def test_windows_cli_refuses_different_case(self):
        with tempfile.TemporaryDirectory() as td:
            tdp = Path(td)
            pak = self._worktree(tdp)
            folded = str(pak.parent.resolve()).swapcase()
            self._assert_cli_refuses(pak, folded + "\\out")


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
