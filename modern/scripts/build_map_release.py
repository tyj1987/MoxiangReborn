"""Phase 7.5d: Attempt Release build of [Server]Map.

Mirrors build_map.py but targets the `MapServer` (Release) target instead of
MapServer_Debug_KOR. The Release build was historically blocked by Bug C-30
(ChannelSystem.cpp accessing KOR-only MSG_CHANNEL_INFO fields unconditionally);
this script is paired with the C-30 fix to test whether Release path now builds
clean and whether its byte size is closer to SWorking's 2,555,904 baseline.

Output: full build log at modern/scripts/build_map_release_full.txt
"""
import os
import re
import shutil
import subprocess

ROOT = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Server]Map"
BD   = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Server]Map\build_map"
SWORKING_EXE = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\SWorking\MapServer.exe"

if os.path.isdir(BD):
    shutil.rmtree(BD)
os.makedirs(BD, exist_ok=True)

def setup_env():
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

print("=== Configure (Win32 / x86) ===")
r = subprocess.run(
    [cmake_exe, "-S", ROOT, "-B", BD,
     "-G", "Visual Studio 17 2022", "-A", "Win32"],
    capture_output=True, shell=False, env=env)
print(r.stdout.decode("mbcs", errors="replace")[-1500:])
if r.returncode != 0:
    print("STDERR:", r.stderr.decode("mbcs", errors="replace")[:2000])
    raise SystemExit(r.returncode)

print()
print("=== Build (Release|Win32 — MapServer target only) ===")
r = subprocess.run(
    [cmake_exe, "--build", BD, "--config", "Release",
     "--target", "MapServer"],
    capture_output=True, shell=False, env=env)
text = r.stdout.decode("mbcs", errors="replace") + r.stderr.decode("mbcs", errors="replace")
errs = re.findall(r'(error)\s+C\d+:', text)
errs_lnk = re.findall(r'(error)\s+LNK\d+:', text)
errs_msb = re.findall(r'(error)\s+MSB\d+:', text)
warns = re.findall(r'(warning)\s+C\d+:', text)
total_errs = len(errs) + len(errs_lnk) + len(errs_msb)
print(f"errors: {total_errs} (C={len(errs)} LNK={len(errs_lnk)} MSB={len(errs_msb)}) | warnings: {len(warns)} | rc: {r.returncode}")

log_path = os.path.join(os.path.dirname(__file__), "build_map_release_full.txt")
with open(log_path, "w", encoding="utf-8", errors="replace") as f:
    f.write(text)
print(f"Full log written to: {log_path}")

print(text[-2500:])
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

print()
print("=== SWorking baseline ===")
if os.path.isfile(SWORKING_EXE):
    bs = os.path.getsize(SWORKING_EXE)
    print(f"  SWorking/MapServer.exe = {bs} bytes")
    for rel, sz in produced:
        if rel.endswith("MapServer.exe") and "\\Release\\" in rel:
            diff = sz - bs
            pct = (diff / bs) * 100 if bs else 0
            print(f"  built  {rel} = {sz} bytes  (diff {diff:+d} / {pct:+.1f}%)")
            if abs(pct) < 10:
                print(f"  >>> WITHIN +/-10% GATE THRESHOLD <<<")
            else:
                print(f"  >>> OUTSIDE +/-10% GATE (delta = {pct:+.1f}%) <<<")
            import hashlib
            def sha(p):
                with open(p, "rb") as fh:
                    return hashlib.sha256(fh.read()).hexdigest()
            print(f"  legacy sha256 = {sha(SWORKING_EXE)}")
            print(f"  fresh  sha256 = {sha(os.path.join(BD, rel))}")
else:
    print("  SWorking/MapServer.exe not found")