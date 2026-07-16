"""Phase 12.x: Build all 5 MapServer_Debug_<LOCALE> targets.

The CMakeLists.txt at [Server]Map defines 5 locale Debug variants:
  - MapServer_Debug_KOR
  - MapServer_Debug_JAPAN
  - MapServer_Debug_CHINA
  - MapServer_Debug_HK
  - MapServer_Debug_TL

Each carries the same source list + a locale-specific preprocessor define
(_KOR_LOCAL_ / _JAPAN_LOCAL_ / etc.) — mirrors the legacy vcproj Debug_<LOCALE>
configs. The base build_map.py deliberately skips these (only builds Release)
for byte-parity speed. This script picks them up.

Sibling scripts:
  - build_distribute_debug_locales.py (Phase 7.5b, fixed by C-36 commit 53026a9)
  - build_agent_debug_locales.py    (Phase 7.5k-B, verified 5/5 in C-36 followup ab5f055)
  - build_map_debug_locales.py       (this script, Phase 12.x)

Map server is the largest of the 3 servers (most sources, longest build
time). Expect this to take longer than Distribute/Agent's 5-locale run.

Output binary naming (cmake OUTPUT_NAME property): "MapServer_<LOCALE>.exe"
(not "MapServer_Debug_<LOCALE>.exe") — same pattern as Distribute/Agent.
"""
import os
import re
import subprocess

ROOT = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Server]Map"
BD   = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Server]Map\build_map"

def setup_env():
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

if not os.path.isdir(BD):
    raise SystemExit(f"build dir missing: {BD}. Configure with cmake first.")

LOCALES = ["KOR", "JAPAN", "CHINA", "HK", "TL"]
results = {}

for locale in LOCALES:
    target = f"MapServer_Debug_{locale}"
    print(f"\n=== Build (Debug|Win32 — {target}) ===")
    r = subprocess.run(
        [cmake_exe, "--build", BD, "--config", "Debug",
         "--target", target],
        capture_output=True, shell=False, env=env)
    text = r.stdout.decode("mbcs", errors="replace") + r.stderr.decode("mbcs", errors="replace")
    errs = re.findall(r'(error)\s+C\d+:', text)
    errs_lnk = re.findall(r'(error)\s+LNK\d+:', text)
    errs_msb = re.findall(r'(error)\s+MSB\d+:', text)
    warns = re.findall(r'(warning)\s+C\d+:', text)
    total_errs = len(errs) + len(errs_lnk) + len(errs_msb)
    print(f"  errors: {total_errs} (C={len(errs)} LNK={len(errs_lnk)} MSB={len(errs_msb)}) | "
          f"warnings: {len(warns)} | rc: {r.returncode}")
    if total_errs == 0 and r.returncode == 0:
        m = re.search(rf'{re.escape(target)}\.exe', text)
        if m:
            print(f"  produced: {m.group(0)}")
    results[locale] = (total_errs, r.returncode, len(warns))

print("\n=== Summary ===")
for locale, (errs, rc, warns) in results.items():
    status = "OK" if (errs == 0 and rc == 0) else "FAIL"
    print(f"  MapServer_Debug_{locale}: {status} "
          f"(errors={errs}, warnings={warns}, rc={rc})")

print("\n=== Built files ===")
LOCALE_BIN = {f"MapServer_{loc}.exe" for loc in LOCALES}
for root, _, files in os.walk(BD):
    for f in files:
        if f in LOCALE_BIN:
            full = os.path.join(root, f)
            print(f"  {os.path.relpath(full, BD)}: {os.path.getsize(full):,} bytes")

failed = [loc for loc, (e, rc, _) in results.items() if e > 0 or rc != 0]
if failed:
    raise SystemExit(f"FAILED builds: {failed}")
