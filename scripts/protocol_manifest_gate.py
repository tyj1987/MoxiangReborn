#!/usr/bin/env python3
"""Compare map and client item_protocol_sha256 manifests.

Print PROTOCOL_MATCH item=<hash> or
PROTOCOL_MISMATCH item=<map_hash>/<client_hash> and exit 0 or 1.
A missing or unreadable manifest, or a value that is not 64 lowercase
hex, is the word missing. Keep scripts/protocol_manifest_gate.ps1 in step.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

_HEX = re.compile(r"^[0-9a-f]{64}$")
_KEY = "item_protocol_sha256="


def read_item_hash(path: str) -> str:
    manifest = Path(path)
    if not manifest.is_file():
        return "missing"
    try:
        text = manifest.read_text(encoding="utf-8")
    except OSError:
        return "missing"
    for line in text.splitlines():
        stripped = line.strip()
        if not stripped.startswith(_KEY):
            continue
        value = stripped[len(_KEY) :].strip()
        if _HEX.fullmatch(value):
            return value
        return "missing"
    return "missing"


def check_manifests(map_path: str, client_path: str) -> int:
    map_hash = read_item_hash(map_path)
    client_hash = read_item_hash(client_path)
    if map_hash == "missing" or client_hash == "missing" or map_hash != client_hash:
        print("PROTOCOL_MISMATCH item=%s/%s" % (map_hash, client_hash))
        return 1
    print("PROTOCOL_MATCH item=%s" % map_hash)
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Compare item protocol manifests")
    parser.add_argument("--map", required=True, help="map server protocol.manifest")
    parser.add_argument("--client", required=True, help="client protocol.manifest")
    args = parser.parse_args(argv)
    return check_manifests(args.map, args.client)


if __name__ == "__main__":
    sys.exit(main())
