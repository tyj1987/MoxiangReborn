"""Phase 7.4a: Configure + Build [Server]Distribute via CMake (no shell).

Follows the convention established by build_basenetwork.py (Phase 7.1) and
build_net.py / build_filestorage.py (Phase 7.2):
  - Generator: Visual Studio 17 2022 + -A Win32 (legacy __asm + x86-only)
  - Sibling _full.txt log next to this script

The CMakeLists.txt at 墨香【源码】\\[Server]Distribute\\CMakeLists.txt defines:
  - DistributeServer (Release)            -- locale-neutral; target 184320 bytes
  - DistributeServer_Debug_KOR/JP/CN/HK/TL -- 5 locale Debug variants

This script ONLY builds the Release target for byte-parity verification
against SWorking/DistributeServer.exe (184,320 bytes). The Debug_<LOCALE>
targets are configured but not built — they're 5x slower and add no signal
beyond the Release build's "does the link line actually work?".
"""
import os
import re
import shutil
import subprocess

ROOT = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Server]Distribute"
BD   = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Server]Distribute\build_distribute"
SWORKING_EXE = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\SWorking\DistributeServer.exe"

if os.path.isdir(BD):
    shutil.rmtree(BD)
os.makedirs(BD, exist_ok=True)

def setup_env():
    """Run vcvars64.bat and capture its env, so cmake picks up MSVC + SDK paths."""
    r = subprocess.run(
        ["cmd.exe", "/c",
         r"C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat && set"],
        capture_output=True, shell=False)
    env = os.environ.copy()
    for line in r.stdout.decode("mbcs", errors="replace").splitlines():
        s = line.strip()
        if not s or "=" not in s:
            continue
        k, _, v = s.partition("=")
        try:
            env[k] = v
        except Exception:
            pass
    return env

env = setup_env()
cmake_exe = r"C:\Program Files\CMake\bin\cmake.exe"

print("=== Configure (Win32 / x86 — legacy __asm + MBCS) ===")
r = subprocess.run(
    [cmake_exe, "-S", ROOT, "-B", BD,
     "-G", "Visual Studio 17 2022", "-A", "Win32"],
    capture_output=True, shell=False, env=env)
print(r.stdout.decode("mbcs", errors="replace"))
if r.returncode != 0:
    print("STDERR:", r.stderr.decode("mbcs", errors="replace")[:2000])
    raise SystemExit(r.returncode)

print()
print("=== Build (Release|Win32 — DistributeServer target only) ===")
# Note: --target DistributeServer only builds THAT target, not its
# 5 Debug_<LOCALE> siblings. We don't want byte-parity noise from 5
# simultaneous builds when we're trying to nail down the Release link.
r = subprocess.run(
    [cmake_exe, "--build", BD, "--config", "Release",
     "--target", "DistributeServer"],
    capture_output=True, shell=False, env=env)
text = r.stdout.decode("mbcs", errors="replace") + r.stderr.decode("mbcs", errors="replace")
errs = re.findall(r'(error)\s+C\d+:', text)
warns = re.findall(r'(warning)\s+C\d+:', text)
print(f"errors: {len(errs)} | warnings: {len(warns)} | rc: {r.returncode}")

# Write the full build log next to this script.
log_path = os.path.join(os.path.dirname(__file__), "build_distribute_full.txt")
with open(log_path, "w", encoding="utf-8", errors="replace") as f:
    f.write(text)
print(f"Full log written to: {log_path}")

print(text[-3000:])
print()
print("=== Files ===")
produced = []
for root, _, files in os.walk(BD):
    for f in files:
        if f.endswith(('.exe', '.dll', '.lib', '.pdb')):
            p = os.path.join(root, f)
            sz = os.path.getsize(p)
            produced.append((os.path.relpath(p, BD), sz))
            print(f"  {os.path.relpath(p, BD)}: {sz} bytes")

# Compare against SWorking baseline
print()
print("=== SWorking baseline ===")
if os.path.isfile(SWORKING_EXE):
    bs = os.path.getsize(SWORKING_EXE)
    print(f"  SWorking/DistributeServer.exe = {bs} bytes")
    for rel, sz in produced:
        if rel.endswith("DistributeServer.exe") and "\\Release\\" in rel:
            diff = sz - bs
            pct = (diff / bs) * 100 if bs else 0
            print(f"  built  {rel} = {sz} bytes  (diff {diff:+d} / {pct:+.1f}%)")
            if diff == 0:
                print("  >>> EXACT BYTE-LEVEL MATCH <<<")
            else:
                import hashlib
                def sha(p):
                    with open(p, "rb") as fh:
                        return hashlib.sha256(fh.read()).hexdigest()
                print(f"  legacy sha256 = {sha(SWORKING_EXE)}")
                print(f"  fresh  sha256 = {sha(os.path.join(BD, rel))}")
else:
    print("  SWorking/DistributeServer.exe not found")