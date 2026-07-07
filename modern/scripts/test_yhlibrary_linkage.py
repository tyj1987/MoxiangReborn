"""Phase 7.1: Link a smoke test against the freshly-built YHLibrary.lib."""
import os
import subprocess

ROOT = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Lib]YHLibrary"
BD   = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Lib]YHLibrary\build_yhlibrary"
SMOKE = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\modern\scripts\yhlibrary_smoke_test.cpp"

# Load vcvars64 env.
def setup_env():
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
    return env

env = setup_env()

cl    = r"C:\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe"
link  = r"C:\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\link.exe"

# Compile smoke test
print("=== Compile smoke test ===")
out_dir = os.path.join(BD, "smoke")
os.makedirs(out_dir, exist_ok=True)
obj = os.path.join(out_dir, "yhlibrary_smoke_test.obj")
exe = os.path.join(out_dir, "yhlibrary_smoke_test.exe")
r = subprocess.run(
    [cl, "/nologo", "/EHsc", "/std:c++17", "/W3", "/MT", "/permissive-",
     "/I", ROOT, "/D", "WIN32", "/D", "_WINDOWS", "/D", "_MBCS",
     "/Fo:" + obj, SMOKE],
    capture_output=True, shell=False, env=env,
)
print(r.stdout.decode("mbcs", errors="replace"))
if r.returncode != 0:
    print("STDERR:", r.stderr.decode("mbcs", errors="replace"))
    raise SystemExit(r.returncode)

# Link against the freshly-built YHLibrary.lib.
print()
print("=== Link ===")
r = subprocess.run(
    [link, "/nologo", "/SUBSYSTEM:CONSOLE",
     "/OUT:" + exe, obj,
     os.path.join(BD, "YHLibrary.lib"),
     "winmm.lib", "wsock32.lib"],
    capture_output=True, shell=False, env=env,
)
print(r.stdout.decode("mbcs", errors="replace"))
if r.returncode != 0:
    print("STDERR:", r.stderr.decode("mbcs", errors="replace"))
    raise SystemExit(r.returncode)

# Run
print()
print("=== Run ===")
r = subprocess.run([exe], capture_output=True, shell=False, env=env)
print(r.stdout.decode("mbcs", errors="replace"))
print(r.stderr.decode("mbcs", errors="replace"))
print("(exit", r.returncode, ")")