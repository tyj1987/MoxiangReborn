#!/usr/bin/env python3
"""Pixel gate for the deterministic Map12 player/monster smoke frame."""

import sys
from pathlib import Path
from PIL import Image


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: verify-entity-frame.py FRAME.tga")
        return 2
    path = Path(sys.argv[1])
    image = Image.open(path).convert("RGB")
    if image.size != (800, 600):
        print(f"FAIL: unexpected dimensions {image.size}")
        return 1
    # The first deterministic spawn is east of LoginPoint 2012 and projects
    # into this region with the legacy 30-degree GameIn camera.
    crop = image.crop((480, 220, 650, 410))
    pixels = list(crop.get_flattened_data())
    dark = sum(1 for r, g, b in pixels if r < 70 and g < 70 and b < 55)
    red = sum(1 for r, g, b in pixels if r > 45 and r > g * 1.5 and r > b * 1.35)
    # The player is camera-centred. These three independent colour groups lock
    # a visible upright silhouette, skin and the original blue waist detail;
    # the terrain-only baseline stays well below all three thresholds.
    player = list(image.crop((375, 260, 425, 375)).get_flattened_data())
    player_dark = sum(1 for r, g, b in player if r < 75 and g < 75 and b < 75)
    player_skin = sum(1 for r, g, b in player if r > 65 and r > g * 1.2 and g > b * 1.02)
    player_blue = sum(1 for r, g, b in player if b > 45 and b > r * 1.15 and b > g * 1.05)
    checks = {
        "player-upright-silhouette": player_dark >= 1000,
        "player-skin": player_skin >= 80,
        "player-blue-detail": player_blue >= 5,
        "monster-dark-body": dark >= 300,
        "monster-red-markings": red >= 10,
    }
    for name, passed in checks.items():
        print(f"{'PASS' if passed else 'FAIL'}: {name}")
    return 0 if all(checks.values()) else 1


if __name__ == "__main__":
    raise SystemExit(main())
