"""Synthetic fixtures for the warn-only MxhResourceReference resource gate."""

import importlib.util
import io
import json
import struct
import subprocess
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[4] / "modern" / "tools" / "resource_reference_gate.py"
SPEC = importlib.util.spec_from_file_location("resource_reference_gate", SCRIPT)
gate = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(gate)

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


def build_original_pak(entries: list[tuple[str | bytes, bytes]], *,
                       offset4: int = 0, count: int | None = None,
                       trailer: bytes = b"") -> bytes:
    """96-byte original layout. Count lives at offset 92, entries at 96.

    ``offset4`` is the uint32 at offset 4. Original Map.pak stores 0 there
    and the real entry count at offset 92. No original game bytes.
    """
    body = bytearray()
    cursor = 96
    encoded: list[tuple[bytes, bytes]] = []
    for name, data in entries:
        name_b = name if isinstance(name, (bytes, bytearray)) else name.encode("gbk")
        encoded.append((bytes(name_b), data))
        name_len = len(name_b)
        real = len(data)
        total = 32 + name_len + 1 + real
        body += struct.pack("<IIII", total, real, name_len, cursor)
        body += b"\x00" * 16
        body += name_b + b"\x00" + data
        cursor += total
    declared = len(encoded) if count is None else count
    header = bytearray(96)
    struct.pack_into("<III", header, 0, 1, offset4, 0)
    struct.pack_into("<I", header, 92, declared)
    return bytes(header) + bytes(body) + trailer


def write_fixture_manifest(directory: Path, manifest: dict) -> dict:
    """Persist a manifest in the test temp dir and read that file back."""
    path = directory / "manifest.json"
    path.write_text(json.dumps(manifest), encoding="utf-8")
    return json.loads(path.read_text(encoding="utf-8"))


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
        fixture = {
            "schema": gate.MANIFEST_SCHEMA,
            "derivation": {"status": "not-run", "unparsed": []},
            "references": [],
            "knownReferencedNoOriginalFile": [],
        }
        with tempfile.TemporaryDirectory() as directory:
            manifest = write_fixture_manifest(Path(directory), fixture)
            self.assertEqual(manifest["derivation"]["status"], "not-run")
            self.assertEqual(manifest["references"], [])
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

    def test_partial_manifest_is_not_a_pass(self):
        fixture = {
            "schema": gate.MANIFEST_SCHEMA,
            "derivation": {
                "status": "partial",
                "unparsed": [{"name": "9.map", "reason": "truncated container"}],
            },
            "references": [
                {"name": "wall.tga", "via": "town.stm", "section": "stm-material", "originalExists": True},
            ],
            "knownReferencedNoOriginalFile": [],
        }
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "wall.tga").write_bytes(b"wall")
            manifest = write_fixture_manifest(root, fixture)
            self.assertEqual(manifest["derivation"]["status"], "partial")
            report = gate.check(manifest, root, strict=False)
        self.assertEqual(report["conclusion"], "partial")
        self.assertEqual(report["exitCode"], 0)
        self.assertNotEqual(report["conclusion"], "clean")
        text = gate.format_report(report)
        self.assertIn("conclusion: partial", text)
        self.assertNotIn("conclusion: clean", text)
        self.assertNotIn("conclusion: pass", text)
        stdout = io.StringIO()
        stderr = io.StringIO()
        with redirect_stdout(stdout), redirect_stderr(stderr):
            gate.emit_warnings(report)
        warning = "resource reference check is partial; this is not a pass"
        self.assertIn(f"::warning::{warning}", stdout.getvalue())
        self.assertIn(f"WARNING: {warning}", stderr.getvalue())
        self.assertIn("no conclusion for 9.map", stdout.getvalue())

    def test_derive_pak_walks_past_a_short_header_count(self):
        # Mirrors original Map.pak: uint32 at offset 4 is 0, the real entry
        # count is the uint32 at offset 92, and that many entries end at EOF.
        entries = [
            ("first.map", b"GX_MAP\nHEIGHT_FIELD\nkept.hfl\n"),
            ("second.map", b"GX_MAP\nSTATIC_MODEL\nalso.stm\n"),
        ]
        real_count = len(entries)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            pak = root / "Map.pak"
            blob = build_original_pak(entries, offset4=0)
            pak.write_bytes(blob)
            self.assertEqual(struct.unpack_from("<I", blob, 4)[0], 0)
            self.assertEqual(struct.unpack_from("<I", blob, 92)[0], real_count)
            manifest = gate.derive(None, pak)
            names = {ref["name"] for ref in manifest["references"]}
            self.assertEqual(manifest["derivation"]["pak"]["headerEntries"], 0)
            self.assertEqual(manifest["derivation"]["pak"]["entriesReadToEof"], real_count)
            self.assertIn("kept.hfl", names)
            self.assertIn("also.stm", names)

    def test_derive_pak_header_zero_walks_all_11267_entries(self):
        count = 11267
        entries = [(f"e{i}.bin", b"x") for i in range(count - 1)]
        entries.append(("zone.map", b"GX_MAP\nHEIGHT_FIELD\nkept.hfl\n"))
        self.assertEqual(len(entries), count)
        with tempfile.TemporaryDirectory() as directory:
            pak = Path(directory) / "Map.pak"
            pak.write_bytes(build_original_pak(entries, offset4=0))
            manifest = gate.derive(None, pak)
        pak_info = manifest["derivation"]["pak"]
        self.assertEqual(pak_info["headerEntries"], 0)
        self.assertEqual(pak_info["entriesReadToEof"], count)
        self.assertIn("kept.hfl", {ref["name"] for ref in manifest["references"]})

    def test_pak_with_neither_or_both_layouts_is_an_error_and_skipped(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            neither = root / "neither.pak"
            neither.write_bytes(build_original_pak([
                ("only.map", b"GX_MAP\nHEIGHT_FIELD\nkept.hfl\n"),
            ]) + b"\x01")
            stderr = io.StringIO()
            with redirect_stderr(stderr):
                manifest = gate.derive(None, neither)
            error = stderr.getvalue()
            self.assertIn("ERROR:", error)
            self.assertIn("neither.pak", error)
            self.assertIn("neither", error)
            self.assertIsNone(manifest["derivation"]["pak"]["entriesReadToEof"])
            self.assertEqual(manifest["references"], [])

            ambiguous = root / "ambiguous.pak"
            name = b"\x00" * 29
            ambiguous.write_bytes(build_original_pak(
                [(name, b""), (name, b"")], offset4=2))
            stderr = io.StringIO()
            with redirect_stderr(stderr):
                manifest = gate.derive(None, ambiguous)
            error = stderr.getvalue()
            self.assertIn("ERROR:", error)
            self.assertIn("ambiguous.pak", error)
            self.assertIn("ambiguous", error)
            self.assertIsNone(manifest["derivation"]["pak"]["entriesReadToEof"])
            self.assertEqual(manifest["references"], [])


if __name__ == "__main__":
    unittest.main()
