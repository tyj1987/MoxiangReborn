"""Phase 7.5b: Build all 5 DistributeServer_Debug_<LOCALE> targets.

The CMakeLists.txt at [Server]Distribute defines 5 locale Debug variants:
  - DistributeServer_Debug_KOR
  - DistributeServer_Debug_JAPAN
  - DistributeServer_Debug_CHINA
  - DistributeServer_Debug_HK
  - DistributeServer_Debug_TL

Each carries the same source list + a locale-specific preprocessor define
(_KOR_LOCAL_ / _JAPAN_LOCAL_ / etc.) — mirrors the legacy vcproj Debug_<LOCALE>
configs. The base build_distribute.py deliberately skips these (only builds
Release) for byte-parity speed. This script picks up where it left off.

Phase 7.5i: KOR target no longer links legacy MD5.lib (mfc71 blocker from
7.5h). CMakeLists KOR-only branch swaps to MD5Checksum_vendor.cpp
(source-level MD5 RFC 1321 port, zero MFC). All 5 locales now build clean.

Output binary naming (cmake OUTPUT_NAME property): "DistributeServer_<LOCALE>.exe"
(not "DistributeServer_Debug_<LOCALE>.exe") — the script's "Built files" section
matches either pattern.
"""
import os
import re
import subprocess

ROOT = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Server]Distribute"
BD   = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Server]Distribute\build_distribute"

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
    raise SystemExit(f"build dir missing: {BD}. Run build_distribute.py first to configure.")

LOCALES = ["KOR", "JAPAN", "CHINA", "HK", "TL"]
results = {}

for locale in LOCALES:
    target = f"DistributeServer_Debug_{locale}"
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
        # find the produced .exe
        m = re.search(rf'{re.escape(target)}\.exe', text)
        if m:
            print(f"  produced: {m.group(0)}")
    results[locale] = (total_errs, r.returncode, len(warns))

print("\n=== Summary ===")
for locale, (errs, rc, warns) in results.items():
    status = "OK" if (errs == 0 and rc == 0) else "FAIL"
    print(f"  DistributeServer_Debug_{locale}: {status} "
          f"(errors={errs}, warnings={warns}, rc={rc})")

print("\n=== Built files ===")
# cmake OUTPUT_NAME strips "_Debug" suffix, so artifact is "DistributeServer_<LOCALE>.exe"
# (not "DistributeServer_Debug_<LOCALE>.exe"). Match the locale names directly.
LOCALE_BIN = {f"DistributeServer_{loc}.exe" for loc in LOCALES}
for root, _, files in os.walk(BD):
    for f in files:
        if f in LOCALE_BIN:
            full = os.path.join(root, f)
            print(f"  {os.path.relpath(full, BD)}: {os.path.getsize(full):,} bytes")

failed = [loc for loc, (e, rc, _) in results.items() if e > 0 or rc != 0]
if failed:
    raise SystemExit(f"FAILED builds: {failed}")
