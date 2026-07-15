"""
ci_line_ending_guard.py — Phase 10.25 CI line-ending guard (modern/ only).

Verifies that NEW or MODIFIED text files in the modern/ tree
use LF line endings, not CRLF. Scoped to modern/ because that's
the only tree the project owner is actively modernizing — the
legacy 墨香【源码】/... tree is original 2003-era Windows code
and may legitimately use CRLF.

The script compares the working tree to the most recent
commit on main, and only checks files that differ (added or
modified). Files that pass through without changing don't
need a re-check.

Usage:
    python modern/scripts/ci_line_ending_guard.py
    # exit 0 = clean (no CRLF in any changed text file)
    # exit 1 = CRLF found in N files (prints list)
    # exit 2 = git error / workspace not found
"""

import subprocess
import sys
from pathlib import Path


# Text extensions we care about. Other extensions (binaries,
# images, etc.) are skipped automatically.
kTextExtensions = {
    ".py", ".ps1", ".bat", ".cmd", ".sh", ".bash",
    ".cpp", ".c", ".h", ".hpp", ".cc", ".cs",
    ".txt", ".md", ".json", ".yml", ".yaml", ".toml",
    ".cmake", ".in", ".gitignore", ".gitattributes",
    ".editorconfig", ".vcxproj", ".vcxproj.filters",
    ".filters", ".props", ".targets", ".sln",
}

# Dirs to skip even within modern/ (third-party / build / scratch).
kSkipDirs = {
    "modern/build", "modern/third_party", "modern/scratch",
    ".git", "node_modules", "__pycache__",
}


def list_changed_files(workspace: Path) -> list[Path]:
    """Return paths of files that differ from HEAD on the
    current branch. Uses `git diff --name-only HEAD` so both
    staged and unstaged changes are included.
    """
    cmd = ["git", "-C", str(workspace), "diff", "--name-only", "HEAD"]
    proc = subprocess.run(
        cmd, capture_output=True, text=True,
        encoding="utf-8", errors="replace", timeout=30,
    )
    if proc.returncode != 0:
        raise RuntimeError(f"git diff failed: {proc.stderr}")
    return [workspace / line.strip().replace("/", "\\")
            for line in proc.stdout.splitlines() if line.strip()]


def has_crlf(path: Path) -> bool:
    """Return True if `path` contains a \\r\\n byte sequence in
    the first 1 MB. We don't read the whole file because
    some test fixtures are large binary blobs with .txt
    extensions; the first 1 MB is enough to catch accidental
    edits.
    """
    try:
        with path.open("rb") as f:
            chunk = f.read(1024 * 1024)
    except (OSError, UnicodeDecodeError):
        return False
    return b"\r\n" in chunk


def main() -> int:
    # Resolve workspace root. D:\Moxian is a reparse point to
    # the CJK mirror; both paths may show up depending on
    # which one PowerShell resolved.
    candidates = [
        Path("D:/Moxian"),
        Path("D:/墨香全套源代码（源码+资源+客户端+服务端+教程）"),
    ]
    workspace = next((p for p in candidates if p.is_dir()), None)
    if workspace is None:
        print("ERROR: workspace root not found", file=sys.stderr)
        return 2

    print(f"Workspace: {workspace}")

    try:
        changed = list_changed_files(workspace)
    except RuntimeError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        return 2

    print(f"Changed files vs HEAD: {len(changed)}")

    # Filter to modern/ tree, text extension, not in skip dir.
    candidates_to_check: list[Path] = []
    for p in changed:
        if not p.is_file():
            continue
        rel = p.relative_to(workspace)
        if not str(rel).startswith("modern"):
            continue
        if any(str(rel).startswith(d.replace("/", "\\")) for d in kSkipDirs):
            continue
        if p.suffix.lower() not in kTextExtensions:
            continue
        candidates_to_check.append(p)

    print(f"Candidates to check (modern/ + text ext + not skipped): "
          f"{len(candidates_to_check)}")

    bad: list[Path] = []
    for p in candidates_to_check:
        if has_crlf(p):
            bad.append(p)

    if bad:
        print(f"\nFAIL: {len(bad)} file(s) in modern/ use CRLF line endings:")
        for p in bad:
            rel = p.relative_to(workspace)
            print(f"  {rel}")
        print(
            "\nFix with: "
            "(Get-Content -LiteralPath <path> -Raw) -replace \"`r`n\", \"`n\" | "
            "Set-Content -LiteralPath <path> -NoNewline -Encoding UTF8",
            file=sys.stderr,
        )
        return 1

    print("PASS: no CRLF in any changed modern/ text file.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
