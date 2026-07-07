"""Phase 7.6: Configure + Build [Server]MurimNet via CMake.

MurimNet is a Map-server variant for the PvP arena system. It shares
Server.cpp with Map but disables Map-specific logic via _MURIMNET_.

Known issue (Bug D-8): 4 compile errors from locale-conditional fields:
  - GetMapNum (CServerSystem) / MunpaName (HERO_TOTALINFO)
    These require _KOR_LOCAL_ or equivalent locale macros.
  - CDataBase::Init(int, int) overload only available under _DEBUG+_USINGTOOL_
  - These fields exist in the legacy build but the VC6 compiler was
    less strict about member-access checking.
"""
import os, re, shutil, subprocess, sys, hashlib

ROOT = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Server]MurimNet"
BD   = os.path.join(ROOT, "build_murimnet")

if os.path.isdir(BD):
    shutil.rmtree(BD)
os.makedirs(BD, exist_ok=True)

def setup_env():
    r = subprocess.run(
        ["cmd.exe", "/c", r"C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat && set"],
        capture_output=True, shell=False)
    env = os.environ.copy()
    for line in r.stdout.decode("mbcs", errors="replace").splitlines():
        s = line.strip()
        if not s or "=" not in s: continue
        k, _, v = s.partition("=")
        try: env[k] = v
        except Exception: pass
    return env

env = setup_env()
cmake_exe = r"C:\Program Files\CMake\bin\cmake.exe"

print("=== Configure (Win32 / x86) ===", flush=True)
r = subprocess.run(
    [cmake_exe, "-S", ROOT, "-B", BD,
     "-G", "Visual Studio 17 2022", "-A", "Win32"],
    capture_output=True, shell=False, env=env)
stdout_text = r.stdout.decode("utf-8", errors="replace")
try: print(stdout_text)
except UnicodeEncodeError: pass
if r.returncode != 0:
    print("STDERR:", r.stderr.decode("utf-8", errors="replace")[:2000])
    raise SystemExit(r.returncode)

print("=== Build (Debug|Win32) ===", flush=True)
r = subprocess.run(
    [cmake_exe, "--build", BD, "--config", "Debug"],
    capture_output=True, shell=False, env=env)
text = r.stdout.decode("utf-8", errors="replace") + r.stderr.decode("utf-8", errors="replace")
errs = re.findall(r'(error)\s+C\d+:', text)
warns = re.findall(r'(warning)\s+C\d+:', text)
print(f"errors: {len(errs)} | warnings: {len(warns)} | rc: {r.returncode}")

log_path = os.path.join(os.path.dirname(__file__), "build_murimnet_full.txt")
with open(log_path, "w", encoding="utf-8", errors="replace") as f:
    f.write(text)
print(f"Full log: {log_path}")

# Print error lines for Bug D-8 diagnostic
for line in text.splitlines():
    if 'error C' in line:
        print(line)
