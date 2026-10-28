#!/usr/bin/env python3
"""Synthetic tests for modern/tools/pak_writer.py and the gate's
supplemental-pak basename collision check.  Random bytes only; no
original data is read or embedded.

    python -m unittest modern/tests/unit/tools/test_pak_writer.py
"""

from __future__ import annotations

import hashlib
import importlib.util
import io
import json
import os
import struct
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

_TOOLS_DIR = Path(__file__).resolve().parents[3] / "tools"
if str(_TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(_TOOLS_DIR))

import pak_writer  # noqa: E402
import unpack_pak  # noqa: E402

_SPEC = importlib.util.spec_from_file_location(
    "resource_reference_gate", _TOOLS_DIR / "resource_reference_gate.py")
gate = importlib.util.module_from_spec(_SPEC)
_SPEC.loader.exec_module(gate)


def write_92_pak(path: Path, entries: list[tuple[str, bytes]]) -> None:
    """PlayDH layout: count at offset 4, entries from 92."""
    out = bytearray(struct.pack("<III", 1, len(entries), 0))
    out += b"\x00" * (92 - len(out))
    for name, data in entries:
        nb = name.encode("ascii")
        out += struct.pack("<IIII", 32 + len(nb) + 1 + len(data), len(data), len(nb), 0)
        out += b"\x00" * 16 + nb + b"\x00" + data
    path.write_bytes(bytes(out))


class PakWriterTest(unittest.TestCase):
    def setUp(self) -> None:
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = Path(self._tmp.name)
        self.src = self.tmp / "src"
        self.src.mkdir()
        self.files = {
            "b_tree.MOD": os.urandom(257),
            "A.stm": os.urandom(1024),
            "z.tif": os.urandom(1),
            "empty.hfl": b"",
        }
        for name, data in self.files.items():
            (self.src / name).write_bytes(data)
        self.manifest = {"entries": [
            {"name": name, "sha256": hashlib.sha256(data).hexdigest(),
             "sourcePak": "MX/client/Package/3DData/Map.pak", "ticket": "TYJ-0"}
            for name, data in self.files.items()
        ]}

    def tearDown(self) -> None:
        self._tmp.cleanup()

    def test_round_trip_96_layout_only_and_sorted(self):
        out = self.tmp / "out" / "zz_supplement.pak"
        pak_writer.write_pak(self.manifest, self.src, out)
        kind, off4, declared, entries = unpack_pak.read_entries(str(out))
        self.assertEqual((kind, off4, declared), ("96-byte", 0, len(self.files)))
        self.assertEqual([e.name for e in entries], ["A.stm", "b_tree.MOD", "empty.hfl", "z.tif"])
        with out.open("rb") as f:
            for e in entries:
                f.seek(e.data_offset)
                self.assertEqual(f.read(e.real_size), self.files[e.name])
        # The 92-byte walk must not also be valid.
        with self.assertRaises(ValueError):
            unpack_pak._probe_92_byte(str(out), out.stat().st_size, off4)

    def test_reproducible(self):
        a, b = self.tmp / "a.pak", self.tmp / "b.pak"
        da = pak_writer.write_pak(self.manifest, self.src, a)
        reordered = {"entries": list(reversed(self.manifest["entries"]))}
        db = pak_writer.write_pak(reordered, self.src, b)
        self.assertEqual(da, db)
        self.assertEqual(a.read_bytes(), b.read_bytes())
        pinned = dict(self.manifest, pakSha256=da)
        self.assertEqual(pak_writer.write_pak(pinned, self.src, self.tmp / "c.pak"), da)
        pinned["pakSha256"] = "f" * 64
        with self.assertRaisesRegex(pak_writer.PakWriteError, "pakSha256"):
            pak_writer.write_pak(pinned, self.src, self.tmp / "d.pak")
        self.assertFalse((self.tmp / "d.pak").exists())

    def test_conflict_with_92_pak_is_case_insensitive_basename(self):
        root = self.tmp / "root"
        root.mkdir()
        write_92_pak(root / "Map.pak", [("Map\\B_TREE.mod", os.urandom(9))])
        out = root / "zz_supplement.pak"
        with self.assertRaisesRegex(pak_writer.PakWriteError, "b_tree.MOD"):
            pak_writer.write_pak(self.manifest, self.src, out, [root])
        self.assertFalse(out.exists())
        self.assertEqual(sorted(p.name for p in root.iterdir()), ["Map.pak"])
        with redirect_stderr(io.StringIO()):
            code = pak_writer.main(["--manifest", str(self._manifest_file()),
                                    "--input-dir", str(self.src), "--out", str(out),
                                    "--conflict-root", str(root)])
        self.assertEqual(code, 1)

    def test_no_conflict_writes(self):
        root = self.tmp / "root"
        root.mkdir()
        write_92_pak(root / "Map.pak", [("other.mod", os.urandom(9))])
        pak_writer.write_pak(self.manifest, self.src, root / "zz_supplement.pak", [root])
        self.assertTrue((root / "zz_supplement.pak").is_file())

    def test_sha_mismatch_and_null_sha_rejected(self):
        bad = json.loads(json.dumps(self.manifest))
        bad["entries"][0]["sha256"] = "0" * 64
        out = self.tmp / "x.pak"
        with self.assertRaisesRegex(pak_writer.PakWriteError, "sha256 mismatch"):
            pak_writer.write_pak(bad, self.src, out)
        bad["entries"][0]["sha256"] = None
        with self.assertRaisesRegex(pak_writer.PakWriteError, "not filled"):
            pak_writer.write_pak(bad, self.src, out)
        self.assertFalse(out.exists())

    def test_duplicate_basename_in_manifest_rejected(self):
        dup = json.loads(json.dumps(self.manifest))
        dup["entries"].append(dict(dup["entries"][1], name="a.STM"))
        (self.src / "a.STM").write_bytes(self.files["A.stm"])
        with self.assertRaisesRegex(pak_writer.PakWriteError, "duplicate basename"):
            pak_writer.write_pak(dup, self.src, self.tmp / "x.pak")

    def test_gate_reports_supplemental_collision(self):
        root = self.tmp / "root"
        root.mkdir()
        pak_writer.write_pak(self.manifest, self.src, root / "zz_supplement.pak")
        write_92_pak(root / "Map.pak", [("map\\z.TIF", os.urandom(5))])
        rows = gate.supplemental_pak_collisions(root)
        self.assertEqual([r["name"] for r in rows], ["z.tif"])
        manifest = {"schema": gate.MANIFEST_SCHEMA, "derivation": {"status": "complete"},
                    "references": [], "knownReferencedNoOriginalFile": []}
        report = gate.check(manifest, root)
        self.assertEqual(report["conclusion"], "collision")
        self.assertEqual(report["exitCode"], 1)
        (root / "Map.pak").unlink()
        write_92_pak(root / "Map.pak", [("map\\other.tif", os.urandom(5))])
        self.assertEqual(gate.supplemental_pak_collisions(root), [])

    def test_gate_unreadable_other_pak_is_collision(self):
        root = self.tmp / "root"
        root.mkdir()
        pak_writer.write_pak(self.manifest, self.src, root / "zz_supplement.pak")
        (root / "Broken.pak").write_bytes(b"\xff" * 7)
        (root / "Lfs.pak").write_text(
            "version https://git-lfs.github.com/spec/v1\noid sha256:" + "0" * 64 + "\nsize 1\n")
        rows = gate.supplemental_pak_collisions(root)
        self.assertEqual([(r["other"], r["reason"]) for r in rows],
                         [("Broken.pak", "other pak unreadable")])
        manifest = {"schema": gate.MANIFEST_SCHEMA, "derivation": {"status": "complete"},
                    "references": [], "knownReferencedNoOriginalFile": []}
        report = gate.check(manifest, root)
        self.assertEqual(report["conclusion"], "collision")
        self.assertEqual(report["exitCode"], 1)
        (root / "Broken.pak").unlink()
        self.assertEqual(gate.supplemental_pak_collisions(root), [])

    def test_cli_requires_conflict_root(self):
        out = self.tmp / "out" / "zz_supplement.pak"
        err = io.StringIO()
        with redirect_stderr(err):
            code = pak_writer.main(["--manifest", str(self._manifest_file()),
                                    "--input-dir", str(self.src), "--out", str(out)])
        self.assertEqual(code, 1)
        self.assertIn("--conflict-root is required", err.getvalue())
        self.assertFalse(out.exists())

    def test_cli_no_conflict_check_warns_and_writes(self):
        out = self.tmp / "out" / "zz_supplement.pak"
        err = io.StringIO()
        with redirect_stderr(err), redirect_stdout(io.StringIO()):
            code = pak_writer.main(["--manifest", str(self._manifest_file()),
                                    "--input-dir", str(self.src), "--out", str(out),
                                    "--no-conflict-check"])
        self.assertEqual(code, 0)
        self.assertIn("WARN", err.getvalue())
        self.assertTrue(out.is_file())

    def test_read_back_rejects_name_differing_only_by_case(self):
        entries = sorted(self.files.items())
        bad = [("b_tree.mod" if n == "b_tree.MOD" else n, d) for n, d in entries]
        pak = self.tmp / "case.pak"
        pak.write_bytes(pak_writer.build_pak_bytes(bad))
        with self.assertRaisesRegex(pak_writer.PakWriteError, "case-sensitive"):
            pak_writer.verify_pak(pak, entries)
        pak.write_bytes(pak_writer.build_pak_bytes(entries))
        pak_writer.verify_pak(pak, entries)

    def test_read_back_rejects_changed_bytes_same_size(self):
        entries = sorted(self.files.items())
        bad = [(n, bytes([d[0] ^ 1]) + d[1:] if n == "A.stm" else d) for n, d in entries]
        pak = self.tmp / "bytes.pak"
        pak.write_bytes(pak_writer.build_pak_bytes(bad))
        with self.assertRaisesRegex(pak_writer.PakWriteError, "bytes mismatch"):
            pak_writer.verify_pak(pak, entries)

    def test_main_exits_nonzero_when_read_back_name_case_differs(self):
        real = pak_writer.build_pak_bytes
        def lowered(entries):
            return real([(n.lower(), d) for n, d in entries])
        out = self.tmp / "out" / "zz_supplement.pak"
        pak_writer.build_pak_bytes = lowered
        try:
            with redirect_stderr(io.StringIO()), redirect_stdout(io.StringIO()):
                code = pak_writer.main(["--manifest", str(self._manifest_file()),
                                        "--input-dir", str(self.src), "--out", str(out),
                                        "--no-conflict-check"])
        finally:
            pak_writer.build_pak_bytes = real
        self.assertNotEqual(code, 0)
        self.assertFalse(out.exists())

    def test_checked_in_manifest_is_names_only(self):
        path = Path(__file__).resolve().parents[4] / "reference" / "manifests" / "playdh-supplemental-fill.json"
        doc = json.loads(path.read_text(encoding="utf-8"))
        entries = doc["entries"]
        self.assertEqual(len(entries), 194)
        self.assertTrue(all(set(e) == {"name", "sourcePak", "sha256", "ticket"} for e in entries))
        self.assertEqual(
            doc["pakSha256"],
            "1d7a87e00bcb54b7f9d39a719973ebc1efcf7c2505711e7d97d98b0157c5cbd5",
        )
        self.assertRegex(doc["pakSha256"], r"^[0-9a-f]{64}$")
        self.assertEqual(doc["buildPath"], "D:\\build-mxr\\supplement\\zz_supplement.pak")
        self.assertNotIn("(", doc["buildPath"])
        keys = [pak_writer._basename_key(e["name"]) for e in entries]
        self.assertEqual(len(keys), len(set(keys)))
        for entry in entries:
            self.assertRegex(entry["sha256"], r"^[0-9a-f]{64}$")
            self.assertIsInstance(entry["sourcePak"], str)
            self.assertTrue(entry["sourcePak"])
        filled, added = entries[:73], entries[73:]
        self.assertEqual(
            hashlib.sha256(
                json.dumps(filled, ensure_ascii=False, separators=(",", ":")).encode()
            ).hexdigest(),
            "a0c615dd15a40f77fb12e17dbae18d088a8e941d939f199b6f79357971d61aa2",
        )
        self.assertEqual(len(added), 121)
        self.assertTrue(all(entry["ticket"] == "TYJ-153" for entry in added))

    def test_refuses_output_in_git_tree_and_tool_repo(self):
        work = self.tmp / "work"
        (work / ".git").mkdir(parents=True)
        out = work / "sub" / "zz_supplement.pak"
        with self.assertRaisesRegex(pak_writer.PakWriteError, "git working tree"):
            pak_writer.write_pak(self.manifest, self.src, out)
        self.assertFalse(out.parent.exists() and any(out.parent.iterdir()))
        with self.assertRaisesRegex(pak_writer.PakWriteError, "inside the repository"):
            pak_writer.write_pak(self.manifest, self.src,
                                 pak_writer.TOOL_REPO / "build" / "zz_supplement.pak")

    def test_refuses_symlink_into_repo(self):
        work = self.tmp / "work"
        (work / ".git").mkdir(parents=True)
        link = self.tmp / "link"
        try:
            link.symlink_to(work, target_is_directory=True)
        except (OSError, NotImplementedError):
            self.skipTest("symlinks unavailable")
        with self.assertRaisesRegex(pak_writer.PakWriteError, "git working tree"):
            pak_writer.write_pak(self.manifest, self.src, link / "zz_supplement.pak")
        self.assertEqual(list(work.iterdir()), [work / ".git"])

    def test_forbidden_roots_arg_and_env(self):
        root = self.tmp / "deploy"
        with self.assertRaisesRegex(pak_writer.PakWriteError, "forbidden root"):
            pak_writer.write_pak(self.manifest, self.src, root / "x.pak", forbidden_roots=[str(root)])
        old = os.environ.get(pak_writer.FORBIDDEN_ENV)
        os.environ[pak_writer.FORBIDDEN_ENV] = str(root)
        try:
            with self.assertRaisesRegex(pak_writer.PakWriteError, "forbidden root"):
                pak_writer.write_pak(self.manifest, self.src, root / "x.pak")
        finally:
            if old is None:
                os.environ.pop(pak_writer.FORBIDDEN_ENV)
            else:
                os.environ[pak_writer.FORBIDDEN_ENV] = old

    def test_original_root_path_function(self):
        for path in (r"D:\MX", r"d:\mx\client\Package", "D:/MX/x.pak", "d:/Mx/"):
            self.assertTrue(pak_writer.is_original_root(path), path)
        for path in (r"D:\MX2", r"D:\work\MX", "C:/MX", "D:/MXH"):
            self.assertFalse(pak_writer.is_original_root(path), path)

    def _manifest_file(self) -> Path:
        path = self.tmp / "manifest.json"
        path.write_text(json.dumps(self.manifest), encoding="utf-8")
        return path


if __name__ == "__main__":
    unittest.main()
