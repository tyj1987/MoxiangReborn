"""Extract an explicitly reviewed terrain palette; never infer source priority."""
import argparse
import hashlib
import json
from pathlib import Path
from unity_asset_extract import extract
from unity_resource_audit import AuditError


def build(manifest, root, plan_path, output):
    root = root.resolve(strict=True)
    output = output.resolve(strict=False)
    if output.is_relative_to(root):
        raise AuditError('Output must be outside immutable source root')
    plan = json.loads(plan_path.read_text(encoding='utf-8'))
    if plan.get('schemaVersion') != 1 or plan.get('releaseReady') is not False:
        raise AuditError('Only explicit development palette plans are supported')
    entries = plan.get('entries', [])
    if not entries or len({e['slot'] for e in entries}) != len(entries):
        raise AuditError('Palette slots must be nonempty and unique')
    names = set()
    for entry in entries:
        name = entry['file']
        if Path(name).name != name or '/' in name or '\\' in name or ':' in name or not name.endswith('.mxhdds'):
            raise AuditError('Palette files must be sibling DDS files')
        if name.lower() in names or type(entry['slot']) is not int or not 0 <= entry['slot'] <= 0x3fff:
            raise AuditError('Duplicate filename or invalid slot')
        names.add(name.lower())
    if output.exists():
        raise AuditError('Output must be new; no implicit overwrite')
    output.mkdir(parents=True)
    result = dict(plan)
    result['entries'] = []
    for entry in entries:
        receipt = extract(manifest, root, entry['sourceId'], output / entry['file'], True)
        if receipt['sha256'] != entry['sha256']:
            raise AuditError('Reviewed palette hash differs from source')
        result['entries'].append(dict(entry, provenance=receipt['provenance']))
    result['planSha256'] = hashlib.sha256(plan_path.read_bytes()).hexdigest()
    (output / 'palette.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    return len(entries)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('manifest', 'root', 'plan', 'output'): parser.add_argument(key, type=Path)
    args = parser.parse_args()
    print(json.dumps({'extracted': build(args.manifest, args.root, args.plan, args.output), 'releaseReady': False}))
