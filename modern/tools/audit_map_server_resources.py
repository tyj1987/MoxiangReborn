#!/usr/bin/env python3
"""Audit the exact server AIGroup resource required by one map.

The modern map server keeps the legacy filename rule: ordinary maps use a
zero-padded two-digit Monster_<nn>.bin name, while special maps use the full
number.  This check is read-only and follows the same Resource/Server root
passed to the service; it never synthesizes or substitutes a profile.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


def resource_name(map_number: int) -> str:
    return f"Monster_{map_number:02d}.bin" if map_number < 100 else f"Monster_{map_number}.bin"


def audit(map_number: int, playdh_root: Path) -> dict[str, object]:
    root = playdh_root.resolve()
    path = root / "Resource" / "Server" / resource_name(map_number)
    row: dict[str, object] = {
        "map": map_number,
        "resource": f"Resource/Server/{path.name}",
        "path": path.as_posix(),
        "exists": path.is_file(),
        "bytes": path.stat().st_size if path.is_file() else 0,
    }
    return row


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("map_number", type=int)
    parser.add_argument("playdh_root", type=Path)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    if not 0 <= args.map_number <= 255:
        parser.error("map_number must be between 0 and 255")
    row = audit(args.map_number, args.playdh_root)
    if args.json:
        print(json.dumps(row, ensure_ascii=False, indent=2))
    else:
        print(f"map={row['map']}")
        print(f"resource={row['resource']}")
        print(f"exists={str(row['exists']).lower()}")
        print(f"bytes={row['bytes']}")
    return 0 if row["exists"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
