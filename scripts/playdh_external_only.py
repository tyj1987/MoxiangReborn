"""Load the temporary external-only PlayDH path list.

The list file is the only place those paths are written. Callers must not
keep a second copy.
"""

from __future__ import annotations

import hashlib
import os
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_LIST = REPO_ROOT / "reference" / "manifests" / "playdh-external-only.txt"
PLAYDH_PREFIX = "modern/data/PlayDH/"
LFS_MARKER = b"version https://git-lfs.github.com/spec/v1"


def list_path(explicit: Path | None = None) -> Path:
    if explicit is not None:
        return explicit
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


def root_has_manifest_file(tree_root: Path, manifest_paths: set[str], listed: set[str]) -> bool:
    """True when this root already holds some non-listed manifest file."""
    seen = 0
    for rel in sorted(manifest_paths):
        if rel in listed:
            continue
        status, _path = classify_relative(tree_root, rel)
        if status == "match":
            return True
        seen += 1
        if seen >= 32:
            break
    return False


def is_repo_playdh(tree_root: Path, repo_tree: Path | None = None) -> bool:
    """True only when tree_root is the checkout PlayDH directory.

    Comparison uses resolved paths and does not fold case.
    """
    baseline = repo_tree if repo_tree is not None else (REPO_ROOT / "modern" / "data" / "PlayDH")
    try:
        left = str(tree_root.resolve())
        right = str(baseline.resolve())
    except OSError:
        return False
    return left == right


def classify_relative(root: Path, relative: str) -> tuple[str, Path | None]:
    """Find relative by listing each directory. Never uses Path.exists.

    Returns match, missing, or case_mismatch. A case-only name is not a match.
    """
    current = root
    mismatched = False
    parts = [part for part in relative.replace("\\", "/").split("/") if part]
    for part in parts:
        try:
            names = os.listdir(current) if current.is_dir() else []
        except OSError:
            return "missing", None
        if part in names:
            current = current / part
            continue
        other = None
        for name in names:
            if name.casefold() == part.casefold():
                other = name
                break
        if other is None:
            return "missing", None
        mismatched = True
        current = current / other
    if mismatched:
        return "case_mismatch", current
    return "match", current


def _inspect_file(path: Path) -> tuple[str, str | None, int | None]:
    data = path.read_bytes()
    if data.startswith(LFS_MARKER):
        text = data.decode("utf-8", "replace")
        oid = None
        size = None
        for line in text.splitlines():
            if line.startswith("oid sha256:"):
                oid = line[len("oid sha256:") :].strip().lower()
            elif line.startswith("size "):
                try:
                    size = int(line[5:].strip())
                except ValueError:
                    return "bad-pointer", None, None
        if oid is None or size is None:
            return "bad-pointer", None, None
        return "pointer", oid, size
    return "file", hashlib.sha256(data).hexdigest(), len(data)


def coverage_lines(root: Path, expected: dict[str, tuple[str, int]]) -> tuple[int, list[str]]:
    """Case-sensitive sha256 coverage for a root that is not the repo tree."""
    counts = {"MATCH": 0, "MISSING": 0, "MISMATCH": 0, "CASE_MISMATCH": 0}
    lines: list[str] = []
    for rel in sorted(expected):
        want, want_size = expected[rel]
        status, path = classify_relative(root, rel)
        if status == "missing":
            counts["MISSING"] += 1
            lines.append("PLAYDH_BASELINE MISSING %s expected=%s" % (rel, want))
            continue
        if status == "case_mismatch":
            counts["CASE_MISMATCH"] += 1
            lines.append("PLAYDH_BASELINE CASE_MISMATCH %s" % rel)
            continue
        kind, digest, size = _inspect_file(path)
        if kind == "bad-pointer":
            counts["MISMATCH"] += 1
            lines.append("PLAYDH_BASELINE LFS_POINTER %s" % rel)
            continue
        if kind == "pointer":
            if digest != want:
                counts["MISMATCH"] += 1
                lines.append(
                    "PLAYDH_BASELINE LFS_OID_MISMATCH %s expected=%s actual=%s" % (rel, want, digest)
                )
            elif size != want_size:
                counts["MISMATCH"] += 1
                lines.append(
                    "PLAYDH_BASELINE LFS_SIZE_MISMATCH %s expected=%s actual=%s" % (rel, want_size, size)
                )
            else:
                counts["MATCH"] += 1
                lines.append("PLAYDH_BASELINE MATCH %s %s" % (rel, digest))
            continue
        if digest != want or size != want_size:
            counts["MISMATCH"] += 1
            lines.append(
                "PLAYDH_BASELINE MISMATCH %s expected=%s actual=%s" % (rel, want, digest)
            )
            continue
        counts["MATCH"] += 1
        lines.append("PLAYDH_BASELINE MATCH %s %s" % (rel, digest))
    lines.append(
        "PLAYDH_BASELINE COVERAGE MATCH=%s MISSING=%s MISMATCH=%s CASE_MISMATCH=%s"
        % (counts["MATCH"], counts["MISSING"], counts["MISMATCH"], counts["CASE_MISMATCH"])
    )
    failed = counts["MISSING"] or counts["MISMATCH"] or counts["CASE_MISMATCH"]
    return (1 if failed else 0), lines


def errors_for_tree(
    tree_root: Path,
    manifest_paths: set[str] | None,
    explicit: Path | None = None,
    repo_tree: Path | None = None,
    expected: dict[str, tuple[str, int]] | None = None,
) -> list[str]:
    """Repo PlayDH: listed files must be absent. Any other root: verify them."""
    lines: list[str] = []
    try:
        repo_paths = load_repo_paths(explicit)
    except (OSError, ValueError) as exc:
        return ["PLAYDH_BASELINE EXTERNAL_LIST %s" % exc]
    names: list[str] = []
    for repo_path in repo_paths:
        try:
            name = playdh_relative(repo_path)
        except ValueError as exc:
            lines.append("PLAYDH_BASELINE EXTERNAL_BAD_PATH %s" % exc)
            continue
        names.append(name)
        if manifest_paths is not None and not manifest_has_entry(manifest_paths, repo_path):
            lines.append("PLAYDH_BASELINE EXTERNAL_NOT_IN_MANIFEST %s" % name)
    if is_repo_playdh(tree_root, repo_tree):
        for name in names:
            status, _path = classify_relative(tree_root, name)
            if status == "match":
                lines.append("PLAYDH_BASELINE EXTERNAL_PRESENT %s" % name)
        return lines
    if expected is None:
        return lines
    listed_expected = {name: expected[name] for name in names if name in expected}
    _code, cover = coverage_lines(tree_root, listed_expected)
    lines.extend(cover)
    return lines
