"""Phase 7.5k-B: Build all 5 AgentServer_Debug_<LOCALE> targets.

The CMakeLists.txt at [Server]Agent defines 5 locale Debug variants
(Phase 7.3, commit 7.5k derives from there):
  - AgentServer_Debug_KOR
  - AgentServer_Debug_JAPAN
  - AgentServer_Debug_CHINA
  - AgentServer_Debug_HK
  - AgentServer_Debug_TL

Each carries the same source list + a locale-specific preprocessor define
(_KOR_LOCAL_ / _JAPAN_LOCAL_ / etc.) — mirrors the legacy vcproj Debug_<LOCALE>
configs. The base build_agent.py deliberately skips these (only builds
Release) for byte-parity speed. This script picks them up.

Known blocker: Debug_HK links ggsrv25.lib (nProtect GameGuard 2.5 SDK — Bug D-6).
The vendored header (ggsrv25.h) exists; the .lib is MISSING from the repo.
HK link will fail until the lib is restored.

Other 4 locales are expected to build clean (Subject to Phase 7.5i-style
fix-ups if new legacy snags surface).

Output binary naming (cmake OUTPUT_NAME property): "AgentServer_<LOCALE>.exe"
(not "AgentServer_Debug_<LOCALE>.exe") — same pattern as Distribute.
"""
import os
import re
import subprocess

ROOT = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Server]Agent"
BD   = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Server]Agent\build_agent"

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
    target = f"AgentServer_Debug_{locale}"
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
    print(f"  AgentServer_Debug_{locale}: {status} "
          f"(errors={errs}, warnings={warns}, rc={rc})")

print("\n=== Built files ===")
LOCALE_BIN = {f"AgentServer_{loc}.exe" for loc in LOCALES}
for root, _, files in os.walk(BD):
    for f in files:
        if f in LOCALE_BIN:
            full = os.path.join(root, f)
            print(f"  {os.path.relpath(full, BD)}: {os.path.getsize(full):,} bytes")

failed = [loc for loc, (e, rc, _) in results.items() if e > 0 or rc != 0]
if failed:
    raise SystemExit(f"FAILED builds: {failed}")
