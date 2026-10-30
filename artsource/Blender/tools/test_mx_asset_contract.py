from pathlib import Path
import tempfile
import unittest

from mx_asset_contract import (
    build_receipt,
    sha256_file,
    validate_asset_id,
    validate_category,
    validate_object_names,
    write_receipt,
)


class AssetContractTests(unittest.TestCase):
    def test_asset_id_contract(self):
        self.assertEqual([], validate_asset_id("MX_ENV_WUGUAN_GATE_001"))
        self.assertTrue(validate_asset_id("bad id"))

    def test_category_contract(self):
        self.assertEqual([], validate_category("Environment"))
        self.assertTrue(validate_category("Misc"))

    def test_duplicate_object_name_is_error(self):
        issues = validate_object_names(["MX_A", "MX_A"])
        self.assertTrue(any(i.code == "object.duplicate_name" and i.severity == "error" for i in issues))

    def test_receipt_hashes_source_and_export(self):
        with tempfile.TemporaryDirectory() as root:
            root = Path(root)
            source = root / "source.blend"
            export = root / "MX_ENV_TEST_001.fbx"
            receipt_path = root / "MX_ENV_TEST_001.fbx.receipt.json"
            source.write_bytes(b"source")
            export.write_bytes(b"export")
            receipt = build_receipt(
                asset_id="MX_ENV_TEST_001",
                category="Environment",
                source_file=source,
                export_file=export,
                blender_version="test",
                scale_meters=1.0,
                forward_axis="-Z",
                up_axis="Y",
                objects=["MX_ENV_TEST_001"],
            )
            self.assertEqual(sha256_file(source), receipt.source_sha256)
            self.assertEqual(sha256_file(export), receipt.export_sha256)
            write_receipt(receipt_path, receipt)
            self.assertTrue(receipt_path.is_file())
            self.assertIn("moxiang.blender-export-receipt.v1", receipt_path.read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
