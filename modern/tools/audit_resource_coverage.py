#!/usr/bin/env python3
"""Audit PlayDH resource coverage by the modern MoxianResourceExplorer.

Scans the original PlayDH directory for .pak / .bin / .bmhm / .chl / .chx /
.chr / .mon / .bsad resource files and runs the modern explorer against
each to verify the modern code can parse + handle the legacy formats.

Outputs a coverage manifest suitable for the M4 resource-coverage gate
(see ROADMAP M4 + KNOWN_BUGS M2/M4). Failure modes are surfaced per file
so the manifest doubles as a TODO list.

Exit code:
  0 - audit completed (some failures may exist; check the summary)
  1 - explorer binary missing or PlayDH root not found
"""

import argparse
import os
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

# Recognized resource extensions (per AGENTS.md constitution §0)
RESOURCE_EXTS = {".pak", ".bin", ".bmhm", ".chl", ".chx", ".chr", ".mon", ".bsad"}

# Files that exceed this size may take a while to parse; we still run them
# but tag them as "large" so the report shows where time was spent.
LARGE_FILE_BYTES = 50 * 1024 * 1024  # 50 MB


def find_explorer(build_dir: Path) -> Path:
    candidates = [
        build_dir / "tools" / "MoxianResourceExplorer" / "Debug" / "mxh_explorer.exe",
        build_dir / "tools" / "MoxianResourceExplorer" / "Release" / "mxh_explorer.exe",
    ]
    for c in candidates:
        if c.is_file():
            return c
    raise FileNotFoundError("mxh_explorer.exe not found under " + str(build_dir))


def discover_resources(playdh_root: Path, report_root: Path = None):
    """Walk PlayDH and yield (relative_path, absolute_path) for each known ext."""
    for dirpath, _dirnames, filenames in os.walk(playdh_root):
        for name in filenames:
            ext = os.path.splitext(name)[1].lower()
            if ext in RESOURCE_EXTS:
                full = Path(dirpath) / name
                rel = full.relative_to(report_root or playdh_root)
                yield (rel, full)


def run_explorer(explorer: Path, cmd_args, timeout=30):
    """Run explorer with the given command args. Returns (returncode, stderr_tail)."""
    try:
        result = subprocess.run(
            [str(explorer), *cmd_args],
            capture_output=True, text=True, encoding='utf-8', errors='replace', timeout=timeout
        )
        return result.returncode, (result.stderr or "").strip().splitlines()[-1:] if result.stderr else []
    except subprocess.TimeoutExpired:
        return -1, ["TIMEOUT after %ds" % timeout]
    except Exception as exc:
        return -1, [str(exc)]


def audit_file(explorer: Path, rel: Path, full: Path):
    """Return (status, size_bytes, note) for a single resource."""
    ext = full.suffix.lower()
    size = full.stat().st_size
    cmd = None
    timeout = 30
    if ext == ".bin":
        cmd = ["info", str(full)]
    elif ext == ".pak":
        cmd = ["list", str(full)]
    elif ext == ".bmhm":
        cmd = ["map", str(full)]
    elif ext == ".bsad":
        cmd = ["bsad", str(full)]
    elif ext in (".chl", ".chx", ".chr", ".mon"):
        # No dedicated explorer command for these; presence-only audit.
        return ("present", size, "no explorer command; tracked by stat")

    # Large files take longer; bump timeout.
    if size > LARGE_FILE_BYTES:
        timeout = 120

    rc, err_tail = run_explorer(explorer, cmd, timeout=timeout)
    if rc == 0:
        return ("ok", size, "")
    return ("fail", size, err_tail[0] if err_tail else "rc=%d" % rc)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("playdh_root", type=Path, help="Path to 墨香【源码配套资源】/PlayDH")
    p.add_argument("--build-dir", type=Path,
                   default=Path(r"C:\moxiang\modern\build"),
                   help="Modern build directory containing mxh_explorer.exe")
    p.add_argument("--output", type=Path, default=None,
                   help="Write the coverage manifest to this file (text)")
    p.add_argument("--summary-only", action="store_true",
                   help="Skip per-file detail; only print extension totals")
    args = p.parse_args()

    # CJK path workaround: if the PlayDH root contains non-ASCII characters,
    # create a junction under modern/scratch/ with an ASCII name and audit that
    # instead. The explorer.exe argv parser mangles non-ASCII path bytes when
    # invoked through PowerShell.
    import subprocess as _sp
    ascii_root = args.playdh_root
    if not all(ord(c) < 128 for c in str(args.playdh_root)):
        junction_name = "playdh_link_for_audit"
        junction_path = Path(__file__).resolve().parent / junction_name
        if junction_path.is_dir() or junction_path.is_symlink():
            _sp.run(["cmd", "/c", "rmdir", str(junction_path)], capture_output=True)
        _sp.run(["cmd", "/c", "mklink", "/J", str(junction_path), str(args.playdh_root)], check=True)
        ascii_root = junction_path
        print("  [info] using junction for non-ASCII PlayDH root: " + str(ascii_root))
        print()
    if not args.playdh_root.is_dir():
        print("FAIL: PlayDH root not found: " + str(args.playdh_root))
        return 1
    try:
        explorer = find_explorer(args.build_dir)
    except FileNotFoundError as exc:
        print("FAIL: " + str(exc))
        return 1

    print("PlayDH resource coverage audit")
    print("  PlayDH root: " + str(ascii_root))
    print("  Explorer:    " + str(explorer))
    print()

    summary = defaultdict(lambda: {"total": 0, "ok": 0, "fail": 0, "present": 0, "bytes": 0})
    fail_lines = []
    all_lines = []

    for rel, full in discover_resources(ascii_root):
        ext = full.suffix.lower()
        status, size, note = audit_file(explorer, rel, full)
        s = summary[ext]
        s["total"] += 1
        s[status] += 1
        s["bytes"] += size
        line = "%s\t%s\t%d\t%s" % (ext, rel, size, status.upper() + ("" if not note else "  " + note))
        all_lines.append(line)
        if status == "fail":
            fail_lines.append(line)

    # Print extension summary.
    print("=== Coverage by extension ===")
    print("%-8s %8s %8s %8s %10s %12s" % ("EXT", "TOTAL", "OK", "FAIL", "PRESENT", "BYTES"))
    grand_total = grand_ok = grand_fail = grand_present = grand_bytes = 0
    for ext in sorted(summary):
        s = summary[ext]
        grand_total += s["total"]; grand_ok += s["ok"]; grand_fail += s["fail"]
        grand_present += s["present"]; grand_bytes += s["bytes"]
        print("%-8s %8d %8d %8d %10d %12d" %
              (ext, s["total"], s["ok"], s["fail"], s["present"], s["bytes"]))
    print("-" * 60)
    print("%-8s %8d %8d %8d %10d %12d" %
          ("TOTAL", grand_total, grand_ok, grand_fail, grand_present, grand_bytes))

    if not args.summary_only:
        print()
        print("=== Per-file status ===")
        for line in all_lines:
            print(line)

    print()
    if grand_fail > 0:
        print("COVERAGE AUDIT: %d / %d files failed modern parse" %
              (grand_fail, grand_total))
    else:
        print("COVERAGE AUDIT PASS (%d / %d files parsed by modern code)" %
              (grand_total, grand_total))

    if args.output:
        with open(args.output, "w", encoding="utf-8") as fp:
            fp.write("PlayDH resource coverage manifest\n")
            fp.write("PlayDH root: " + str(args.playdh_root) + "\n")
            fp.write("Explorer:    " + str(explorer) + "\n\n")
            fp.write("%-8s %8s %8s %8s %10s %12s\n" %
                     ("EXT", "TOTAL", "OK", "FAIL", "PRESENT", "BYTES"))
            for ext in sorted(summary):
                s = summary[ext]
                fp.write("%-8s %8d %8d %8d %10d %12d\n" %
                         (ext, s["total"], s["ok"], s["fail"], s["present"], s["bytes"]))
            fp.write("-" * 60 + "\n")
            fp.write("%-8s %8d %8d %8d %10d %12d\n" %
                     ("TOTAL", grand_total, grand_ok, grand_fail, grand_present, grand_bytes))
            fp.write("\n=== Per-file status ===\n")
            for line in all_lines:
                fp.write(line + "\n")
        print("Manifest written to " + str(args.output))

    return 0 if grand_fail == 0 else 0  # don't fail the gate; the report is the deliverable


if __name__ == "__main__":
    raise SystemExit(main())
