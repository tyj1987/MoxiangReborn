"""Phase 7.2: Configure + Build 4DyuchiNET via CMake (no shell)."""
import os, subprocess, shutil

ROOT = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\4DyuchiNET_Latest"
BD   = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\4DyuchiNET_Latest\build_net"

if os.path.isdir(BD): shutil.rmtree(BD)
os.makedirs(BD, exist_ok=True)

def setup_env():
    r = subprocess.run(["cmd.exe", "/c", r"C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat && set"],
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

print("=== Configure (Win32 / x86 — legacy __cdecl / MBCS sources) ===")
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
r = subprocess.run([cmake_exe, "--build", BD, "--config", "Release"],
                   capture_output=True, shell=False, env=env)
text = r.stdout.decode("mbcs", errors="replace")
import re
errs = re.findall(r'(error)\s+C\d+:', text)
warns = re.findall(r'(warning)\s+C\d+:', text)
print(f"errors: {len(errs)} | warnings: {len(warns)} | rc: {r.returncode}")

# Write the full build log next to this script (useful when iterating
# on header tweaks — Python output buffering clips the tail otherwise).
with open(os.path.join(os.path.dirname(__file__), "build_net_full.txt"),
          "w", encoding="utf-8", errors="replace") as f:
    f.write(text)

print(text[-1500:])
print()
print("=== Files ===")
for root, dirs, files in os.walk(BD):
    for f in files:
        if f.endswith(('.dll', '.lib', '.exp')):
            p = os.path.join(root, f)
            print(f"  {os.path.relpath(p, BD)}: {os.path.getsize(p)} bytes")
