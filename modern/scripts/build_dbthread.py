"""Phase 7.1: Configure + Build DBThread via CMake (no shell)."""
import os, subprocess, shutil

ROOT = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Lib]DBThread"
BD   = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Lib]DBThread\build_dbthread"

if os.path.isdir(BD): shutil.rmtree(BD)
os.makedirs(BD, exist_ok=True)

def setup_env():
    r = subprocess.run(["cmd.exe","/c", r"C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat && set"], capture_output=True, shell=False)
    enc = "mbcs"
    env = os.environ.copy()
    for line in r.stdout.decode(enc, errors="replace").splitlines():
        s = line.strip()
        if not s or "=" not in s: continue
        k,_,v = s.partition("=")
        try: env[k] = v
        except: pass
    return env

env = setup_env()
cmake_exe = r"C:\Program Files\CMake\bin\cmake.exe"

print("=== Configure (Win32 / x86 — needed for ODBC SDWORD* / SQLLEN* compatibility) ===")
r = subprocess.run([cmake_exe, "-S", ROOT, "-B", BD, "-G", "Visual Studio 17 2022", "-A", "Win32", "-DCMAKE_BUILD_TYPE=Release"], capture_output=True, shell=False, env=env)
print(r.stdout.decode("mbcs", errors="replace"))
if r.returncode != 0:
    print("STDERR:", r.stderr.decode("mbcs",errors="replace")[:2000]); raise SystemExit(r.returncode)

print()
print("=== Build ===")
r = subprocess.run([cmake_exe, "--build", BD, "--config", "Release"], capture_output=True, shell=False, env=env)
text = r.stdout.decode("mbcs", errors="replace")
import re
errs = re.findall(r'(error)\s+C\d+:', text)
warns = re.findall(r'(warning)\s+C\d+:', text)
print(f"errors: {len(errs)} | warnings: {len(warns)} | rc: {r.returncode}")
print(text[-2500:])
print()
print("=== Files in BD ===")
for root, dirs, files in os.walk(BD):
    for f in files:
        if f.endswith(('.dll', '.lib', '.exp')):
            p = os.path.join(root, f)
            print(f"  {os.path.relpath(p, BD)}: {os.path.getsize(p)} bytes")