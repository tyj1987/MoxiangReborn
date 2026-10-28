"""Build a VM112-only original-server stage archive without touching resources."""

import hashlib
import json
import shutil
import subprocess
import sys
import zipfile
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "modern" / "build-vitals-r18-clean" / "vm112-stage-r18-candidate"
ARCHIVE = OUTPUT.with_suffix(".zip")
RESOURCE_ROOT = ROOT / "modern/data/PlayDH"
DEBUG_CRT = Path(r"C:\BuildTools\VC\Redist\MSVC\14.44.35112\debug_nonredist\x86\Microsoft.VC143.DebugCRT")
DEBUG_UCRT = Path(r"C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x86\ucrt")
FILES = {
    "modern/build/tools/MoxianLoginServer/mxh_login_server.exe": ROOT / "modern/build-vitals-r18-clean/tools/MoxianLoginServer/mxh_login_server.exe",
    "modern/build/tools/MoxianAgentServer/mxh_agent_server_CHINA.exe": ROOT / "modern/build-vitals-r18-clean/tools/MoxianAgentServer/mxh_agent_server_CHINA.exe",
    "modern/build/tools/MoxianMapServer/mxh_map_server_CHINA.exe": ROOT / "modern/build-vitals-r18-clean/tools/MoxianMapServer/mxh_map_server_CHINA.exe",
    "modern/build/tools/MoxianDbTool/mxh_db_tool.exe": ROOT / "modern/build-vitals-r18-clean/tools/MoxianDbTool/mxh_db_tool.exe",
    "modern/build/tests/unit/db/mxh_db_tests.exe": ROOT / "modern/build-vitals-r18-clean/tests/unit/db/mxh_db_tests.exe",
    "runtime/msvcp140d.dll": DEBUG_CRT / "msvcp140d.dll",
    "runtime/vcruntime140d.dll": DEBUG_CRT / "vcruntime140d.dll",
    "runtime/ucrtbased.dll": DEBUG_UCRT / "ucrtbased.dll",
    "deploy/scripts/start_original_vm112.ps1": ROOT / "deploy/scripts/start_original_vm112.ps1",
    "deploy/scripts/assert_original_writer_contract.ps1": ROOT / "deploy/scripts/assert_original_writer_contract.ps1",
    "deploy/scripts/start_modern.ps1": ROOT / "deploy/scripts/start_modern.ps1",
    "deploy/resource-profiles.json": ROOT / "deploy/resource-profiles.json",
    "deploy/VM112_R18_STAGE.md": ROOT / "deploy/VM112_R18_STAGE.md",
    "reference/manifests/playdh-current.sha256.json": ROOT / "reference/manifests/playdh-current.sha256.json",
}


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def git(*args):
    return subprocess.check_output(["git", *args], cwd=ROOT)


def main():
    if OUTPUT.exists() or ARCHIVE.exists():
        raise FileExistsError("Stage output already exists; inspect it before rebuilding")
    pointer_entries = []
    for name in ("Character.pak", "Effect.pak", "Map.pak", "monster.pak"):
        pointer = ROOT / "modern/data/PlayDH" / name
        raw = git("show", f"HEAD:modern/data/PlayDH/{name}").decode("ascii")
        if not raw.startswith("version https://git-lfs.github.com/spec/v1\n"):
            raise ValueError(f"HEAD does not contain an LFS pointer: {name}")
        lines = dict(line.split(" ", 1) for line in raw.splitlines() if " " in line)
        expected_sha = lines["oid"].removeprefix("sha256:")
        expected_size = int(lines["size"])
        original = RESOURCE_ROOT / pointer.name
        if original.stat().st_size != expected_size or sha256(original) != expected_sha:
            raise ValueError(f"Original resource differs from LFS pointer: {pointer.name}")
        if pointer.stat().st_size == expected_size:
            if sha256(pointer) != expected_sha:
                raise ValueError(f"Hydrated worktree resource differs from LFS pointer: {name}")
        elif pointer.read_text(encoding="ascii") != raw:
            raise ValueError(f"Worktree resource is neither canonical nor the LFS pointer: {name}")
        pointer_entries.append({"name": pointer.name, "bytes": expected_size, "sha256": expected_sha})
    if len(pointer_entries) != 4:
        raise ValueError(f"Expected four LFS PAKs, found {len(pointer_entries)}")

    # Resource hydration is bound separately by the full manifest; source
    # patch identity covers the implementation and deployment scripts.
    change_set = bytearray(git("diff", "--binary", "HEAD", "--", ".", ":!modern/data/PlayDH"))
    untracked = []
    for name in git("ls-files", "--others", "--exclude-standard", "-z").decode().split("\0"):
        if not name:
            continue
        digest = sha256(ROOT / name)
        untracked.append({"path": name, "sha256": digest})
        change_set.extend(f"\nUNTRACKED {name} SHA256 {digest}\n".encode())
    patch_sha = hashlib.sha256(change_set).hexdigest()

    OUTPUT.mkdir(parents=True)
    entries = []
    for relative, source in FILES.items():
        target = OUTPUT / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
        entries.append({"path": relative, "bytes": target.stat().st_size, "sha256": sha256(target)})
    patch_path = OUTPUT / "source-change-set.patch"
    patch_path.write_bytes(change_set)
    entries.append({"path": patch_path.name, "bytes": patch_path.stat().st_size,
                    "sha256": sha256(patch_path)})
    manifest = {
        "schema": 1,
        "stage": r"C:\Moxiang\original\stage-r18-20261009",
        "listenerPorts": {"login": 36001, "agent": 37001, "map": 38001},
        "generatedUtc": datetime.now(timezone.utc).isoformat(),
        "sourceHead": git("rev-parse", "HEAD").decode().strip(),
        "sourcePatchSha256": patch_sha,
        "sourceUntracked": untracked,
        "resourcePaks": pointer_entries,
        "writerContract": {
            "id": "original-vm112-r18-mssql-v3",
            "stageBuild": "china-x86-debug-20261009",
            "databaseHost": "192.168.2.238",
            "database": "MoxiangOriginalR18Test_20261009",
            "schemaVersion": 4,
            "vitalsContract": "original-vitals-cas-v2",
            "progressionContract": "original-exp-points-v3",
            "numericAttributes": "base-4-int-nullable-checked-v1",
            "resourceProfile": "playdh-current",
            "resourceManifestSha256": sha256(ROOT / "reference/manifests/playdh-current.sha256.json"),
        },
        "files": entries,
    }
    (OUTPUT / "stage-manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    with zipfile.ZipFile(ARCHIVE, "w", compression=zipfile.ZIP_DEFLATED,
                         compresslevel=6) as archive:
        for path in sorted(OUTPUT.rglob("*")):
            if path.is_file():
                archive.write(path, path.relative_to(OUTPUT).as_posix())
    print(json.dumps({"archive": str(ARCHIVE), "bytes": ARCHIVE.stat().st_size,
                      "sha256": sha256(ARCHIVE), "sourceHead": manifest["sourceHead"],
                      "sourcePatchSha256": patch_sha, "resourcePaks": len(pointer_entries),
                      "files": len(entries)}, indent=2))


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(f"package-original-vm112: {exc}", file=sys.stderr)
        raise SystemExit(1)
