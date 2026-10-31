#!/usr/bin/env python3
"""Synthetic self-test for the mxh_client warning gate.

Counts 0 and 36 must pass. Count 37 must fail. An unparsable count must
fail. The gate script is scripts/check-client-warnings.py.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GATE = ROOT / "check-client-warnings.py"
MAX_WARNINGS = 36

# raw --count value, expected exit, status word, count token in the gate line
CASES = (
    ("0", 0, "OK", "0"),
    ("36", 0, "OK", "36"),
    ("37", 1, "FAIL", "37"),
    ("not-a-count", 1, "FAIL", "unparsable"),
)


def _gate_line(status: str, token: str) -> str:
    return f"MXH_CLIENT_WARNINGS_GATE {status} count={token} max={MAX_WARNINGS}"


def main() -> int:
    failed = False
    if not GATE.is_file():
        for _raw, _rc, _status, token in CASES:
            print(
                "MXH_CLIENT_WARNINGS_GATE SELFTEST FAIL "
                f"count={token} reason=missing scripts/check-client-warnings.py"
            )
        return 1

    for raw, expect_rc, status, token in CASES:
        proc = subprocess.run(
            [sys.executable, str(GATE), "--count", raw],
            capture_output=True,
            text=True,
            check=False,
        )
        expected = _gate_line(status, token)
        lines = proc.stdout.splitlines()
        ok = proc.returncode == expect_rc and expected in lines
        if ok:
            print(expected)
            print(f"MXH_CLIENT_WARNINGS_GATE SELFTEST OK count={token}")
            continue
        failed = True
        print(
            "MXH_CLIENT_WARNINGS_GATE SELFTEST FAIL "
            f"count={token} rc={proc.returncode} "
            f"stdout={proc.stdout!r} stderr={proc.stderr!r}"
        )
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
