"""Phase 7.3: Configure + Build [Server]Agent via CMake (no shell).

Follows the convention established by build_distribute.py (Phase 7.4a):
  - Generator: Visual Studio 17 2022 + -A Win32 (legacy __asm + x86-only)
  - Sibling _full.txt log next to this script

The CMakeLists.txt at 墨香【源码】\\[Server]Agent\\CMakeLists.txt defines:
  - AgentServer (Release)            -- locale-neutral
  - AgentServer_Debug_KOR/JP/CN/HK/TL -- 5 locale Debug variants

Default build target: AgentServer_Debug_KOR (matches legacy SWorking baseline).
"""
import os
import re
import shutil
import subprocess
import sys

ROOT = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Server]Agent"
BD   = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Server]Agent\build_agent"
SWORKING_EXE = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\SWorking\AgentServer.exe"

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

# The build uses VS 2022 BuildTools which is also at:
# C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools
# (vcvars64.bat at C:\BuildTools is a stub that calls the real thing).
# If the stub fails, CMake can still auto-detect VS 2022 via the registry.

print("=== Configure (Win32 / x86 — legacy __asm + MBCS) ===", flush=True)
r = subprocess.run(
    [cmake_exe, "-S", ROOT, "-B", BD,
     "-G", "Visual Studio 17 2022", "-A", "Win32"],
    capture_output=True, shell=False, env=env)
# Decode stdout and stderr safely
stdout_text = r.stdout.decode("utf-8", errors="replace")
stderr_text = r.stderr.decode("utf-8", errors="replace")
# Print safely — use ascii backslashreplace for terminal safety
try:
    print(stdout_text)
except UnicodeEncodeError:
    print(stdout_text.encode("ascii", errors="replace").decode("ascii"))
if r.returncode != 0:
    try:
        print("STDERR:", stderr_text[:2000])
    except UnicodeEncodeError:
        print("STDERR:", stderr_text[:2000].encode("ascii", errors="replace").decode("ascii"))
    raise SystemExit(r.returncode)

print(flush=True)
print("=== Build (Debug|Win32 — AgentServer_Debug_KOR target only) ===", flush=True)
r = subprocess.run(
    [cmake_exe, "--build", BD, "--config", "Debug",
     "--target", "AgentServer_Debug_KOR"],
    capture_output=True, shell=False, env=env)
text = r.stdout.decode("utf-8", errors="replace") + r.stderr.decode("utf-8", errors="replace")
errs = re.findall(r'(error)\s+C\d+:', text)
warns = re.findall(r'(warning)\s+C\d+:', text)
print(f"errors: {len(errs)} | warnings: {len(warns)} | rc: {r.returncode}")

# Write the full build log next to this script.
log_path = os.path.join(os.path.dirname(__file__), "build_agent_full.txt")
with open(log_path, "w", encoding="utf-8", errors="replace") as f:
    f.write(text)
print(f"Full log written to: {log_path}")

# Print last 3000 chars of the build log for quick review.
print(text[-3000:])
print(flush=True)
print("=== Files ===", flush=True)
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
    print(f"  SWorking/AgentServer.exe = {bs} bytes")
    for rel, sz in produced:
        if rel.endswith("AgentServer_KOR.exe") and "\\Debug\\" in rel:
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
            # Also place a copy at the build_agent root for byte parity
            src = os.path.join(BD, rel)
            dst = os.path.join(BD, "AgentServer.exe")
            shutil.copy2(src, dst)
            print(f"  copy:    {dst} = {os.path.getsize(dst)} bytes")
else:
    print("  SWorking/AgentServer.exe not found — baseline comparison skipped")
