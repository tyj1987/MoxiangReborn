"""Phase 7.2: Configure + Build 4DyuchiFileStorage via CMake (no shell)."""
import os, subprocess, shutil

ROOT = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\4DyuchiFileStorage"
BD   = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\4DyuchiFileStorage\build_filestorage"

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

# Pre-stage SS3DGFunc.lib next to the build output so the link line can
# reference it as a bare filename. CMake's configure_file and
# file(COPY) both corrupt absolute paths containing CJK bytes when the
# project directory lives under a Chinese-named parent; copying the
# file into the build dir via Python's pathlib avoids that bug.
import shutil
_lib_src = os.path.join(os.path.dirname(ROOT), "4DyuchiGXGFunc", "SS3DGFunc.lib")
_lib_dst_dir = os.path.join(BD, "Release")
os.makedirs(_lib_dst_dir, exist_ok=True)
_lib_dst = os.path.join(_lib_dst_dir, "SS3DGFunc.lib")
shutil.copy2(_lib_src, _lib_dst)
print(f"Copied SS3DGFunc.lib to {_lib_dst}")

print("=== Configure (Win32 / x86 — legacy __asm) ===")
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
print(text[-2000:])
print()
print("=== Files ===")
for root, dirs, files in os.walk(BD):
    for f in files:
        if f.endswith(('.dll', '.lib', '.exp')):
            p = os.path.join(root, f)
            print(f"  {os.path.relpath(p, BD)}: {os.path.getsize(p)} bytes")