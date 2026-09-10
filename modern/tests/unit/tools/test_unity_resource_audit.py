#!/usr/bin/env python3
"""Independent behavior tests for unity_resource_audit.py."""

from __future__ import annotations

import copy
import hashlib
import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path

THIS_DIR = Path(__file__).resolve().parent
TOOLS_DIR = THIS_DIR.parents[2] / "tools"
if str(TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(TOOLS_DIR))

import gen_hfl_placeholders  # noqa: E402
import unity_resource_audit as audit  # noqa: E402


COMMIT = "1" * 40


def make_pak(path: Path, entries: list[tuple[str, bytes]]) -> None:
    with path.open("wb") as stream:
        stream.write(struct.pack("<III", 1, len(entries), 0))
        stream.write(b"\0" * 80)
        for name, data in entries:
            encoded = name.encode("latin-1")
            total = 32 + len(encoded) + 1 + len(data)
            stream.write(struct.pack("<IIIIIIII", total, len(data), len(encoded), 0, 0, 0, 0, 0))
            stream.write(encoded + b"\0" + data)


def make_hfl_template() -> bytes:
    desc = bytearray(108)
    struct.pack_into("<f", desc, 16, 512.0)
    struct.pack_into("<I", desc, 32, 1)
    struct.pack_into("<I", desc, 44, 0)
    struct.pack_into("<II", desc, 52, 4, 4)
    struct.pack_into("<ff", desc, 60, 1000.0, 1000.0)
    return struct.pack("<I", 1) + bytes(desc) + struct.pack("<16f", *([1.0] * 16)) + struct.pack("<I", 0)


def create_profile(root: Path, map_entries: list[tuple[str, bytes]] | None = None) -> None:
    entries_by_pack = {name: [(f"{Path(name).stem}/only.bin", name.encode("ascii"))]
                       for name in audit.REQUIRED_PAKS}
    if map_entries is not None:
        entries_by_pack["Map.pak"] = map_entries
    for name in audit.REQUIRED_PAKS:
        make_pak(root / name, entries_by_pack[name])


class UnityResourceAuditTests(unittest.TestCase):
    def test_content_fingerprint_detects_placeholder_and_rejects_name_only(self):
        template = make_hfl_template()
        generated = gen_hfl_placeholders.synthesize_placeholder(template, 42)
        self.assertEqual(audit.classify_hfl("Map/42.hfl", generated)["kind"], "generated-placeholder")
        self.assertEqual(audit.classify_hfl("Map/43.hfl", generated)["kind"], "unknown")
        self.assertEqual(audit.classify_hfl("Map/42.hfl", template)["kind"], "unknown")

    def test_manifest_is_deterministic_and_conflict_has_no_implicit_winner(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "Resource" / "Map").mkdir(parents=True)
            (root / "Resource" / "Map" / "shared.bin").write_bytes(b"loose")
            create_profile(root, [("shared.bin", b"packed")])
            first = audit.build_manifest(root, "fixture", COMMIT)
            second = audit.build_manifest(root, "fixture", COMMIT)
            self.assertEqual(first, second)
            conflict = next(item for item in first["assets"] if item["logicalPath"] == "Map/shared.bin")
            self.assertEqual(conflict["relation"], "conflict")
            self.assertTrue(conflict["requiresSelection"])
            self.assertIsNone(conflict["selectedSourceId"])
            self.assertEqual({item["sourceType"] for item in conflict["sources"]}, {"loose", "pak"})

    def test_explicit_source_selection_closes_ambiguity(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "Resource" / "Map").mkdir(parents=True)
            (root / "Resource" / "Map" / "shared.bin").write_bytes(b"same")
            create_profile(root, [("shared.bin", b"same")])
            baseline = audit.build_manifest(root, "fixture", COMMIT)
            duplicate = next(item for item in baseline["assets"] if item["logicalPath"] == "Map/shared.bin")
            chosen = duplicate["sources"][0]["sourceId"]
            selection = root / "selection.fixture.json"
            selection.write_text(json.dumps({
                "schemaVersion": 1,
                "profileId": "fixture",
                "selections": {duplicate["stableId"]: chosen},
            }), encoding="utf-8")
            resolved = audit.build_manifest(root, "fixture", COMMIT, selection)
            asset = next(item for item in resolved["assets"] if item["stableId"] == duplicate["stableId"])
            self.assertEqual(asset["selectedSourceId"], chosen)
            self.assertFalse(any(item["code"] == "source-selection"
                                 for item in resolved["releaseValidation"]["blockers"]))

    def test_archive_parent_path_is_reported_and_release_fails_closed(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            create_profile(root, [("../escape.bin", b"bad")])
            manifest = audit.build_manifest(root, "fixture", COMMIT)
            self.assertEqual(len(manifest["unsafePaths"]), 1)
            self.assertEqual(manifest["unsafePaths"][0]["path"], "../escape.bin")
            self.assertTrue(any(item["code"] == "unsafe-path"
                                for item in manifest["releaseValidation"]["blockers"]))

    def test_placeholder_hfl_is_a_release_blocker_even_when_it_is_the_only_source(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "Resource" / "Map").mkdir(parents=True)
            placeholder = gen_hfl_placeholders.synthesize_placeholder(make_hfl_template(), 7)
            (root / "Resource" / "Map" / "7.hfl").write_bytes(placeholder)
            create_profile(root)
            manifest = audit.build_manifest(root, "fixture", COMMIT)
            blockers = manifest["releaseValidation"]["blockers"]
            self.assertTrue(any(item["code"] == "hfl-provenance" and
                                item["detail"] == "generated-placeholder" for item in blockers))

    def test_inventory_digest_detects_manifest_tampering(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "plain.bin").write_bytes(b"original")
            create_profile(root)
            manifest = audit.build_manifest(root, "fixture", COMMIT)
            tampered = copy.deepcopy(manifest)
            tampered["assets"][0]["sources"][0]["sha256"] = hashlib.sha256(b"tampered").hexdigest()
            result = audit.validate_manifest_document(tampered, "integrity")
            self.assertTrue(any(item["code"] == "inventory-digest" for item in result["blockers"]))

    def test_attestation_cannot_relabel_content_proven_placeholder(self):
        source = {"sourceId": "fixture", "sha256": "a" * 64,
                  "provenance": {"kind": "generated-placeholder"}}
        issues = []
        audit._apply_attestation(source, {"classification": "verified-original",
            "sha256": "a" * 64, "evidence": "claimed review"}, issues)
        self.assertEqual(source["provenance"]["kind"], "generated-placeholder")
        self.assertEqual(issues[0]["code"], "placeholder-attestation")

    def test_nonexistent_selected_source_cannot_pass_release(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            create_profile(root)
            manifest = audit.build_manifest(root, "fixture", COMMIT)
            manifest["assets"][0]["selectedSourceId"] = "nonexistent"
            result = audit.validate_manifest_document(manifest, "release")
            self.assertTrue(any(item["code"] == "source-selection" for item in result["blockers"]))

    def test_refuses_to_write_manifest_inside_resource_root(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            create_profile(root)
            manifest = audit.build_manifest(root, "fixture", COMMIT)
            with self.assertRaises(audit.AuditError):
                audit._write_manifest(root / "manifest.json", root, manifest)
            self.assertFalse((root / "manifest.json").exists())

    def test_provenance_changes_invalidate_document_checksum(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            create_profile(root)
            manifest = audit.build_manifest(root, "fixture", COMMIT)
            manifest["assets"][0]["sources"][0]["provenance"] = {"kind": "verified-original"}
            result = audit.validate_manifest_document(manifest, "integrity")
            self.assertTrue(any(item["code"] == "document-digest" for item in result["blockers"]))

    def test_rehash_detects_source_changed_after_audit(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            create_profile(root)
            (root / "plain.bin").write_bytes(b"before")
            manifest = audit.build_manifest(root, "fixture", COMMIT)
            self.assertTrue(audit.verify_source_bytes(manifest, root)["passed"])
            (root / "plain.bin").write_bytes(b"after!")
            result = audit.verify_source_bytes(manifest, root)
            self.assertFalse(result["passed"])
            self.assertTrue(any(item["subject"] == "plain.bin" for item in result["blockers"]))


if __name__ == "__main__":
    unittest.main(verbosity=2)
