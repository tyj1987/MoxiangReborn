#!/usr/bin/env python3
"""Audit terrain texture dependencies for one shipped map.

The legacy HFL stores authoring-time .tga names while the current Map.pak
contains the converted DDS entries.  This audit compares only the textures
marked used by the HFL against the actual pack index, case-insensitively, and
never changes canonical resources.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path


def find_explorer(build_dir: Path) -> Path:
    candidates = (
        build_dir / "tools" / "MoxianResourceExplorer" / "mxh_explorer.exe",
        build_dir / "tools" / "MoxianResourceExplorer" / "Debug" / "mxh_explorer.exe",
        build_dir / "tools" / "MoxianResourceExplorer" / "Release" / "mxh_explorer.exe",
    )
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    raise FileNotFoundError(f"mxh_explorer.exe not found under {build_dir}")


def run(explorer: Path, *args: str) -> str:
    result = subprocess.run(
        [str(explorer), *args],
        check=False,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if result.returncode != 0:
        detail = (result.stderr or result.stdout).strip()
        raise RuntimeError(f"explorer {' '.join(args)} failed: {detail}")
    return result.stdout


def parse_hfl_names(text: str) -> set[str]:
    names: set[str] = set()
    for line in text.splitlines():
        match = re.match(r"^\s*\d+\s+\d+\s+(.+?)\s+used=1\s*$", line)
        if not match:
            continue
        name = match.group(1).strip()
        if name == "1":
            continue
        if name.lower().endswith(".tga"):
            name = name[:-4] + ".dds"
        names.add(name.casefold())
    return names


def parse_pack_names(text: str) -> set[str]:
    names: set[str] = set()
    for line in text.splitlines():
        match = re.match(r"^\s*\d+\s+(.+?)\s*$", line)
        if match:
            names.add(match.group(1).strip().casefold())
    return names


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("map_number", type=int, help="shipped map number, e.g. 10 or 101")
    parser.add_argument("playdh_root", type=Path)
    parser.add_argument("--build-dir", type=Path, default=Path("modern/build"))
    parser.add_argument("--output", type=Path)
    parser.add_argument("--json", action="store_true",
                        help="emit a machine-readable JSON report")
    args = parser.parse_args()

    if args.map_number < 0 or args.map_number > 255:
        parser.error("map_number must be between 0 and 255")
    root = args.playdh_root.resolve()
    map_pack = root / "Map.pak"
    if not map_pack.is_file():
        print(f"FAIL: Map.pak not found: {map_pack}", file=sys.stderr)
        return 2
    explorer = find_explorer(args.build_dir)
    map_name = str(args.map_number)

    with tempfile.TemporaryDirectory(prefix="mxh-map-audit-") as temp:
        temp_root = Path(temp)
        hfl_path = temp_root / f"{map_name}.hfl"
        # Report an absent map entry as a deterministic profile blocker.  The
        # old path let the explorer traceback leak through, obscuring whether
        # the map or its textures were actually missing.
        pack_output = run(explorer, "list", str(map_pack))
        pack_names = parse_pack_names(pack_output)
        if f"{map_name}.hfl".casefold() not in pack_names:
            print(
                f"FAIL: map {args.map_number} HFL entry is absent from Map.pak "
                f"(expected {map_name}.hfl); profile does not ship this map",
                file=sys.stderr,
            )
            return 2
        # Explorer writes the extracted entry into the requested directory.
        run(explorer, "extract-pak", str(map_pack), f"{map_name}.hfl", "-o", str(temp_root))
        if not hfl_path.is_file():
            print(f"FAIL: extracted HFL missing: {hfl_path}", file=sys.stderr)
            return 2
        hfl_output = run(explorer, "hfl", str(hfl_path))

    required = parse_hfl_names(hfl_output)
    available = parse_pack_names(pack_output)
    missing = sorted(required - available)
    report = {
        "map": args.map_number,
        "hfl_used_texture_count": len(required),
        "pak_entry_count": len(available),
        "missing_texture_count": len(missing),
        "missing": missing,
    }
    if args.json:
        text = json.dumps(report, ensure_ascii=False, indent=2) + "\n"
    else:
        result = [
            f"map={args.map_number}",
            f"hfl_used_texture_count={len(required)}",
            f"pak_entry_count={len(available)}",
            f"missing_texture_count={len(missing)}",
        ]
        result.extend(f"missing={name}" for name in missing)
        text = "\n".join(result) + "\n"
    print(text, end="")
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text, encoding="utf-8")
    return 1 if missing else 0


if __name__ == "__main__":
    raise SystemExit(main())
