import importlib.util
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts/package-original-vm112-current.py"


spec = importlib.util.spec_from_file_location("package_original_vm112", SCRIPT)
assert spec and spec.loader
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class OriginalPackageResourceGateTests(unittest.TestCase):
    def test_map10_stage_requires_exact_map10_map12_and_map2_groups(self):
        rows = module.validate_server_ai_groups(10, False)
        self.assertEqual([row["map"] for row in rows], [10, 12, 2])
        self.assertTrue(all(row["bytes"] > 0 for row in rows))

    def test_map203_stage_rejects_missing_exact_group(self):
        with self.assertRaisesRegex(RuntimeError, r"map 203.*Monster_203\.bin"):
            module.validate_server_ai_groups(203, False)


if __name__ == "__main__":
    unittest.main()
