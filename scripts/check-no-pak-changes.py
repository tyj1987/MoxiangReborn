#!/usr/bin/env python3
"""Fail when a change adds or modifies any *.pak path (case-insensitive).

Original game bytes are not committed going forward; the supplemental
fill pak (zz_supplement.pak) is built locally and verified by sha256 in
reference/manifests/playdh-supplemental-fill.json.  LFS pointer files
are blobs at a .pak path too, so they count.

    python3 scripts/check-no-pak-changes.py [--base <ref>] [--repo <dir>]

Diff mode: when <ref> (default origin/codex/reorg-original-20261009)
resolves and has a merge-base with HEAD, every added, copied, modified,
renamed or type-changed path in merge-base...HEAD is checked.

Tree mode: CI checks out one shallow commit, so the base ref is absent.
Then every *.pak in the HEAD tree must be one of the pinned PlayDH paths
with its pinned blob id (the LFS pointer blobs at 48bfab4).  Anything
new or different fails.
"""

from __future__ import annotations

import argparse
import subprocess
import sys

DEFAULT_BASE = "origin/codex/reorg-original-20261009"
PINNED_PAKS = {
    "modern/data/PlayDH/Character.pak": "432deff19e096f68a789469c3a6ceba0fe346b2f",
    "modern/data/PlayDH/Effect.pak": "c39ef71d6a7723e957b0c333d29b182dff0e310e",
    "modern/data/PlayDH/Map.pak": "37eda4894017faca9c4d306a5f54b6b7e24b2ee2",
    "modern/data/PlayDH/Pet.pak": "2c251db5cb7179c33ef02b6e0d29e42cdb2a4f0c",
    "modern/data/PlayDH/Titan.pak": "82c3dd6dd0f8deef65144b396c4ac63ae5609c73",
    "modern/data/PlayDH/monster.pak": "8d6f8f1cfe344f495c11617d0994e5aa813fe49b",
    "modern/data/PlayDH/npc.pak": "207e048176b3934c2d36eaa98f44178c4f884e7c",
}


def _git(repo: str, *args: str) -> subprocess.CompletedProcess:
    return subprocess.run(["git", "-C", repo, *args], capture_output=True, text=True)


def is_pak(path: str) -> bool:
    return path.casefold().endswith(".pak")


def diff_violations(repo: str, base: str) -> list[str] | None:
    """None when the base cannot be used."""
    if _git(repo, "rev-parse", "--verify", "--quiet", base + "^{commit}").returncode:
        return None
    mb = _git(repo, "merge-base", base, "HEAD")
    if mb.returncode:
        return None
    out = _git(repo, "-c", "core.quotepath=off", "diff", "--name-only", "--no-renames",
               "--diff-filter=ACMRT", mb.stdout.strip(), "HEAD")
    if out.returncode:
        raise RuntimeError(out.stderr.strip())
    return [p for p in out.stdout.splitlines() if is_pak(p)]


def tree_violations(repo: str) -> list[str]:
    out = _git(repo, "-c", "core.quotepath=off", "ls-tree", "-r", "HEAD")
    if out.returncode:
        raise RuntimeError(out.stderr.strip())
    bad = []
    for line in out.stdout.splitlines():
        meta, path = line.split("\t", 1)
        if is_pak(path) and PINNED_PAKS.get(path) != meta.split()[2]:
            bad.append(path)
    return bad


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--base", default=DEFAULT_BASE)
    parser.add_argument("--repo", default=".")
    args = parser.parse_args(argv)
    try:
        bad = diff_violations(args.repo, args.base)
        mode = f"diff against merge-base with {args.base}"
        if bad is None:
            bad = tree_violations(args.repo)
            mode = "tree (base ref unavailable; pinned PlayDH pak blobs)"
    except RuntimeError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2
    print(f"check-no-pak-changes: {mode}")
    for path in bad:
        print(f"ERROR: .pak path added or modified: {path}", file=sys.stderr)
    if bad:
        print("Original game bytes are not committed; build paks locally (modern/tools/pak_writer.py).",
              file=sys.stderr)
        return 1
    print("check-no-pak-changes: OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
