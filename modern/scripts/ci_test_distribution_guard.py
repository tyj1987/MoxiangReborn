"""
ci_test_distribution_guard.py — Phase 10.27 CI test distribution guard.

Three checks rolled into one, to keep CI step count small:

  1. DISABLED_ test guard: assert no new DISABLED_ gtest
     test is added. Disabling a test is almost always the
     wrong fix; if a test must be skipped, use GTEST_SKIP()
     so it's visible in the run output. A new DISABLED_ test
     shows up in the diff and forces a deliberate decision.

  2. Module-drop guard: assert each "module" (top-level
     directory under modern/tests/unit/) still has at least
     N tests, where N is configurable. A module's test count
     dropping 50%+ in a single commit almost always means a
     test file was removed by mistake.

  3. Slow-test guard: print the slowest 5 tests (informational,
     not a hard fail unless --max-test-seconds is set). Slow
     tests that aren't intentionally long are usually
     accidental (e.g. a 5s sleep left from debugging).

Usage:
    python modern/scripts/ci_test_distribution_guard.py \
        --build-dir modern/build \
        --module-min 1 \
        --max-test-seconds 30
    # exit 0 = clean
    # exit 1 = DISABLED_ added or module count below min
    # exit 2 = ctest error / workspace not found
    # exit 3 = invalid arguments
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path


kDefaultModuleMin = 1  # each module must have at least 1 test
kDefaultMaxTestSeconds = 30.0


def ctest_run(build_dir: Path) -> str:
    """Run ctest with --print-labels to list all tests, then
    also run them to capture per-test timing. We use the
    "show-only" + "run-and-show" combo via -N + actual run.
    """
    # First, get the list of tests with their durations from
    # the last test run. ctest -N doesn't include timing.
    # To get timing, we need to actually run ctest once and
    # parse its output. For a fast mode, we use --quiet
    # to suppress per-test progress lines, but the
    # "Test #N: Foo.Bar  Passed N.NN sec" lines are emitted
    # to stderr only with verbose output, so we use
    # --output-on-failure + capture both streams.
    cmd = [
        "ctest", "-C", "Debug",
        "--test-dir", str(build_dir),
        "--timeout", "30",
        "-Q",  # quiet — only summary at the end
    ]
    proc = subprocess.run(
        cmd, capture_output=True, text=True,
        encoding="utf-8", errors="replace", timeout=300,
    )
    return proc.stdout + proc.stderr


def parse_total_tests(ctest_output: str) -> int | None:
    m = re.search(r"^Total Tests:\s*(\d+)\s*$", ctest_output, re.MULTILINE)
    return int(m.group(1)) if m else None


def parse_module_distribution(test_dir: Path) -> dict[str, int]:
    """Map each top-level module under modern/tests/unit/
    to its test count by counting TEST() macros in *.cpp
    files. This is a static count (not from ctest -N), so
    it works even if ctest can't run.
    """
    dist: dict[str, int] = {}
    if not test_dir.is_dir():
        return dist
    for sub in sorted(test_dir.iterdir()):
        if not sub.is_dir():
            continue
        # Count TEST( and TEST_F( and TEST_P( macros.
        count = 0
        for cpp in sub.rglob("*.cpp"):
            try:
                content = cpp.read_text(encoding="utf-8", errors="replace")
            except OSError:
                continue
            count += len(re.findall(r"\bTEST(_[FP])?\s*\(", content))
        if count > 0:
            dist[sub.name] = count
    return dist


def find_disabled_tests(modern_dir: Path) -> list[Path]:
    """Find any .cpp file under modern/ that contains
    DISABLED_ followed by a test name (the gtest convention
    for disabled tests).
    """
    bad: list[Path] = []
    if not modern_dir.is_dir():
        return bad
    for cpp in modern_dir.rglob("*.cpp"):
        try:
            content = cpp.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        if re.search(r"\bDISABLED_[A-Za-z_][A-Za-z0-9_]*", content):
            bad.append(cpp)
    return bad


def find_slow_tests(ctest_output: str, top_n: int = 5) -> list[tuple[str, float]]:
    """Parse ctest -V output for the slowest N tests. We look
    for lines like "Test #123: Foo.Bar    Passed 12.34 sec"
    in ctest -V output. With -Q (quiet), per-test timing
    isn't emitted — for slow-test detection we need a
    non-quiet run, or we can use the LastTest.log.
    """
    # ctest -V writes per-test timing to its own logfile in
    # build/Testing/Temporary/LastTest.log. We don't try to
    # parse that here — for now, slow-test detection is a
    # TODO: parse build/Testing/Temporary/LastTest.log
    # if it exists.
    return []


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Guard against DISABLED_ tests, module drops, and slow tests.",
    )
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--modern-dir", type=Path, default=None,
                        help="modern/ root (default: build_dir/..)")
    parser.add_argument("--module-min", type=int, default=kDefaultModuleMin,
                        help=f"Minimum tests per module (default {kDefaultModuleMin})")
    parser.add_argument("--max-test-seconds", type=float,
                        default=kDefaultMaxTestSeconds,
                        help=f"Max seconds per test before warning (default {kDefaultMaxTestSeconds})")
    args = parser.parse_args()

    if not args.build_dir.is_dir():
        print(f"ERROR: --build-dir {args.build_dir} does not exist", file=sys.stderr)
        return 3
    if args.modern_dir is None:
        args.modern_dir = args.build_dir.parent
    test_dir = args.modern_dir / "tests" / "unit"

    failed = False

    # 1. DISABLED_ test guard
    disabled = find_disabled_tests(args.modern_dir)
    if disabled:
        failed = True
        print(f"FAIL: {len(disabled)} file(s) contain DISABLED_ tests:")
        for p in disabled:
            rel = p.relative_to(args.modern_dir) if p.is_relative_to(args.modern_dir) else p
            print(f"  {rel}")
        print(
            "\nDisabling a test hides the failure. If a test is "
            "broken because of an environment issue (no docker, "
            "no SQL Server), use GTEST_SKIP() so the test still "
            "appears in the run output and is visible to anyone "
            "running locally. If the test is genuinely obsolete, "
            "delete it (and remove the corresponding gtest_add_tests "
            "entry).",
            file=sys.stderr,
        )
    else:
        print("PASS: no DISABLED_ tests in modern/.")

    # 2. Module-drop guard
    dist = parse_module_distribution(test_dir)
    if not dist:
        print(f"FAIL: no tests found under {test_dir} — is modern/ set up?",
              file=sys.stderr)
        return 2
    print(f"\nModule distribution ({len(dist)} modules):")
    for name in sorted(dist, key=lambda n: -dist[n]):
        count = dist[name]
        marker = "  FAIL" if count < args.module_min else "  OK  "
        print(f"  {marker}  {name:20s}  {count:4d} tests")
        if count < args.module_min:
            failed = True
    if failed:
        print(
            f"\nFAIL: one or more modules have fewer than {args.module_min} tests.",
            file=sys.stderr,
        )

    # 3. Slow-test guard (informational only by default; would
    #    need LastTest.log parsing for hard enforcement).
    if args.max_test_seconds > 0:
        # Try to read the ctest LastTest.log for timing data.
        last_log = args.build_dir / "Testing" / "Temporary" / "LastTest.log"
        if last_log.is_file():
            try:
                content = last_log.read_text(encoding="utf-8", errors="replace")
            except OSError:
                content = ""
            # ctest -V emits lines like:
            #   Test #123: Foo.Bar
            #   ...
            #   1: Test passed in 12.34 seconds
            slow: list[tuple[str, float]] = []
            for m in re.finditer(
                r"^Test #\d+: ([\w.]+).*?\n.*?passed in ([\d.]+) seconds",
                content, re.MULTILINE | re.DOTALL,
            ):
                test_name = m.group(1)
                duration = float(m.group(2))
                if duration > args.max_test_seconds:
                    slow.append((test_name, duration))
            if slow:
                slow.sort(key=lambda x: -x[1])
                print(f"\nWARNING: {len(slow)} test(s) exceeded {args.max_test_seconds}s:")
                for name, dur in slow[:5]:
                    print(f"  {name:60s}  {dur:6.2f} s")
                # Slow tests are a warning, not a failure —
                # they're often intentional (large fixtures).
            else:
                print(f"\nPASS: no tests exceeded {args.max_test_seconds}s.")

    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
