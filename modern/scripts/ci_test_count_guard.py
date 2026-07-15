"""
ci_test_count_guard.py — Phase 10.25 CI test-count guard.

Runs ctest in --print-labels mode and asserts the total test
count is at least a configured minimum. Catches silent test
regressions where a code change accidentally disables or
removes tests without anyone noticing.

Usage (CI):
    python modern/scripts/ci_test_count_guard.py \
        --build-dir modern/build --min-tests 700

Exit codes:
    0 = test count >= min (PASS)
    1 = test count < min (FAIL — silent regression)
    2 = ctest invocation error (FAIL — environment)
    3 = invalid arguments (FAIL — config)

The minimum is intentionally a few hundred below the current
count to absorb legitimate test-file renames or restructures
without flagging, but well above the count where someone
could remove a whole test file and not notice. Adjust the
default by editing the kDefaultMinTests constant.
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path


# Default minimum test count. CI passes this number to the
# script on the command line, so this is just a fallback.
# Keep it ~10% below the current count to absorb legitimate
# refactors without false alarms.
kDefaultMinTests = 700


def ctest_total(ctest_output: str) -> int | None:
    """Extract the "Total Tests" line from `ctest -N` output.

    ctest -N prints lines like:
        Total Tests: 783
    We parse that and return 783.
    Returns None if the line is missing or unparseable.
    """
    # ctest emits "Total Tests: <N>" once, somewhere in the
    # middle of the label-list output.
    m = re.search(r"^Total Tests:\s*(\d+)\s*$", ctest_output, re.MULTILINE)
    if m:
        return int(m.group(1))
    return None


def run_ctest_listing(build_dir: Path) -> str:
    """Invoke `ctest -N` to print the test list without running
    any tests. We use the same config the actual test run uses
    (Debug, matching the CI matrix).
    """
    cmd = [
        "ctest",
        "-C", "Debug",
        "--test-dir", str(build_dir),
        "-N",  # list mode, no execution
    ]
    proc = subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        timeout=60,
    )
    if proc.returncode != 0:
        raise RuntimeError(
            f"ctest -N failed (rc={proc.returncode}):\n"
            f"  stdout: {proc.stdout[:200]}\n"
            f"  stderr: {proc.stderr[:200]}"
        )
    return proc.stdout


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Guard against silent test count regression in CI.",
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        required=True,
        help="Path to the CMake build directory (e.g. modern/build).",
    )
    parser.add_argument(
        "--min-tests",
        type=int,
        default=kDefaultMinTests,
        help=(
            f"Minimum total test count required to pass. "
            f"Default: {kDefaultMinTests}."
        ),
    )
    args = parser.parse_args()

    if not args.build_dir.is_dir():
        print(f"ERROR: --build-dir {args.build_dir} does not exist", file=sys.stderr)
        return 3

    print(f"Listing tests in {args.build_dir} (this does not run them)...")
    try:
        ctest_out = run_ctest_listing(args.build_dir)
    except RuntimeError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        return 2

    total = ctest_total(ctest_out)
    if total is None:
        print(
            "ERROR: could not parse 'Total Tests' from ctest output.\n"
            "First 500 chars of output:\n" + ctest_out[:500],
            file=sys.stderr,
        )
        return 2

    print(f"Total tests: {total}")
    print(f"Min required: {args.min_tests}")

    if total < args.min_tests:
        diff = args.min_tests - total
        print(
            f"\nFAIL: test count regression.\n"
            f"  current: {total}\n"
            f"  minimum: {args.min_tests}\n"
            f"  missing: {diff} test(s)\n"
            f"\n"
            f"If this is intentional (e.g. you removed a deprecated test\n"
            f"file), update the --min-tests value in\n"
            f".github/workflows/ci.yml. If not, fix the test that was\n"
            f"accidentally disabled or removed.",
            file=sys.stderr,
        )
        return 1

    print("PASS: test count meets the minimum.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
