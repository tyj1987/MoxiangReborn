#!/usr/bin/env python3
"""Stage audited extracted entity models and textures as Unity derivatives."""
from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
from pathlib import Path

from PIL import Image


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--extracted", required=True, type=Path)
    parser.add_argument("--pak-name", required=True)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--prefix", default="L")
    parser.add_argument("--kinds", required=True, nargs="+", type=int)
    parser.add_argument("--motion-exporter", type=Path)
    args = parser.parse_args()
    manifest = json.loads((args.extracted / "manifest.json").read_text(encoding="utf-8"))
    entries = {entry["name"].lower(): entry for entry in manifest["entries"]}
    for kind in args.kinds:
        stem = f"{args.prefix}{kind:03d}".lower()
        chx = args.extracted / f"{stem}.chx"
        model = args.extracted / f"{stem}_lod2.mod"
        descriptor = args.extracted / f"{stem}_lod2.mxhmodel"
        for required in (chx, model, descriptor):
            if not required.is_file(): raise FileNotFoundError(required)
        model_doc = json.loads(descriptor.read_text(encoding="utf-8"))
        texture_names = sorted({Path(item["textureName"]).stem.lower() for item in model_doc["materials"]})
        target = args.out / f"M{kind:03d}"
        textures = target / "Textures"
        textures.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(descriptor, target / f"{stem}_lod2.mxhmodel")
        texture_sources = []
        for texture_stem in texture_names:
            candidates = [args.extracted / f"{texture_stem}.dds", args.extracted / f"{texture_stem}.tif"]
            source = next((path for path in candidates if path.is_file()), None)
            if source is None: raise FileNotFoundError(f"texture for {texture_stem}")
            if source.suffix.lower() == ".dds":
                derivative = textures / f"{texture_stem}.mxhdds"
                shutil.copyfile(source, derivative)
            else:
                derivative = textures / f"{texture_stem}.png"
                with Image.open(source) as image:
                    image.convert("RGBA").save(derivative, format="PNG", optimize=False)
            entry = entries[source.name.lower()]
            if sha(source) != entry["sha256"]: raise ValueError(f"source hash mismatch: {source}")
            texture_sources.append({"pak": args.pak_name, **entry, "derivative": derivative.name,
                                    "derivativeSha256": sha(derivative)})
        motion_entries = [entry for name, entry in entries.items() if name.startswith(stem + "_") and name.endswith(".anm")]
        motion_derivatives = []
        if args.motion_exporter:
            motion_dir = target / "Motions"
            motion_dir.mkdir(parents=True, exist_ok=True)
            for entry in sorted(motion_entries, key=lambda item: item["name"]):
                source = args.extracted / entry["name"].lower()
                derivative = motion_dir / (source.stem + ".mxhmotion")
                derivative.unlink(missing_ok=True)
                subprocess.run([args.motion_exporter, source, derivative, entry["sha256"],
                                f"{args.pak_name}:{entry['name']}"], check=True)
                motion_derivatives.append({"name": str(derivative.relative_to(target)).replace('\\', '/'),
                                           "sha256": sha(derivative)})
        provenance = {
            "schemaVersion": 1, "stableId": f"monster-visual-kind-{kind}",
            "classification": "recovered-original", "source": {
                "chx": {"pak": args.pak_name, **entries[chx.name.lower()]},
                "model": {"pak": args.pak_name, **entries[model.name.lower()]},
                "textures": texture_sources, "motions": [{"pak": args.pak_name, **entry} for entry in motion_entries]},
            "derivativeModel": descriptor.name, "derivativeModelSha256": sha(target / descriptor.name),
            "motionDerivatives": motion_derivatives
        }
        (target / "provenance.json").write_text(json.dumps(provenance, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        print(f"kind={kind} textures={len(texture_sources)} motions={len(motion_entries)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
