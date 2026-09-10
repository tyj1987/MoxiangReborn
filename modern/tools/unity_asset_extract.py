#!/usr/bin/env python3
"""Extract one explicitly identified source for development, without loose-first fallback."""
import argparse
import hashlib
from pathlib import Path
import sys

from unity_resource_audit import AuditError, _read_manifest, _sha256_file, normalize_relative_path, validate_manifest_document


def extract(manifest_path: Path, root: Path, source_id: str, output: Path,
            allow_unverified: bool = False) -> dict:
    manifest = _read_manifest(manifest_path)
    if not validate_manifest_document(manifest, "integrity")["passed"]:
        raise AuditError("manifest integrity validation failed")
    candidates = [(asset, source) for asset in manifest["assets"] for source in asset["sources"]
                  if source["sourceId"] == source_id]
    if len(candidates) != 1:
        raise AuditError("source ID is not unique in manifest")
    asset, source = candidates[0]
    kind = source.get("provenance", {}).get("kind", "unknown")
    if kind == "generated-placeholder":
        raise AuditError("content-proven placeholder cannot be extracted for remaster import")
    if kind != "verified-original" and not allow_unverified:
        raise AuditError("unverified source requires explicit development-only --allow-unverified")
    root = root.resolve(strict=True)
    output = output.resolve(strict=False)
    if output.is_relative_to(root) or output.exists():
        raise AuditError("output must be new and outside immutable source root")
    if source["sourceType"] == "pak":
        relative = normalize_relative_path(source["container"])
        path = (root / relative).resolve(strict=True)
        if not path.is_relative_to(root):
            raise AuditError("container escapes source root")
        pack = next((p for p in manifest["packs"] if p["path"] == source["container"]), None)
        if not pack or path.stat().st_size != pack["bytes"] or _sha256_file(path) != pack["sha256"]:
            raise AuditError("container hash/size does not match baseline")
        with path.open("rb") as stream:
            stream.seek(source["entryOffset"])  # schema v1: payload byte offset, not header offset
            data = stream.read(source["bytes"])
    else:
        path = (root / normalize_relative_path(source["physicalPath"])).resolve(strict=True)
        if not path.is_relative_to(root):
            raise AuditError("loose source escapes source root")
        data = path.read_bytes()
    if len(data) != source["bytes"] or hashlib.sha256(data).hexdigest() != source["sha256"]:
        raise AuditError("source bytes differ from baseline")
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("xb") as stream:
        stream.write(data)
    return {"logicalPath": asset["logicalPath"], "sourceId": source_id,
            "sha256": source["sha256"], "provenance": kind, "releaseReady": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("root", type=Path)
    parser.add_argument("source_id")
    parser.add_argument("output", type=Path)
    parser.add_argument("--allow-unverified", action="store_true")
    args = parser.parse_args()
    try:
        import json
        print(json.dumps(extract(args.manifest, args.root, args.source_id, args.output, args.allow_unverified), sort_keys=True))
        return 0
    except (AuditError, OSError, ValueError) as error:
        print(str(error), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
