#!/usr/bin/env python3
import argparse, struct, sys
from pathlib import Path

EXPECTED_FRAMES = [
    ("connect",),
    ("login",),
    ("charselect",),
    ("charmake",),
    ("gamein",),
]

def load_tga_pixels(path):
    data = path.read_bytes()
    if len(data) < 18:
        raise ValueError("short TGA")
    width, height, depth = struct.unpack_from("<HHB", data, 12)
    if data[1] != 0 or data[2] != 2 or depth not in (24, 32):
        raise ValueError("not uncompressed true-color TGA")
    bytes_per_pixel = depth // 8
    offset = 18 + data[0]
    pixels = []
    for index in range(offset, offset + width * height * bytes_per_pixel, bytes_per_pixel):
        blue, green, red = data[index:index + 3]
        pixels.append((red, green, blue))
    return width, height, pixels

def main():
    p = argparse.ArgumentParser()
    p.add_argument("dir", type=Path)
    p.add_argument("--permissive", action="store_true")
    a = p.parse_args()
    if not a.dir.is_dir():
        print("FAIL: not a directory: " + str(a.dir))
        return 1
    failures = []
    bg = (16, 24, 48)  # BeginRender clear colour 0xff101830
    for (name,) in EXPECTED_FRAMES:
        frame = a.dir / ("state-" + name + ".tga")
        if not frame.exists():
            msg = "missing: " + frame.name
            if a.permissive:
                print("SKIP: " + msg); continue
            print("FAIL: " + msg); failures.append(msg); continue
        try:
            width, height, pixels = load_tga_pixels(frame)
        except (OSError, ValueError) as exc:
            print("FAIL: " + frame.name + ": " + str(exc)); failures.append(frame.name); continue
        non_bg = sum(1 for px in pixels if px != bg)
        ok = non_bg > 100
        status = "PASS" if ok else "FAIL"
        print(status + ": " + frame.name + " non_bg_pixels=" + str(non_bg) + "/" + str(len(pixels)))
        if not ok: failures.append(frame.name)
    if failures:
        print("FAIL: " + str(len(failures)) + " state frames failed"); return 1
    print("STATE_FRAMES PASS"); return 0

if __name__ == "__main__":
    raise SystemExit(main())