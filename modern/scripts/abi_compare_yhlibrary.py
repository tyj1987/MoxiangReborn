"""Phase 7.1: Compare the user-visible mangled symbols between the new
and the legacy YHLibrary.lib. Skip compiler-internal labels (those that
start with `$`).

Strategy:
1. Use `dumpbin /SYMBOLS` on both libs.
2. Filter out lines whose symbol starts with `$` (internal linker labels).
3. Diff the remaining sets — they should be the user-visible ABI surface.
"""
import os
import subprocess

NEW = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Lib]YHLibrary\build_yhlibrary\YHLibrary.lib"
OLD = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Lib]YHLibrary\YHLibrary.lib"
OUT_DIR = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\modern\build_yhlibrary"

def run(line):
    setup_r = subprocess.run(
        ["cmd.exe", "/c", r"C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat && set"],
        capture_output=True, shell=False,
    )
    enc = "mbcs"
    env = os.environ.copy()
    for line2 in setup_r.stdout.decode(enc, errors="replace").splitlines():
        s = line2.strip()
        if not s or "=" not in s: continue
        k, _, v = s.partition("=")
        try: env[k] = v
        except: pass
    return subprocess.run(line, capture_output=True, shell=False, env=env)

dumpbin = r"C:\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe"

# Re-dump (idempotent)
for tag, path in [("new", NEW), ("old", OLD)]:
    out = os.path.join(OUT_DIR, f"syms_{tag}.txt")
    r = run([dumpbin, "/SYMBOLS", path])
    with open(out, "wb") as f:
        f.write(r.stdout)

def extract_user_visible(path):
    """Return set of mangled symbol names whose textual representation
    starts with a non-`$` character (i.e. user-visible ABI surface)."""
    out = set()
    with open(path, "rb") as f:
        for raw in f:
            try:
                line = raw.decode("mbcs", errors="replace")
            except Exception:
                continue
            line = line.rstrip()
            if "|" not in line:
                continue
            sym = line.split("|", 1)[1].strip()
            if not sym:
                continue
            if sym.startswith("$"):
                continue  # internal labels (junk to compare)
            out.add(sym)
    return out

new_set = extract_user_visible(os.path.join(OUT_DIR, "syms_new.txt"))
old_set = extract_user_visible(os.path.join(OUT_DIR, "syms_old.txt"))

print(f"new user-visible symbols: {len(new_set)}")
print(f"old user-visible symbols: {len(old_set)}")

added   = new_set - old_set
removed = old_set - new_set
common  = new_set & old_set

print(f"common: {len(common)}")
print(f"in new only: {len(added)}")
print(f"in old only: {len(removed)}")

# Pick the most important ones — the public types.
def is_public(s: str) -> bool:
    # Mangle-friendly heuristic: starts with `?` and contains a class marker.
    # The legacy YHLibrary exposes: CHSEL, CHSEL_STREAM, CStrClass, CPtrList,
    # CConnection, CFile, cPtrList, cLinkedList*, cLooseLinkedList, cConstLinkedList,
    # CMemoryPool, CIndexGenerator, Encryptor (helper functions), etc.
    if not s.startswith("?"):
        return False
    # Skip thunk / chain artifacts (they usually contain "@@$" or "$R")
    if "$R" in s or "$T" in s:
        return False
    return True

new_pub = {s for s in new_set if is_public(s)}
old_pub = {s for s in old_set if is_public(s)}

added_p = sorted(new_pub - old_pub)
removed_p = sorted(old_pub - new_pub)
common_p = sorted(new_pub & old_pub)

print(f"\n[PUBLIC mangled] new: {len(new_pub)}, old: {len(old_pub)}, common: {len(common_p)}")
print(f"  added (new has, old missing): {len(added_p)}")
print(f"  removed (old has, new missing): {len(removed_p)}")

if added_p:
    print("\n[NEW public symbols] first 30:")
    for s in added_p[:30]:
        print(" +", s)
if removed_p:
    print("\n[REMOVED public symbols] first 30:")
    for s in removed_p[:30]:
        print(" -", s)

# Save full diff for the report
import json
with open(os.path.join(OUT_DIR, "abi_diff.json"), "w", encoding="utf-8") as f:
    json.dump({
        "new_total": len(new_set),
        "old_total": len(old_set),
        "new_public": len(new_pub),
        "old_public": len(old_pub),
        "common_public": len(common_p),
        "added_public": added_p,
        "removed_public": removed_p,
    }, f, ensure_ascii=False, indent=2)