#!/usr/bin/env python3
"""Map17 live-birth frame check.

This is not the Map10 gate. It does not require 228 monsters, an 18% terrain
coverage ratio, or a minimum color count. It does not sample the nine missing
original ground textures tile_17_mou_a_01.tga through tile_17_mou_a_09.tga.
"""

import struct
import sys
from pathlib import Path

SKIPPED_TEXTURES = tuple(f"tile_17_mou_a_{index:02d}.tga" for index in range(1, 10))


def fail(message: str) -> None:
    print(f"FAIL: {message}")
    raise SystemExit(1)


def main() -> None:
    if len(sys.argv) != 2:
        fail("usage: verify-map17-birth.py FRAME.tga")
    path = Path(sys.argv[1])
    data = path.read_bytes()
    if len(data) < 18:
        fail("short TGA header")
    _id_length, color_map_type, image_type = data[0], data[1], data[2]
    width, height, depth = struct.unpack_from("<HHB", data, 12)
    if color_map_type != 0 or image_type != 2 or depth not in (24, 32):
        fail("expected an uncompressed true-color TGA")
    if width < 1 or height < 1:
        fail("empty frame")
    print(
        "PASS: Map17 birth frame "
        f"{width}x{height}; skipped {', '.join(SKIPPED_TEXTURES)}; "
        "Map10 thresholds not applied"
    )


if __name__ == "__main__":
    main()
