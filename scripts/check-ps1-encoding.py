#!/usr/bin/env python3
"""Report scripts/*.ps1 files that Windows PowerShell 5.1 may misread.

A file passes when it is pure ASCII or starts with a UTF-8 BOM (EF BB BF).
Exit non-zero only when scripts/rog-gate-build.ps1 is missing or non-compliant.
Other failures are printed and do not change the exit code.
"""

from __future__ import annotations

import sys
from pathlib import Path

ENFORCE = "rog-gate-build.ps1"


def is_ps51_text(data: bytes) -> bool:
    if data.startswith(b"\xef\xbb\xbf"):
        return True
    return all(byte < 128 for byte in data)


def main() -> int:
    root = Path(__file__).resolve().parent
    enforced = root / ENFORCE
    failures: list[str] = []
    for path in sorted(root.glob("*.ps1")):
        if not is_ps51_text(path.read_bytes()):
            failures.append(path.name)
    for name in failures:
        if name == ENFORCE:
            print("FAIL: %s is not pure ASCII and has no UTF-8 BOM" % name)
        else:
            print("report: %s is not pure ASCII and has no UTF-8 BOM" % name)
    if not enforced.is_file():
        print("FAIL: %s is missing" % ENFORCE)
        return 1
    if ENFORCE in failures:
        return 1
    if failures:
        print("PowerShell encoding check PASSED for %s; other scripts reported" % ENFORCE)
    else:
        print("PowerShell encoding check PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
