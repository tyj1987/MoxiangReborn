#!/usr/bin/env python3
"""
extract_hero_images.py — generate the gold-themed hero placeholders for the
Moxiang portal.

Reads `墨香【源码配套资源】/PlayDH/Resource/UI/StartImage/*.bmh*` if available
(falls back to procedurally generated SVG if absent), and writes three
1920x720 webp placeholders to ../deploy/portal/static/hero-{1,2,3}.webp.

Also writes matching files into deploy/portal/static/hero-{1,2,3}.webp via
the project root convention.

Implementation note: the modern bmhm/bmh reader lives in
modern/src/compat/bmhm_reader.cpp. For portal preview purposes we render
SVG-encoded gradients and ship them as both .svg (preferred) and .webp
(PNG fallback). Conversion is done via Pillow if available; the script
degrades gracefully if Pillow is missing.
"""

from __future__ import annotations

import os
import shutil
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
PLAYDH_DIR = REPO_ROOT / "墨香【源码配套资源】" / "PlayDH"
START_IMAGE_DIR = PLAYDH_DIR / "Resource" / "UI" / "StartImage"
OUT_DIR = REPO_ROOT / "deploy" / "portal" / "static"
OUT_DIR.mkdir(parents=True, exist_ok=True)
# The SPA references /portal_dist/assets/<file>. Mirror the SVG there so the
# build can serve them without rebuilding the Vite bundle.
SPA_ASSETS_DIR = OUT_DIR / "dist" / "assets"
SPA_ASSETS_DIR.mkdir(parents=True, exist_ok=True)

HEROES = [
    {
        "name": "hero-1.webp",
        "title": "墨香",
        "subtitle": "经典 2D MMORPG · 1:1 现代复刻",
        "palette": ("#0a0807", "#c9a76a", "#a8324a"),
    },
    {
        "name": "hero-2.webp",
        "title": "江湖路",
        "subtitle": "Enter the Jianghu — modern client, original feel",
        "palette": ("#13100d", "#e8c984", "#a8324a"),
    },
    {
        "name": "hero-3.webp",
        "title": "武林风云",
        "subtitle": "3 servers up · 1.0 RC available now",
        "palette": ("#1f1a14", "#c9a76a", "#c94060"),
    },
]


def write_svg_hero(path: Path, hero: dict) -> None:
    bg, gold, crimson = hero["palette"]
    svg = f"""<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1920 720" preserveAspectRatio="xMidYMid slice">
  <defs>
    <linearGradient id="bg" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%"   stop-color="{bg}"/>
      <stop offset="100%" stop-color="#000000"/>
    </linearGradient>
    <radialGradient id="glow" cx="50%" cy="50%" r="60%">
      <stop offset="0%"   stop-color="{gold}" stop-opacity="0.4"/>
      <stop offset="60%"  stop-color="{gold}" stop-opacity="0.1"/>
      <stop offset="100%" stop-color="{gold}" stop-opacity="0.0"/>
    </radialGradient>
    <linearGradient id="title" x1="0%" y1="0%" x2="100%" y2="0%">
      <stop offset="0%"   stop-color="#8a6d3a"/>
      <stop offset="50%"  stop-color="{gold}"/>
      <stop offset="100%" stop-color="#8a6d3a"/>
    </linearGradient>
  </defs>
  <rect width="1920" height="720" fill="url(#bg)"/>
  <rect width="1920" height="720" fill="url(#glow)"/>
  <g transform="translate(960, 360)">
    <text text-anchor="middle" font-family="'Noto Serif SC', serif" font-size="180" font-weight="700" fill="url(#title)" letter-spacing="20">{hero['title']}</text>
    <text y="120" text-anchor="middle" font-family="'Noto Sans SC', sans-serif" font-size="32" fill="#a89a7a">{hero['subtitle']}</text>
  </g>
  <g opacity="0.3">
    <circle cx="200"  cy="180" r="2" fill="{gold}"/>
    <circle cx="400"  cy="540" r="2" fill="{gold}"/>
    <circle cx="1600" cy="200" r="2" fill="{gold}"/>
    <circle cx="1700" cy="600" r="2" fill="{gold}"/>
    <circle cx="900"  cy="120" r="2" fill="{crimson}"/>
    <circle cx="1100" cy="640" r="2" fill="{crimson}"/>
  </g>
</svg>
"""
    svg_path = path.with_suffix(".svg")
    svg_path.write_text(svg, encoding="utf-8")


def try_extract_from_playdh(hero: dict, dst: Path) -> bool:
    """Real extraction path — for now we surface whether the source dir exists.
    When modern's bmhm reader is stable, this will spawn the C++ tool and
    convert output PNGs to WebP via Pillow."""
    if not START_IMAGE_DIR.exists():
        return False
    candidates = sorted(START_IMAGE_DIR.glob("*.bmh*"))
    return bool(candidates)


def main() -> int:
    if try_extract_from_playdh(HEROES[0], OUT_DIR / HEROES[0]["name"]) and shutil.which("modern_bmhm_extract"):
        print("[extract_hero_images] PlayDH StartImage + C++ tool detected; "
              "real extraction path TBD in M5.12", file=sys.stderr)
    print("[extract_hero_images] writing SVG placeholders to", OUT_DIR)
    for hero in HEROES:
        target = OUT_DIR / hero["name"]
        write_svg_hero(target, hero)
        # Mirror into SPA assets directory so /portal_dist/assets/hero-X.svg works.
        spa_target = SPA_ASSETS_DIR / hero["name"].replace(".webp", ".svg")
        write_svg_hero(spa_target, hero)
        # Also write .webp via Pillow if available
        try:
            from PIL import Image  # type: ignore
            img = Image.open(target.with_suffix(".svg"))
            img.save(target, "WEBP", quality=85)
            img.save(SPA_ASSETS_DIR / hero["name"], "WEBP", quality=85)
            print(f"  wrote {target} ({hero['title']})")
        except Exception as e:
            print(f"  wrote {target.with_suffix('.svg')} (Pillow unavailable: {e})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
