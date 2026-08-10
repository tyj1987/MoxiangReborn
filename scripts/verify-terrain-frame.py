#!/usr/bin/env python3
"""Validate that a captured client frame contains a substantial terrain view."""

import struct
import sys
from pathlib import Path


def fail(message: str) -> None:
    print(f"FAIL: {message}")
    raise SystemExit(1)


path = Path(sys.argv[1])
data = path.read_bytes()
if len(data) < 18:
    fail("short TGA header")

id_length, color_map_type, image_type = data[0], data[1], data[2]
width, height, depth = struct.unpack_from("<HHB", data, 12)
if color_map_type != 0 or image_type != 2 or depth not in (24, 32):
    fail("expected an uncompressed true-color TGA")

stride = depth // 8
pixels = data[18 + id_length :]
if len(pixels) != width * height * stride:
    fail("pixel payload size does not match the header")

background = tuple(pixels[:3])
foreground = 0
green_probe = 0
colors = set()
for offset in range(0, len(pixels), stride):
    bgr = tuple(pixels[offset : offset + 3])
    if max(abs(bgr[i] - background[i]) for i in range(3)) > 12:
        foreground += 1
        if len(colors) < 4096:
            colors.add(tuple(component // 8 for component in bgr))
    if bgr[1] > 240 and bgr[0] < 20 and bgr[2] < 20:
        green_probe += 1

coverage = foreground / (width * height)
if not 0.18 <= coverage <= 0.75:
    fail(f"terrain coverage out of range: {coverage:.1%}")
if len(colors) < 100:
    fail(f"terrain color diversity is too low: {len(colors)}")
if green_probe > width:
    fail(f"diagnostic green geometry remains: {green_probe} pixels")

print(f"PASS: terrain frame {width}x{height}, coverage={coverage:.1%}, colors={len(colors)}")
