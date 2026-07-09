"""
Phase 8 smoke for MoxianAgentServer 5-locale build matrix.

Verifies that all 5 locale targets (KOR/CHINA/JAPAN/HK/TL) produce an
EXE that:
  1. Starts up cleanly (no immediate exit)
  2. Logs the locale name on startup (via [main] log lines)
  3. Logs that the server is listening on its port

Each binary is launched via subprocess.Popen with stdout/stderr redirected
to a per-locale file. We give the server ~2s to print its banner, open DB,
and bind the TCP listener, then taskkill the process. The output file is
then read back and grep'd for the required markers.

`subprocess.Popen` with explicit file handles works reliably on Windows
for capturing line-buffered output even when the child is hard-killed —
the OS flushes the redirected file handle on process termination.

PASS: 5/5 binaries all log "[main]" + locale name + "listening"
FAIL: any binary missing expected log lines
"""
from __future__ import annotations

import subprocess
import sys
import time
from pathlib import Path

BUILD = Path(r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\modern\build\tools\MoxianAgentServer\Debug")
LOCALES = ["KOR", "CHINA", "JAPAN", "HK", "TL"]


def smoke_locale(locale: str) -> tuple[bool, str]:
    exe = BUILD / f"mxh_agent_server_{locale}.exe"
    if not exe.exists():
        return False, f"EXE not found: {exe}"

    out_log = BUILD / f"smoke_{locale.lower()}.out"
    if out_log.exists():
        out_log.unlink()

    # Pick a unique port per locale to avoid TIME_WAIT collisions across runs.
    port = 17001 + LOCALES.index(locale)

    # Use Popen with explicit file handles. The previous approach
    # (cmd /c start /b > out.log 2>&1) silently dropped output on this
    # build host — cmd's redirect was racing the hard-kill. Direct Popen
    # is reliable.
    with open(out_log, "wb") as out_f:
        proc = subprocess.Popen(
            [str(exe), "--port", str(port)],
            cwd=str(BUILD),
            stdout=out_f,
            stderr=subprocess.STDOUT,
            creationflags=subprocess.CREATE_NO_WINDOW if sys.platform == "win32" else 0,
        )

    # Give the server time to: print banner → open DB → start TCP listen.
    time.sleep(2.5)

    # Force-kill via taskkill (sends WM_CLOSE then terminates).
    subprocess.run(["taskkill", "/F", "/IM", exe.name],
                   capture_output=True, check=False)
    try:
        proc.wait(timeout=2)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()

    # Give the FS time to flush the redirect file.
    time.sleep(0.3)

    out_text = out_log.read_text(encoding="utf-8", errors="replace") if out_log.exists() else ""

    required = [
        "[main] Moxian AgentServer",     # server banner
        f"locale   = {locale}",           # locale-specific log line
        "listening on 0.0.0.0:",          # TCP listen started
    ]
    missing = [m for m in required if m not in out_text]
    if missing:
        return False, f"missing markers: {missing}\n--- output ---\n{out_text or '(empty)'}"

    return True, f"OK (lines={len(out_text.splitlines())}, port={port})"


def main() -> int:
    if not BUILD.exists():
        print(f"FAIL: build dir not found: {BUILD}")
        return 2

    print(f"Phase 8 AgentServer build-matrix smoke — {len(LOCALES)} locales\n")
    pass_count = 0
    failures: list[str] = []
    for locale in LOCALES:
        ok, msg = smoke_locale(locale)
        status = "PASS" if ok else "FAIL"
        print(f"  [{status}] {locale:6s} — {msg}")
        if ok:
            pass_count += 1
        else:
            failures.append(locale)

    print(f"\nResult: {pass_count}/{len(LOCALES)} locales clean")
    return 0 if pass_count == len(LOCALES) else 1


if __name__ == "__main__":
    sys.exit(main())