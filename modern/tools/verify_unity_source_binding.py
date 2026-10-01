"""Verify that a Unity-derived descriptor is bound to an explicitly selected audited source."""
import argparse
import gzip
import json
from pathlib import Path


def load_json(path: Path):
    if path.suffix == '.gz':
        with gzip.open(path, 'rt', encoding='utf-8') as stream:
            return json.load(stream)
    return json.loads(path.read_text(encoding='utf-8'))


def verify(manifest_path: Path, descriptor_path: Path) -> dict:
    manifest = load_json(manifest_path)
    descriptor = load_json(descriptor_path)
    source_id = descriptor.get('sourceId')
    source_sha = descriptor.get('sourceSha256')
    matches = []
    for asset in manifest.get('assets', []):
        for source in asset.get('sources', []):
            if source.get('sourceId') == source_id:
                matches.append((asset, source))
    if len(matches) != 1:
        raise ValueError(f'descriptor sourceId must resolve exactly once: {source_id}')
    asset, source = matches[0]
    if asset.get('selectedSourceId') != source_id:
        raise ValueError(f'audited source is not explicitly selected for {asset.get("logicalPath")}')
    if source.get('sha256') != source_sha:
        raise ValueError('descriptor sourceSha256 does not match audited source bytes')
    if str(asset.get('logicalPath', '')).lower().endswith('.hfl') and source.get('provenance', {}).get('kind') != 'verified-original':
        raise ValueError('heightfield source lacks verified-original provenance')
    return {
        'profileId': manifest.get('profileId'),
        'logicalPath': asset.get('logicalPath'),
        'stableId': asset.get('stableId'),
        'sourceId': source_id,
        'sha256': source_sha,
        'sourceType': source.get('sourceType'),
        'provenance': source.get('provenance', {}).get('kind'),
        'passed': True,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path)
    parser.add_argument('descriptor', type=Path)
    args = parser.parse_args()
    try:
        print(json.dumps(verify(args.manifest, args.descriptor), indent=2))
        return 0
    except (OSError, json.JSONDecodeError, ValueError) as exc:
        print(json.dumps({'passed': False, 'error': str(exc)}, indent=2))
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
