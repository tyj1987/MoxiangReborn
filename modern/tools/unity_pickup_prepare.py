"""Prepare an owned, newly created SQLite character through real Player protocols.

This prepares and validates input; it does not claim a viable combat profile.
"""
import json
import hashlib
import re
import sqlite3
import subprocess
import time
from pathlib import Path


def verify_resource_inputs(repo: Path):
    from unity_pickup_fixture import RESOURCE_HASHES
    hashes = dict(RESOURCE_HASHES, **{'ItemList.bin': '07d25fb98ee7f02aae3b5950ab4472847742d989775078985576c7c94a3957bd'})
    for relative, digest in hashes.items():
        if hashlib.sha256((repo/'modern/data/PlayDH/Resource'/relative).read_bytes()).hexdigest() != digest:
            raise ValueError('Preparation source changed; re-audit item limits and target stats: '+relative)
    return hashes


def verify_owned_fixture(database: Path, output: Path, account: str, character=None):
    if (database.is_symlink() or output.is_symlink() or database.resolve() != output.resolve()/'fixture.db'
            or output.parent.name != 'three-server' or not re.fullmatch('[0-9a-f]{32}', output.name)
            or account != 'ux_' + output.name[:8]):
        raise ValueError('Preparation requires the harness-owned fresh SQLite path/account')
    with sqlite3.connect(database.resolve().as_uri()+'?mode=ro', uri=True) as db:
        identities = db.execute('SELECT account_id,user_idx FROM modern_account_identity').fetchall()
        if len(identities) != 1 or identities[0][0] != account:
            raise ValueError('Preparation refuses shared or foreign accounts')
        rows = db.execute('SELECT chrid,userid,level FROM character_info').fetchall()
        if character is None:
            if rows: raise ValueError('Preparation requires a new account without characters')
        elif rows != [(character, str(identities[0][1]), 1)]:
            raise ValueError('Preparation refuses foreign, additional, or edited-level characters')


def queue_starter_weapon(database: Path, output: Path, account: str, character: int):
    verify_owned_fixture(database, output, account, character)
    with sqlite3.connect(database.resolve().as_uri()+'?mode=rw', uri=True) as db:
        if db.execute('SELECT COUNT(*) FROM modern_player_item').fetchone()[0]:
            raise ValueError('Preparation refuses pre-existing inventory')
        if db.execute('SELECT COUNT(*) FROM modern_item_grant').fetchone()[0]:
            raise ValueError('Expected unused operational grant queue')
        # Normal operational grant request; only MapServer may claim or create inventory.
        cursor = db.execute('INSERT INTO modern_item_grant '
            '(idempotency_key,character_id,item_id,item_count,status,created_by,reason) VALUES(?,?,?,?,?,?,?)',
            ('pickup-prepare-'+output.name, character, 11000, 1, 'pending',
             'isolated-pickup-preparation', 'starter weapon protocol equip/reconnect validation'))
        return cursor.lastrowid


def run_preparation(player: Path, output: Path, env: dict, database: Path, account: str, repo: Path):
    result = {'passed': False, 'preparationPassed': False, 'combatReady': False, 'humanAcceptance': False,
              'phases': [], 'parameters': {'map': 10, 'level': 1, 'weapon': 11000, 'quantity': 1,
              'grantChannel': 'modern_item_grant pending -> MapServer claim', 'equipmentPosition': 81,
              'skills': 'server default bindings only; no learned-skill rows inserted'}, 'selectedTarget': None}
    try:
        result['resourceHashes'] = verify_resource_inputs(repo)
        verify_owned_fixture(database, output, account)
        character = grant_id = None
        for phase in ('prepare-create', 'prepare-equip', 'prepare-verify'):
            directory = output/phase
            directory.mkdir()
            child_env = env.copy()
            child_env.update(MXH_SMOKE_PICKUP_LOOP='1', MXH_SMOKE_PICKUP_PHASE=phase)
            if character is not None:
                child_env.update(MXH_SMOKE_PREPARE_CHARACTER=str(character), MXH_SMOKE_PREPARE_ITEM=str(0x80000000 | grant_id))
            completed = subprocess.run([str(player), '--mxh-smoke-output', str(directory), '--mxh-pickup-loop',
                '-logFile', str(directory/'player.log')], cwd=player.parent, env=child_env, timeout=75)
            if completed.returncode != 0: raise ValueError(f'{phase}: Player failed; inspect phase report')
            report = json.loads((directory/'pickup-report.json').read_text(encoding='utf-8'))
            events = [json.loads(line) for line in (directory/'pickup-events.jsonl').read_text(encoding='utf-8').splitlines()]
            if (report.get('phase') != phase or report.get('runId') != env['MXH_RUN_ID'] or report.get('error')
                    or any(report.get(key) is not True for key in ('passed', 'gameIn', 'presentationReady', 'disconnected'))
                    or report.get('mapNumber') != 10 or not any(e['type'] == 4 and e['result'] == 'Ok' for e in events)):
                raise ValueError('Incomplete preparation protocol evidence')
            if phase == 'prepare-create':
                if report.get('characterCreated') is not True: raise ValueError('CharacterMake result missing')
                character = report['playerId']
                verify_owned_fixture(database, output, account, character)
                with sqlite3.connect(database.resolve().as_uri()+'?mode=ro', uri=True) as db:
                    equipment = db.execute('SELECT slot,item_idx FROM modern_character_equipment WHERE chrid=? ORDER BY slot', (character,)).fetchall()
                    if equipment != [(1,11000),(2,23000),(3,27000)]: raise ValueError('Server starter equipment differs from default UI creation')
                grant_id = queue_starter_weapon(database, output, account, character)
                result.update(characterId=character, grantId=grant_id, creationEquipment=equipment)
            else:
                if report.get('playerId') != character: raise ValueError('Reconnected another character')
                if phase == 'prepare-equip' and (report.get('equipmentAck') is not True or
                        not any(e['type'] == 30 and e['result'] == 'Ok' for e in events)):
                    raise ValueError('Missing authoritative equipment acknowledgement')
                deadline = time.monotonic() + 20
                while True:
                    with sqlite3.connect(database.resolve().as_uri()+'?mode=ro', uri=True, timeout=1) as db:
                        grant = db.execute('SELECT status FROM modern_item_grant WHERE grant_id=?', (grant_id,)).fetchone()
                        row = db.execute('SELECT container,slot,db_idx,item_idx,item_param FROM modern_player_item WHERE player_id=? AND db_idx=?',
                                         (character, 0x80000000 | grant_id)).fetchone()
                    if grant == ('claimed',) and row == (1,1,0x80000000 | grant_id,11000,1): break
                    if time.monotonic() >= deadline: raise TimeoutError('MapServer claim/equipment persistence missing')
                    time.sleep(0.1)
                worn = [i for i in report['after'] if i['databaseId'] == row[2]]
                if len(worn) != 1 or worn[0]['position'] != 81 or worn[0]['itemId'] != 11000 or worn[0]['itemParameter'] != 1:
                    raise ValueError('Authoritative reconnect inventory differs from SQLite equipment')
            result['phases'].append({'phase': phase, 'report': str(directory/'pickup-report.json')})
        result['preparationPassed'] = True
        result['error'] = 'combat-stats-initialization-gap: normal starter preparation is verified, but no audited viable scene target exists with default attack10'
        result['visibleMonsterIds'] = report.get('visibleMonsterIds', [])
        result['visibleMonsterKinds'] = report.get('visibleMonsterKinds', [])
        stats = {105: (5096,451), 73: (5436,480), 103: (5788,510),
                 102: (6150,541), 104: (6523,572), 219: (435024,1854)}
        result['candidateEvaluation'] = [
            {'kind': kind, 'hp': stats[kind][0] if kind in stats else None,
             'defense': stats[kind][1] if kind in stats else None, 'eligible': False,
             'reason': 'default attack10 cannot meet 120s budget' if kind in stats else 'unaudited scene kind'}
            for kind in sorted(set(result['visibleMonsterKinds']))]
        result['skillInitialization'] = {'rowsInserted': 0, 'learnedSkillProtocolVerified': False,
            'reason': 'Only server default quickbar bindings exist; learned-skill persistence is not connected'}
        if not result['visibleMonsterIds'] or len(result['visibleMonsterIds']) != len(result['visibleMonsterKinds']):
            result['error'] = 'scene-target-unavailable: no consistent materialized target evidence'
    except (OSError, ValueError, KeyError, TypeError, sqlite3.Error, subprocess.SubprocessError) as error:
        result['error'] = type(error).__name__ + ': ' + str(error)
    (output/'pickup-preparation-summary.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    return result
