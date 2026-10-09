"""Regression checks for the operator runner's safe endpoint plan."""

import json
import subprocess
import unittest
from pathlib import Path


RUNNER = Path(__file__).resolve().parents[1] / "run-human-acceptance.ps1"


def plan(*arguments):
    return subprocess.run(
        ["powershell.exe", "-NoProfile", "-ExecutionPolicy", "Bypass",
         "-File", str(RUNNER), "-PlanOnly", *arguments],
        capture_output=True, text=True, timeout=30,
    )


class HumanAcceptanceEndpointTests(unittest.TestCase):
    def test_default_is_pve_client_without_local_services_or_db_config(self):
        result = plan()
        self.assertEqual(result.returncode, 0, result.stderr)
        planned = json.loads(result.stdout)
        self.assertEqual(planned["mode"], "original-client-to-pve")
        self.assertFalse(planned["startsLocalServers"])
        self.assertFalse(planned["needsLocalDatabaseConfig"])
        self.assertEqual(planned["profile"], "playdh-current")
        self.assertEqual(planned["acceptanceStatus"], "not_evaluated")
        self.assertEqual(planned["buildDirectory"], "")
        self.assertEqual(
            planned["launcherArguments"][:8],
            ["--login-host", "192.168.2.244", "--login-port", 16001,
             "--agent-port", 17001, "--map-port", 18001],
        )

    def test_endpoint_override_reaches_launcher_arguments(self):
        result = plan("-LoginHost", "game.example.test", "-LoginPort", "16011",
                      "-AgentPort", "17011", "-MapPort", "18011")
        self.assertEqual(result.returncode, 0, result.stderr)
        args = json.loads(result.stdout)["launcherArguments"]
        self.assertEqual(args[:8], ["--login-host", "game.example.test",
                                    "--login-port", 16011, "--agent-port", 17011,
                                    "--map-port", 18011])

    def test_local_server_and_database_options_are_not_accepted(self):
        result = plan("-StartLocalServers")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("StartLocalServers", result.stderr)
        result = plan("-Backend", "mssql_odbc")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Backend", result.stderr)

    def test_loopback_is_rejected_before_any_gui_process(self):
        for host in ("localhost", "127.0.0.1", "0.0.0.0"):
            with self.subTest(host=host):
                result = plan("-LoginHost", host)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Loopback", result.stderr)

    def test_explicit_build_directory_is_reported_without_launching(self):
        result = plan("-BuildDirectory", "modern/build-gui-r13")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)["buildDirectory"],
                         "modern/build-gui-r13")


if __name__ == "__main__":
    unittest.main()
