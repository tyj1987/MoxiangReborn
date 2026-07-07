"""Phase 7.1 + Phase 7.4a gap-fill: Configure + Build YHLibrary via CMake (no shell).

Originally used Ninja + x64 (Phase 7.1). Phase 7.4a Distribute migration
requires x86 (legacy Win32) to match the legacy SWorking/DistributeServer.exe
ABI and to satisfy the `__asm { int 3 }` x86-only patterns in BaseNetwork /
DBThread / 4DyuchiNET_Latest.

Switched to Visual Studio 17 2022 + -A Win32 (matches build_basenetwork.py
and build_net.py conventions).
"""
import os, subprocess, shutil

ROOT = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Lib]YHLibrary"
BD   = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Lib]YHLibrary\build_yhlibrary"

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

print("=== Configure (Win32 / x86 — Phase 7.4a ABI alignment) ===")
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

# Write the full build log next to this script.
log_path = os.path.join(os.path.dirname(__file__), "build_yhlibrary_full.txt")
with open(log_path, "w", encoding="utf-8", errors="replace") as f:
    f.write(text)
print(f"Full log written to: {log_path}")

print(text[-2000:])
print()
print("=== Files ===")
produced = None
for root, _, files in os.walk(BD):
    for f in files:
        if f.endswith(('.lib', '.obj', '.exp')):
            p = os.path.join(root, f)
            print(f"  {os.path.relpath(p, BD)}: {os.path.getsize(p)} bytes")
            if f == "YHLibrary.lib":
                produced = p
if produced:
    print(f"[OK] YHLibrary.lib at {produced} ({os.path.getsize(produced)} bytes)")
else:
    print("[FAIL] YHLibrary.lib not produced")
    raise SystemExit(1)