#!/usr/bin/env python3
"""visual-compare.py — M-R0/M-R4 视觉对比

对比两张 TGA 截图（modern vs legacy 或 modern baseline vs M-R4 之后），输出 SSIM + 直方图差异。

usage:
  python scripts/visual-compare.py modern.tga legacy.tga
  python scripts/visual-compare.py modern.tga legacy.tga --threshold 0.95
  python scripts/visual-compare.py modern.tga legacy.tga --json result.json

返回值:
  0: SSIM >= threshold (通过)
  1: SSIM < threshold (失败)
  2: 文件/解析错误
"""
import argparse, json, struct, sys
from pathlib import Path
from typing import Tuple

# 内置 TGA 解码（TGA = Truevision Graphics Adapter，modern MoxianClient 自己的 .tga 输出）
def load_tga_pixels(path: Path) -> Tuple[int, int, list]:
    data = path.read_bytes()
    if len(data) < 18:
        raise ValueError(f"short TGA: {len(data)} bytes")
    # TGA header (18 bytes):
    #   byte 0: ID length
    #   byte 1: color map type
    #   byte 2: image type (2 = uncompressed true-color)
    #   bytes 3-5: color map spec
    #   bytes 6-7: x origin
    #   bytes 8-9: y origin
    #   bytes 10-11: width
    #   bytes 12-13: height
    #   byte 14: pixel depth (24/32)
    #   byte 15: image descriptor
    if data[1] != 0 or data[2] != 2 or data[16] not in (24, 32):
        raise ValueError(f"not uncompressed true-color TGA (cmap={data[1]} type={data[2]} depth={data[16]})")
    width, height = struct.unpack_from("<HH", data, 12)
    depth = data[16]
    bytes_per_pixel = depth // 8
    offset = 18 + data[0]
    pixels = []
    for i in range(offset, offset + width * height * bytes_per_pixel, bytes_per_pixel):
        b, g, r = data[i], data[i+1], data[i+2]
        pixels.append((r, g, b))
    return width, height, pixels

def rgb_to_gray(r, g, b):
    return int(0.299 * r + 0.587 * g + 0.114 * b)

def downsample(pixels, w, h, factor):
    """简单下采样（块平均）"""
    new_w, new_h = w // factor, h // factor
    out = []
    for by in range(new_h):
        for bx in range(new_w):
            r_sum = g_sum = b_sum = 0
            for dy in range(factor):
                for dx in range(factor):
                    r, g, b = pixels[(by*factor + dy)*w + bx*factor + dx]
                    r_sum += r; g_sum += g; b_sum += b
            out.append((r_sum // (factor*factor), g_sum // (factor*factor), b_sum // (factor*factor)))
    return new_w, new_h, out

def ssim_simple(a, b, w, h, window=8):
    """简化版 SSIM（无 luminance/chroma 分离，仅 luminance+structure）

    完整 SSIM 见 scikit-image；这里用 8x8 块平均实现一个轻量近似。
    """
    if len(a) != len(b):
        raise ValueError("pixel count mismatch")
    total = 0
    count = 0
    C1 = (0.01 * 255) ** 2
    C2 = (0.03 * 255) ** 2
    for by in range(0, h, window):
        for bx in range(0, w, window):
            xs = []
            ys = []
            for dy in range(window):
                for dx in range(window):
                    x = bx + dx
                    y = by + dy
                    if x >= w or y >= h: continue
                    idx = y * w + x
                    r1, g1, b1 = a[idx]
                    r2, g2, b2 = b[idx]
                    xs.append(0.299*r1 + 0.587*g1 + 0.114*b1)
                    ys.append(0.299*r2 + 0.587*g2 + 0.114*b2)
            if not xs: continue
            n = len(xs)
            mx = sum(xs) / n
            my = sum(ys) / n
            vx = sum((x-mx)**2 for x in xs) / n
            vy = sum((y-my)**2 for y in ys) / n
            vxy = sum((xs[i]-mx)*(ys[i]-my) for i in range(n)) / n
            num = (2*mx*my + C1) * (2*vxy + C2)
            den = (mx*mx + my*my + C1) * (vx + vy + C2)
            if den == 0:
                total += 1.0
            else:
                total += num / den
            count += 1
    return total / count if count else 0.0

def histogram_distance(a, b, bins=16):
    """RGB 直方图差异（0 = 一致，1 = 完全不同）"""
    def hist(pixels):
        h = [0] * (bins * bins * bins)
        for r, g, b in pixels:
            h[((r*bins//256) * bins + (g*bins//256)) * bins + (b*bins//256)] += 1
        total = sum(h)
        return [c/total for c in h] if total else h
    ha = hist(a); hb = hist(b)
    return sum(abs(ha[i] - hb[i]) for i in range(len(ha))) / 2  # total variation distance

def main():
    p = argparse.ArgumentParser()
    p.add_argument("modern", type=Path)
    p.add_argument("legacy", type=Path)
    p.add_argument("--threshold", type=float, default=0.95,
                   help="SSIM threshold for pass (default 0.95)")
    p.add_argument("--downsample", type=int, default=2,
                   help="downsample factor (default 2, faster SSIM)")
    p.add_argument("--json", type=Path,
                   help="write result to JSON file")
    args = p.parse_args()

    if not args.modern.exists():
        print(f"FAIL: missing {args.modern}", file=sys.stderr)
        return 2
    if not args.legacy.exists():
        print(f"FAIL: missing {args.legacy}", file=sys.stderr)
        return 2

    try:
        w1, h1, p1 = load_tga_pixels(args.modern)
        w2, h2, p2 = load_tga_pixels(args.legacy)
    except (OSError, ValueError) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 2

    if (w1, h1) != (w2, h2):
        # 不一致：尝试用小的那张做对比（或直接 fail）
        print(f"FAIL: size mismatch {w1}x{h1} vs {w2}x{h2}", file=sys.stderr)
        return 1

    if args.downsample > 1:
        _, _, p1s = downsample(p1, w1, h1, args.downsample)
        _, _, p2s = downsample(p2, w2, h2, args.downsample)
        sw, sh = w1 // args.downsample, h1 // args.downsample
    else:
        p1s, p2s, sw, sh = p1, p2, w1, h1

    ssim = ssim_simple(p1s, p2s, sw, sh, window=8)
    hist_dist = histogram_distance(p1s, p2s)

    result = {
        "modern": str(args.modern),
        "legacy": str(args.legacy),
        "width": w1, "height": h1,
        "ssim": round(ssim, 4),
        "histogram_distance": round(hist_dist, 4),
        "threshold": args.threshold,
        "passed": ssim >= args.threshold
    }
    output = json.dumps(result, indent=2)
    if args.json:
        args.json.write_text(output)
    else:
        print(output)

    return 0 if result["passed"] else 1

if __name__ == "__main__":
    raise SystemExit(main())
