"""The VM112 writer guard must reject a bad DB reply before process launch."""

import os
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
GUARD = ROOT / "deploy/scripts/assert_original_writer_contract.ps1"
WRAPPER = ROOT / "deploy/scripts/start_original_vm112.ps1"
PACKAGE = ROOT / "scripts/package-original-vm112.py"


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

    def test_current_wrapper_requires_explicit_database_contract(self):
        source = (ROOT / "deploy/scripts/start_current_vm112.ps1").read_text(encoding="utf-8")
        self.assertIn("[string]$DatabaseName = 'MoxiangOriginalTest_20261009'", source)
        self.assertIn("$manifest.writerContract.database -ne $DatabaseName", source)
        self.assertIn("[regex]::Escape($DatabaseName)", source)

    def test_guard_pins_new_nullable_attribute_writer_contract(self):
        source = GUARD.read_text(encoding="utf-8")
        self.assertIn("DB_NAME() = N'$DatabaseName'", source)
        self.assertIn("MAX(version) FROM dbo.modern_schema_version) = 4", source)
        self.assertIn("sys.check_constraints", source)
        for name in ("gengol", "minchub", "cheryuk", "simmek"):
            self.assertIn(f"ck_character_info_base_{name}", source)
        for name in ("max_level", "exp_flag", "level_up_point", "hp_mp_flag"):
            self.assertIn(f"ck_modern_player_state_{name}", source)
        self.assertIn("ck_modern_player_vitals_state", source)
        self.assertIn("ck.is_not_trusted = 0", source)
        self.assertIn("pkc.name = N'player_id'", source)
        self.assertIn("keys.key_ordinal > 0", source)
        self.assertIn("N'(player_id>0and((initialized=0", source)
        for condition in ("TYPE_NAME(c.user_type_id) = N'int'", "c.is_nullable = 1",
                          "c.default_object_id = 0", "ck.is_disabled = 0",
                          "ck.is_not_trusted = 0", "N'<=65535'"):
            self.assertIn(condition, source)

    def test_r18_stage_package_and_wrapper_pin_the_same_isolated_identity(self):
        wrapper = WRAPPER.read_text(encoding="utf-8")
        package = PACKAGE.read_text(encoding="utf-8")
        for value in ("stage-r18-20261009", "MoxiangOriginalR18Test_20261009",
                      "original-vm112-r18-mssql-v3", "original-vitals-cas-v2",
                      "original-exp-points-v3", "36001", "37001", "38001"):
            self.assertIn(value, wrapper)
            self.assertIn(value, package)
        self.assertIn("[int]$writerContract.schemaVersion -ne 4", wrapper)
        self.assertIn('"schemaVersion": 4', package)
        self.assertNotIn("original-vm112-modern-mssql-v1", wrapper)


if __name__ == "__main__":
    unittest.main()
