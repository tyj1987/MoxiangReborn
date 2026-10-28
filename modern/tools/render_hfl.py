#!/usr/bin/env python3
"""
render_hfl.py — Decode a 4Dyuchi .hfl height field and render a top-down
                heightmap PNG.  Pure Python, no GPU/PlayDH required, so we
                can prove the resource is real without launching the client.

Usage:
  python render_hfl.py <input.hfl> <output.png> [downscale=4]
"""

from __future__ import annotations

import argparse
import struct
import sys
import zlib
from pathlib import Path


def parse_hfl(path: Path):
    """Return (height_count_x, height_count_z, heights) or raise."""
    data = path.read_bytes()
    if len(data) < 4 + 108:
        raise ValueError(f"HFL too short: {len(data)} bytes")
    (version,) = struct.unpack_from("<I", data, 0)
    if version == 0 or version > 0x10:
        raise ValueError(f"unsupported HFL version {version}")
    d = 4
    # 5 floats at d+0..d+16, then 3 u32, 1 u8 dlc, 1 u8 blend, 2 pad
    left, top, right, bottom, face_size, *_ = struct.unpack_from("<Ifffff", data, d)
    fpoa, ocx, ocz, dlc, _blend_pad, idx_l0, _pad, _pad2, tex_count, _pad3 = struct.unpack_from(
        "<IIIBBIIIII", data, d + 20)
    # explicit re-reads because struct doesn't allow loose unpacking
    fpoa, ocx, ocz = struct.unpack_from("<III", data, d + 20)
    dlc, blend = struct.unpack_from("<BB", data, d + 32)
    idx_l0, _, _, tex_count, _ = struct.unpack_from("<IIIII", data, d + 36)
    hcx, hcz = struct.unpack_from("<II", data, d + 52)
    width, height = struct.unpack_from("<ff", data, d + 60)
    if hcx == 0 or hcz == 0:
        raise ValueError(f"HFL has zero grid: hcx={hcx} hcz={hcz}")
    cursor = 4 + 108
    if cursor + hcx * hcz * 4 > len(data):
        raise ValueError(f"HFL truncated heights: need {hcx*hcz*4} bytes, have {len(data)-cursor}")
    heights = struct.unpack_from(f"<{hcx*hcz}f", data, cursor)
    return hcx, hcz, heights, width, height


def render(hcx, hcz, heights, width, height, downscale: int, out_path: Path):
    out_w = max(1, hcx // downscale)
    out_h = max(1, hcz // downscale)
    # Find min/max for normalization
    h_min = min(heights)
    h_max = max(heights)
    span = max(1e-3, h_max - h_min)
    # Pre-compute 8-bit palette: height -> RGB(0..255, 0..255, 0..255)
    raw = bytearray(out_w * out_h * 3)
    for oy in range(out_h):
        sy_lo = oy * downscale
        sy_hi = min(hcz, sy_lo + downscale)
        for ox in range(out_w):
            sx_lo = ox * downscale
            sx_hi = min(hcx, sx_lo + downscale)
            acc = 0.0
            count = 0
            for sy in range(sy_lo, sy_hi):
                for sx in range(sx_lo, sx_hi):
                    acc += heights[sy * hcx + sx]
                    count += 1
            h_avg = acc / max(1, count)
            t = (h_avg - h_min) / span
            # green low -> tan -> brown -> white snow caps
            r = int(min(255, max(0, t * 510 - 255)))  # 0..255 by t
            g = int(min(255, max(0, t * 360)))
            b = int(min(255, max(0, 64 + (1 - abs(t - 0.5) * 2) * 64)))
            # actually a simpler terrain palette
            r = int(min(255, 80 + t * 175))
            g = int(min(255, 40 + t * 200))
            b = int(min(255, 20 + t * 120))
            i = (oy * out_w + ox) * 3
            raw[i] = r
            raw[i + 1] = g
            raw[i + 2] = b
    # write a minimal 24-bit BMP so we don't depend on Pillow
    write_bmp(out_path, out_w, out_h, bytes(raw))
    return out_w, out_h


def write_bmp(path: Path, w: int, h: int, bgr: bytes) -> None:
    # BMP rows are padded to 4 bytes; pixels are BGR.
    row_bytes = (w * 3 + 3) & ~3
    pixel_bytes = row_bytes * h
    file_size = 14 + 40 + pixel_bytes
    header = b"BM" + struct.pack("<IHHI", file_size, 0, 0, 14 + 40)
    dib = struct.pack(
        "<IIIHHIIIIII",
        40, w, h, 1, 24, 0, pixel_bytes, 2835, 2835, 0, 0)
    body = bytearray()
    for y in range(h - 1, -1, -1):
        row = bgr[y * w * 3:(y + 1) * w * 3]
        body.extend(row)
        body.extend(b"\x00" * (row_bytes - w * 3))
    path.write_bytes(header + dib + bytes(body))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("input", help="input .hfl path")
    ap.add_argument("output", help="output .bmp path")
    ap.add_argument("--downscale", type=int, default=4)
    args = ap.parse_args()
    in_path = Path(args.input)
    out_path = Path(args.output)
    hcx, hcz, heights, width, height = parse_hfl(in_path)
    w, h = render(hcx, hcz, heights, width, height, args.downscale, out_path)
    print(f"[render_hfl] {in_path} -> {out_path}: {hcx}x{hcz} -> {w}x{h}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
