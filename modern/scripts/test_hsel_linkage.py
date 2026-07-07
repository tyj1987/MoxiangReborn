"""Link a smoke test against the freshly-built HSEL.lib."""
import os
import subprocess

ROOT = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Lib]HSEL"
BD   = r"D:\Moxian\modern\build_legacy_hsel"
SMOKE = r"D:\Moxian\modern\scripts\hsel_smoke_test.cpp"

# load vcvars64 env
r = subprocess.run(
    ["cmd.exe", "/c", r"C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat && set"],
    capture_output=True, shell=False,
)
enc = "mbcs"
env = os.environ.copy()
for line in r.stdout.decode(enc, errors="replace").splitlines():
    s = line.strip()
    if not s or "=" not in s: continue
    k, _, v = s.partition("=")
    try: env[k] = v
    except: pass

cl = r"C:\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe"
link = r"C:\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\link.exe"

# Compile
print("=== Compile smoke test ===")
out_dir = os.path.join(BD, "smoke")
os.makedirs(out_dir, exist_ok=True)
obj = os.path.join(out_dir, "hsel_smoke_test.obj")
exe = os.path.join(out_dir, "hsel_smoke_test.exe")
r = subprocess.run(
    [cl, "/nologo", "/EHsc", "/std:c++17", "/W3",
     "/I", ROOT, "/D", "WIN32", "/D", "_MBCS",
     "/Fo:" + obj, SMOKE],
    capture_output=True, shell=False, env=env,
)
print(r.stdout.decode(enc, errors="replace"))
if r.returncode != 0:
    print("STDERR:", r.stderr.decode(enc, errors="replace"))
    raise SystemExit(r.returncode)

# Link against HSEL.lib
print()
print("=== Link ===")
r = subprocess.run(
    [link, "/nologo",
     "/OUT:" + exe, obj,
     os.path.join(BD, "HSEL.lib"),
     "winmm.lib"],
    capture_output=True, shell=False, env=env,
)
print(r.stdout.decode(enc, errors="replace"))
if r.returncode != 0:
    print("STDERR:", r.stderr.decode(enc, errors="replace"))
    raise SystemExit(r.returncode)

# Run
print()
print("=== Run ===")
r = subprocess.run([exe], capture_output=True, shell=False, env=env)
print(r.stdout.decode(enc, errors="replace"))
print("(exit", r.returncode, ")")
