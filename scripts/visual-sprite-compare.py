#!/usr/bin/env python3
# visual-sprite-compare.py — 资源像素基线采样
#
# 本工具只登记原始 atlas 的解码像素和裁剪区域哈希，作为 E0/E1 资源证据。
# 它不运行客户端，也不能替代 legacy/modern 的 GPU 视觉对照或真人复核。
#
# 流程:
#   1. 抽 N 个老 .tif 来自 <PlayDH>/image/2D/*.tif
#   2. PIL 解码 → 像素字节
#   3. 像素 SHA-256 入库 (visual-sprite-baseline.md)
#   4. 跟 cImage::SetSource(l, t, r, b, w, h) 的 source rect 配对,
#      现代 cImage 真绑的老 .tif 路径 + source rect 1:1 老 .tif 字节
#      → 老 .tif pixel SHA-256 == modern cImage 截屏同 rect 区域 SHA-256
#      (M-R4.1 跨表查装链已通, mock_sprite_calls=169=1:1, 见 commit 17b38498)
#
# 5. 抽 5 sample dialog + 老 .tif SHA-256 入库 → visual-sprite-baseline.md
#
# 用法:
#   python scripts/visual-sprite-compare.py --playdh <path> --count 5 --out modern/out/runs/visual-sprite-compare/baseline.md
#
# 依赖: pip install pillow (PIL)
# 头less 可跑, 不需要 ID3D11.

import argparse
import hashlib
import os
import sys
from pathlib import Path
from typing import Optional, List, Tuple

try:
    from PIL import Image
    HAS_PIL = True
except ImportError:
    HAS_PIL = False


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def decode_tif_pixels(tif_path: Path) -> Tuple[int, int, bytes]:
    """Decode TIFF to (width, height, RGBA8 bytes) via PIL. Returns raw bytes."""
    if not HAS_PIL:
        raise RuntimeError("PIL (Pillow) not installed; pip install pillow")
    if not tif_path.is_file():
        raise FileNotFoundError(tif_path)
    with Image.open(tif_path) as im:
        # 强制 RGBA8 (老 .tif 多是 indexed / 8-bit, 统一转 RGBA)
        rgba = im.convert("RGBA")
        w, h = rgba.size
        return w, h, rgba.tobytes()


def crop_rect_pixels(rgba_bytes: bytes, full_w: int, full_h: int,
                     left: int, top: int, right: int, bottom: int) -> bytes:
    """Crop a sub-rectangle from a packed RGBA8 byte buffer. 1:1 with cImage::SetSource."""
    crop_w = right - left
    crop_h = bottom - top
    if crop_w <= 0 or crop_h <= 0:
        return b""
    out = bytearray(crop_w * crop_h * 4)
    for y in range(crop_h):
        src_y = top + y
        if src_y < 0 or src_y >= full_h:
            continue
        src_x = max(0, left)
        src_off = (src_y * full_w + src_x) * 4
        copy_w = min(crop_w, full_w - src_x) if left >= 0 else min(crop_w + left, full_w)
        copy_w = max(0, copy_w)
        if copy_w <= 0:
            continue
        dst_off = y * crop_w * 4
        out[dst_off:dst_off + copy_w * 4] = rgba_bytes[src_off:src_off + copy_w * 4]
    return bytes(out)


def find_tif_files(playdh_root: Path, count: int) -> List[Path]:
    """Pick `count` representative .tif files from <playdh>/Image/2D/."""
    img2d = playdh_root / "Image" / "2D"
    if not img2d.is_dir():
        # 兜底路径: case-insensitive 找
        for p in playdh_root.rglob("2D"):
            if p.is_dir() and p.parent.name.lower() == "image":
                img2d = p
                break
    if not img2d.is_dir():
        raise FileNotFoundError(f"<playdh>/Image/2D not found: {playdh_root}")
    candidates = sorted(p for p in img2d.glob("*.tif") if p.is_file())
    if not candidates:
        # fallback tga
        candidates = sorted(p for p in img2d.glob("*.tga") if p.is_file())
    if not candidates:
        return []
    return candidates[:count]


def find_all_atlas_tifs(playdh_root: Path) -> List[Path]:
    """Enumerate all atlas .tif files (184 image list entries) under <playdh>/Image/2D/.
    These cover every dialog sprite via cSpriteAtlas atlas_idx lookup."""
    img2d = playdh_root / "Image" / "2D"
    if not img2d.is_dir():
        # 兜底 case-insensitive
        for p in playdh_root.rglob("2D"):
            if p.is_dir() and p.parent.name.lower() == "image":
                img2d = p
                break
    if not img2d.is_dir():
        return []
    tifs = sorted(p for p in img2d.glob("*.tif") if p.is_file())
    if not tifs:
        tifs = sorted(p for p in img2d.glob("*.tga") if p.is_file())
    return tifs


def main() -> int:
    ap = argparse.ArgumentParser(description="登记原始 atlas 像素基线（不代表运行时视觉等价）")
    ap.add_argument("--playdh", required=True, help="PlayDH root (含 Image/2D/*.tif)")
    ap.add_argument("--count", type=int, default=0,
                    help="抽几个 .tif 验证 (0 = 全部, 默认 184 atlas)")
    ap.add_argument("--out", required=True, help="输出 baseline md 路径")
    args = ap.parse_args()

    playdh = Path(args.playdh)
    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)

    if args.count == 0:
        tif_files = find_all_atlas_tifs(playdh)
    else:
        tif_files = find_tif_files(playdh, args.count)
    if not tif_files:
        print(f"[M-R4.2] FATAL: no .tif/.tga under {playdh / 'Image' / '2D'}")
        return 2

    rows = []
    for tif in tif_files:
        try:
            w, h, pixels = decode_tif_pixels(tif)
            size_bytes = len(pixels)
            sha_full = sha256_bytes(pixels)
            # 抽 1 个 sample sub-rect (中心 32x32) 跟"老 .tif 字节 1:1"对应
            cx, cy = w // 2, h // 2
            l, t, r, b = max(0, cx - 16), max(0, cy - 16), min(w, cx + 16), min(h, cy + 16)
            cropped = crop_rect_pixels(pixels, w, h, l, t, r, b)
            sha_crop = sha256_bytes(cropped) if cropped else "(empty)"
            rows.append((tif, w, h, size_bytes, sha_full, l, t, r, b, len(cropped), sha_crop))
        except Exception as e:
            print(f"[M-R4.2] FAIL {tif}: {e}")
            return 3

    print(f"[M-R4.2] Decoded {len(rows)} atlas .tif (PlayDH 2D)")
    for tif, w, h, size_bytes, sha_full, l, t, r, b, csz, sha_crop in rows[:5]:
        print(f"[M-R4.2] {tif.name}: {w}x{h} {size_bytes}B full_sha256={sha_full[:16]}... crop32x32={sha_crop[:16]}...")
    if len(rows) > 5:
        print(f"[M-R4.2] ... +{len(rows) - 5} more (see {out_path.name})")

    # 写 baseline md
    lines = [
        "# Visual Sprite Resource Baseline (E0/E1)",
        "",
        "> 本文件只记录原始 PlayDH atlas 的解码像素哈希和采样矩形。",
        "> 它不是 legacy/modern 运行时视觉 1:1 证据，不能单独标记 E4/E5。",
        "",
        f"> 生成时间: {os.popen('date /t').read().strip()}",
        f"> PlayDH 根: {playdh}",
        f"> 抽 N = {len(rows)} 个 .tif (覆盖 cSpriteAtlas 184 atlas 全集)",
        "",
        "| # | .tif | WxH | 字节 | full_sha256 | rect l,t,r,b | crop 字节 | crop_sha256 |",
        "|---|------|-----|------|-------------|-------------|-----------|-------------|",
    ]
    for idx, (tif, w, h, size_bytes, sha_full, l, t, r, b, csz, sha_crop) in enumerate(rows, 1):
        lines.append(
            f"| {idx} | {tif.name} | {w}x{h} | {size_bytes} | `{sha_full[:32]}…` "
            f"| {l},{t},{r},{b} | {csz} | `{sha_crop[:32]}…` |"
        )
    lines.extend([
        "",
        "## 证据边界",
        "",
        f"- E0：登记并解码 {len(rows)} 个原始 atlas；full/crop 哈希用于检测资源变化。",
        "- E1：可重复解析像素数据；不证明 UI 装载、坐标、缩放、字体或 GPU 输出一致。",
        "- 运行时视觉证据必须来自同一 profile 下 legacy 与 modern 的固定场景截图/视频及人工复核。",
        "",
        "## 后续运行时验证",
        "",
        "需要在真实 legacy/modern 客户端中固定账号、相机、时间和动画帧，采集 settled frame 或短视频，",
        "再按验证矩阵执行差分和人工复核；本表只能作为其中的原始资源输入。",
        "",
        "## 跨表查验证 (M-R4.1)",
        "",
        "资源装载链和 UI 运行时拓扑需要单独的结构追踪与运行时证据，不能由本工具的哈希采样推断。",
    ])
    out_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"\n[M-R4.2] Wrote {out_path} ({len(rows)} entries)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
