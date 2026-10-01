"""Fail-fast audit for the existing Map10 bare-character pickup fixture.

Values below were decoded with the production MonsterCatalog, AiGroup and
SkillList parsers at 0d456d7. They are not replacement game data or stat overrides.
"""
import hashlib
import json
import math
from pathlib import Path

RESOURCE_HASHES = {
    'MonsterList.bin': 'fb7ee93e66ea9321577fe4a5031e98689d346b6fdbd5852e05d69ad7a952eebe',
    'SkillList.bin': '9b5d1fac408c610252e419f6c55b12e3bc38f9ed436fdebbdc01ec664da906b7',
    'Server/Monster_10.bin': '50033323336100da141443f3b563c62312586d149edf5cfdd78262b26d15cc01',
}


def optimistic_damage_budget(physical_attack, skill_physical, defense, hp, seconds=120, interval=0.8):
    # Current skill_caster_calculate_damage: base is clamped before 1.5x crit;
    # integer conversion truncates. This upper bound ignores misses and death.
    critical_hit = int(max(1, physical_attack + skill_physical - defense) * 1.5)
    attempts = math.floor(seconds / interval) + 1
    return {'maximumCriticalDamage': critical_hit, 'maximumAttempts': attempts,
            'optimisticDamageBudget': critical_hit * attempts, 'targetHp': hp,
            'canPossiblyKillWithinDeadline': critical_hit * attempts >= hp}


def require_viable_pickup_fixture(repo: Path, output: Path):
    resources = repo / 'modern/data/PlayDH/Resource'
    observed = {}
    for path in RESOURCE_HASHES:
        source = resources / path
        observed[path] = hashlib.sha256(source.read_bytes()).hexdigest() if source.is_file() else None
    matches = observed == RESOURCE_HASHES
    report = {
        'profile': 'map10-level48-bare-skill1', 'resourceHashesMatchAudit': matches,
        'resourceHashes': observed, 'expectedResourceHashes': RESOURCE_HASHES,
        'actor': {'level': 48, 'equippedItems': [], 'attributes': 'unknown; no authoritative import',
                  'skillId': 1, 'skillPhysicalAttack': 0, 'skillAttributeAttack': 0},
        'target': {'map': 10, 'objectId': 50023, 'kind': 73, 'level': 55,
                   'hp': 5436, 'defense': 480, 'attackMin': 108, 'attackMax': 135},
        'historicalBudgetAt0d456d7': optimistic_damage_budget(10, 0, 480, 5436),
        'readyForPlayerRun': False, 'gameplayAccepted': False,
        'blocker': 'attribute-import-required' if matches else 'fixture-audit-source-mismatch',
        'explanation': 'This old level48 fixture has no authoritative base attributes and cannot enter '
                       'the production map. Level cannot reconstruct freely allocated attributes. '
                       'The attack10 budget is historical evidence only; do not backfill guessed stats.',
        'nextStep': 'Use --prepare-pickup-input for normal new-character creation and separately audit '
                    'a viable target; see docs/EQUIPMENT_COMBAT_HYDRATION_20261001.md',
    }
    (output / 'pickup-fixture-audit.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    raise RuntimeError(report['blocker'])
