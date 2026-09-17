"""Asset integrity gates: no engine, external resources or third-party modules."""
from __future__ import annotations
import json
import pathlib
import sys
import tempfile
import unittest
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / "tools" / "reart"))
from manifest import AssetError, BASELINE, PROFILE, PROJECT, checked_path, digest, read_json, relative_name, validate

class ManifestTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = pathlib.Path(self.temp.name)
        (self.root / "sample").mkdir()
        (self.root / "sample" / "mesh.mod").write_bytes(b"authored-test-fixture")
        p = self.root / "sample" / "mesh.mod"
        self.data = {"schema": 1, "project": PROJECT, "profile": PROFILE, "baseline_sha": BASELINE, "legacy_fallback": False,
            "coverage": {"active_gameplay_inventory_complete": False}, "required_assets": ["sample/weapon"],
            "assets": [{"asset_id": "sample/weapon", "stage": "authored", "technical_sample": True, "gameplay_binding": {"item_id": None},
                        "provenance": {"review": "pending", "method": "original_parametric"}, "dependencies": [], "acceptance_evidence": [],
                        "files": [{"path": "sample/mesh.mod", "sha256": digest(p), "bytes": p.stat().st_size}]}]}
        self.save()
    def save(self):
        (self.root / "manifest.json").write_text(json.dumps(self.data), encoding="utf-8")
    def reject(self, change, pattern=None):
        change(); self.save()
        with self.assertRaisesRegex(AssetError, pattern or "."): validate(self.root)
    def test_valid_development_pack_is_not_release(self):
        result = validate(self.root)
        self.assertEqual(result["assets"], 1); self.assertFalse(result["release_eligible"]); self.assertEqual(result["integrity"], "pass")
    def test_release_blocks_sample(self):
        with self.assertRaisesRegex(AssetError, "Release blocked"): validate(self.root, release=True)
    def test_no_other_project(self):
        self.reject(lambda: self.data.update(project="unrelated/project"), "Wrong project")
    def test_no_other_profile(self):
        self.reject(lambda: self.data.update(profile="playdh-current"), "Wrong project")
    def test_no_silent_fallback(self):
        self.reject(lambda: self.data.update(legacy_fallback=True), "fallback")
    def test_missing_fallback_policy(self):
        self.reject(lambda: self.data.pop("legacy_fallback"), "fallback")
    def test_baseline_change_requires_reaudit(self):
        self.reject(lambda: self.data.update(baseline_sha="0" * 40), "Baseline")
    def test_empty_ledger(self):
        self.reject(lambda: self.data.update(assets=[]), "ledger")
    def test_corruption(self):
        (self.root / "sample" / "mesh.mod").write_bytes(b"modified-artifact")
        with self.assertRaisesRegex(AssetError, "mismatch"): validate(self.root)
    def test_missing_file(self):
        (self.root / "sample" / "mesh.mod").unlink()
        with self.assertRaisesRegex(AssetError, "Missing asset"): validate(self.root)
    def test_duplicate_id(self):
        self.reject(lambda: self.data["assets"].append(self.data["assets"][0]), "duplicate asset ID")
    def test_invalid_stage(self):
        self.reject(lambda: self.data["assets"][0].update(stage="looks-finished"), "stage")
    def test_unknown_dependency(self):
        self.reject(lambda: self.data["assets"][0].update(dependencies=["missing/asset"]), "Missing dependency")
    def test_dependency_cycle(self):
        self.reject(lambda: self.data["assets"][0].update(dependencies=["sample/weapon"]), "cycle")
    def test_duplicate_dependency(self):
        self.reject(lambda: self.data["assets"][0].update(dependencies=["sample/weapon"] * 2), "Duplicate dependency")
    def test_missing_required(self):
        self.reject(lambda: self.data.update(required_assets=["missing/asset"]), "required")
    def test_path_traversal(self):
        self.reject(lambda: self.data["assets"][0]["files"][0].update(path="../other.mod"), "Non-canonical")
    def test_windows_paths(self):
        for name in ["C:/file", "\\\\server\\file", "..\\outside", "/absolute", "a//b", "a/./b", "a/../b", "a:b", "a\x00b"]:
            with self.subTest(name=name), self.assertRaises(AssetError): relative_name(name)
    def test_symlink_file(self):
        source = self.root / "sample" / "mesh.mod"; link = self.root / "sample" / "link.mod"
        try: link.symlink_to(source)
        except OSError: self.skipTest("Symlinks unavailable on this host")
        self.reject(lambda: self.data["assets"][0]["files"][0].update(path="sample/link.mod"), "Symlink")
    def test_symlink_directory(self):
        link = self.root / "alias"
        try: link.symlink_to(self.root / "sample", target_is_directory=True)
        except OSError: self.skipTest("Symlinks unavailable on this host")
        with self.assertRaisesRegex(AssetError, "Symlink"): checked_path(self.root, "alias/mesh.mod")
    def test_duplicate_json_keys(self):
        (self.root / "bad.json").write_text('{"a":1,"a":2}', encoding="utf-8")
        with self.assertRaisesRegex(AssetError, "Duplicate JSON"): read_json(self.root / "bad.json")
    def test_non_finite_json(self):
        for word in ["NaN", "Infinity", "-Infinity"]:
            (self.root / "bad.json").write_text('{"a":' + word + '}', encoding="utf-8")
            with self.assertRaisesRegex(AssetError, "numeric"): read_json(self.root / "bad.json")
    def test_bad_checksum(self):
        self.reject(lambda: self.data["assets"][0]["files"][0].update(sha256=""), "checksum")
    def test_missing_binding(self):
        self.reject(lambda: self.data["assets"][0].pop("gameplay_binding"), "binding")
    def test_missing_provenance(self):
        self.reject(lambda: self.data["assets"][0].pop("provenance"), "provenance")
    def test_unbacked_evidence(self):
        self.reject(lambda: self.data["assets"][0].update(acceptance_evidence=[{"kind": "format", "result": "pass", "path": "nonexistent.json", "sha256": "0" * 64}]), "Missing asset")
    def test_changed_evidence(self):
        (self.root / "result.json").write_text('{"result":"pass"}', encoding="utf-8")
        self.reject(lambda: self.data["assets"][0].update(acceptance_evidence=[{"kind": "format", "result": "pass", "path": "result.json", "sha256": "0" * 64}]), "Evidence checksum")
    def test_case_collision(self):
        p = self.root / "sample" / "MESH.mod"
        if not p.exists(): p.write_bytes(b"another-file")
        self.data["assets"][0]["files"].append({"path": "sample/MESH.mod", "sha256": digest(p), "bytes": p.stat().st_size})
        self.save()
        with self.assertRaisesRegex(AssetError, "collision"): validate(self.root)

if __name__ == "__main__": unittest.main()
