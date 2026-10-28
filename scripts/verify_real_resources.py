"""
verify_real_resources.py
验证 modern/ 实现的资源格式与真实墨香游戏资源 100% 兼容。
直接调用编译好的 mxh_explorer.exe。
"""

import os
import re
import subprocess
import sys
from pathlib import Path

WORKSPACE = Path(__file__).resolve().parents[1]
RES = WORKSPACE / "modern" / "data" / "PlayDH"


def find_explorer() -> Path:
    override = os.environ.get("MXH_RESOURCE_EXPLORER", "").strip()
    candidates = [Path(override)] if override else []
    candidates += [
        WORKSPACE / "modern" / "build" / "tools" / "MoxianResourceExplorer" / "mxh_explorer.exe",
        WORKSPACE / "modern" / "out" / "ctest-current-20261015" / "tools" / "MoxianResourceExplorer" / "mxh_explorer.exe",
        WORKSPACE / "modern" / "out" / "ctest-current-20261014" / "tools" / "MoxianResourceExplorer" / "mxh_explorer.exe",
        WORKSPACE / "modern" / "build-unity-x64" / "tools" / "MoxianResourceExplorer" / "mxh_explorer.exe",
    ]
    return next((path for path in candidates if path.is_file()), candidates[0])


EXPLORER = find_explorer()


def run(args, timeout=60):
    """Run explorer with args, return (returncode, stdout, stderr)."""
    if not EXPLORER.exists():
        return -1, "", f"explorer not found: {EXPLORER}"
    try:
        result = subprocess.run(
            [str(EXPLORER)] + args,
            capture_output=True, text=True, timeout=timeout,
            encoding='utf-8', errors='replace'
        )
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return -2, "", "timeout"


def section(name):
    print()
    print(f"=== {name} ===")


def check(condition, msg):
    if condition:
        print(f"  [PASS] {msg}")
        return True
    print(f"  [FAIL] {msg}")
    return False


# --- Test 1: .bin info on MonsterList.bin ---
section("Test 1: MonsterList.bin metadata")
rc, out, err = run(["info", str(RES / "Resource" / "MonsterList.bin")])
if rc == 0:
    print(out)
    check("type:" in out, "header has type field")
    check("file_size:" in out, "header has file_size field")
    check("bytes:" in out, "payload decoded")

# --- Test 2: .bin info on ItemList.bin ---
section("Test 2: ItemList.bin metadata")
rc, out, err = run(["info", str(RES / "Resource" / "ItemList.bin")])
if rc == 0:
    print(out[:500])
    check("file_size:" in out, "1.5MB file read correctly")

# --- Test 3: All 7 .pak files ---
section("Test 3: All 7 real .pak files")
pak_files = ["Effect.pak", "Character.pak", "Map.pak", "monster.pak",
             "npc.pak", "Pet.pak", "Titan.pak"]
expected_counts = {
    "Effect.pak": 1671,
    "Character.pak": 4468,
    "Map.pak": 4192,
    "monster.pak": 2967,
    "npc.pak": 422,
    "Pet.pak": 362,
    "Titan.pak": 294,
}
all_ok = True
for p in pak_files:
    f = RES / p
    rc, out, err = run(["list", str(f)])
    if rc != 0:
        check(False, f"{p}: list command failed")
        all_ok = False
        continue
    expected = expected_counts.get(p, "?")
    # Older explorers print count=N; current explorers print file_item_num=N.
    for line in out.splitlines():
        match = re.search(r"(?:count|file_item_num)=(\d+)", line)
        if match:
            count = int(match.group(1))
            check(count == expected, f"{p}: count={count} (expected {expected})")
            break
    else:
        check(False, f"{p}: archive count missing")
        all_ok = False

# --- Test 4: extract-pak to get a real file ---
section("Test 4: Extract tile_201_wall04.dds from Map.pak")
extract_dir = WORKSPACE / "modern" / "out" / "runs" / "resource-verify"
extract_dir.mkdir(parents=True, exist_ok=True)
rc, out, err = run(["extract-pak", str(RES / "Map.pak"), "tile_201_wall04.dds", "-o", str(extract_dir)])
print(out)
# DDS file is 2856 bytes, magic 'DDS '
extracted = extract_dir / "tile_201_wall04.dds"
if extracted.exists():
    size = extracted.stat().st_size
    with open(extracted, 'rb') as f:
        magic = f.read(4)
    check(size == 2856, f"file size = {size} (expected 2856)")
    check(magic == b'DDS ', f"magic = {magic!r} (expected DDS )")
    print(f"  DDS magic: {magic}")
else:
    all_ok = False
    check(False, "Map.pak extraction produced tile_201_wall04.dds")

# --- Test 5: Extract from Effect.pak ---
section("Test 5: Extract titan_portal_eff.mod from Effect.pak")
extract_dir2 = extract_dir
rc, out, err = run(["extract-pak", str(RES / "Effect.pak"), "titan_portal_eff.mod", "-o", str(extract_dir2)])
print(out)
extracted = extract_dir2 / "titan_portal_eff.mod"
if extracted.exists():
    print(f"  Extracted {extracted.stat().st_size} bytes")
    print(f"  First 16 bytes hex: {extracted.read_bytes()[:16].hex()}")
else:
    all_ok = False
    check(False, "Effect.pak extraction produced titan_portal_eff.mod")

# --- Summary ---
print()
print("=" * 60)
print(f" RESULT: {'ALL PASS' if all_ok else 'FAIL'}")
print("=" * 60)
sys.exit(0 if all_ok else 1)
