"""Validate measurement-v2 CSV and summarize valid samples only; no Unity dependency."""
import argparse
import csv
import json
import math
from pathlib import Path

FIELDS = {'schema', 'groundValid', 'groundCollider', 'bodyMeshCount', 'boundsValid',
          'renderBoundsGap', 'footMarkerValid', 'footMarkerGap', 'reason'}


def flag(row, name):
    value = row[name].strip().lower()
    if value not in {'true', 'false', '1', '0'}:
        raise ValueError(f'invalid boolean {name}')
    return value in {'true', '1'}


def summary(values):
    if not values:
        return {'valid': 0, 'mean': None, 'p95': None}
    ordered = sorted(values)
    return {'valid': len(values), 'mean': math.fsum(values) / len(values),
            'p95': ordered[math.ceil(.95 * len(values)) - 1]}


def analyze(stream):
    reader = csv.DictReader(stream)
    if not FIELDS.issubset(reader.fieldnames or []):
        raise ValueError('measurement-v2 columns required; old feet_gap is not anatomical evidence')
    total = ground_count = 0
    bounds, feet = [], []
    reasons = {}
    for row in reader:
        total += 1
        if row['schema'] != 'npc-ground-v2':
            raise ValueError(f'row {total}: unsupported schema')
        ground, body, foot = (flag(row, key) for key in ('groundValid', 'boundsValid', 'footMarkerValid'))
        count = int(row['bodyMeshCount'])
        if count < 0 or body != (count > 0):
            raise ValueError(f'row {total}: inconsistent body mesh count')
        if (body or foot) and not ground:
            raise ValueError(f'row {total}: gap cannot be valid without a floor hit')
        if ground and not row['groundCollider'].strip():
            raise ValueError(f'row {total}: valid hit must identify floor collider')
        ground_count += ground
        if not row['reason'].strip():
            raise ValueError(f'row {total}: missing reason')
        reasons[row['reason']] = reasons.get(row['reason'], 0) + 1
        for valid, name, values in ((body, 'renderBoundsGap', bounds), (foot, 'footMarkerGap', feet)):
            value = float(row[name])
            if valid:
                if not math.isfinite(value):
                    raise ValueError(f'row {total}: valid {name} must be finite')
                values.append(value)
            elif not math.isnan(value):
                raise ValueError(f'row {total}: invalid {name} must remain NaN, never zero-filled')
    return {'schema': 'npc-ground-v2-summary', 'samples': total,
            'groundValid': ground_count, 'groundInvalid': total - ground_count,
            'renderBoundsGap': summary(bounds), 'footMarkerGap': summary(feet),
            'reasons': reasons, 'acceptancePassed': None}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('csv', type=Path)
    args = parser.parse_args()
    with args.csv.open(newline='', encoding='utf-8-sig') as source:
        print(json.dumps(analyze(source), ensure_ascii=False, indent=2, allow_nan=False))
