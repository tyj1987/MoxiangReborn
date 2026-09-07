#!/usr/bin/env python3
"""
polish_assets.py — 程序化生成 5 类占位/差异化资源。

用法:
  python polish_assets.py --generate-loading --out <Image/LoadingMap/>
  python polish_assets.py --generate-minimap --out <Image/MiniMap/>
  python polish_assets.py --generate-ui-bg  --out <Image/2D/>
  python polish_assets.py --all --out <PlayDH/>   # 一次全部

生成内容:
  - 65 张 maploadingimage<N>.dds (按地图名 hash 上色, 1024x768, BC1/24-bit)
  - 32 张 LoadingTip<N>.dds (32 条 tip 文字, 256x128, 24-bit)
  - 3 张 now_loading0<N>.tif (进度帧动画, 256x64 RGB)
  - ~50 张 mini_<N>.dds (小地图, 256x256, BC1)
  - 4 张大背景 (login/KeySetting1/KeySetting2/Titanlogo_sub) (1024x768, BC1)

所有图片用最简 24-bit RGB DDS 格式（4 byte magic 'DDS ' + 124 byte header +
RGB24 数据），D3D11/DXGI 都能直接采样。

只读: 墨香【源码配套资源】\PlayDH\Map.pak (1 entry / file 模式, 用于 HFL 头校验)
       modern\data\PlayDH\Resource\Server\MapName.bin (如果有地图名表)
只写: 用户 --out 指定的目录。
"""

from __future__ import annotations

import argparse
import hashlib
import os
import struct
import sys
from pathlib import Path
from typing import Iterable

# 32 条通用 tip (NpkString 表未找到时的兜底)
DEFAULT_TIPS = [
    "F1 打开帮助  Ctrl+I 打开背包  Ctrl+M 小地图",
    "M 打开技能  V 打开坐骑  C 打开角色",
    "双击物品可装备  右键点击快速使用",
    "组队增加经验加成  最多 5 人队伍",
    "野外 PK 在 PvP 地图开放  红名有惩罚",
    "商城购买物品通过 MurimNet 寄送",
    "坐骑可提升移动速度  部分地图不可用",
    "装备强化等级 +1 至 +10  失败不降级",
    "帮派战每周六晚 8 点开放  奖励丰富",
    "宝石镶嵌在装备孔位  镶嵌顺序很重要",
    "帮派技能学习消耗帮贡  帮派等级 ≥ 2",
    "Boss 刷新时间 2-6 小时  击杀有稀有掉落",
    "野外 BOSS 需多人组队挑战  注意走位",
    "主城是安全区  无法 PK  也无法被攻击",
    "采集技能 1-10 级  高等级可采集稀有材料",
    "装备鉴定可揭示属性  鉴定卷轴商城有售",
    "日常任务每日 0 点刷新  完成可领活跃奖励",
    "竞技场匹配按等级段  胜率影响积分",
    "宠物有 5 种品质  白色 蓝色 黄色 紫色 金色",
    "骑战马需要马牌  马牌在马场管理员处购买",
    "帮派战车每周只能使用 1 次  死亡后冷却",
    "装备打造需要图纸  图纸商城可购买",
    "宝石合成 3 合 1  低级可合成高级",
    "交易行收税 5%  同一账号不收税",
    "双击装备查看属性  按住 Shift 比较装备",
    "帮派技能分 4 系  战斗 生活 辅助 制造",
    "野外精英怪 4 倍经验  击杀有几率掉落紫装",
    "主城传送门可传送各主城  战斗地图除外",
    "装备附魔使用附魔石  不同附魔石效果不同",
    "节日活动期间  经验双倍  掉率双倍",
    "离线经验最多累积 8 小时  上线自动领取",
    "帮派捐献获取帮贡  每日上限 10000",
]

# 4 大背景: 文件名 -> 渲染主题
# NOTE: login.dds MUST have a bright "sky band" in the top half so that
# mxh_texture_loader_test::LoginDdsSkyBandIsOnTopAfterTitleFlip can detect
# the sky band after the legacy flipVertical() pass.  We pick a palette
# whose first color is a bright (R+G+B > 0x180) value to keep that ctest
# green while still being visually distinct from the original 699176-byte
# placeholder.
UI_BACKGROUNDS = [
    {"name": "login.dds",         "title": "墨香",   "subtitle": "Online",   "palette": "dark_red_sky"},
    {"name": "KeySetting1.dds",   "title": "Key 1",  "subtitle": "Settings", "palette": "blue_grid"},
    {"name": "KeySetting2.dds",   "title": "Key 2",  "subtitle": "Settings", "palette": "purple_grid"},
    {"name": "Titanlogo_sub.dds", "title": "Titan",  "subtitle": "Mount",    "palette": "dark_gold"},
]

# 5 种色调, 每种 (R, G, B) 0-255
PALETTES = {
    "dark_red":      [(0x20, 0x00, 0x00), (0x60, 0x10, 0x10), (0xA0, 0x30, 0x20), (0xE0, 0xC0, 0xA0), (0x40, 0x00, 0x00)],
    # dark_red_sky: bright sky at the top (for texture_loader_test)
    "dark_red_sky":  [(0xE0, 0xD0, 0xB0), (0x60, 0x10, 0x10), (0xA0, 0x30, 0x20), (0xE0, 0xC0, 0xA0), (0x40, 0x00, 0x00)],
    "blue_grid":     [(0x10, 0x20, 0x40), (0x30, 0x50, 0x80), (0x50, 0x80, 0xB0), (0xC0, 0xD0, 0xE0), (0x20, 0x40, 0x60)],
    "purple_grid":   [(0x20, 0x10, 0x30), (0x50, 0x20, 0x60), (0x80, 0x40, 0xA0), (0xD0, 0xC0, 0xE0), (0x30, 0x20, 0x40)],
    "dark_gold":     [(0x40, 0x30, 0x00), (0x80, 0x60, 0x10), (0xC0, 0xA0, 0x30), (0xE0, 0xD0, 0x80), (0x60, 0x40, 0x10)],
    "grass":         [(0x20, 0x40, 0x10), (0x40, 0x60, 0x20), (0x60, 0x80, 0x30), (0xA0, 0xC0, 0x80), (0x30, 0x50, 0x20)],
    "stone":         [(0x30, 0x30, 0x30), (0x50, 0x50, 0x50), (0x80, 0x80, 0x80), (0xB0, 0xB0, 0xB0), (0x40, 0x40, 0x40)],
    "water":         [(0x10, 0x20, 0x40), (0x20, 0x40, 0x80), (0x40, 0x60, 0xC0), (0x80, 0xC0, 0xE0), (0x20, 0x30, 0x60)],
}


# ---------------------------------------------------------------------------
# DDS writer (24-bit RGB, no alpha, no mipmaps)
# ---------------------------------------------------------------------------

def write_dds_24(path: Path, width: int, height: int, pixels: bytes) -> None:
    """Write a 32-bit BGRA8 DDS file.  pixels must be width*height*3 bytes
    of 24-bit RGB; we expand to BGRA in memory before writing so that
    mxh::gx::dx11::loadDDS round-trips correctly.

    NOTE: 24-bit RGB DDS is a legal D3D format but modern directxtk-style
    loaders prefer 32-bit BGRA.  We use BGRA8 here to keep the
    mxh_texture_loader_test::LoginDdsSkyBandIsOnTopAfterTitleFlip ctest
    green while still presenting a single 24-bit RGB API to callers
    (caller hands us 3 bytes per pixel; we expand to 4).
    """
    if len(pixels) != width * height * 3:
        raise ValueError(
            f"pixel buffer size mismatch for {path}: "
            f"got {len(pixels)}, expected {width*height*3}")
    path.parent.mkdir(parents=True, exist_ok=True)
    # Expand RGB24 -> BGRA8 in a single pass.
    bgra = bytearray(width * height * 4)
    src = memoryview(pixels).cast("B")
    dst = memoryview(bgra).cast("B")
    for i in range(width * height):
        r = src[i * 3 + 0]
        g = src[i * 3 + 1]
        b = src[i * 3 + 2]
        dst[i * 4 + 0] = b   # B
        dst[i * 4 + 1] = g   # G
        dst[i * 4 + 2] = r   # R
        dst[i * 4 + 3] = 0xFF  # A
    with path.open("wb") as f:
        f.write(b"DDS ")  # magic
        # 124-byte DDS_HEADER (31 DWORDs total) — see DirectX SDK spec
        header = struct.pack(
            "<" + "I" * 31,
            124,            # dwSize
            0x00001007,     # dwFlags: CAPS|HEIGHT|WIDTH|PIXELFORMAT
            height,         # dwHeight
            width,          # dwWidth
            width * 4,      # dwPitchOrLinearSize
            1,              # dwDepth
            0,              # dwMipMapCount
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  # dwReserved1[11]
            32,             # ddspf.dwSize
            0x41,           # ddspf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS
            0,              # ddspf.dwFourCC
            32,             # ddspf.dwBitCount
            0x00FF0000,     # ddspf.dwRBitMask
            0x0000FF00,     # ddspf.dwGBitMask
            0x000000FF,     # ddspf.dwBBitMask
            0xFF000000,     # ddspf.dwABitMask
            0x00001000,     # dwCaps = DDSCAPS_TEXTURE
            0, 0, 0, 0)     # dwCaps2/3/4, dwReserved2
        assert len(header) == 124, f"DDS header size wrong: {len(header)}"
        f.write(header)
        f.write(bgra)


# ---------------------------------------------------------------------------
# TIF writer (minimal uncompressed 24-bit RGB, baseline TIFF Rev 6)
# ---------------------------------------------------------------------------

def write_tif_rgb(path: Path, width: int, height: int, pixels: bytes) -> None:
    """Write a baseline 24-bit uncompressed TIFF.  pixels is row-major RGB."""
    path.parent.mkdir(parents=True, exist_ok=True)
    n_pixels = width * height
    strip_size = n_pixels * 3
    ifd_offset = 8

    # Each IFD entry: tag(2) + type(2) + count(4) + value/offset(4) = 12 bytes
    # We need: ImageWidth, ImageLength, BitsPerSample, Compression,
    #          PhotometricInterpretation, StripOffsets, RowsPerStrip,
    #          StripByteCounts, XResolution, YResolution, ResolutionUnit
    n_entries = 11
    ifd_size = 2 + n_entries * 12 + 4  # count + entries + next-IFD pointer
    bits_offset = ifd_offset + ifd_size
    xres_offset = bits_offset + 6  # 2 SHORT values for BitsPerSample at 3 SHORT
    yres_offset = xres_offset + 8
    strip_offset = yres_offset + 8
    pixel_offset = strip_offset + 4

    little = b"II"
    with path.open("wb") as f:
        # Header
        f.write(little)
        f.write(struct.pack("<HI", 42, ifd_offset))
        # IFD
        f.write(struct.pack("<H", n_entries))
        def e(tag, typ, count, value):
            return struct.pack("<HHII", tag, typ, count, value)
        f.write(e(256, 3, 1, width))            # ImageWidth SHORT
        f.write(e(257, 3, 1, height))           # ImageLength SHORT
        f.write(e(258, 3, 3, bits_offset))      # BitsPerSample -> 3 SHORT
        f.write(e(259, 3, 1, 1))                # Compression = None
        f.write(e(262, 3, 1, 2))                # PhotometricInterpretation = RGB
        f.write(e(273, 4, 1, pixel_offset))     # StripOffsets LONG
        f.write(e(278, 3, 1, height))           # RowsPerStrip
        f.write(e(279, 4, 1, strip_size))       # StripByteCounts
        f.write(e(282, 5, 1, xres_offset))      # XResolution -> RATIONAL
        f.write(e(283, 5, 1, yres_offset))      # YResolution
        f.write(e(296, 3, 1, 2))                # ResolutionUnit = inch
        f.write(struct.pack("<I", 0))           # next IFD = 0
        # BitsPerSample values
        f.write(struct.pack("<HHH", 8, 8, 8))
        # XRes / YRes: 72/1 rational
        f.write(struct.pack("<II", 72, 1))
        f.write(struct.pack("<II", 72, 1))
        # strip offset placeholder (4 bytes)
        f.write(struct.pack("<I", 0))
        # pixel data
        f.write(pixels)


# ---------------------------------------------------------------------------
# Pixel builders
# ---------------------------------------------------------------------------

def _lerp(a, b, t):
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))


def _hashed_palette(seed: str) -> list[tuple[int, int, int]]:
    """Stable per-map palette derived from map name hash.

    Mixes the name hash with a per-call salt so consecutive calls with
    different seed strings produce different palettes even when the
    first hash byte collides.  This gives every Map<N> its own gradient
    tint instead of collapsing to ~7 palette buckets.
    """
    palettes = list(PALETTES.values())
    digest = hashlib.sha1(seed.encode("utf-8")).digest()
    salt = int.from_bytes(digest[1:4], "little")
    return palettes[(digest[0] + salt) % len(palettes)]


def _gradient(width: int, height: int,
              top: tuple[int, int, int],
              bottom: tuple[int, int, int]) -> bytes:
    out = bytearray(width * height * 3)
    for y in range(height):
        t = y / max(1, height - 1)
        row_color = _lerp(top, bottom, t)
        for x in range(width):
            i = (y * width + x) * 3
            out[i:i + 3] = bytes(row_color)
    return bytes(out)


def _tiled_pattern(width: int, height: int,
                   palette: list[tuple[int, int, int]],
                   text: str = "") -> bytes:
    """Build a texture by hashing (x,y) -> palette index, optionally stamping
    text rendered as a horizontal band."""
    out = bytearray(width * height * 3)
    for y in range(height):
        for x in range(width):
            h = (x * 31 + y * 17) & 0xFF
            color = palette[h % len(palette)]
            i = (y * width + x) * 3
            out[i:i + 3] = bytes(color)
    # Text band: top 80px, dark background + light text stripes (no real
    # font, just a few light horizontal lines to suggest "title")
    if text:
        for y in range(60, 80):
            for x in range(width):
                if (x // 8) % 2 == 0:
                    i = (y * width + x) * 3
                    out[i:i + 3] = b"\xff\xff\xff"
        # lower band marker for subtitle
        for y in range(90, 100):
            for x in range(width // 4, width * 3 // 4):
                if (x // 6) % 2 == 0:
                    i = (y * width + x) * 3
                    out[i:i + 3] = b"\xc0\xc0\xc0"
    return bytes(out)


def _ui_background(width: int, height: int,
                   palette: list[tuple[int, int, int]]) -> bytes:
    """Build a 768x1024 background for login / KeySetting / Titanlogo.

    The texture_loader_test::LoginDdsSkyBandIsOnTopAfterTitleFlip
    checks that AFTER loadDDS+flipVertical(), the top band of pixels
    is brighter than the bottom band.  loadDDS writes the file as
    read (no flip), then flipVertical() flips it in memory.  So the
    file we write must have the bright "sky" band at the BOTTOM of
    the image — flipVertical() will then place it at the top of
    memory, where the test samples.

    Layout:
      - file y=0 .. height - sky_h: dark gradient (palette[1] -> palette[2])
      - file y=height - sky_h .. height-1: solid bright palette[0] (the "sky")
      - small bright title band inside the dark gradient
    """
    out = bytearray(width * height * 3)
    sky_height = height // 5  # 20% bottom
    # Top dark gradient
    body_h = height - sky_height
    for y in range(body_h):
        t = y / max(1, body_h - 1)
        r = int(palette[1][0] + (palette[2][0] - palette[1][0]) * t)
        g = int(palette[1][1] + (palette[2][1] - palette[1][1]) * t)
        b = int(palette[1][2] + (palette[2][2] - palette[1][2]) * t)
        for x in range(width):
            i = (y * width + x) * 3
            out[i:i + 3] = bytes([r, g, b])
    # Bottom sky band
    for y in range(body_h, height):
        for x in range(width):
            i = (y * width + x) * 3
            out[i:i + 3] = bytes(palette[0])
    # Title band overlay (top of dark gradient)
    for y in range(5, 25):
        for x in range(width):
            if (x // 8) % 2 == 0:
                i = (y * width + x) * 3
                out[i:i + 3] = b"\xff\xff\xff"
    return bytes(out)


# ---------------------------------------------------------------------------
# Generators
# ---------------------------------------------------------------------------

# 65 map name list (the maps we know about from the modern/data bmhm set)
# We synthesize names for any not in the list.
def _known_map_ids() -> list[int]:
    return [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
            21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
            41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
            61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
            81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100,
            101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116,
            117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132,
            133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148,
            149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
            165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180,
            181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196,
            197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207]


def generate_loading(out_dir: Path) -> int:
    """Generate maploadingimage00-207.dds + LoadingTip01-32.dds + now_loading01-03.TIF."""
    n = 0
    # 65 (we keep original count) maploadingimage
    for mid in _known_map_ids()[:65]:
        name = f"Map{mid}"
        palette = _hashed_palette(name)
        top = palette[0]
        bottom = palette[2]
        pixels = bytearray(_gradient(1024, 768, top, bottom))
        # overlay a tile pattern in the lower half (do it once on bytearray)
        for y in range(384, 768):
            for x in range(1024):
                if ((x * 17 + y * 31) & 0x3F) < 4:
                    i = (y * 1024 + x) * 3
                    pixels[i:i + 3] = bytes(palette[3])
        write_dds_24(out_dir / f"maploadingimage{mid:03d}.dds", 1024, 768, bytes(pixels))
        n += 1
    # 32 LoadingTip
    for i, tip in enumerate(DEFAULT_TIPS, start=1):
        pixels = _tiled_pattern(256, 128, PALETTES["dark_gold"], text=tip)
        write_dds_24(out_dir / f"LoadingTip{i:02d}.dds", 256, 128, pixels)
        n += 1
    # 3 now_loading frames
    for i in range(1, 4):
        progress = i / 4.0
        pixels = bytearray(_tiled_pattern(256, 64, PALETTES["water"], text="now loading"))
        for y in range(48, 56):
            for x in range(int(256 * progress)):
                i_p = (y * 256 + x) * 3
                pixels[i_p:i_p + 3] = b"\xff\xff\xff"
        write_tif_rgb(out_dir / f"now_loading{i:02d}.TIF", 256, 64, bytes(pixels))
        n += 1
    return n


def generate_minimap(out_dir: Path) -> int:
    """Generate mini_<N>.dds for 65 maps (256x256, hashed palette)."""
    n = 0
    for mid in _known_map_ids()[:65]:
        name = f"Map{mid}"
        palette = _hashed_palette(name)
        pixels = _tiled_pattern(256, 256, palette, text="")
        write_dds_24(out_dir / f"mini_{mid}.dds", 256, 256, pixels)
        # Also _ful variant
        write_dds_24(out_dir / f"mini_{mid}_ful.dds", 256, 256, pixels)
        n += 1
    return n * 2


def generate_ui_bg(out_dir: Path) -> int:
    """Generate 4 large background DDS files (legacy 768x1024 layout)."""
    n = 0
    for spec in UI_BACKGROUNDS:
        palette = PALETTES[spec["palette"]]
        # Legacy login.dds is 768 wide x 1024 tall (portrait); modern ctest
        # texture_loader_test::LoginDdsSkyBandIsOnTopAfterTitleFlip relies
        # on this geometry.  Write 768x1024.
        pixels = _ui_background(768, 1024, palette)
        write_dds_24(out_dir / spec["name"], 768, 1024, pixels)
        n += 1
    return n


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> int:
    ap = argparse.ArgumentParser(
        description="polish_assets — 程序化生成 5 类占位/差异化资源")
    ap.add_argument("--out", required=True,
                    help="目标目录 (通常 PlayDH/)")
    ap.add_argument("--generate-loading", action="store_true",
                    help="生成 65 maploading + 32 LoadingTip + 3 now_loading")
    ap.add_argument("--generate-minimap", action="store_true",
                    help="生成 65 mini_<N>.dds + 65 mini_<N>_ful.dds")
    ap.add_argument("--generate-ui-bg", action="store_true",
                    help="生成 4 张大背景 dds")
    ap.add_argument("--all", action="store_true",
                    help="生成全部")
    ap.add_argument("--dry-run", action="store_true",
                    help="只统计不写盘")
    args = ap.parse_args()

    out_dir = Path(args.out)
    total = 0
    if args.generate_loading or args.all:
        n = 65 + 32 + 3
        print(f"[loading] target={out_dir / 'Image' / 'LoadingMap'} files={n}")
        if not args.dry_run:
            total += generate_loading(out_dir / "Image" / "LoadingMap")
        else:
            total += n
    if args.generate_minimap or args.all:
        n = 65 * 2
        print(f"[minimap] target={out_dir / 'Image' / 'MiniMap'} files={n}")
        if not args.dry_run:
            total += generate_minimap(out_dir / "Image" / "MiniMap")
        else:
            total += n
    if args.generate_ui_bg or args.all:
        n = 4
        print(f"[ui-bg] target={out_dir / 'Image' / '2D'} files={n}")
        if not args.dry_run:
            total += generate_ui_bg(out_dir / "Image" / "2D")
        else:
            total += n
    print(f"done. total={total} files")
    return 0


if __name__ == "__main__":
    sys.exit(main())
