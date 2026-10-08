"""Transfer one audited stage ZIP from PVE private storage via VM112 QGA."""

import argparse
import base64
import hashlib
import json
import subprocess
import sys
from pathlib import Path


STAGE = r"C:\Moxiang\original\stage-20261009"
PACKAGE = STAGE + r"\package.zip"
CHUNK_SIZE = 512 * 1024


def guest(script, input_bytes=None):
    encoded = base64.b64encode(script.encode("utf-16le")).decode("ascii")
    command = ["qm", "guest", "exec", "112"]
    if input_bytes is not None:
        command += ["--pass-stdin", "1"]
    command += ["--", "powershell.exe", "-NoProfile", "-EncodedCommand", encoded]
    completed = subprocess.run(command, input=input_bytes, capture_output=True,
                               timeout=90, check=False)
    if completed.returncode:
        raise RuntimeError(completed.stderr.decode(errors="replace"))
    result = json.loads(completed.stdout)
    if result.get("exitcode") != 0:
        raise RuntimeError(json.dumps(result))
    return result.get("out-data", "").strip()


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package", type=Path)
    parser.add_argument("--expected-sha256")
    parser.add_argument("--probe", action="store_true")
    args = parser.parse_args()
    if args.probe:
        output = guest("""
            $source = [Console]::OpenStandardInput()
            $target = New-Object IO.MemoryStream
            try { $source.CopyTo($target); $target.Length } finally { $target.Dispose(); $source.Dispose() }
        """, b"QGA_PING")
        if output != "8":
            raise ValueError(f"VM112 QGA binary stdin probe failed: {output}")
        print("VM112 QGA binary stdin probe passed")
        return
    if args.package is None or not args.expected_sha256:
        parser.error("--package and --expected-sha256 are required for transfer")
    if not args.package.is_file() or sha256(args.package) != args.expected_sha256:
        raise ValueError("PVE private package hash mismatch")

    guest("""
        $stage = 'C:\\Moxiang\\original\\stage-20261009'
        if (Test-Path -LiteralPath $stage) { throw 'VM112 stage already exists' }
        New-Item -ItemType Directory -Path $stage -Force | Out-Null
    """)
    total = 0
    with args.package.open("rb") as stream:
        while chunk := stream.read(CHUNK_SIZE):
            guest("""
                $path = 'C:\\Moxiang\\original\\stage-20261009\\package.zip'
                $source = [Console]::OpenStandardInput()
                $target = [IO.File]::Open($path, [IO.FileMode]::Append, [IO.FileAccess]::Write, [IO.FileShare]::None)
                try { $source.CopyTo($target) } finally { $target.Dispose(); $source.Dispose() }
            """, chunk)
            total += len(chunk)
            print(f"transferred {total}/{args.package.stat().st_size}", flush=True)
    actual = guest("""
        (Get-FileHash -Algorithm SHA256 -LiteralPath 'C:\\Moxiang\\original\\stage-20261009\\package.zip').Hash.ToLowerInvariant()
    """)
    if actual != args.expected_sha256:
        raise ValueError(f"VM112 package hash mismatch: {actual}")
    guest("""
        Add-Type -AssemblyName System.IO.Compression.FileSystem
        [IO.Compression.ZipFile]::ExtractToDirectory(
            'C:\\Moxiang\\original\\stage-20261009\\package.zip',
            'C:\\Moxiang\\original\\stage-20261009')
    """)
    print(json.dumps({"stage": STAGE, "packageSha256": actual, "bytes": total,
                      "extracted": True}))


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(f"transfer-original-vm112: {exc}", file=sys.stderr)
        raise SystemExit(1)
