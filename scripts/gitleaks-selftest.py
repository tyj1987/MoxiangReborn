#!/usr/bin/env python3
"""Synthetic gitleaks self-test and redacted current-tree report.

Samples are written under a temporary directory outside the repository.
The report printer emits rule id and path:line only.
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

# v8.30.1 uses exit code 1 for a scan error, including a partial scan.
# --exit-code is only the status for a completed scan that found leaks.
LEAK_EXIT = 2
LFS_POINTER_PREFIX = b"version https://git-lfs.github.com/spec/v1"
# A tracked *.pak passes only when .gitattributes sets filter=lfs and the
# file is pointer text. Otherwise the CI guard prints NON_LFS_PAK and fails.
PAK_POINTER_CHECK = True


def _repo_rel(path: Path, root: Path) -> str:
    try:
        return path.resolve().relative_to(root.resolve()).as_posix()
    except ValueError:
        return path.as_posix()


def _filter_attrs(root: Path, rels: list[str]) -> dict[str, str]:
    """Map repo-relative paths to the filter attribute from .gitattributes."""
    proc = subprocess.run(
        ["git", "-C", str(root), "check-attr", "filter", "--", *rels],
        check=True,
        capture_output=True,
        text=True,
    )
    attrs: dict[str, str] = {}
    for line in proc.stdout.splitlines():
        marker = ": filter: "
        if marker not in line:
            continue
        rel, value = line.split(marker, 1)
        attrs[rel] = value.strip()
    return attrs


def _is_lfs_pointer(path: Path) -> bool:
    if not path.is_file():
        return False
    with path.open("rb") as handle:
        return handle.read(len(LFS_POINTER_PREFIX)).startswith(LFS_POINTER_PREFIX)


def non_lfs_pak_lines(paths: list[Path], root: Path) -> list[str]:
    """Return NON_LFS_PAK lines for tracked paks that are not LFS pointers.

    A pak passes only when .gitattributes sets filter=lfs and the file
    starts with the Git LFS pointer line. The report is path and byte
    size only.
    """
    if not PAK_POINTER_CHECK:
        return []
    rels: list[str] = []
    by_rel: dict[str, Path] = {}
    for path in paths:
        rel = _repo_rel(path, root)
        if not rel.lower().endswith(".pak"):
            continue
        rels.append(rel)
        by_rel[rel] = path
    if not rels:
        return []
    attrs = _filter_attrs(root, rels)
    lines: list[str] = []
    for rel in rels:
        path = by_rel[rel]
        size = path.stat().st_size if path.is_file() else 0
        if attrs.get(rel) == "lfs" and _is_lfs_pointer(path):
            continue
        lines.append(f"NON_LFS_PAK {rel} bytes={size}")
    return lines


def tracked_pak_paths(root: Path) -> list[Path]:
    proc = subprocess.run(
        ["git", "-C", str(root), "ls-files", "-z"],
        check=True,
        capture_output=True,
    )
    paths: list[Path] = []
    for rel in proc.stdout.split(b"\0"):
        if not rel.lower().endswith(b".pak"):
            continue
        paths.append(root / rel.decode("utf-8", "surrogateescape"))
    return paths


def _pak_guard_selftest() -> int:
    tmp = Path(tempfile.mkdtemp(prefix="gitleaks-pak-"))
    try:
        subprocess.run(["git", "init", "-q", str(tmp)], check=True)
        # Only the pointer fixture is an LFS path. The other pak is tracked
        # content with no filter=lfs rule, same shape as a non-LFS archive.
        (tmp / ".gitattributes").write_text(
            "good.pak filter=lfs diff=lfs merge=lfs -text\n",
            encoding="ascii",
        )
        bad = tmp / "bad.pak"
        bad.write_bytes(b"not-a-git-lfs-pointer\n")
        good = tmp / "good.pak"
        good.write_text(
            "version https://git-lfs.github.com/spec/v1\n"
            "oid sha256:" + ("0" * 64) + "\n"
            "size 1\n",
            encoding="ascii",
        )
        bad_lines = non_lfs_pak_lines([bad], tmp)
        good_lines = non_lfs_pak_lines([good], tmp)
        bad_ok = bad_lines == [f"NON_LFS_PAK bad.pak bytes={bad.stat().st_size}"]
        good_ok = good_lines == []

        def report(name: str, ok: bool) -> None:
            print(f"GITLEAKS_SELFTEST {name} {'OK' if ok else 'FAIL'}")

        report("non-lfs-pak", bad_ok)
        report("lfs-pointer-pak", good_ok)
        return 0 if bad_ok and good_ok else 1
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def _run_gitleaks(binary: str, config: str, target: str, report: Path) -> int:
    proc = subprocess.run(
        [
            binary,
            "dir",
            target,
            "--config",
            config,
            "--redact=100",
            "--no-banner",
            "--log-level",
            "error",
            "--report-format",
            "json",
            "--report-path",
            str(report),
            "--exit-code",
            str(LEAK_EXIT),
        ],
        check=False,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    return proc.returncode


def _load_findings(report: Path) -> list[dict] | None:
    if not report.is_file() or report.stat().st_size == 0:
        return None
    try:
        data = json.loads(report.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return None
    if not isinstance(data, list):
        return None
    return data


def _scan_status_ok(rc: int, findings: list[dict] | None) -> bool:
    if findings is None or rc not in (0, LEAK_EXIT):
        return False
    if rc == 0:
        return not findings
    return bool(findings)


def _print_hits(findings: list[dict], root: Path) -> int:
    rows: list[str] = []
    root_text = str(root)
    for item in findings:
        rule = str(item.get("RuleID") or "")
        file_name = str(item.get("File") or "")
        line = item.get("StartLine", "")
        if file_name.startswith(root_text):
            file_name = file_name[len(root_text) :].lstrip("/\\")
        rows.append(f"{rule} {file_name}:{line}")
    for row in sorted(rows):
        print(row)
    return 1 if rows else 0


def _selftest(binary: str, config: str) -> int:
    tmp = tempfile.mkdtemp(prefix="gitleaks-selftest-")
    try:
        root = Path(tmp)
        (root / "pos_byte_literal.py").write_text(
            "key = b'example-not-a-key'\n"
            "KEY = b'example-not-a-key'\n"
            "xor_key = b'example-not-a-key'\n"
            "_key = b'example-not-a-key'\n",
            encoding="utf-8",
        )
        (root / "pos_byte_array.py").write_text(
            "xor_table = bytes([0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07])\n"
            "key_table = bytearray([0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07])\n",
            encoding="utf-8",
        )
        (root / "pos_cpp_key.cpp").write_text(
            "uint8_t xorKey[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};\n"
            "static const unsigned char byteTable[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};\n"
            "std::array<uint8_t, 8> xorTable = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};\n"
            "const std::array<std::uint8_t, 8> KeyBytes{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};\n",
            encoding="utf-8",
        )
        parser = root / "modern" / "tools"
        parser.mkdir(parents=True)
        (parser / "unpack_pak.py").write_text(
            "xor_key = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07]\n",
            encoding="utf-8",
        )
        (root / "pos_password.py").write_text(
            'password = "example-not-a-key"\n',
            encoding="utf-8",
        )
        (root / "pos_password.ps1").write_text(
            '[string]$Password = "example-not-a-key"\n',
            encoding="utf-8",
        )
        negative = root / "gitleaks-selftest" / "allowlisted"
        negative.mkdir(parents=True)
        (negative / "placeholder_password.py").write_text(
            'password = "test-only-not-a-password"\n'
            'password = "<从环境变量读取>"\n',
            encoding="utf-8",
        )
        (root / "neg_short_array.py").write_text(
            "xor_table = bytes([0x00, 0x01, 0x02])\n",
            encoding="utf-8",
        )

        report = root / "report.json"
        rc = _run_gitleaks(binary, config, str(root), report)
        findings = _load_findings(report)
        scanner_ok = _scan_status_ok(rc, findings) and rc == LEAK_EXIT
        if findings is None:
            findings = []
        failed = not scanner_ok

        def files_for(rule: str, suffix: str) -> list[str]:
            return [
                str(item.get("File") or "")
                for item in findings
                if item.get("RuleID") == rule and str(item.get("File") or "").endswith(suffix)
            ]

        def report_rule(name: str, ok: bool) -> None:
            nonlocal failed
            if not ok:
                failed = True
            print(f"GITLEAKS_SELFTEST {name} {'OK' if ok else 'FAIL'}")

        report_rule(
            "python-byte-literal-key",
            len(files_for("python-byte-literal-key", "pos_byte_literal.py")) >= 4,
        )
        report_rule(
            "python-named-byte-array",
            len(files_for("python-named-byte-array", "pos_byte_array.py")) >= 2,
        )
        report_rule(
            "cpp-key-byte-array",
            len(files_for("cpp-key-byte-array", "pos_cpp_key.cpp")) >= 4,
        )
        report_rule(
            "parser-file-key-table",
            len(files_for("parser-file-key-table", "modern/tools/unpack_pak.py")) >= 1,
        )
        report_rule(
            "hardcoded-password-literal",
            len(files_for("hardcoded-password-literal", "pos_password.py")) >= 1
            and len(files_for("hardcoded-password-literal", "pos_password.ps1")) >= 1,
        )
        negative_hits = [
            item
            for item in findings
            if str(item.get("File") or "").endswith("placeholder_password.py")
            or str(item.get("File") or "").endswith("neg_short_array.py")
        ]
        report_rule("allowlisted-placeholder", scanner_ok and not negative_hits)
        if _pak_guard_selftest() != 0:
            failed = True
        return 1 if failed else 0
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--gitleaks", default="")
    parser.add_argument("--config", default="")
    parser.add_argument("--scan", default="", help="Directory to scan. Omit to run the self-test.")
    parser.add_argument("--check-paks", default="", help="Fail if a tracked *.pak is not an LFS pointer.")
    args = parser.parse_args(argv)
    if args.check_paks:
        root = Path(args.check_paks).resolve()
        lines = non_lfs_pak_lines(tracked_pak_paths(root), root)
        for line in lines:
            print(line)
        return 1 if lines else 0
    if not args.gitleaks or not args.config:
        print("gitleaks and config are required", file=sys.stderr)
        return 1
    if args.scan:
        scan_root = Path(args.scan).resolve()
        report = Path(tempfile.mkdtemp(prefix="gitleaks-report-")) / "report.json"
        try:
            rc = _run_gitleaks(args.gitleaks, args.config, str(scan_root), report)
            findings = _load_findings(report)
            if not _scan_status_ok(rc, findings):
                print("gitleaks dir failed", file=sys.stderr)
                return 1
            return _print_hits(findings or [], scan_root)
        finally:
            shutil.rmtree(report.parent, ignore_errors=True)
    return _selftest(args.gitleaks, args.config)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
