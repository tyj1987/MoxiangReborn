"""audit_minimap.py must not default to the repo PlayDH tree.

A missing checkout directory is a failure, not a skip. The process is
started with MXH_PLAYDH_ROOT unset. No resource bytes are read.
"""

import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[4]
AUDIT = REPO / "modern" / "tools" / "audit_minimap.py"
PLAYDH = REPO / "modern" / "data" / "PlayDH"


def _run(args, env):
    return subprocess.run(
        [sys.executable, str(AUDIT), *args],
        cwd=tempfile.gettempdir(),
        env=env,
        capture_output=True,
        text=True,
        check=False,
    )


class AuditMinimapRootTests(unittest.TestCase):
    def setUp(self):
        self.assertTrue(PLAYDH.is_dir(), "full checkout must contain modern/data/PlayDH")
        self.assertTrue(AUDIT.is_file(), "audit_minimap.py is missing")
        self.env = os.environ.copy()
        self.env.pop("MXH_PLAYDH_ROOT", None)

    def test_unset_exits_1(self):
        proc = _run(["--resolve-only"], self.env)
        combined = proc.stdout + proc.stderr
        self.assertEqual(proc.returncode, 1, combined)
        self.assertIn("MXH_PLAYDH_ROOT", combined)
        self.assertIn("--playdh", combined)
        self.assertIn("--resource-root", combined)
        self.assertNotIn("modern/data/PlayDH", combined)
        self.assertNotIn("modern\\data\\PlayDH", combined)

    def test_explicit_synthetic_root_succeeds(self):
        with tempfile.TemporaryDirectory(prefix="mxh-audit-playdh-") as tmp:
            root = Path(tmp) / "playdh"
            root.mkdir()
            (root / "marker.txt").write_text("synthetic\n", encoding="ascii")
            proc = _run(["--resolve-only", "--playdh", str(root)], self.env)
            combined = proc.stdout + proc.stderr
            self.assertEqual(proc.returncode, 0, combined)
            self.assertIn(f"resource_root={root}", proc.stdout)
            self.assertNotIn("modern/data/PlayDH", combined)
            self.assertNotIn("modern\\data\\PlayDH", combined)

    def test_env_synthetic_root_succeeds(self):
        with tempfile.TemporaryDirectory(prefix="mxh-audit-playdh-env-") as tmp:
            root = Path(tmp) / "playdh"
            root.mkdir()
            (root / "marker.txt").write_text("synthetic\n", encoding="ascii")
            env = dict(self.env)
            env["MXH_PLAYDH_ROOT"] = str(root)
            proc = _run(["--resolve-only"], env)
            combined = proc.stdout + proc.stderr
            self.assertEqual(proc.returncode, 0, combined)
            self.assertIn(f"resource_root={root}", proc.stdout)
            self.assertNotIn("modern/data/PlayDH", combined)


if __name__ == "__main__":
    unittest.main()
