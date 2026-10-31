"""Load the temporary external-only PlayDH path list.

The list file is the only place those paths are written. Callers must not
keep a second copy.
"""

from __future__ import annotations

import os
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_LIST = REPO_ROOT / "reference" / "manifests" / "playdh-external-only.txt"
PLAYDH_PREFIX = "modern/data/PlayDH/"


def list_path(explicit: Path | None = None) -> Path:
    if explicit is not None:
        return explicit
    override = os.environ.get("PLAYDH_EXTERNAL_ONLY_LIST", "").strip()
    if override:
        return Path(override)
    return DEFAULT_LIST


def load_repo_paths(explicit: Path | None = None) -> list[str]:
    """Exact repo-relative paths. Blank lines and # comments are ignored."""
    path = list_path(explicit)
    if not path.is_file():
        raise FileNotFoundError("external-only list missing: %s" % path)
    rows: list[str] = []
    seen: set[str] = set()
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if any(ch in line for ch in "*?[]"):
            raise ValueError("external-only list refuses a glob: %s" % line)
        rel = line.replace("\\", "/")
        if rel in seen:
            raise ValueError("external-only list has a duplicate path")
        seen.add(rel)
        rows.append(rel)
    return rows


def playdh_relative(repo_path: str) -> str:
    rel = repo_path.replace("\\", "/")
    if not rel.startswith(PLAYDH_PREFIX):
        raise ValueError("external-only path is outside the PlayDH tree")
    name = rel[len(PLAYDH_PREFIX) :]
    if not name or name.startswith("/") or ".." in name.split("/"):
        raise ValueError("external-only path is outside the PlayDH tree")
    return name


def playdh_names(explicit: Path | None = None) -> list[str]:
    return [playdh_relative(path) for path in load_repo_paths(explicit)]


def manifest_has_entry(manifest_paths: set[str], repo_path: str) -> bool:
    rel = repo_path.replace("\\", "/")
    name = playdh_relative(rel)
    return name in manifest_paths or rel in manifest_paths


def errors_for_tree(
    tree_root: Path,
    manifest_paths: set[str] | None,
    explicit: Path | None = None,
) -> list[str]:
    """Listed paths must be in the manifest and must not exist in the tree."""
    lines: list[str] = []
    try:
        repo_paths = load_repo_paths(explicit)
    except (OSError, ValueError) as exc:
        return ["PLAYDH_BASELINE EXTERNAL_LIST %s" % exc]
    for repo_path in repo_paths:
        try:
            name = playdh_relative(repo_path)
        except ValueError as exc:
            lines.append("PLAYDH_BASELINE EXTERNAL_BAD_PATH %s" % exc)
            continue
        if manifest_paths is not None and not manifest_has_entry(manifest_paths, repo_path):
            lines.append("PLAYDH_BASELINE EXTERNAL_NOT_IN_MANIFEST %s" % name)
        candidate = tree_root / name
        if candidate.exists():
            lines.append("PLAYDH_BASELINE EXTERNAL_PRESENT %s" % name)
    return lines
