"""Synthetic fixtures for the warn-only MxhResourceReference resource gate."""

import hashlib
import importlib.util
import io
import json
import os
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
import pak_writer  # noqa: E402

LFS_POINTER = b"version https://git-lfs.github.com/spec/v1\noid sha256:abc\nsize 1\n"


def manifest_text(document: dict) -> str:
    """Same bytes ``derive`` writes, without a timestamp or a resolved path."""
    return json.dumps(document, ensure_ascii=False, indent=2) + "\n"


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


def build_playdh_pak(entries: list[tuple[str, bytes]]) -> bytes:
    """92-byte PlayDH layout: count u32 at offset 4, entries from offset 92.

    Each entry: 32-byte header (total, real, name_len, then zeros), name,
    NUL, data. Synthetic bytes only.
    """
    out = bytearray(struct.pack("<III", 1, len(entries), 0)) + b"\x00" * (92 - 12)
    for name, data in entries:
        name_b = name.encode("latin-1")
        total = 32 + len(name_b) + 1 + len(data)
        out += struct.pack("<IIIIIIII", total, len(data), len(name_b), 0, 0, 0, 0, 0)
        out += name_b + b"\x00" + data
    return bytes(out)


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
            # None of the recorded-absent names are files here. The guard
            # must leave this manifest byte-identical to the pre-guard derive.
            self.assertEqual(set(manifest), {
                "schema",
                "derivation",
                "references",
                "excludedNotReferenced",
                "notReferencedByOriginal",
                "knownReferencedNoOriginalFile",
            })
            self.assertEqual(
                manifest["knownReferencedNoOriginalFile"], list(gate.KNOWN_NO_ORIGINAL))
            index = gate._build_exists_index(source)
            self.assertEqual(gate._dds_stem_name("tile_17_mou_a_03.tga"), "tile_17_mou_a_03.dds")
            self.assertEqual(gate._dds_stem_name("Monster_203.bin"), "Monster_203.dds")
            self.assertEqual(gate._dds_stem_name("test4"), "test4.dds")
            for name in gate.KNOWN_NO_ORIGINAL:
                self.assertIsNone(index.locate(name), name)
                dds = gate._dds_stem_name(name)
                self.assertNotEqual(gate._ascii_fold(dds), gate._ascii_fold(name))
                self.assertIsNone(index.locate(dds), dds)
            blob = manifest_text(manifest)
            self.assertEqual(
                hashlib.sha256(blob.encode("utf-8")).hexdigest(),
                "85d85efd7c1431f58de333bd4a51506c42b43b0aa759a26c3d98965464257715")
            output = source / "derived.json"
            stdout = io.StringIO()
            stderr = io.StringIO()
            with redirect_stdout(stdout), redirect_stderr(stderr):
                code = gate.main([
                    "derive", "--source", str(source), "--output", str(output)])
            self.assertEqual(code, 0, stderr.getvalue())
            self.assertEqual(stderr.getvalue(), "")
            self.assertEqual(output.read_text(encoding="utf-8"), blob)

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



def synthetic_obfuscated_map(byte2: int, body_len: int = 256, seed: int = 7) -> bytes:
    """Fake 4+20-byte header and random body. Not game data, not decoded."""
    import random
    rng = random.Random(seed)
    body = bytes(rng.randrange(256) for _ in range(body_len))
    header = bytearray(rng.randrange(256) for _ in range(20))
    header[2] = byte2
    payload = bytes(header) + body
    return struct.pack("<I", 4 + len(payload)) + payload


class ObfuscatedMapReasonTests(unittest.TestCase):
    def test_synthetic_obfuscated_map_is_classified(self):
        data = synthetic_obfuscated_map(0x5A)
        self.assertTrue(gate.is_obfuscated_map(data))
        self.assertFalse(gate.is_obfuscated_map(data[:-1]))
        # Length field must cover the whole entry, not just the bytes after it.
        wrong = struct.pack("<I", len(data) - 4) + data[4:]
        self.assertFalse(gate.is_obfuscated_map(wrong))

    def test_mostly_printable_obfuscated_body_is_still_classified(self):
        import random
        rng = random.Random(3)
        payload = bytes(rng.randrange(256) for _ in range(20)) + bytes(
            rng.randrange(0x20, 0x7F) for _ in range(512))
        data = struct.pack("<I", 4 + len(payload)) + payload
        self.assertTrue(gate.is_obfuscated_map(data))

    def test_plaintext_gx_map_is_not_obfuscated(self):
        text = b"GX_MAP\r\nHEIGHT_FIELD terrain.hfl\r\n"
        data = struct.pack("<I", 4 + len(text)) + text
        self.assertFalse(gate.is_obfuscated_map(b"GX_" + data[3:]))
        self.assertFalse(gate.is_obfuscated_map(data))

    def test_derive_reason_and_summary_hold_no_header_bytes(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory)
            (source / "1.map").write_bytes(synthetic_obfuscated_map(0x5A, seed=1))
            (source / "2.map").write_bytes(synthetic_obfuscated_map(0x11, seed=2))
            (source / "3.map").write_bytes(b"\x00\xff" * 40)
            (source / "0.map").write_text(map_text(), encoding="latin-1")
            (source / "terrain.hfl").write_bytes(build_hfl(["a.tga"], 0))
            (source / "town.stm").write_bytes(build_stm(["wall.TGA"]))
            stats: dict = {}
            manifest = gate.derive(source, None, local_header_stats=stats)
        derivation = manifest["derivation"]
        reasons = {item["name"]: item["reason"] for item in derivation["unparsed"]}
        self.assertEqual(reasons, {
            "1.map": gate.OBFUSCATED_MAP_REASON,
            "2.map": gate.OBFUSCATED_MAP_REASON,
            "3.map": gate.NO_MARKERS_REASON,
        })
        for item in derivation["unparsed"]:
            self.assertEqual(set(item), {"name", "reason"})
        self.assertEqual(derivation["unparsedSummary"], {
            "total": 3,
            "byReason": {gate.NO_MARKERS_REASON: 1, gate.OBFUSCATED_MAP_REASON: 2},
        })
        self.assertIn("terrain.hfl", {ref["name"] for ref in manifest["references"]})
        self.assertEqual(sum(stats.values()), 2)
        rendered = json.dumps(manifest)
        self.assertNotIn("headerByte", rendered)
        self.assertNotIn("Byte2", rendered)

    def test_check_keeps_obfuscated_maps_as_no_conclusion(self):
        manifest = {
            "schema": gate.MANIFEST_SCHEMA,
            "derivation": {"status": "partial", "unparsed": [
                {"name": "1.map", "reason": gate.OBFUSCATED_MAP_REASON},
                {"name": "2.map", "reason": gate.OBFUSCATED_MAP_REASON},
            ]},
            "references": [],
        }
        with tempfile.TemporaryDirectory() as directory:
            report = gate.check(manifest, Path(directory))
        self.assertEqual(report["counts"]["noConclusion"], 2)

    def test_local_header_stats_flag_is_opt_in_and_stdout_only(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "src"
            source.mkdir()
            (source / "1.map").write_bytes(synthetic_obfuscated_map(0x5A))
            out = Path(directory) / "m.json"
            buf = io.StringIO()
            with redirect_stdout(buf):
                gate.main(["derive", "--source", str(source), "--output", str(out)])
            self.assertNotIn("header byte", buf.getvalue())
            buf = io.StringIO()
            with redirect_stdout(buf):
                gate.main(["derive", "--source", str(source), "--output", str(out), "--local-header-stats"])
            self.assertIn("local-only", buf.getvalue())
            self.assertNotIn("header", out.read_text(encoding="utf-8").lower().replace("headerentries", ""))


# Made-up 8-byte key chosen by this test. Unrelated to any real key and not
# derived from header bytes; the synthetic header below is all zeros.
TEST_XOR_KEY = bytes([0x13, 0x37, 0xC0, 0xDE, 0x42, 0x99, 0x5E, 0xA1])
TEST_DECODER_VERSION = "synthetic-test-decoder-1"
FAKE_PLAIN_MAP = "\n".join([
    "GX_MAP", "HEIGHT_FIELD", "fake_terrain.hfl", "GX_OBJECT", "fake_house.mod", "",
]).encode("latin-1")

PLUGIN_SOURCE = f"""
DECODER_VERSION = {TEST_DECODER_VERSION!r}
_KEY = {TEST_XOR_KEY!r}

def decode_map(name, data):
    if name == "nokey.map":
        return None
    body = data[24:]
    return bytes(b ^ _KEY[i % 8] for i, b in enumerate(body))
"""

SOURCED_PLUGIN_SOURCE = PLUGIN_SOURCE + """
def key_source(name):
    return "table-test" if name == "1.map" else None
"""

BAD_SOURCE_PLUGIN_SOURCE = PLUGIN_SOURCE + """
def key_source(name):
    return "Not A Label!"
"""

NONE_PLUGIN_SOURCE = """
DECODER_VERSION = "synthetic-none-1"

def decode_map(name, data):
    return None
"""


def synthetic_xor_map(plain: bytes) -> bytes:
    body = bytes(b ^ TEST_XOR_KEY[i % 8] for i, b in enumerate(plain))
    payload = bytes(20) + body
    return struct.pack("<I", 4 + len(payload)) + payload


class LocalDecoderHookTests(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.tmp = Path(self._tmp.name)
        self.addCleanup(self._tmp.cleanup)
        self.obf = synthetic_xor_map(FAKE_PLAIN_MAP)
        self.assertTrue(gate.is_obfuscated_map(self.obf))
        self.pak = self.tmp / "Map.pak"
        self.pak.write_bytes(build_original_pak([
            ("1.map", self.obf),
            ("nokey.map", synthetic_xor_map(FAKE_PLAIN_MAP + b"\n")),
            ("fake_terrain.hfl", build_hfl(["fake_tex.tga"], 0)),
        ]))
        self.plugin_path = self.tmp / "plugin" / "fake_decoder.py"
        self.plugin_path.parent.mkdir()
        self.plugin_path.write_text(PLUGIN_SOURCE, encoding="utf-8")

    def derive_with_plugin(self):
        plugin = gate.load_local_decoder(self.plugin_path)
        return gate.derive(None, self.pak, local_decoder=plugin)

    def test_plugin_conclusions_are_tagged_and_manifest_holds_no_secrets(self):
        manifest = self.derive_with_plugin()
        refs = {ref["name"]: ref for ref in manifest["references"]}
        for name in ("fake_terrain.hfl", "fake_house.mod", "fake_tex.tga"):
            self.assertEqual(refs[name]["derivedBy"], "local-decoder")
        info = manifest["derivation"]["localDecoder"]
        self.assertEqual(info["version"], TEST_DECODER_VERSION)
        self.assertEqual(info["sourcePakSha256"], hashlib.sha256(self.pak.read_bytes()).hexdigest())
        self.assertEqual(info["decodedMaps"], [
            {"name": "1.map", "sha256": hashlib.sha256(self.obf).hexdigest()}])
        self.assertEqual(manifest["derivation"]["unparsed"], [
            {"name": "nokey.map", "reason": "obfuscated-map-key-unavailable"}])
        rendered = json.dumps(manifest)
        # Section names are schema fields; plaintext runs, key and header are not.
        plain_run = json.dumps(FAKE_PLAIN_MAP.decode("latin-1"))[1:21]
        for leak in ("GX_MAP", plain_run, TEST_XOR_KEY.hex(), "header"):
            self.assertNotIn(leak, rendered.replace("headerEntries", ""))

    def test_key_source_is_recorded_per_map_and_counted(self):
        sourced = self.tmp / "plugin" / "sourced_decoder.py"
        sourced.write_text(SOURCED_PLUGIN_SOURCE, encoding="utf-8")
        manifest = gate.derive(None, self.pak, local_decoder=gate.load_local_decoder(sourced))
        info = manifest["derivation"]["localDecoder"]
        self.assertEqual(info["decodedMaps"], [
            {"name": "1.map", "sha256": hashlib.sha256(self.obf).hexdigest(), "keySource": "table-test"}])
        self.assertEqual(info["keySourceCounts"], {"table-test": 1})

    def test_plugin_without_key_source_writes_no_key_source_fields(self):
        info = self.derive_with_plugin()["derivation"]["localDecoder"]
        self.assertNotIn("keySourceCounts", info)
        self.assertTrue(all("keySource" not in entry for entry in info["decodedMaps"]))

    def test_bad_key_source_label_is_refused(self):
        bad = self.tmp / "plugin" / "bad_source_decoder.py"
        bad.write_text(BAD_SOURCE_PLUGIN_SOURCE, encoding="utf-8")
        with self.assertRaises(gate.LocalDecoderError):
            gate.derive(None, self.pak, local_decoder=gate.load_local_decoder(bad))

    def test_plugin_returning_none_marks_key_unavailable(self):
        none_plugin = self.tmp / "plugin" / "none_decoder.py"
        none_plugin.write_text(NONE_PLUGIN_SOURCE, encoding="utf-8")
        manifest = gate.derive(None, self.pak, local_decoder=gate.load_local_decoder(none_plugin))
        reasons = {item["name"]: item["reason"] for item in manifest["derivation"]["unparsed"]}
        self.assertEqual(reasons, {"1.map": "obfuscated-map-key-unavailable",
                                   "nokey.map": "obfuscated-map-key-unavailable"})
        self.assertEqual(manifest["references"], [])

    def test_no_plugin_without_explicit_arg_or_env(self):
        self.assertIsNone(gate.resolve_local_decoder_path(None, environ={}))
        self.assertEqual(
            gate.resolve_local_decoder_path(None, environ={gate.LOCAL_DECODER_ENV: str(self.plugin_path)}),
            self.plugin_path)
        out = self.tmp / "m.json"
        old = os.environ.pop(gate.LOCAL_DECODER_ENV, None)
        try:
            with redirect_stdout(io.StringIO()):
                gate.main(["derive", "--pak", str(self.pak), "--output", str(out)])
        finally:
            if old is not None:
                os.environ[gate.LOCAL_DECODER_ENV] = old
        manifest = json.loads(out.read_text(encoding="utf-8"))
        self.assertNotIn("localDecoder", manifest["derivation"])
        self.assertEqual({item["reason"] for item in manifest["derivation"]["unparsed"]}, {"obfuscated-map"})
        with redirect_stdout(io.StringIO()):
            gate.main(["derive", "--pak", str(self.pak), "--output", str(out),
                       "--local-decoder", str(self.plugin_path)])
        self.assertIn("localDecoder", json.loads(out.read_text(encoding="utf-8"))["derivation"])

    def test_local_decoder_without_pak_is_refused(self):
        plugin = gate.load_local_decoder(self.plugin_path)
        with self.assertRaises(gate.LocalDecoderError):
            gate.derive(self.tmp, None, local_decoder=plugin)
        out = self.tmp / "nopak.json"
        with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
            with self.assertRaises(SystemExit) as ctx:
                gate.main(["derive", "--source", str(self.tmp), "--output", str(out),
                           "--local-decoder", str(self.plugin_path)])
        self.assertNotEqual(ctx.exception.code, 0)
        self.assertFalse(out.exists())

    def test_plugin_inside_repo_is_refused(self):
        with self.assertRaises(gate.LocalDecoderError):
            gate.load_local_decoder(SCRIPT)
        with self.assertRaises(gate.LocalDecoderError):
            gate.load_local_decoder(self.plugin_path, repo_root=self.tmp)

    def test_check_separates_local_decoder_counts(self):
        manifest = write_fixture_manifest(self.tmp, self.derive_with_plugin())
        root = self.tmp / "checkout"
        root.mkdir()
        unverified = gate.check(manifest, root)
        self.assertEqual(unverified["localDecoder"]["state"], "unverified")
        self.assertEqual(unverified["counts"]["localDecoder"], 3)
        self.assertEqual(unverified["exitCode"], 0)
        self.assertIn("local-decoder=3 state=unverified", gate.format_report(unverified))
        verified = gate.check(manifest, root, pak=self.pak)
        self.assertEqual(verified["localDecoder"]["state"], "verified")
        self.assertEqual(verified["counts"]["noConclusion"], unverified["counts"]["noConclusion"])

    def test_pak_sha_mismatch_makes_local_conclusions_not_run(self):
        manifest = write_fixture_manifest(self.tmp, self.derive_with_plugin())
        other = self.tmp / "Other.pak"
        other.write_bytes(build_original_pak([("x.map", b"GX_MAP\n")]))
        root = self.tmp / "checkout"
        root.mkdir()
        report = gate.check(manifest, root, pak=other)
        self.assertEqual(report["localDecoder"]["state"], "stale")
        stale = [row for row in report["noConclusion"] if "not-run (stale)" in row["reason"]]
        self.assertEqual({row["name"] for row in stale}, {"fake_terrain.hfl", "fake_house.mod", "fake_tex.tga"})

    def test_old_manifest_report_has_no_local_decoder_fields(self):
        manifest = {"schema": gate.MANIFEST_SCHEMA,
                    "derivation": {"status": "partial", "unparsed": [{"name": "9.map", "reason": "obfuscated-map"}]},
                    "references": [{"name": "a.tga", "via": "1.map", "section": "x", "originalExists": True}]}
        report = gate.check(manifest, self.tmp, pak=self.pak)
        self.assertEqual(set(report["counts"]),
                         {"present", "missing", "referencedButNoOriginalFile", "noConclusion",
                          "pakLfsPointer", "pakIndexUnreadable", "caseMismatch", "extensionSubstituted"})
        self.assertNotIn("localDecoder", report["counts"])
        self.assertNotIn("localDecoder", report)
        self.assertNotIn("local-decoder", gate.format_report(report))


class PakLfsPointerAndFidelityTests(unittest.TestCase):
    """TYJ-147: pointer paks, case-only matches and extension swaps."""

    @staticmethod
    def manifest(names: list[str]) -> dict:
        return {
            "schema": gate.MANIFEST_SCHEMA,
            "derivation": {"status": "complete", "unparsed": []},
            "references": [
                {"name": name, "via": "town.map", "section": "GX_OBJECT", "originalExists": True}
                for name in names
            ],
            "knownReferencedNoOriginalFile": [],
        }

    def test_pointer_detection_is_by_content_not_size(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "big.pak").write_bytes(LFS_POINTER + b"x" * 4096)
            (root / "tiny.pak").write_bytes(b"version 1")
            self.assertTrue(gate.is_lfs_pointer(root / "big.pak"))
            self.assertFalse(gate.is_lfs_pointer(root / "tiny.pak"))

    def test_pointer_pak_makes_unresolved_refs_pak_lfs_pointer_not_missing(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "Map.pak").write_bytes(LFS_POINTER)
            (root / "npc.pak").write_bytes(build_playdh_pak([("data\\npc\\guard.mod", b"g")]))
            report = gate.check(self.manifest(["inside_map.hfl", "guard.mod"]), root)
            self.assertEqual(report["counts"]["missing"], 0)
            self.assertEqual(report["counts"]["pakLfsPointer"], 1)
            self.assertEqual(report["counts"]["present"], 1)
            row = report["pakLfsPointer"][0]
            self.assertEqual((row["name"], row["reason"]), ("inside_map.hfl", "pak-lfs-pointer"))
            self.assertEqual(row["paks"], ["Map.pak"])
            self.assertNotIn("inside_map.hfl", {r["name"] for r in report["present"]})
            self.assertEqual(report["conclusion"], "no conclusion")
            issues = gate.codequality_report(report)
            self.assertTrue(any("pak-lfs-pointer" in i["description"] for i in issues))
            self.assertFalse(any(i["severity"] == "major" for i in issues))

    def test_real_pak_entry_is_found(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "Effect.pak").write_bytes(build_playdh_pak([
                ("effect\\fire.dds", b"f"), ("effect\\smoke.tga", b"s"),
            ]))
            report = gate.check(self.manifest(["fire.dds", "smoke.tga"]), root)
            self.assertEqual(report["counts"]["present"], 2)
            self.assertEqual(report["counts"]["missing"], 0)
            self.assertEqual(report["counts"]["caseMismatch"], 0)
            self.assertEqual(report["conclusion"], "clean")

    def test_case_only_match_is_found_and_listed_minor(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "npc.pak").write_bytes(build_playdh_pak([("npc\\Guard_A.TGA", b"g")]))
            report = gate.check(self.manifest(["guard_a.tga"]), root)
            self.assertEqual(report["counts"]["present"], 1)
            self.assertEqual(report["counts"]["caseMismatch"], 1)
            row = report["caseMismatch"][0]
            self.assertEqual((row["reason"], row["severity"]), ("case-mismatch", "minor"))
            self.assertEqual(row["checkout"], ["Guard_A.TGA"])
            issues = gate.codequality_report(report)
            self.assertEqual([i["severity"] for i in issues if "case-mismatch" in i["description"]], ["minor"])

    def test_tga_vs_dds_is_extension_substituted_not_found_not_missing(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "Map.pak").write_bytes(build_playdh_pak([("map\\106_wall00.dds", b"w")]))
            report = gate.check(self.manifest(["106_wall00.tga"]), root)
            self.assertEqual(report["counts"]["present"], 0)
            self.assertEqual(report["counts"]["missing"], 0)
            self.assertEqual(report["counts"]["extensionSubstituted"], 1)
            row = report["extensionSubstituted"][0]
            self.assertEqual((row["name"], row["reason"]), ("106_wall00.tga", "extension-substituted"))
            self.assertEqual(row["checkout"], ["106_wall00.dds"])
            self.assertEqual(report["conclusion"], "warnings")
            issues = gate.codequality_report(report)
            self.assertTrue(any("extension-substituted" in i["description"] for i in issues))

    def test_unreadable_pak_makes_unresolved_refs_pak_index_unreadable(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            # Fits neither layout: count 5 at offset 4, garbage table.
            (root / "Broken.pak").write_bytes(struct.pack("<III", 1, 5, 0) + b"\xff" * 120)
            (root / "npc.pak").write_bytes(build_playdh_pak([("npc\\guard.mod", b"g")]))
            stderr = io.StringIO()
            with redirect_stderr(stderr):
                report = gate.check(self.manifest(["lost.mod", "guard.mod"]), root)
            self.assertIn("Broken.pak", stderr.getvalue())
            self.assertEqual(report["counts"]["missing"], 0)
            self.assertEqual(report["counts"]["present"], 1)
            self.assertEqual(report["counts"]["pakIndexUnreadable"], 1)
            row = report["pakIndexUnreadable"][0]
            self.assertEqual((row["name"], row["reason"], row["paks"]),
                             ("lost.mod", "pak-index-unreadable", ["Broken.pak"]))
            self.assertEqual(report["conclusion"], "no conclusion")
            issues = gate.codequality_report(report)
            self.assertTrue(any("pak-index-unreadable" in i["description"] for i in issues))

    def test_unreadable_pak_with_supplemental_is_counted_once_as_collision(self):
        """One unreadable pak lands in exactly one place: the collision row."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "Broken.pak").write_bytes(struct.pack("<III", 1, 5, 0) + b"\xff" * 120)
            (root / "zz_supplement.pak").write_bytes(build_playdh_pak([("npc\\guard.mod", b"g")]))
            stderr = io.StringIO()
            with redirect_stderr(stderr):
                report = gate.check(self.manifest(["lost.mod", "guard.mod"]), root)
            collisions = report["pakBasenameCollisions"]
            self.assertEqual([(r["other"], r["reason"]) for r in collisions],
                             [("Broken.pak", "other pak unreadable")])
            self.assertEqual(report["conclusion"], "collision")
            self.assertEqual(report["exitCode"], 1)
            self.assertEqual(report["counts"]["pakIndexUnreadable"], 0)
            self.assertEqual(report["pakIndexUnreadable"], [])
            self.assertEqual(report["counts"]["missing"], 0)
            places = sum("Broken.pak" in json.dumps(r) for r in collisions)
            places += sum("Broken.pak" in json.dumps(r) for r in report["pakIndexUnreadable"])
            places += sum("Broken.pak" in json.dumps(r.get("unreadablePaks", []))
                          for r in report["pakLfsPointer"])
            self.assertEqual(places, 1)
            lost = [r for r in report["noConclusion"] if r["name"] == "lost.mod"]
            self.assertEqual(len(lost), 1)
            self.assertIn("pakBasenameCollisions", lost[0]["reason"])

    def test_pointer_and_unreadable_paks_keep_both_visible(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "Map.pak").write_bytes(LFS_POINTER)
            (root / "Broken.pak").write_bytes(struct.pack("<III", 1, 5, 0) + b"\xff" * 120)
            with redirect_stderr(io.StringIO()):
                report = gate.check(self.manifest(["lost.mod"]), root)
            self.assertEqual(report["counts"]["missing"], 0)
            self.assertEqual(report["counts"]["pakLfsPointer"], 1)
            self.assertEqual(report["counts"]["pakIndexUnreadable"], 0)
            row = report["pakLfsPointer"][0]
            self.assertEqual((row["paks"], row["unreadablePaks"]), (["Map.pak"], ["Broken.pak"]))

    def test_absent_without_pointer_pak_is_missing(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "npc.pak").write_bytes(build_playdh_pak([("npc\\other.mod", b"o")]))
            report = gate.check(self.manifest(["gone.mod"]), root)
            self.assertEqual([r["name"] for r in report["missing"]], ["gone.mod"])
            self.assertEqual(report["counts"]["pakLfsPointer"], 0)
            self.assertEqual(report["counts"]["extensionSubstituted"], 0)

    def test_existing_count_fields_are_kept(self):
        with tempfile.TemporaryDirectory() as directory:
            report = gate.check(self.manifest([]), Path(directory))
            for key in ("present", "missing", "referencedButNoOriginalFile", "noConclusion"):
                self.assertIn(key, report["counts"])


class OriginalExistsScopeTests(unittest.TestCase):
    """originalExists searches every pak under --source, not only Map.pak.

    Archives are built with pak_writer (96-byte). The 92-byte fixture uses
    the same synthetic layout the reader already accepts; pak_writer does
    not emit that layout.
    """

    def _source(self, root: Path) -> Path:
        source = root / "client"
        (source / "packs").mkdir(parents=True)
        (source / "nested").mkdir()
        (source / "0.map").write_text("\n".join([
            "GX_MAP",
            "HEIGHT_FIELD",
            "terrain.hfl",
            "STATIC_MODEL",
            "town.stm",
            "GX_OBJECT",
            "house.mod",
            "absent.bin",
            "empty.tga",
        ]), encoding="latin-1")
        (source / "terrain.hfl").write_bytes(build_hfl(["only-in-character.tga"], 0))
        (source / "town.stm").write_bytes(build_stm([
            "LooseWall.tga",
            "CaseHit.TGA",
            "author.tga",
            "nested/deep.tga",
            "empty.tga",
            "shadow.tga",
        ]))
        (source / "LooseWall.tga").write_bytes(b"loose-wall")
        (source / "casehit.tga").write_bytes(b"case-hit")
        (source / "nested" / "Deep.TGA").write_bytes(b"deep")
        (source / "Map.pak").write_bytes(pak_writer.build_pak_bytes([
            ("decoy.tga", b"decoy"),
        ]))
        # Zero-length shadow.tga is first. find() returns that entry and
        # has_payload rejects it, so the later payload does not count.
        (source / "packs" / "Character.pak").write_bytes(pak_writer.build_pak_bytes([
            ("only-in-character.tga", b"chr"),
            ("House.MOD", b"mod"),
            ("author.dds", b"dds"),
            ("textures\\hidden.tga", b"hid"),
            ("LooseWall.tga", b"from-pak"),
            ("empty.tga", b""),
            ("shadow.tga", b""),
            ("shadow.tga", b"payload"),
        ]))
        return source

    def test_derive_indexes_every_pak_and_is_byte_identical(self):
        with tempfile.TemporaryDirectory() as directory:
            source = self._source(Path(directory))
            first = gate.derive(source, None)
            second = gate.derive(source, None)
            blob = manifest_text(first)
            self.assertEqual(blob, manifest_text(second))
            self.assertNotIn(str(source), blob)
            self.assertNotRegex(blob, r"[A-Za-z]:[\\/]")
            self.assertEqual(first["schema"], gate.MANIFEST_SCHEMA)
            self.assertEqual(first["derivation"]["status"], "complete")
            self.assertEqual(first["derivation"]["pak"]["path"], "")

            rows = {ref["name"]: ref for ref in first["references"]}
            self.assertEqual(rows["only-in-character.tga"]["originalHit"], "pak:packs/Character.pak")
            self.assertTrue(rows["only-in-character.tga"]["originalExists"])
            self.assertNotIn("Map.pak", rows["only-in-character.tga"]["originalHit"])
            self.assertEqual(rows["LooseWall.tga"]["originalHit"], "loose:LooseWall.tga")
            self.assertTrue(rows["LooseWall.tga"]["originalExists"])
            self.assertEqual(rows["CaseHit.TGA"]["originalHit"], "loose:casehit.tga")
            self.assertTrue(rows["CaseHit.TGA"]["originalExists"])
            self.assertEqual(rows["house.mod"]["originalHit"], "pak:packs/Character.pak")
            self.assertTrue(rows["house.mod"]["originalExists"])
            self.assertFalse(rows["absent.bin"]["originalExists"])
            self.assertEqual(rows["absent.bin"]["originalHit"], "")
            self.assertFalse(rows["author.tga"]["originalExists"])
            self.assertEqual(rows["author.tga"]["originalHit"], "")
            self.assertEqual(rows["nested/deep.tga"]["originalHit"], "loose:nested/Deep.TGA")
            self.assertTrue(rows["nested/deep.tga"]["originalExists"])
            self.assertFalse(rows["empty.tga"]["originalExists"])
            self.assertEqual(rows["empty.tga"]["via"], "0.map")
            self.assertFalse(rows["shadow.tga"]["originalExists"])

            index = gate._build_exists_index(source)
            self.assertEqual([pack.rel for pack in index.packs], ["Map.pak", "packs/Character.pak"])
            self.assertEqual(index.locate("decoy.tga").label, "pak:Map.pak")
            self.assertIsNone(index.locate("hidden.tga"))
            self.assertIsNone(index.locate("other/hidden.tga"))
            self.assertEqual(index.locate("textures/hidden.tga").label, "pak:packs/Character.pak")
            self.assertEqual(
                index.locate("dir\\only-in-character.tga").label, "pak:packs/Character.pak")
            self.assertIsNone(index.locate("Deep.TGA"))
            self.assertIsNone(index.locate(""))
            self.assertIsNone(index.locate("author.tga"))
            self.assertEqual(index.locate("author.dds").label, "pak:packs/Character.pak")

            absorbed: list[dict] = []
            gate._absorb(
                absorbed, [],
                [{"name": "house.mod", "section": "GX_OBJECT"}],
                "", index)
            self.assertEqual(absorbed[0]["via"], "")
            self.assertTrue(absorbed[0]["originalExists"])
            self.assertEqual(absorbed[0]["originalHit"], "pak:packs/Character.pak")

            with_pak_a = gate.derive(source, source / "Map.pak")
            with_pak_b = gate.derive(source, source / "Map.pak")
            self.assertEqual(manifest_text(with_pak_a), manifest_text(with_pak_b))
            self.assertEqual(with_pak_a["derivation"]["pak"]["entriesReadToEof"], 1)
            for ref in with_pak_a["references"]:
                self.assertNotIn(str(source), ref["originalHit"])

            empty = source / "empty-checkout"
            empty.mkdir()
            report = gate.check(first, empty)
            empty_via = [row for row in report["referencedButNoOriginalFile"] if row["via"] == ""]
            self.assertEqual(len(empty_via), len(gate.KNOWN_NO_ORIGINAL))
            self.assertEqual({row["name"] for row in empty_via}, set(gate.KNOWN_NO_ORIGINAL))

            hand = {
                "schema": gate.MANIFEST_SCHEMA,
                "derivation": {"status": "complete", "unparsed": []},
                "references": [{
                    "name": "only-in-character.tga",
                    "via": "",
                    "section": "hfl-used-texture",
                    "originalExists": True,
                }],
                "knownReferencedNoOriginalFile": [],
            }
            present = gate.check(hand, source)
            kept = [row for row in present["present"] if row["name"] == "only-in-character.tga"]
            self.assertEqual(kept, [{"name": "only-in-character.tga", "via": ""}])

    def test_mount_order_is_tier_then_filename(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory)
            (source / "A.pak").write_bytes(pak_writer.build_pak_bytes([("shared.tga", b"A")]))
            (source / "a.pak").write_bytes(pak_writer.build_pak_bytes([("shared.tga", b"a")]))
            (source / "0.map").write_text("GX_MAP\nGX_OBJECT\nshared.tga\n", encoding="latin-1")
            manifest = gate.derive(source, None)
            shared = next(ref for ref in manifest["references"] if ref["name"] == "shared.tga")
            self.assertEqual(shared["originalHit"], "pak:A.pak")
            self.assertTrue(shared["originalExists"])

        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory)
            (source / "a_original.pak").write_bytes(pak_writer.build_pak_bytes([
                ("tier.tga", b"96"),
            ]))
            (source / "z_playdh.pak").write_bytes(build_playdh_pak([("tier.tga", b"92")]))
            (source / "0.map").write_text("GX_MAP\nGX_OBJECT\ntier.tga\n", encoding="latin-1")
            manifest = gate.derive(source, None)
            tier = next(ref for ref in manifest["references"] if ref["name"] == "tier.tga")
            self.assertEqual(tier["originalHit"], "pak:z_playdh.pak")
            self.assertTrue(tier["originalExists"])

    def test_client_root_relative_loose_and_outside_miss_and_maps_stay_put(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            client = root / "client"
            server = root / "server"
            (client / "Resource" / "Map").mkdir(parents=True)
            server.mkdir()
            (client / "0.map").write_text("\n".join([
                "GX_MAP",
                "GX_OBJECT",
                "Resource/Map/wall.tga",
                "only-server.tga",
                "server/hero.mod",
            ]), encoding="latin-1")
            (client / "Resource" / "Map" / "wall.tga").write_bytes(b"wall")
            (server / "hero.mod").write_bytes(b"mod")
            (server / "Server.pak").write_bytes(pak_writer.build_pak_bytes([
                ("only-server.tga", b"nope"),
                ("sneak.map", b"GX_MAP\nHEIGHT_FIELD\nsneak.hfl\n"),
            ]))
            (client / "Other.pak").write_bytes(pak_writer.build_pak_bytes([
                ("sneak.map", b"GX_MAP\nHEIGHT_FIELD\nsneak.hfl\n"),
            ]))
            manifest = gate.derive(root, None, client_root=client)
            rows = {ref["name"]: ref for ref in manifest["references"]}
            self.assertEqual(rows["Resource/Map/wall.tga"]["originalHit"], "loose:Resource/Map/wall.tga")
            self.assertTrue(rows["Resource/Map/wall.tga"]["originalExists"])
            self.assertFalse(rows["only-server.tga"]["originalExists"])
            self.assertEqual(rows["only-server.tga"]["originalHit"], "")
            self.assertFalse(rows["server/hero.mod"]["originalExists"])
            self.assertEqual(manifest["derivation"]["parsedMapCount"], 1)
            self.assertNotIn("sneak.hfl", rows)
            self.assertNotIn("sneak.map", {item["name"] for item in manifest["derivation"]["unparsed"]})
            parse_index, _info = gate._build_parse_index(root, None)
            maps = {name for name in parse_index.by_base if name.endswith(".map")}
            self.assertEqual(maps, {"0.map"})
            exists = gate._build_exists_index(client)
            self.assertEqual([pack.rel for pack in exists.packs], ["Other.pak"])
            self.assertIsNone(exists.locate("only-server.tga"))

    def _derive_cli(self, client: Path, output: Path):
        stdout = io.StringIO()
        stderr = io.StringIO()
        with redirect_stdout(stdout), redirect_stderr(stderr):
            code = gate.main([
                "derive",
                "--source", str(client),
                "--client-root", str(client),
                "--output", str(output),
            ])
        return code, stdout.getvalue(), stderr.getvalue()

    def test_derive_fails_when_known_dds_is_a_loose_file(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            client = root / "client"
            client.mkdir()
            (client / "0.map").write_text("GX_MAP\nGX_OBJECT\nwall.tga\n", encoding="latin-1")
            (client / "tile_17_mou_a_03.dds").write_bytes(b"synthetic-dds")
            output = root / "out.json"
            output.write_text("sentinel", encoding="utf-8")
            code, stdout, stderr = self._derive_cli(client, output)
            self.assertEqual(code, 1)
            self.assertEqual(stdout, "")
            self.assertEqual(output.read_text(encoding="utf-8"), "sentinel")
            self.assertEqual(stderr.splitlines(), [
                "ERROR: known-no-original name is present; manifest not written",
                "tile_17_mou_a_03.dds -> loose:tile_17_mou_a_03.dds",
            ])
            self.assertNotIn(str(client), stderr)
            with self.assertRaises(gate.KnownNoOriginalFound) as caught:
                gate.derive(client, None, client_root=client)
            self.assertEqual(
                caught.exception.hits,
                [("tile_17_mou_a_03.dds", "loose:tile_17_mou_a_03.dds")])

    def test_derive_fails_when_known_tga_is_inside_a_synthetic_pak(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            client = root / "client"
            client.mkdir()
            (client / "0.map").write_text("GX_MAP\nGX_OBJECT\nwall.tga\n", encoding="latin-1")
            (client / "tiles.pak").write_bytes(pak_writer.build_pak_bytes([
                ("tile_17_mou_a_03.tga", b"synthetic-tga"),
            ]))
            output = root / "out.json"
            output.write_text("sentinel", encoding="utf-8")
            code, stdout, stderr = self._derive_cli(client, output)
            self.assertEqual(code, 1)
            self.assertEqual(stdout, "")
            self.assertEqual(output.read_text(encoding="utf-8"), "sentinel")
            self.assertEqual(stderr.splitlines(), [
                "ERROR: known-no-original name is present; manifest not written",
                "tile_17_mou_a_03.tga -> pak:tiles.pak",
            ])
            self.assertNotIn(str(client), stderr)


class TrimAndCompareTests(unittest.TestCase):
    def _pair(self, *, via="1.map", exists=True):
        return {
            "schema": gate.MANIFEST_SCHEMA,
            "derivation": {
                "status": "complete",
                "parsedMapCount": 1,
                "decodedMapsIgnored": False,
            },
            "references": [
                {
                    "name": "a.tga",
                    "via": via,
                    "section": "GX_OBJECT",
                    "originalExists": exists,
                    "originalHit": "loose:a.tga" if exists else "",
                },
                {
                    "name": "tile_17_mou_a_01.tga",
                    "via": "17.map",
                    "section": "hfl-used-texture",
                    "originalExists": False,
                    "originalHit": "",
                },
            ],
            "knownReferencedNoOriginalFile": [],
        }

    def test_trim_keeps_local_decoder_and_drops_paths(self):
        document = self._pair()
        document["derivation"]["pak"] = {"path": r"D:\MX\client\Package\3DData\Map.pak"}
        document["derivation"]["localDecoder"] = {
            "version": "synthetic",
            "sourcePakSha256": "ab" * 32,
            "decodedMaps": [{"name": "1.map", "sha256": "cd" * 32, "keySource": "table-test"}],
            "keySourceCounts": {"table-test": 1},
            "loadedFrom": r"D:\secret\local_decoder.py",
        }
        document["derivation"]["decoderPath"] = "/opt/check153/decoder.py"
        trimmed = gate.trim_manifest(document)
        blob = json.dumps(trimmed)
        self.assertIn("localDecoder", blob)
        self.assertNotIn("local_decoder.py", blob)
        self.assertNotIn("decoder.py", blob)
        self.assertNotIn("secret", blob)
        info = trimmed["derivation"]["localDecoder"]
        self.assertEqual(set(info), {"version", "sourcePakSha256", "decodedMaps", "keySourceCounts"})
        self.assertEqual(info["decodedMaps"], [
            {"name": "1.map", "sha256": "cd" * 32, "keySource": "table-test"},
        ])
        self.assertEqual(trimmed["derivation"]["pak"]["path"], r"D:\MX\client\Package\3DData\Map.pak")
        self.assertEqual(manifest_text(trimmed), manifest_text(gate.trim_manifest(trimmed)))

    def test_trim_of_derive_matches_path_free_baseline(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            pak = root / "Map.pak"
            pak.write_bytes(build_original_pak([
                ("1.map", synthetic_xor_map(FAKE_PLAIN_MAP)),
            ]))
            plugin = root / "outside" / "fake_decoder.py"
            plugin.parent.mkdir()
            plugin.write_text(SOURCED_PLUGIN_SOURCE, encoding="utf-8")
            leaked = gate.derive(None, pak, local_decoder=gate.load_local_decoder(plugin))
            info = leaked["derivation"]["localDecoder"]
            self.assertEqual(set(info), {"version", "sourcePakSha256", "decodedMaps", "keySourceCounts"})
            baseline = json.loads(json.dumps(leaked))
            leaked["derivation"]["decoderPath"] = r"D:\secret\local_decoder.py"
            leaked["derivation"]["localDecoder"]["loadedFrom"] = "/opt/check153/decoder.py"
            leaked["pluginFile"] = r"C:\tools\plugin.py"
            trimmed = gate.trim_manifest(leaked)
            report = gate.compare_manifest_report(baseline, trimmed)
            self.assertIn("OTHER_FIELD_DIFFS 0\n", report)
            self.assertIn("DECODED_MAPS_EQUAL\n", report)
            self.assertNotIn("FALSE_BY_HIT", report)
            self.assertEqual(
                trimmed["derivation"]["localDecoder"]["decodedMaps"],
                baseline["derivation"]["localDecoder"]["decodedMaps"],
            )
            self.assertNotIn(".py", json.dumps(trimmed))

    def test_compare_reports_hit_fields_separately_from_other_diffs(self):
        old = self._pair(exists=True)
        new = self._pair(exists=False)
        new["references"][0]["originalHit"] = ""
        text = gate.compare_manifest_report(old, new)
        self.assertIn("OTHER_FIELD_DIFFS 0\n", text)
        self.assertIn("PARSED_MAP_COUNT old=1 new=1\n", text)
        self.assertIn("DECODED_MAPS_EQUAL\n", text)
        self.assertIn("FALSE_BY_EXT tga 2\n", text)
        self.assertNotIn("FALSE_BY_HIT", text)
        self.assertIn("FALSE_BY_VIA 1.map 1\n", text)
        self.assertIn("FALSE_BY_VIA 17.map 1\n", text)
        self.assertIn("TILE tile_17_mou_a_01.tga false\n", text)
        self.assertIn("TILE tile_17_mou_a_09.tga absent\n", text)
        self.assertNotIn("sha256", text)
        self.assertNotIn("payload", text)

        changed = self._pair(via="other.map", exists=False)
        diff = gate.compare_manifest_report(old, changed)
        self.assertIn("OTHER_FIELD references[0].via\n", diff)
        self.assertNotIn("OTHER_FIELD_DIFFS 0\n", diff)

        old_maps = self._pair()
        new_maps = self._pair()
        old_maps["derivation"]["localDecoder"] = {"decodedMaps": [{"name": "1.map"}, {"name": "2.map"}]}
        new_maps["derivation"]["localDecoder"] = {"decodedMaps": [{"name": "2.map"}, {"name": "3.map"}]}
        listed = gate.compare_manifest_report(old_maps, new_maps)
        self.assertIn("DECODED_MAPS_ONLY_OLD 1.map\n", listed)
        self.assertIn("DECODED_MAPS_ONLY_NEW 3.map\n", listed)

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            old_path = root / "old.json"
            new_path = root / "new.json"
            trimmed_path = root / "trimmed.json"
            old_path.write_text(manifest_text(old), encoding="utf-8")
            new_path.write_text(manifest_text(new), encoding="utf-8")
            stdout = io.StringIO()
            with redirect_stdout(stdout):
                code = gate.main(["compare-manifests", str(old_path), str(new_path)])
            self.assertEqual(code, 0)
            self.assertIn("OTHER_FIELD_DIFFS 0\n", stdout.getvalue())
            with redirect_stdout(io.StringIO()):
                self.assertEqual(gate.main(["trim", "--input", str(new_path), "--output", str(trimmed_path)]), 0)
            saved = json.loads(trimmed_path.read_text(encoding="utf-8"))
            self.assertEqual(saved["references"][0]["name"], "a.tga")


if __name__ == "__main__":
    unittest.main()
