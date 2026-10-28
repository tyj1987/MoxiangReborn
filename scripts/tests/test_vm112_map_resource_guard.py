from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
WRAPPER = ROOT / "deploy/scripts/start_current_vm112.ps1"

class Vm112MapResourceGuardTests(unittest.TestCase):
    def test_start_wrapper_checks_all_configured_map_ai_groups_before_launch(self):
        source = WRAPPER.read_text(encoding="utf-8")
        self.assertIn("$requiredMapNumbers = @($MapNumber, 12, 2)", source)
        self.assertIn("$requiredMapNumbers += 17", source)
        self.assertIn("Monster_$mapSuffix.bin", source)
        self.assertIn("missing server AIGroup resource for map", source)
        self.assertLess(source.index("missing server AIGroup resource for map"), source.index("Start-Process"))

if __name__ == "__main__":
    unittest.main()
