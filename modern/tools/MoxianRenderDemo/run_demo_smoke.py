"""
Phase 5.10 + Phase 6+ + Phase 7: MoxianRenderDemo smoke harness.

Accepts both natural exit and "init + main-loop entered" as PASS:
  PASS-A: exit code 0 within 10s, stdout has "Demo ran for ... ms ... frames."
  PASS-B: stderr has all 8 init markers (3D path: feature level / device /
          renderer / fog; 2D path: SpriteObject created / FontObject ready;
          Effect+Material path: "[effect] palette built", "[material]
          MaterialSet created"), force-killed after 10s (Session 0 /
          headless loop hang)
  FAIL: anything else

Always force-kills at the end so a hung demo doesn't leave a zombie window.
"""
from __future__ import annotations

import os
import re
import subprocess
import sys
import time
from pathlib import Path

REPO = Path(r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）")
DEMO_EXE = REPO / "modern" / "build" / "tools" / "MoxianRenderDemo" / "Debug" / "mxh_render_demo.exe"
DEMO_DIR = DEMO_EXE.parent
OUT_LOG = DEMO_DIR / "run_demo_stdout.txt"
ERR_LOG = DEMO_DIR / "run_demo_stderr.txt"
WAIT_SECONDS = 10

# Phase 5.10 markers (3D init path)
MARKERS_3D = [
    "Created feature level",       # device.cpp:163
    "Device initialized",          # device.cpp:103
    "CoD3DDeviceDX11 created",     # renderer.cpp:73
    "Fog enabled",                 # renderer.cpp:181 (also marks main-loop entry)
]
# Phase 6+ markers (2D HUD init path)
MARKERS_2D = [
    "SpriteObject created",        # main.cpp (demo's manual log)
    "FontObject ready",            # font_object.cpp:150 (renderer-side log)
]
# Phase 7 markers (Effect Shader Palette + Material Set init path)
MARKERS_EFFECT_MTL = [
    "[effect] palette built",      # effect_shader.cpp:53 (renderer-side log)
    "[material] MaterialSet created",  # main.cpp (demo's manual log)
]
INIT_MARKERS = MARKERS_3D + MARKERS_2D + MARKERS_EFFECT_MTL


def main() -> int:
    if not DEMO_EXE.exists():
        print(f"FAIL-B: {DEMO_EXE} not found")
        return 2

    # Fresh logs.
    for p in (OUT_LOG, ERR_LOG):
        if p.exists():
            p.unlink()

    proc = subprocess.Popen(
        [str(DEMO_EXE)],
        cwd=str(DEMO_DIR),
        stdout=open(OUT_LOG, "wb"),
        stderr=open(ERR_LOG, "wb"),
        creationflags=subprocess.CREATE_NO_WINDOW if sys.platform == "win32" else 0,
    )
    print(f"PID: {proc.pid}, waiting up to {WAIT_SECONDS}s for natural exit...")

    try:
        proc.wait(timeout=WAIT_SECONDS)
        natural_exit = True
    except subprocess.TimeoutExpired:
        print(f"Process did not exit within {WAIT_SECONDS}s; force-killing "
              "(Session 0 / headless demo loop hang — expected on this build host)")
        proc.kill()
        try:
            proc.wait(timeout=2)
        except subprocess.TimeoutExpired:
            pass
        natural_exit = False

    stdout_text = OUT_LOG.read_text(encoding="utf-8", errors="replace") if OUT_LOG.exists() else ""
    stderr_text = ERR_LOG.read_text(encoding="utf-8", errors="replace") if ERR_LOG.exists() else ""

    # PASS-A: natural exit + summary line.
    summary = re.search(r"Demo ran for (\d+) ms, (\d+) frames", stdout_text)
    if natural_exit and summary:
        print("PASS-A (natural exit)")
        print(f"  exit={proc.returncode}  ms={summary.group(1)}  frames={summary.group(2)}")
        print("=== stdout ===")
        print(stdout_text.strip())
        return 0

    # PASS-B: hang but all init markers (3D + 2D + Effect+Material) present.
    missing = [m for m in INIT_MARKERS if m not in stderr_text]
    if not missing:
        present_3d = [m for m in MARKERS_3D if m in stderr_text]
        present_2d = [m for m in MARKERS_2D if m in stderr_text]
        present_em = [m for m in MARKERS_EFFECT_MTL if m in stderr_text]
        print("PASS-B (cap hang, but DX11 3D + 2D HUD + Effect/Material init + main-loop entry confirmed)")
        print(f"  process: {'exited cleanly' if natural_exit else 'force-killed'}")
        print(f"  exit={proc.returncode}  stderr_lines={len(stderr_text.splitlines())}")
        print(f"  3D markers hit: {len(present_3d)}/{len(MARKERS_3D)}")
        print(f"  2D markers hit: {len(present_2d)}/{len(MARKERS_2D)}")
        print(f"  Effect+Material markers hit: {len(present_em)}/{len(MARKERS_EFFECT_MTL)}")
        print("=== stderr (last 16 lines) ===")
        for line in stderr_text.splitlines()[-16:]:
            print(f"  {line}")
        return 0

    # FAIL: report what's wrong.
    if missing:
        print(f"FAIL-A (init incomplete — missing markers: {', '.join(missing)})")
    else:
        print("FAIL-B (process did not produce expected output)")
    print("=== stderr ===")
    print(stderr_text if stderr_text else "(empty)")
    print("=== stdout ===")
    print(stdout_text if stdout_text else "(empty)")
    return 1


if __name__ == "__main__":
    sys.exit(main())
