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
        return 1 if failed else 0
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--gitleaks", required=True)
    parser.add_argument("--config", required=True)
    parser.add_argument("--scan", default="", help="Directory to scan. Omit to run the self-test.")
    args = parser.parse_args(argv)
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
