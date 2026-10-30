"""Extend source selections with mechanically safe original PAK choices."""
import argparse
import gzip
import json
from pathlib import Path


def load_manifest(path: Path):
    with gzip.open(path, 'rt', encoding='utf-8') if path.suffix == '.gz' else path.open(encoding='utf-8') as stream:
        return json.load(stream)


def extend(manifest: dict, selection: dict) -> tuple[dict, int]:
    selections = selection.setdefault('selections', {})
    attestations = selection.setdefault('attestations', {})
    added = 0
    for asset in manifest.get('assets', []):
        pak = [source for source in asset.get('sources', []) if source.get('sourceType') == 'pak']
        loose = [source for source in asset.get('sources', []) if source.get('sourceType') != 'pak']
        if len(pak) != 1:
            continue
        is_hfl = str(asset.get('logicalPath', '')).lower().endswith('.hfl')
        if is_hfl:
            if loose and any(source.get('provenance', {}).get('kind') != 'generated-placeholder' for source in loose):
                continue
        elif (not asset.get('requiresSelection') or asset.get('relation') != 'duplicate' or
              len({source.get('sha256') for source in asset.get('sources', [])}) != 1):
            continue
        source = pak[0]
        stable_id, source_id = asset['stableId'], source['sourceId']
        existing = selections.get(stable_id)
        if existing not in (None, source_id):
            raise ValueError(f'existing selection conflicts with safe PAK HFL rule: {asset["logicalPath"]}')
        selections[stable_id] = source_id
        if is_hfl and source_id not in attestations:
            attestations[source_id] = {
                'classification': 'verified-original',
                'sha256': source['sha256'],
                'evidence': (f'{source.get("container")} entry {source.get("entryIndex")}; '
                             f'audited original PAK container and structurally valid HFL bytes; '
                             f'non-PAK candidates are absent or content-proven generated placeholders'),
            }
        if existing is None:
            added += 1
    return selection, added


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path)
    parser.add_argument('selection', type=Path)
    args = parser.parse_args()
    selection = json.loads(args.selection.read_text(encoding='utf-8'))
    updated, added = extend(load_manifest(args.manifest), selection)
    args.selection.write_text(json.dumps(updated, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'addedSelections': added, 'totalSelections': len(updated['selections']),
                      'totalAttestations': len(updated['attestations'])}))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
