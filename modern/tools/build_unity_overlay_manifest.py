"""Build and verify the unity-remaster-v1 development overlay manifest."""
import argparse
import hashlib
import json
from pathlib import Path


def stable_id(logical_path: str) -> str:
    return 'mxh:' + hashlib.sha256(('unity-remaster-overlay-v1\0' + logical_path.lower()).encode()).hexdigest()


def build(audit_report: dict, overlay_root: Path) -> dict:
    assets = []
    for entry in audit_report.get('extras', []):
        logical = entry['path'].replace('\\', '/')
        target = overlay_root / logical
        raw = target.read_bytes()
        digest = hashlib.sha256(raw).hexdigest()
        if len(raw) != entry['bytes'] or digest != entry['sha256']:
            raise ValueError(f'overlay bytes do not match relocation audit: {logical}')
        placeholder = logical.lower().endswith('.hfl')
        generated_dds = logical.lower().endswith('.dds')
        assets.append({
            'stableId': stable_id(logical),
            'logicalPath': logical,
            'originalLoosePath': f'modern/data/PlayDH/{logical}',
            'overlayPath': target.relative_to(overlay_root.parent.parent.parent).as_posix(),
            'bytes': len(raw),
            'sha256': digest,
            'classification': ('development-placeholder' if placeholder else
                               'generated-ui-placeholder' if generated_dds else 'recovered-duplicate'),
            'converterVersion': ('gen-hfl-placeholder-v1' if placeholder else
                                 'polish-assets-v1' if generated_dds else 'byte-preserving-relocation-v1'),
            'dependencies': ['Map.pak!/10.stm'] if not placeholder else [],
            'releaseAllowed': False,
        })
    assets.sort(key=lambda item: item['logicalPath'].lower())
    if len(assets) != 241:
        raise ValueError(f'expected 241 relocated derivative assets, found {len(assets)}')
    inventory = hashlib.sha256('\n'.join(
        f'{item["logicalPath"]}\0{item["bytes"]}\0{item["sha256"]}' for item in assets
    ).encode()).hexdigest()
    return {'schemaVersion': 1, 'profileId': 'unity-remaster-v1',
            'kind': 'development-source-overlay', 'releaseAllowed': False,
            'assetCount': len(assets), 'inventorySha256': inventory, 'assets': assets}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('audit_report', type=Path)
    parser.add_argument('overlay_root', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    report = json.loads(args.audit_report.read_text(encoding='utf-8'))
    manifest = build(report, args.overlay_root.resolve())
    args.output.write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'assetCount': manifest['assetCount'],
                      'inventorySha256': manifest['inventorySha256']}))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
