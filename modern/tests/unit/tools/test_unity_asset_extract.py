import json
from pathlib import Path
import tempfile
import unittest

from test_unity_resource_audit import audit, create_profile, COMMIT
from unity_asset_extract import extract


class UnityAssetExtractTests(unittest.TestCase):
    def test_requires_explicit_development_flag_and_verifies_container(self):
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory)
            root = base / "source"
            root.mkdir()
            create_profile(root)
            manifest = audit.build_manifest(root, "fixture", COMMIT)
            path = base / "baseline.json.gz"
            audit._write_manifest(path, root, manifest)
            selected = next(s for a in manifest["assets"] for s in a["sources"] if s["sourceType"] == "pak")
            output = base / "output.bin"
            with self.assertRaises(audit.AuditError):
                extract(path, root, selected["sourceId"], output)
            result = extract(path, root, selected["sourceId"], output, True)
            self.assertFalse(result["releaseReady"])
            self.assertEqual(audit._sha256_file(output), selected["sha256"])
            with (root / selected["container"]).open("ab") as stream:
                stream.write(b"changed")
            with self.assertRaises(audit.AuditError):
                extract(path, root, selected["sourceId"], base / "second.bin", True)
            self.assertFalse((base / "second.bin").exists())

    def test_refuses_source_overwrite_and_unknown_source(self):
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory)
            root = base / "source"
            root.mkdir()
            create_profile(root)
            manifest = audit.build_manifest(root, "fixture", COMMIT)
            path = base / "baseline.json"
            audit._write_manifest(path, root, manifest)
            source = manifest["assets"][0]["sources"][0]["sourceId"]
            with self.assertRaises(audit.AuditError):
                extract(path, root, source, root / "new.bin", True)
            with self.assertRaises(audit.AuditError):
                extract(path, root, "not-in-manifest", base / "output.bin", True)


if __name__ == "__main__":
    unittest.main(verbosity=2)
