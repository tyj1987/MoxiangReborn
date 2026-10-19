from __future__ import annotations

import importlib.util
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "modern" / "tools" / "audit_map_server_resources.py"
spec = importlib.util.spec_from_file_location("audit_map_server_resources", SCRIPT)
assert spec and spec.loader
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class MapServerResourceAuditTests(unittest.TestCase):
    def test_ordinary_maps_use_two_digit_name(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            path = root / "Resource" / "Server" / "Monster_10.bin"
            path.parent.mkdir(parents=True)
            path.write_bytes(b"profile")
            row = module.audit(10, root)
            self.assertTrue(row["exists"])
            self.assertEqual(row["resource"], "Resource/Server/Monster_10.bin")

    def test_special_maps_use_full_number_and_missing_is_false(self):
        with tempfile.TemporaryDirectory() as temp:
            row = module.audit(203, Path(temp))
            self.assertFalse(row["exists"])
            self.assertEqual(row["resource"], "Resource/Server/Monster_203.bin")


if __name__ == "__main__":
    unittest.main()
