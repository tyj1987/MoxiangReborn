"""Test HSEL CMakeLists.txt end-to-end via subprocess (no shell)."""
import os
import subprocess
import sys

ROOT = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Lib]HSEL"
BD = r"D:\Moxian\modern\build_legacy_hsel"

# Reset build dir
if os.path.isdir(BD):
    import shutil
    shutil.rmtree(BD)
os.makedirs(BD, exist_ok=True)

env = os.environ.copy()
# Ensure MSVC + Ninja are reachable through vcvars64.
# Easiest: invoke cmd.exe with /c that loads vcvars64 first, then runs cmake.

def run_via_cmd(line) -> tuple[int, str, str]:
    """Run `line` (str or list) with vcvars64 env loaded."""
    setup_r = subprocess.run(
        ["cmd.exe", "/c", r"C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat && set"],
        capture_output=True, shell=False,
    )
    enc = "mbcs"
    setup_out = setup_r.stdout.decode(enc, errors="replace")
    env = os.environ.copy()
    for line2 in setup_out.splitlines():
        s = line2.strip()
        if not s or "=" not in s:
            continue
        k, _, v = s.partition("=")
        try:
            env[k] = v
        except Exception:
            pass
    r = subprocess.run(
        line,
        capture_output=True, shell=False, env=env,
    )
    return r.returncode, r.stdout.decode(enc, errors="replace"), r.stderr.decode(enc, errors="replace")

print("=== Configure ===")
cmake_exe = r"C:\Program Files\CMake\bin\cmake.exe"
rc, out, err = run_via_cmd(
    [cmake_exe, "-S", ROOT, "-B", BD, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release"]
)
print(out)
if rc != 0:
    print("STDERR:", err)
    sys.exit(rc)

print()
print("=== Build ===")
rc, out, err = run_via_cmd(
    [cmake_exe, "--build", BD]
)
print(out)
if rc != 0:
    print("STDERR:", err)
    sys.exit(rc)

# Check that HSEL.lib was produced.
print()
produced = None
for cand in (os.path.join(BD, "Release", "HSEL.lib"), os.path.join(BD, "HSEL.lib")):
    if os.path.isfile(cand):
        produced = cand
        break
if produced:
    print(f"[OK] HSEL.lib at {produced} ({os.path.getsize(produced)} bytes)")
else:
    print("[FAIL] HSEL.lib not produced")
    for root, _, files in os.walk(BD):
        for f in files:
            if f.endswith(".lib") or f.endswith(".obj"):
                print("  candidate:", os.path.join(root, f))
    sys.exit(1)
