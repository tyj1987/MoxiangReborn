#!/usr/bin/env python3
"""Synthetic git repos for scripts/check-no-pak-changes.py."""

from __future__ import annotations

import importlib.util
import subprocess
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[4] / "scripts" / "check-no-pak-changes.py"
SPEC = importlib.util.spec_from_file_location("check_no_pak_changes", SCRIPT)
check = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(check)

LFS_POINTER = "version https://git-lfs.github.com/spec/v1\noid sha256:abc\nsize 1\n"


def git(repo: Path, *args: str) -> str:
    return subprocess.run(
        ["git", "-C", str(repo), "-c", "user.name=t", "-c", "user.email=t@t", *args],
        check=True, capture_output=True, text=True).stdout


class CheckNoPakChangesTest(unittest.TestCase):
    def setUp(self) -> None:
        self._tmp = tempfile.TemporaryDirectory()
        self.repo = Path(self._tmp.name)
        git(self.repo, "init", "-q", "-b", "main")
        (self.repo / "old.PAK").write_bytes(b"base")
        (self.repo / "a.txt").write_text("a")
        git(self.repo, "add", "-A")
        git(self.repo, "commit", "-qm", "base")
        git(self.repo, "branch", "base")

    def tearDown(self) -> None:
        self._tmp.cleanup()

    def commit(self, rel: str, data: str) -> None:
        path = self.repo / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(data)
        git(self.repo, "add", "-A", "-f")
        git(self.repo, "commit", "-qm", rel)

    def test_clean_change_passes(self):
        self.commit("b.txt", "b")
        self.assertEqual(check.diff_violations(str(self.repo), "base"), [])
        self.assertEqual(check.main(["--repo", str(self.repo), "--base", "base"]), 0)

    def test_added_pak_any_case_fails(self):
        self.commit("x/ZZ_Supplement.Pak", "bytes")
        self.assertEqual(check.diff_violations(str(self.repo), "base"), ["x/ZZ_Supplement.Pak"])
        self.assertEqual(check.main(["--repo", str(self.repo), "--base", "base"]), 1)

    def test_modified_pak_and_lfs_pointer_fail(self):
        self.commit("old.PAK", LFS_POINTER)
        self.assertEqual(check.diff_violations(str(self.repo), "base"), ["old.PAK"])

    def test_deleted_pak_is_not_an_addition(self):
        git(self.repo, "rm", "-q", "old.PAK")
        git(self.repo, "commit", "-qm", "rm")
        self.assertEqual(check.diff_violations(str(self.repo), "base"), [])

    def test_missing_base_falls_back_to_pinned_tree(self):
        self.assertIsNone(check.diff_violations(str(self.repo), "no-such-ref"))
        self.assertEqual(check.tree_violations(str(self.repo)), ["old.PAK"])
        self.assertEqual(check.main(["--repo", str(self.repo), "--base", "no-such-ref"]), 1)


if __name__ == "__main__":
    unittest.main()
