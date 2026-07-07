"""Phase 7.1: Configure + Build BaseNetwork via CMake (no shell)."""
import os, subprocess, shutil

ROOT = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Lib]BaseNetwork"
BD   = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Lib]BaseNetwork\build_basenetwork"

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

print("=== Configure ===")
r = subprocess.run([cmake_exe, "-S", ROOT, "-B", BD, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release"], capture_output=True, shell=False, env=env)
print(r.stdout.decode("mbcs", errors="replace"))
if r.returncode != 0:
    print("STDERR:", r.stderr.decode("mbcs",errors="replace")[:2000]); raise SystemExit(r.returncode)

print()
print("=== Build ===")
r = subprocess.run([cmake_exe, "--build", BD], capture_output=True, shell=False, env=env)
text = r.stdout.decode("mbcs", errors="replace")
# Show tail (errors) and head
print(text[-3500:])
if r.returncode != 0:
    print("STDERR:", r.stderr.decode("mbcs",errors="replace")[:3000]); raise SystemExit(r.returncode)

print()
print("=== Files in BD ===")
for f in sorted(os.listdir(BD)):
    p = os.path.join(BD, f)
    if os.path.isfile(p):
        print(f"  {f}: {os.path.getsize(p)} bytes")