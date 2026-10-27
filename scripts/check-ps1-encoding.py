#!/usr/bin/env python3
"""Report scripts/*.ps1 files that Windows PowerShell 5.1 may misread.

Enforced files must be pure ASCII with no BOM:
  scripts/rog-gate-build.ps1
  scripts/clean-deploy.ps1
  scripts/rog-gate-build.cmd
  scripts/clean-deploy.cmd

Each enforced .cmd must clear PSModulePath and launch powershell.exe.
Other scripts/*.ps1 files are reported when they are neither pure ASCII
nor UTF-8 with a BOM. Those reports do not change the exit code.
Exit non-zero when an enforced file is missing or non-compliant.
"""

from __future__ import annotations

import sys
from pathlib import Path

ENFORCED_PS1 = (
    "rog-gate-build.ps1",
    "clean-deploy.ps1",
)
ENFORCED_CMD = (
    ("rog-gate-build.cmd", "rog-gate-build.ps1"),
    ("clean-deploy.cmd", "clean-deploy.ps1"),
)


def is_ps51_text(data: bytes) -> bool:
    if data.startswith(b"\xef\xbb\xbf"):
        return True
    return all(byte < 128 for byte in data)


def is_pure_ascii_no_bom(data: bytes) -> bool:
    if not data or data.startswith(b"\xef\xbb\xbf"):
        return False
    return all(byte < 128 for byte in data)


def main() -> int:
    root = Path(__file__).resolve().parent
    enforced_failures: list[str] = []
    reports: list[str] = []
    present_ps1 = {path.name for path in root.glob("*.ps1")}
    for path in sorted(root.glob("*.ps1")):
        data = path.read_bytes()
        if path.name in ENFORCED_PS1:
            if not is_pure_ascii_no_bom(data):
                enforced_failures.append(
                    "%s is not pure ASCII without a BOM" % path.name
                )
        elif not is_ps51_text(data):
            reports.append(path.name)
    for name in ENFORCED_PS1:
        if name not in present_ps1:
            enforced_failures.append("%s is missing" % name)
    for cmd_name, script_name in ENFORCED_CMD:
        path = root / cmd_name
        if not path.is_file():
            enforced_failures.append("%s is missing" % cmd_name)
            continue
        data = path.read_bytes()
        if not is_pure_ascii_no_bom(data):
            enforced_failures.append(
                "%s is not pure ASCII without a BOM" % cmd_name
            )
            continue
        text = data.decode("ascii")
        if 'set "PSModulePath="' not in text or "powershell.exe" not in text:
            enforced_failures.append(
                "%s does not clear PSModulePath before powershell.exe" % cmd_name
            )
        elif script_name not in text:
            enforced_failures.append("%s does not launch %s" % (cmd_name, script_name))
    for item in enforced_failures:
        print("FAIL: %s" % item)
    for name in reports:
        print("report: %s is not pure ASCII and has no UTF-8 BOM" % name)
    if enforced_failures:
        return 1
    if reports:
        print(
            "PowerShell encoding check PASSED for enforced scripts; other scripts reported"
        )
    else:
        print("PowerShell encoding check PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
