"""Evidence gates for an opt-in, generated-account, loopback SQLite Player run."""
from __future__ import annotations

import json
import sqlite3
import subprocess
import time
from pathlib import Path

FIELDS = ('databaseId', 'itemId', 'position', 'durability', 'rareId', 'quickPosition', 'itemParameter')


def inventory(items: list[dict]) -> list[tuple]:
    rows = sorted(tuple(item[key] for key in FIELDS) for item in items)
    if any(any(type(value) is not int or value < 0 for value in row) or row[0] == 0 or row[1] == 0 for row in rows):
        raise ValueError('Invalid inventory identity/value')
    if len({row[0] for row in rows}) != len(rows) or len({row[2] for row in rows}) != len(rows):
        raise ValueError('Duplicate persistent ID or inventory slot')
    return rows


def validate_phase(report: dict, events: list[dict], phase: str, previous: list[dict], run_id: str) -> None:
    if report.get('version') != 1 or report.get('phase') != phase or report.get('runId') != run_id:
        raise ValueError('Wrong evidence version/run/phase')
    for field in ('passed', 'gameIn', 'presentationReady', 'disconnected'):
        if report.get(field) is not True:
            raise ValueError(f'Missing {field} evidence')
    if report.get('error') or report.get('playerId') != 111 or report.get('mapNumber') != 10:
        raise ValueError('Player error or wrong fixture identity/map')
    if report.get('humanAcceptance') is not False or report.get('mouseInteraction') is not False:
        raise ValueError('Probe must not claim human/mouse acceptance')
    if not any(e.get('type') == 4 and e.get('result') == 'Ok' for e in events):
        raise ValueError('No native disconnect confirmation')
    before, after = inventory(report['before']), inventory(report['after'])
    if before != inventory(previous):
        raise ValueError('Reconnect inventory differs from previous persisted session')
    if phase == 'verify':
        if after != before: raise ValueError('Verify-only session mutated inventory')
        return
    for field in ('hit', 'zeroLife', 'dropObserved', 'pickupSubmitted', 'pickupAck', 'inventoryFresh'):
        if report.get(field) is not True: raise ValueError(f'Missing {field} evidence')
    if not set(before).issubset(after) or len(after) != len(before) + 1:
        raise ValueError('Existing items changed or pickup did not add exactly one item')
    added = next(row for row in after if row not in before)
    if added[0] != report.get('acquiredDatabaseId') or added[1] != report.get('itemId') or added[6] != report.get('count'):
        raise ValueError('Pickup differs from authoritative inventory')
    if added[0] in {row[0] for row in before}:
        raise ValueError('Persistent ID reused')
    # Validate native sequence evidence independently of the report booleans.
    def matching(kind, object_id):
        return [e for e in events if e['type'] == kind and e['argument0'] == object_id and e['result'] == 'Ok']
    hits = [e for e in matching(34, 50023) if e['argument1'] > 0 and e['reserved0'] != 0]
    deaths = [e for e in matching(15, 50023) if e['argument1'] == 0]
    drops = [e for e in matching(17, report['dropId']) if e['argument1'] == report['itemId']]
    acks = [e for e in matching(18, report['dropId']) if e['argument1'] == report['itemId']]
    complete = [e for e in matching(24, 111) if e['reserved0'] == 121 and e['argument1'] == 124]
    if not all((hits, deaths, drops, acks, complete)):
        raise ValueError('Missing protocol evidence for hit/zero life/drop/ack/full inventory')
    death, drop, ack = deaths[-1], drops[-1], acks[-1]
    fresh = [e for e in complete if e['sequence'] > ack['sequence']]
    if not fresh or not death['sequence'] < drop['sequence'] < ack['sequence']:
        raise ValueError('Invalid kill/drop/pickup/inventory sequence')
    if len({(e['session'], e['map']) for e in (hits[-1], death, drop, ack, fresh[-1])}) != 1:
        raise ValueError('Evidence mixed across session/map generations')


def read_persisted(database: Path) -> list[tuple]:
    with sqlite3.connect(database.resolve().as_uri() + '?mode=ro', uri=True, timeout=1) as db:
        rows = db.execute('SELECT db_idx,item_idx,slot,durability,rare_idx,quick_position,item_param,container '
                          'FROM modern_player_item WHERE player_id=111 ORDER BY db_idx').fetchall()
    if any(row[7] != 0 for row in rows):
        raise ValueError('Unexpected non-carried item in isolated pickup fixture')
    return sorted(row[:7] for row in rows)


def run_pickup_loop(player: Path, output: Path, env: dict, database: Path, restart_servers) -> dict:
    result = {'passed': False, 'humanAcceptance': False, 'mouseInteraction': False,
              'serverRestarts': 0, 'phases': [], 'dropAssociation': 'single account, only target 50023 attacked; native drop event does not expose source monster ID'}
    previous: list[dict] = []
    try:
        if read_persisted(database): raise ValueError('Pickup fixture must start with empty inventory')
        for phase in ('first', 'second', 'verify'):
            phase_dir = output / ('pickup-' + phase)
            phase_dir.mkdir()
            child_env = env.copy()
            child_env.update(MXH_SMOKE_PICKUP_LOOP='1', MXH_SMOKE_PICKUP_PHASE=phase)
            completed = subprocess.run([str(player), '--mxh-smoke-output', str(phase_dir), '--mxh-pickup-loop',
                '-logFile', str(phase_dir / 'player.log'), '-screen-width', '1280', '-screen-height', '720',
                '-screen-fullscreen', '0'], cwd=player.parent, env=child_env, timeout=240)
            if completed.returncode != 0: raise ValueError(f'{phase}: Player failed with exit {completed.returncode}')
            report = json.loads((phase_dir / 'pickup-report.json').read_text(encoding='utf-8'))
            events = [json.loads(line) for line in (phase_dir / 'pickup-events.jsonl').read_text(encoding='utf-8').splitlines()]
            validate_phase(report, events, phase, previous, env['MXH_RUN_ID'])
            expected = inventory(report['after'])
            deadline = time.monotonic() + 20
            while True:
                persisted = read_persisted(database)
                if persisted == expected: break
                if time.monotonic() >= deadline: raise ValueError(f'{phase}: SQLite persistence deadline exceeded')
                time.sleep(0.1)
            (phase_dir / 'persisted-inventory.json').write_text(json.dumps(persisted, indent=2), encoding='utf-8')
            result['phases'].append({'phase': phase, 'passed': True, 'report': str(phase_dir / 'pickup-report.json'), 'persisted': persisted})
            previous = report['after']
            if phase != 'verify':
                restart_servers()  # Own subprocesses, same database; no state/reward edits between sessions.
                result['serverRestarts'] += 1
        if len(inventory(previous)) != 2: raise ValueError('Expected two distinct persisted pickup IDs')
        result['passed'] = True
    except (OSError, ValueError, KeyError, TypeError, sqlite3.Error, subprocess.SubprocessError, RuntimeError) as error:
        result['error'] = type(error).__name__ + ': ' + str(error)
    (output / 'pickup-loop-summary.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    return result
