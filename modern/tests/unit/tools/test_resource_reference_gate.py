"""Synthetic fixtures for the warn-only MxhResourceReference resource gate."""

import importlib.util
import io
import json
import struct
import subprocess
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[4] / "modern" / "tools" / "resource_reference_gate.py"
SPEC = importlib.util.spec_from_file_location("resource_reference_gate", SCRIPT)
gate = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(gate)

REPO_MANIFEST = Path(__file__).resolve().parents[4] / "reference" / "manifests" / "original-resource-references.json"
LFS_POINTER = b"version https://git-lfs.github.com/spec/v1\noid sha256:abc\nsize 1\n"


def build_hfl(names: list[str], used_slot: int) -> bytes:
    desc = bytearray(gate.HFL_DESC_SIZE)
    struct.pack_into("<I", desc, 44, len(names))
    struct.pack_into("<II", desc, 52, 1, 1)
    struct.pack_into("<II", desc, 96, 1, 1)
    out = bytearray()
    out += struct.pack("<I", 1)
    out += desc
    out += struct.pack("<f", 1.0)
    out += struct.pack("<I", len(names))
    for index, name in enumerate(names):
        raw = name.encode("latin-1")[:127]
        out += struct.pack("<H", index)
        out += raw + b"\x00" * (gate.HFL_NAME_SIZE - len(raw))
    out += struct.pack("<II", 1, 1)
    out += struct.pack("<H", used_slot)
    return bytes(out)


def build_stm(names: list[str]) -> bytes:
    out = bytearray()
    out += struct.pack("<II", 1, len(names))
    for index, name in enumerate(names):
        record = bytearray(gate.STM_MATERIAL_SIZE)
        struct.pack_into("<I", record, 4, index)
        raw = name.encode("latin-1")[:127]
        record[36:36 + len(raw)] = raw
        out += record
    out += struct.pack("<I", 0)
    return bytes(out)


def map_text() -> str:
    return "\n".join([
        "GX_MAP",
        "HEIGHT_FIELD",
        "terrain.hfl",
        "STATIC_MODEL town.stm",
        "GX_OBJECT",
        "house.mod",
        "tile_17_mou_a_01.tga",
        "test4",
        "Monster_203.bin",
        "GX_LIGHT",
        "// 0.hfl must not become a requirement from a comment",
        "4.ttb",
    ])


class ResourceReferenceGateTests(unittest.TestCase):
    def test_hfl_and_stm_keep_original_extensions_and_used_rows_only(self):
        hfl = build_hfl(["tile_17_mou_a_01.tga", "ignored.tga", "1"], used_slot=0)
        self.assertEqual(gate.parse_hfl_used_textures(hfl), ["tile_17_mou_a_01.tga"])
        stm = build_stm(["wall.tga", "double.tga.tga"])
        self.assertEqual(gate.parse_stm_textures(stm), ["wall.tga", "double.tga.tga"])

    def test_map_text_and_binary_use_stored_names_not_the_map_filename(self):
        text = map_text().encode("latin-1")
        names = [ref["name"] for ref in gate.parse_map_references(text)]
        self.assertIn("terrain.hfl", names)
        self.assertIn("town.stm", names)
        self.assertIn("house.mod", names)
        self.assertIn("test4", names)
        self.assertNotIn("0.hfl", names)
        self.assertNotIn("4.ttb", names)
        binary = b"GX_MAP\x00HEIGHT_FIELD\x00terrain.hfl\x00STATIC_MODEL\x00town.stm\x00"
        binary_names = {ref["name"] for ref in gate.parse_map_references(binary)}
        self.assertEqual(binary_names, {"terrain.hfl", "town.stm"})

    def test_derive_does_not_require_unreferenced_hfl_or_ttb(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory)
            (source / "0.map").write_text(map_text(), encoding="latin-1")
            (source / "0.hfl").write_bytes(b"placeholder-height")
            (source / "4.hfl").write_bytes(b"placeholder-height")
            (source / "0.ttb").write_bytes(b"placeholder-tiles")
            (source / "198.ttb").write_bytes(b"placeholder-tiles")
            (source / "terrain.hfl").write_bytes(
                build_hfl(["tile_17_mou_a_01.tga", "ignored.tga", "1"], 0))
            (source / "town.stm").write_bytes(build_stm(["wall.TGA"]))
            (source / "wall.TGA").write_bytes(b"original-wall")
            manifest = gate.derive(source, None)
            names = [ref["name"] for ref in manifest["references"]]
            self.assertEqual(manifest["derivation"]["status"], "complete")
            for banned in ("0.hfl", "4.hfl", "0.ttb", "4.ttb", "198.ttb", "ignored.tga", "1"):
                self.assertNotIn(banned, names)
            self.assertIn("terrain.hfl", names)
            self.assertIn("wall.TGA", names)
            wall = next(ref for ref in manifest["references"] if ref["name"] == "wall.TGA")
            self.assertTrue(wall["originalExists"])
            self.assertTrue(wall["name"].endswith(".TGA"))
            mou = next(ref for ref in manifest["references"] if ref["name"] == "tile_17_mou_a_01.tga")
            self.assertFalse(mou["originalExists"])

    def test_check_categories_warn_only_and_strict_switch(self):
        manifest = {
            "schema": gate.MANIFEST_SCHEMA,
            "derivation": {"status": "complete", "unparsed": []},
            "references": [
                {"name": "wall.tga", "via": "town.stm", "section": "stm-material", "originalExists": True},
                {"name": "needed.bin", "via": "town.map", "section": "GX_OBJECT", "originalExists": True},
                {"name": "tile_17_mou_a_02.tga", "via": "terrain.hfl", "section": "hfl-used-texture", "originalExists": False},
                {"name": "0.hfl", "via": "bad.map", "section": "HEIGHT_FIELD", "originalExists": True},
                {"name": "only-pointer.tga", "via": "town.stm", "section": "stm-material", "originalExists": True},
            ],
            "knownReferencedNoOriginalFile": list(gate.KNOWN_NO_ORIGINAL),
        }
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "wall.tga").write_bytes(b"wall")
            (root / "only-pointer.tga").write_bytes(LFS_POINTER)
            (root / "Monster_203.bin").write_bytes(b"substitute-not-original")
            report = gate.check(manifest, root, strict=False)
            self.assertEqual(report["conclusion"], "no conclusion")
            self.assertEqual(report["exitCode"], 0)
            self.assertEqual([row["name"] for row in report["present"]], ["wall.tga"])
            self.assertEqual([row["name"] for row in report["missing"]], ["needed.bin"])
            informational = {row["name"] for row in report["referencedButNoOriginalFile"]}
            self.assertIn("tile_17_mou_a_01.tga", informational)
            self.assertIn("tile_17_mou_a_09.tga", informational)
            self.assertIn("test4", informational)
            self.assertIn("Monster_203.bin", informational)
            self.assertNotIn("0.hfl", {row["name"] for row in report["missing"]})
            self.assertNotIn("0.hfl", informational)
            self.assertTrue(any(row["name"] == "only-pointer.tga" for row in report["noConclusion"]))
            rendered = gate.format_report(report).lower()
            self.assertIn("no conclusion", rendered)
            self.assertNotIn("conclusion: pass", rendered)
            self.assertNotIn("conclusion: clean", rendered)
            strict = gate.check(manifest, root, strict=True)
            self.assertEqual(strict["exitCode"], 1)
            self.assertEqual(strict["conclusion"], "no conclusion")

    def test_cli_exits_zero_until_the_strict_switch_is_set(self):
        manifest = {
            "schema": gate.MANIFEST_SCHEMA,
            "derivation": {"status": "complete", "unparsed": []},
            "references": [
                {"name": "needed.bin", "via": "town.map", "section": "GX_OBJECT", "originalExists": True},
            ],
            "knownReferencedNoOriginalFile": [],
        }
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path = root / "manifest.json"
            path.write_text(json.dumps(manifest), encoding="utf-8")
            command = [sys.executable, "-B", str(SCRIPT), "check",
                       "--manifest", str(path), "--root", str(root)]
            warned = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(warned.returncode, 0, warned.stderr)
            self.assertIn("conclusion: warnings", warned.stdout)
            self.assertIn("::warning::", warned.stdout)
            self.assertIn("WARNING: missing referenced resource needed.bin", warned.stderr)
            strict = subprocess.run(command + ["--strict"], capture_output=True, text=True)
            self.assertEqual(strict.returncode, 1)
            env = dict(**{k: v for k, v in __import__("os").environ.items()})
            env[gate.STRICT_ENV] = "1"
            from_env = subprocess.run(command, capture_output=True, text=True, env=env)
            self.assertEqual(from_env.returncode, 1)

    def test_unchecked_manifest_is_no_conclusion_not_a_pass(self):
        self.assertTrue(REPO_MANIFEST.is_file())
        manifest = json.loads(REPO_MANIFEST.read_text(encoding="utf-8"))
        self.assertEqual(manifest["derivation"]["status"], "not-run")
        self.assertEqual(manifest["references"], [])
        with tempfile.TemporaryDirectory() as directory:
            report = gate.check(manifest, Path(directory), strict=False)
        self.assertEqual(report["conclusion"], "no conclusion")
        self.assertEqual(report["exitCode"], 0)
        self.assertEqual(report["missing"], [])
        text = gate.format_report(report)
        self.assertIn("no conclusion", text)
        self.assertNotIn("conclusion: clean", text)
        self.assertNotIn("conclusion: pass", text)
        buffer = io.StringIO()
        with redirect_stdout(buffer):
            gate.emit_warnings(report)
        self.assertIn("::warning::", buffer.getvalue())

    def test_derive_pak_walks_past_a_short_header_count(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            pak = root / "Map.pak"
            # Header claims 1 entry. A second .map is still stored and must be read to EOF.
            entries = [
                ("first.map", b"GX_MAP\nHEIGHT_FIELD\nkept.hfl\n"),
                ("second.map", b"GX_MAP\nSTATIC_MODEL\nalso.stm\n"),
            ]
            blob = bytearray(b"\x00" * 92)
            struct.pack_into("<III", blob, 0, 1, 1, 0)
            for name, payload in entries:
                raw_name = name.encode("ascii")
                blob += struct.pack("<IIIIIIII", 0, len(payload), len(raw_name), 0, 0, 0, 0, 0)
                blob += raw_name + b"\x00" + payload
            pak.write_bytes(blob)
            manifest = gate.derive(None, pak)
            names = {ref["name"] for ref in manifest["references"]}
            self.assertEqual(manifest["derivation"]["pak"]["headerEntries"], 1)
            self.assertEqual(manifest["derivation"]["pak"]["entriesReadToEof"], 2)
            self.assertIn("kept.hfl", names)
            self.assertIn("also.stm", names)


if __name__ == "__main__":
    unittest.main()
