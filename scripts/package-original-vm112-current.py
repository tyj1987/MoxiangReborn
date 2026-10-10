"""Create a reproducible VM112 original-server package from a chosen build.

The package is intentionally separate from the live stage.  It records the
exact source revision, build outputs, deployment scripts, and resource
manifest, so a later transfer can verify every byte before stopping services.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import zipfile
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RESOURCE_ROOT = ROOT / "modern" / "data" / "PlayDH"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def git(*args: str) -> str:
    return subprocess.check_output(["git", *args], cwd=ROOT, text=True).strip()


def resource_pointers() -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    for name in ("Character.pak", "Effect.pak", "Map.pak", "monster.pak"):
        pointer = RESOURCE_ROOT / name
        raw = git("show", f"HEAD:modern/data/PlayDH/{name}")
        fields = dict(line.split(" ", 1) for line in raw.splitlines() if " " in line)
        if fields.get("version") != "https://git-lfs.github.com/spec/v1":
            raise RuntimeError(f"{name} is not an LFS pointer at HEAD")
        expected = fields["oid"].removeprefix("sha256:")
        expected_bytes = int(fields["size"])
        if pointer.stat().st_size != expected_bytes or sha256(pointer) != expected:
            raise RuntimeError(f"hydrated resource differs from pointer: {name}")
        rows.append({"name": name, "bytes": expected_bytes, "sha256": expected})
    return rows


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--stage-path", default=r"C:\Moxiang\original\stage-20261009")
    parser.add_argument("--database", default="MoxiangOriginalTest_20261009")
    args = parser.parse_args()
    build = (ROOT / args.build_dir).resolve() if not args.build_dir.is_absolute() else args.build_dir
    output = (ROOT / args.output).resolve() if not args.output.is_absolute() else args.output
    archive = output.with_suffix(".zip")
    if output.exists() or archive.exists():
        raise RuntimeError(f"refusing to overwrite existing package: {output}")

    mappings = {
        "modern/build/tools/MoxianLoginServer/mxh_login_server.exe": build / "tools/MoxianLoginServer/mxh_login_server.exe",
        "modern/build/tools/MoxianAgentServer/mxh_agent_server_CHINA.exe": build / "tools/MoxianAgentServer/mxh_agent_server_CHINA.exe",
        "modern/build/tools/MoxianMapServer/mxh_map_server_CHINA.exe": build / "tools/MoxianMapServer/mxh_map_server_CHINA.exe",
        "modern/build/tools/MoxianDbTool/mxh_db_tool.exe": build / "tools/MoxianDbTool/mxh_db_tool.exe",
        "modern/build/tests/unit/db/mxh_db_tests.exe": build / "tests/unit/db/mxh_db_tests.exe",
        "deploy/scripts/start_original_vm112.ps1": ROOT / "deploy/scripts/start_original_vm112.ps1",
        "deploy/scripts/assert_original_writer_contract.ps1": ROOT / "deploy/scripts/assert_original_writer_contract.ps1",
        "deploy/scripts/start_modern.ps1": ROOT / "deploy/scripts/start_modern.ps1",
        "deploy/resource-profiles.json": ROOT / "deploy/resource-profiles.json",
        "deploy/VM112_STAGE.md": ROOT / "deploy/VM112_STAGE.md",
        "reference/manifests/playdh-current.sha256.json": ROOT / "reference/manifests/playdh-current.sha256.json",
    }
    missing = [str(src) for src in mappings.values() if not src.is_file()]
    if missing:
        raise FileNotFoundError("missing package inputs: " + ", ".join(missing))

    resources = resource_pointers()
    output.mkdir(parents=True)
    entries: list[dict[str, object]] = []
    for relative, source in mappings.items():
        target = output / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(source.read_bytes())
        entries.append({"path": relative, "bytes": target.stat().st_size, "sha256": sha256(target)})

    manifest = {
        "schema": 2,
        "stage": args.stage_path,
        "generatedUtc": datetime.now(timezone.utc).isoformat(),
        "sourceHead": git("rev-parse", "HEAD"),
        "sourceDirty": bool(git("status", "--porcelain")),
        "buildDirectory": str(build),
        "resourcePaks": resources,
        "writerContract": {
            "id": "original-vm112-modern-mssql-v1",
            "databaseHost": "192.168.2.238",
            "database": args.database,
            "schemaVersion": 1,
            "resourceProfile": "playdh-current",
            "resourceManifestSha256": sha256(ROOT / "reference/manifests/playdh-current.sha256.json"),
        },
        "listenerPorts": {"login": 16001, "agent": 17001, "map10": 18001, "map12": 18002},
        "files": entries,
    }
    (output / "stage-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    with zipfile.ZipFile(archive, "x", compression=zipfile.ZIP_DEFLATED, compresslevel=6) as bundle:
        for path in sorted(output.rglob("*")):
            if path.is_file():
                bundle.write(path, path.relative_to(output).as_posix())
    print(json.dumps({"archive": str(archive), "sha256": sha256(archive), "sourceHead": manifest["sourceHead"], "files": len(entries), "resources": len(resources)}, indent=2))


if __name__ == "__main__":
    main()
