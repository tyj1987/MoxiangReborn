#!/usr/bin/env python3
"""Gate the mxh_client compiler warning count.

36 passes. A count above 36 fails. A count that cannot be parsed fails.
The threshold is fixed; it is not read from the environment.

  python3 scripts/check-client-warnings.py --count 36
  python3 scripts/check-client-warnings.py --log build.log
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

MAX_WARNINGS = 36
_PREFIX = "MXH_CLIENT_WARNINGS="


def parse_count(text: str) -> int | None:
    value = text.strip()
    if not value.isascii() or not value.isdigit():
        return None
    return int(value)


def parse_log(path: Path) -> int | None:
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return None
    found: int | None = None
    seen = False
    for line in text.splitlines():
        if not line.startswith(_PREFIX):
            continue
        seen = True
        parsed = parse_count(line[len(_PREFIX) :])
        if parsed is None:
            return None
        found = parsed
    if not seen:
        return None
    return found


def emit(status: str, token: str) -> int:
    print(f"MXH_CLIENT_WARNINGS_GATE {status} count={token} max={MAX_WARNINGS}")
    return 0 if status == "OK" else 1


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Gate mxh_client compiler warnings.")
    parser.add_argument("--count", default=None)
    parser.add_argument("--log", default=None)
    args = parser.parse_args(argv)
    if args.count is None and args.log is None:
        return emit("FAIL", "unparsable")
    if args.count is not None:
        count = parse_count(args.count)
    else:
        count = parse_log(Path(args.log))
    if count is None:
        return emit("FAIL", "unparsable")
    status = "OK" if count <= MAX_WARNINGS else "FAIL"
    return emit(status, str(count))


if __name__ == "__main__":
    sys.exit(main())
