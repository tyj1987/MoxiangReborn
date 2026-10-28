"""Transfer an audited VM112 stage through an authorized PVE host.

The local machine never needs ``qm``.  The PVE host executes QEMU Guest Agent
commands, while package bytes are sent in sub-megabyte chunks so the QGA stdin
limit is respected.  This script only creates an isolated stage and verifies
it; it never stops or starts game services.
"""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import subprocess
from pathlib import Path


CHUNK_SIZE = 512 * 1024


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


class PveGuest:
    def __init__(self, host: str, vmid: int) -> None:
        self.host = host
        self.vmid = str(vmid)

    def exec(self, script: str, data: bytes | None = None, timeout: int = 120) -> str:
        encoded = base64.b64encode(script.encode("utf-16le")).decode("ascii")
        remote = [
            "qm", "guest", "exec", self.vmid, "--timeout", str(timeout),
        ]
        if data is not None:
            remote += ["--pass-stdin", "1"]
        remote += [
            "--", "powershell.exe", "-NoProfile", "-NonInteractive",
            "-EncodedCommand", encoded,
        ]
        completed = subprocess.run(
            ["ssh", "-T", "-o", "BatchMode=yes", "-o", "ConnectTimeout=10", self.host, *remote],
            input=data,
            capture_output=True,
            check=False,
        )
        stdout = completed.stdout.decode(errors="replace")
        stderr = completed.stderr.decode(errors="replace")
        if completed.returncode:
            raise RuntimeError(f"ssh/qm failed: {stdout}\n{stderr}")
        # qm prints one JSON object.  Preserve out-data verbatim because it may
        # contain a JSON value emitted by the guest command.
        try:
            result = json.loads(stdout)
        except json.JSONDecodeError as exc:
            raise RuntimeError(f"invalid qm guest response: {stdout}\n{stderr}") from exc
        if int(result.get("exitcode", 1)) != 0:
            raise RuntimeError(f"guest command failed: {stdout}\n{stderr}")
        return str(result.get("out-data", "")).strip()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pve-host", required=True, help="authorized SSH target, for example root@192.168.2.200")
    parser.add_argument("--vmid", type=int, default=112)
    parser.add_argument("--package", type=Path, required=True)
    parser.add_argument("--expected-sha256", required=True)
    parser.add_argument("--stage", required=True, help=r"guest path such as C:\Moxiang\original\stage-name")
    parser.add_argument("--resource-source-stage", help="existing guest stage whose PlayDH tree is copied into the new stage")
    parser.add_argument("--verify-existing", action="store_true", help="verify an already transferred/extracted stage without changing it")
    args = parser.parse_args()

    package = args.package.resolve()
    if not package.is_file() or sha256(package).lower() != args.expected_sha256.lower():
        raise ValueError("local package hash mismatch")
    guest = PveGuest(args.pve_host, args.vmid)
    stage = args.stage.rstrip("\\/")
    total = package.stat().st_size
    if not args.verify_existing:
        guest.exec(
            f"$stage='{stage}'; if(Test-Path -LiteralPath $stage){{throw 'stage already exists'}}; New-Item -ItemType Directory -Path $stage -Force | Out-Null"
        )
        sent = 0
        with package.open("rb") as stream:
            while chunk := stream.read(CHUNK_SIZE):
                guest.exec(
                    f"$path='{stage}\\package.zip'; $src=[Console]::OpenStandardInput(); $dst=[IO.File]::Open($path,[IO.FileMode]::Append,[IO.FileAccess]::Write,[IO.FileShare]::None); try{{$src.CopyTo($dst)}} finally{{$dst.Dispose();$src.Dispose()}}",
                    chunk,
                )
                sent += len(chunk)
                print(f"transferred {sent}/{total}", flush=True)
    remote_hash = guest.exec(f"(Get-FileHash -Algorithm SHA256 -LiteralPath '{stage}\\package.zip').Hash.ToLowerInvariant()")
    if remote_hash.lower() != args.expected_sha256.lower():
        raise RuntimeError(f"remote package hash mismatch: {remote_hash}")
    if not args.verify_existing:
        guest.exec(
            f"Add-Type -AssemblyName System.IO.Compression.FileSystem; [IO.Compression.ZipFile]::ExtractToDirectory('{stage}\\package.zip','{stage}')"
        )
    if args.resource_source_stage and not args.verify_existing:
        source = args.resource_source_stage.rstrip("\\/")
        guest.exec(
            f"$source='{source}\\modern\\data\\PlayDH'; $target='{stage}\\modern\\data\\PlayDH'; if(-not (Test-Path -LiteralPath $source -PathType Container)){{throw 'resource source stage missing'}}; New-Item -ItemType Directory -Path $target -Force | Out-Null; robocopy $source $target /MIR /COPY:DAT /DCOPY:DAT /R:1 /W:1 /NFL /NDL /NJH /NJS; if($LASTEXITCODE -ge 8){{throw 'resource mirror failed'}}; Expand-Archive -LiteralPath '{stage}\\package.zip' -DestinationPath '{stage}' -Force",
            # A full PlayDH mirror is ~1.7 GiB and can exceed ten minutes on
            # a busy guest even when the copy itself is making progress.
            timeout=1800,
        )
    result = json.loads(
        guest.exec(
            f"$manifest=Get-Content '{stage}\\stage-manifest.json' -Raw | ConvertFrom-Json; $root='{stage}\\modern\\data\\PlayDH'; $files=@(Get-ChildItem -LiteralPath $root -File -Recurse); $required=@(); if($null -ne $manifest.requiredMapNumbers){{ $required=@($manifest.requiredMapNumbers) }}; $ai=@(); foreach($map in $required){{ $suffix=if([int]$map -lt 100){{('{{0:D2}}' -f [int]$map)}}else{{[string]$map}}; $path=Join-Path $root ('Resource\\Server\\Monster_'+$suffix+'.bin'); if(-not (Test-Path -LiteralPath $path -PathType Leaf)){{throw 'required server AIGroup missing after resource mirror: '+$path}}; $ai += [ordered]@{{map=[int]$map;path=$path;bytes=(Get-Item -LiteralPath $path).Length}} }}; [ordered]@{{stage='{stage}';sourceHead=[string]$manifest.sourceHead;packageSha256=(Get-FileHash '{stage}\\package.zip' -Algorithm SHA256).Hash.ToLowerInvariant();resourceFiles=$files.Count;resourceBytes=[int64](($files | Measure-Object Length -Sum).Sum);resourceProfile=[string]$manifest.writerContract.resourceProfile;requiredMapNumbers=$required;serverAiGroups=$ai}} | ConvertTo-Json -Compress"
        )
    )
    result.update({"transfer": "PASS", "packageBytes": total})
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
