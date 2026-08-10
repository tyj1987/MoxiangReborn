#!/usr/bin/env python3
"""Validate the deterministic 3D content of an uncompressed TGA demo frame."""

from __future__ import annotations

import argparse
import struct
from collections import Counter
from pathlib import Path


def load_tga(path: Path) -> tuple[int, int, list[tuple[int, int, int]]]:
    data = path.read_bytes()
    if len(data) < 18:
        raise ValueError("file is shorter than a TGA header")
    id_len, color_map, image_type = data[0], data[1], data[2]
    width, height, depth = struct.unpack_from("<HHB", data, 12)
    if color_map != 0 or image_type != 2 or depth not in (24, 32):
        raise ValueError("expected an uncompressed 24/32-bit true-color TGA")
    bytes_per_pixel = depth // 8
    offset = 18 + id_len
    expected = width * height * bytes_per_pixel
    if len(data) - offset < expected:
        raise ValueError("pixel payload is truncated")
    pixels = []
    for index in range(offset, offset + expected, bytes_per_pixel):
        blue, green, red = data[index : index + 3]
        pixels.append((red, green, blue))
    return width, height, pixels


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("frame", type=Path)
    parser.add_argument("--inspect", action="store_true")
    args = parser.parse_args()

    try:
        width, height, pixels = load_tga(args.frame)
    except (OSError, ValueError) as exc:
        print(f"FAIL: {exc}")
        return 1

    background = (32, 32, 128)
    non_background = [
        color for color in pixels
        if max(abs(color[channel] - background[channel]) for channel in range(3)) > 12
    ]
    green = [
        color for color in pixels
        if color[1] >= 96 and color[1] > color[0] * 1.5 and color[1] > color[2] * 1.5
    ]
    x0, x1 = width // 3, width * 2 // 3
    y0, y1 = height // 4, height * 3 // 4
    central = [pixels[y * width + x] for y in range(y0, y1) for x in range(x0, x1)]
    central_object = [
        color for color in central
        if max(abs(color[channel] - background[channel]) for channel in range(3)) > 24
        and not (color[1] >= 96 and color[1] > color[0] * 1.5 and color[1] > color[2] * 1.5)
    ]
    dark_texels = sum(1 for red, green_value, blue in central_object
                      if max(red, green_value, blue) <= 80)
    bright_texels = sum(1 for red, green_value, blue in central_object
                        if min(red, green_value, blue) >= 110)

    checks = {
        "dimensions=800x600": (width, height) == (800, 600),
        "background-present": len(pixels) - len(non_background) >= len(pixels) // 2,
        "grid-present": len(green) >= 100,
        "cube-present": len(central_object) >= 1000,
        "checker-texture-present": dark_texels >= 1000 and bright_texels >= 1000,
        # The crossed backdrop contributes more than 1,100 green samples when
        # unobstructed. Fewer samples proves the foreground cube won the depth
        # test where the diagonals pass behind it.
        "cube-occludes-grid": 100 <= len(green) < 1100,
    }
    if args.inspect:
        print("top-colors:")
        for color, count in Counter(pixels).most_common(12):
            print(f"  {color}: {count}")
        print(f"non-background={len(non_background)} green={len(green)} central-object={len(central_object)} dark={dark_texels} bright={bright_texels}")
    for name, passed in checks.items():
        print(f"{'PASS' if passed else 'FAIL'}: {name}")
    return 0 if all(checks.values()) else 1


if __name__ == "__main__":
    raise SystemExit(main())
