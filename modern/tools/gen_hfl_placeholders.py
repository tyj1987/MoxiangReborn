#!/usr/bin/env python3
"""
gen_hfl_placeholders.py -- synthesize HFL placeholder files for maps
that have a Map<N>.bmhm manifest in PlayDH/Resource/Map/ but no
matching <N>.hfl height field.

The legacy Moxian client expects a HFL (Height FieLd) per map id;
without one, the modern loader falls back to a flat 1x1 grid and the
in-game map shows a black void.  This tool seeds placeholder HFL
files so every shipped map id can at least enter GameIn and render a
visible (if procedurally generated) terrain.

Strategy
--------
- Read one real HFL as a template (10.hfl by default, the smallest
  shipped one we have).
- Parse the HFL header using the same struct layout as
  modern/tools/render_hfl.py (4 byte version + 108 byte desc +
  hcx*hcz floats + texture table).  We do not need pixel-perfect
  faithfulness -- just enough for the modern client to accept the
  file and render something other than black.
- Mutate a few header fields so each placeholder is unique:
    * width / height  (from per-map deterministic pseudo-random)
    * height_count_x  (a small fixed value, 16, so the file stays
      small and renders fast)
    * height_count_z  (same)
    * height grid     (procedurally synthesized from map id, so
      two map ids never produce identical terrain)
- Write to <N>.hfl in the target directory; only synthesize maps
  whose <N>.hfl is missing AND whose Map<N>.bmhm is present (we do
  not invent new map ids).

This tool is reversible -- a real HFL can replace any placeholder at
any time; the modern HflHeightField parser is unchanged.

Usage
-----
    python gen_hfl_placeholders.py --src modern/data/PlayDH/Resource/Map/10.hfl \\
        --out modern/data/PlayDH/Resource/Map/                # sync missing
    python gen_hfl_placeholders.py --src 10.hfl --out . --dry-run
    python gen_hfl_placeholders.py --src 10.hfl --out . --map-ids 0,1,2,5
"""

from __future__ import annotations

import argparse
import os
import re
import struct
import sys
from pathlib import Path

# Layout constants copied from modern/tools/render_hfl.py and
# modern/src/hfl_height_field.cpp::kDiskDescSize.
HFL_VERSION_SIZE = 4
HFL_DESC_SIZE = 108
HEADER_BYTES = HFL_VERSION_SIZE + HFL_DESC_SIZE  # 112

# Placeholder grid resolution.  16x16 = 256 floats = 1024 bytes -- a
# small, fast-rendering grid that still gives every map a distinct
# silhouette in the minimap and over-the-shoulder view.
PLACEHOLDER_GRID = 16

# Texture count for placeholder.  Modern client tolerates 0 textures
# if the file ends right after the height grid (no texture table).
# We mirror the template's stored texture count instead to avoid the
# "invalid HFL texture table" rejection from parse_hfl.
KEEP_TEMPLATE_TEXTURES = True


def parse_hfl_header(data: bytes):
    """Return (version, desc_bytes, hcx, hcz, width, height, tex_count, heights).

    Mirrors the layout in modern/src/hfl_height_field.cpp::parse_hfl.
    After the 4-byte version, the desc field starts and all offsets
    below are relative to d=4 -- they match the C++ read() calls
    exactly.  Raises ValueError on truncation or zero-dimension fields.
    """
    if len(data) < HEADER_BYTES:
        raise ValueError(f"HFL truncated: have {len(data)}, need {HEADER_BYTES}")
    version = struct.unpack_from("<I", data, 0)[0]
    if version == 0 or version > 0x10:
        raise ValueError(f"unsupported HFL version {version}")
    d = 4
    # d+0..d+19 = 5 floats, d+20..d+31 = 3 u32, d+32..d+35 = 2 u32 (detail + pad),
    # d+36..d+39 = idx_l0, d+40..d+43 = pad, d+44..d+47 = textureCount,
    # d+48..d+51 = pad, d+52..d+55 = height_count_x, d+56..d+59 = height_count_z,
    # d+60..d+63 = width (f32), d+64..d+67 = height (f32).
    dlc, blend = struct.unpack_from("<BB", data, d + 31)
    idx_l0, _, tex_count, _, _ = struct.unpack_from("<IIIII", data, d + 36)
    hcx, hcz = struct.unpack_from("<II", data, d + 52)
    width, height = struct.unpack_from("<ff", data, d + 60)
    if hcx == 0 or hcz == 0:
        raise ValueError(f"HFL has zero grid: hcx={hcx} hcz={hcz}")
    cursor = HEADER_BYTES
    if cursor + hcx * hcz * 4 > len(data):
        raise ValueError(f"HFL truncated heights: need {hcx*hcz*4}, have {len(data)-cursor}")
    heights = struct.unpack_from(f"<{hcx*hcz}f", data, cursor)
    return version, data[4:HEADER_BYTES], hcx, hcz, width, height, tex_count, heights


def discover_maps(map_dir: Path) -> set[int]:
    """Set of map ids that have a Map<N>.bmhm manifest in map_dir."""
    found: set[int] = set()
    for entry in map_dir.iterdir():
        if not entry.is_file():
            continue
        m = re.match(r"^Map(\d+)\.bmhm$", entry.name, re.IGNORECASE)
        if m:
            found.add(int(m.group(1)))
    return found


def synthesize_heights(map_id: int, hcx: int, hcz: int) -> list[float]:
    """Procedurally synthesize a per-map height grid so two map ids
    never collide.  Uses a tiny LCG seeded from the map id -- it is
    deterministic, dependency-free, and produces recognizable
    ridges/valleys at the placeholder scale.
    """
    state = (map_id * 2654435761) & 0xFFFFFFFF
    heights: list[float] = []
    for i in range(hcx * hcz):
        # Numerical Recipes LCG step
        state = (1664525 * state + 1013904223) & 0xFFFFFFFF
        n = state / 0xFFFFFFFF  # 0..1
        # Add a gentle dome so the silhouette is more interesting
        # than a flat sheet of random values.
        x = i % hcx
        z = i // hcx
        cx = (hcx - 1) / 2.0
        cz = (hcz - 1) / 2.0
        dome = 0.4 * max(0.0, 1.0 - ((x - cx) ** 2 + (z - cz) ** 2) / (cx * cx + cz * cz + 1))
        heights.append(float(n) * 0.6 + dome)
    return heights


def synthesize_placeholder(template: bytes, map_id: int) -> bytes:
    """Return a new HFL bytes object based on `template` but with
    per-map-id grid, width, and height mutated.
    """
    version, desc, hcx, hcz, width, height, tex_count, _heights = parse_hfl_header(template)

    # Mutate hcx/hcz (desc offsets 52, 56) and width/height (desc offsets
    # 60, 64).  The desc slice is data[4:HEADER_BYTES], so these offsets
    # match the d-relative offsets used by modern/src/hfl_height_field.cpp
    # exactly.  Earlier versions of this code wrote 48/56, which is
    # off-by-4 and was caught by the round-trip unit test.
    new_hcx = PLACEHOLDER_GRID
    new_hcz = PLACEHOLDER_GRID
    new_width = float(2000 + (map_id % 64) * 50)
    new_height = float(2000 + ((map_id * 7) % 64) * 50)
    new_desc = bytearray(desc)
    struct.pack_into("<II", new_desc, 52, new_hcx, new_hcz)
    struct.pack_into("<ff", new_desc, 60, new_width, new_height)

    # Synthesize the new height grid.
    heights = synthesize_heights(map_id, new_hcx, new_hcz)
    height_bytes = struct.pack(f"<{new_hcx * new_hcz}f", *heights)

    # Reassemble: version(4) + desc(108) + heights + texture table.
    out = bytearray()
    out += struct.pack("<I", version)
    out += bytes(new_desc)
    out += height_bytes

    # Copy the template's stored texture count and texture table
    # verbatim so parse_hfl's "invalid texture table" check still
    # passes (storedTextureCount == textureCount).
    cursor = HEADER_BYTES + hcx * hcz * 4
    if KEEP_TEMPLATE_TEXTURES and cursor < len(template):
        out += template[cursor:]
    return bytes(out)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(
        description="gen_hfl_placeholders -- synthesize missing <N>.hfl from a real template")
    ap.add_argument("--src", required=True,
                    help="Path to a real HFL to use as template (e.g. 10.hfl)")
    ap.add_argument("--out", required=True,
                    help="Target Resource/Map/ directory to write <N>.hfl into")
    ap.add_argument("--map-ids", default="",
                    help="Optional comma-separated map ids to override auto-discovery")
    ap.add_argument("--force", action="store_true",
                    help="Overwrite existing <N>.hfl files (use after a tool bug fix)")
    ap.add_argument("--dry-run", action="store_true",
                    help="Print what would be written; do not touch disk")
    args = ap.parse_args(argv)

    template_path = Path(args.src)
    if not template_path.is_file():
        print(f"[gen-hfl] template not found: {template_path}", file=sys.stderr)
        return 2
    template = template_path.read_bytes()
    try:
        _v, _d, hcx, hcz, w, h, tc, _hs = parse_hfl_header(template)
    except ValueError as e:
        print(f"[gen-hfl] template parse error: {e}", file=sys.stderr)
        return 2
    print(f"[gen-hfl] template={template_path} "
          f"grid={hcx}x{hcz} world={w:.0f}x{h:.0f} tex={tc}")

    out_dir = Path(args.out)
    if not out_dir.is_dir():
        print(f"[gen-hfl] out dir not found: {out_dir}", file=sys.stderr)
        return 2

    if args.map_ids:
        target_ids = sorted(int(s) for s in args.map_ids.split(",") if s.strip())
    else:
        target_ids = sorted(discover_maps(out_dir))

    # Skip maps that already have a real HFL on disk, unless --force
    # rewrites every map (used to repair placeholders emitted by an
    # earlier buggy version of this tool).
    todo: list[int] = []
    for mid in target_ids:
        if (not args.force) and (out_dir / f"{mid}.hfl").is_file():
            continue
        todo.append(mid)
    print(f"[gen-hfl] out={out_dir} maps={len(target_ids)} to_write={len(todo)} "
          f"force={args.force} dry_run={args.dry_run}")

    if args.dry_run:
        for mid in todo[:10]:
            print(f"[gen-hfl]   would write {mid}.hfl")
        if len(todo) > 10:
            print(f"[gen-hfl]   ... and {len(todo) - 10} more")
        return 0

    for mid in todo:
        out = synthesize_placeholder(template, mid)
        (out_dir / f"{mid}.hfl").write_bytes(out)
    print(f"[gen-hfl] wrote {len(todo)} placeholder HFL files")
    return 0


def main_with_args(argv: list[str]) -> int:
    """Test-friendly entry point: forwards argv to main()."""
    return main(argv)


if __name__ == "__main__":
    sys.exit(main())
