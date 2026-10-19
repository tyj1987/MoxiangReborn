from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
TRANSFER = ROOT / "scripts/transfer-original-vm112-via-pve.py"


class Vm112TransferResourceGuardTests(unittest.TestCase):
    def test_transfer_verifies_manifest_required_ai_groups_after_mirror(self):
        source = TRANSFER.read_text(encoding="utf-8")
        self.assertIn("$manifest.requiredMapNumbers", source)
        self.assertIn("required server AIGroup missing after resource mirror", source)
        self.assertIn("serverAiGroups=$ai", source)
        self.assertIn("$required=@(); if($null -ne $manifest.requiredMapNumbers)", source)
        self.assertIn("timeout=1800", source)


if __name__ == "__main__":
    unittest.main()
