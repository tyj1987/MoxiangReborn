"""The VM112 writer guard must reject a bad DB reply before process launch."""

import os
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
GUARD = ROOT / "deploy/scripts/assert_original_writer_contract.ps1"
WRAPPER = ROOT / "deploy/scripts/start_original_vm112.ps1"


class Vm112WriterContractTests(unittest.TestCase):
    def run_fake_dbtool(self, rows, exit_code=0):
        with tempfile.TemporaryDirectory() as directory:
            fake = Path(directory) / "fake-dbtool.cmd"
            commands = ["@echo off", *[f"echo {row}" for row in rows],
                        f"exit /b {exit_code}"]
            fake.write_text("\r\n".join(commands) + "\r\n", encoding="ascii")
            env = os.environ.copy()
            env["MXH_DATABASE_CONFIG"] = "test fixture only"
            return subprocess.run(
                ["powershell.exe", "-NoProfile", "-ExecutionPolicy", "Bypass",
                 "-File", str(GUARD), "-DbToolPath", str(fake)],
                capture_output=True, text=True, env=env, timeout=20)

    def test_exact_single_success_row_is_required(self):
        passed = self.run_fake_dbtool(["contract_ok", "1", "(1 rows)"])
        self.assertEqual(passed.returncode, 0, passed.stderr)
        rejected = self.run_fake_dbtool(["contract_ok", "0", "(1 rows)"])
        self.assertNotEqual(rejected.returncode, 0)
        self.assertIn("no servers were started", rejected.stderr)

    def test_missing_or_failed_query_is_rejected(self):
        self.assertNotEqual(self.run_fake_dbtool(["contract_ok", "1"]).returncode, 0)
        self.assertNotEqual(
            self.run_fake_dbtool(["contract_ok", "1", "(1 rows)"], 7).returncode, 0)

    def test_wrapper_calls_guard_before_any_server_process(self):
        source = WRAPPER.read_text(encoding="utf-8")
        self.assertLess(source.index("assert_original_writer_contract.ps1"),
                        source.index("Start-Process"))
        self.assertLess(source.index("if (-not $Start)"),
                        source.index("assert_original_writer_contract.ps1"))

    def test_guard_pins_new_nullable_attribute_writer_contract(self):
        source = GUARD.read_text(encoding="utf-8")
        self.assertIn("DB_NAME() = N'MoxiangOriginalTest_20261009'", source)
        self.assertIn("sys.check_constraints", source)
        for name in ("gengol", "minchub", "cheryuk", "simmek"):
            self.assertIn(f"ck_character_info_base_{name}", source)
        for condition in ("TYPE_NAME(c.user_type_id) = N'int'", "c.is_nullable = 1",
                          "c.default_object_id = 0", "ck.is_disabled = 0",
                          "ck.is_not_trusted = 0", "N'<=65535'"):
            self.assertIn(condition, source)


if __name__ == "__main__":
    unittest.main()
