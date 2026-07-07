"""Phase 7.1 gap-fill: Configure + Build BaseNetwork via CMake (no shell).

Follows the convention established by build_net.py (Phase 7.2) and
build_filestorage.py (Phase 7.2):
  - Generator: Visual Studio 17 2022 + -A Win32 (legacy __asm / x86-only)
  - Sibling _full.txt log next to this script

The original Phase 7.1 script used Ninja, which produces a different
output layout (.ninja_log + build.ninja at the build-dir root, no .sln
files) and skips the Win32 toolchain. Phase 7.2 convention locked
in VS17 + Win32 to match the ABI of the legacy prebuilt DLLs.
"""
import os, subprocess, shutil

ROOT = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Lib]BaseNetwork"
BD   = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Lib]BaseNetwork\build_basenetwork"

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
     "-G", "Visual Studio 17 2022", "-A", "Win32",
     "-DCMAKE_BUILD_TYPE=Release"],
    capture_output=True, shell=False, env=env)
print(r.stdout.decode("mbcs", errors="replace"))
if r.returncode != 0:
    print("STDERR:", r.stderr.decode("mbcs", errors="replace")[:2000])
    raise SystemExit(r.returncode)

print()
print("=== Build ===")
r = subprocess.run(
    [cmake_exe, "--build", BD, "--config", "Release"],
    capture_output=True, shell=False, env=env)
text = r.stdout.decode("mbcs", errors="replace")

import re
errs = re.findall(r'(error)\s+C\d+:', text)
warns = re.findall(r'(warning)\s+C\d+:', text)
print(f"errors: {len(errs)} | warnings: {len(warns)} | rc: {r.returncode}")

# Write the full build log next to this script (matches the build_net.py /
# build_filestorage.py pattern). Python's stdout buffering clips the tail
# otherwise, which is annoying when iterating on header tweaks.
log_path = os.path.join(os.path.dirname(__file__), "build_basenetwork_full.txt")
with open(log_path, "w", encoding="utf-8", errors="replace") as f:
    f.write(text)
print(f"Full log written to: {log_path}")

print(text[-2000:])
print()
print("=== Files ===")
for root, dirs, files in os.walk(BD):
    for f in files:
        if f.endswith(('.dll', '.lib', '.exp')):
            p = os.path.join(root, f)
            print(f"  {os.path.relpath(p, BD)}: {os.path.getsize(p)} bytes")