"""Isolated loopback-only real server + Unity Player smoke; never production acceptance."""
from __future__ import annotations
import argparse
import atexit
import hashlib
import json
import os
import secrets
import socket
import sqlite3
import subprocess
import struct
import time
import uuid
import xml.etree.ElementTree as ET
from pathlib import Path


def require_viable_pickup_fixture(repo: Path, output: Path):
    from unity_pickup_fixture import require_viable_pickup_fixture as audit
    audit(repo, output)


def sql_text(value: str) -> str:
    return "'" + value.replace("'", "''") + "'"


def dbtool_run(dbtool: Path, env: dict, quiet: dict, command: str, sql: str) -> subprocess.CompletedProcess:
    return subprocess.run([str(dbtool), command, '--db-env', 'MXH_UNITY_SMOKE_DATABASE', sql],
                          env=env, check=True, capture_output=True, text=True, **quiet)


def dbtool_rows(dbtool: Path, env: dict, quiet: dict, sql: str) -> list[tuple[str, ...]]:
    lines = dbtool_run(dbtool, env, quiet, 'query', sql).stdout.splitlines()
    if len(lines) < 2 or not lines[-1].startswith('('):
        raise RuntimeError('Unexpected mxh_db_tool query output')
    return [tuple(line.split('\t')) for line in lines[1:-1]]


def cleanup_mssql_fixture(dbtool: Path, env: dict, quiet: dict, account: str, observer_account: str,
                          fixture_name: str, observer_name: str) -> None:
    identities = dbtool_rows(dbtool, env, quiet,
        f"SELECT user_idx FROM modern_account_identity WHERE account_id IN ({sql_text(account)},{sql_text(observer_account)})")
    user_ids = ','.join(sql_text(row[0]) for row in identities)
    ownership = f"charname IN ({sql_text(fixture_name)},{sql_text(observer_name)})"
    if user_ids:
        ownership += f" OR userid IN ({user_ids})"
    cleanup = (
        f"DELETE FROM modern_player_quest_sub WHERE player_id IN (SELECT chrid FROM character_info WHERE {ownership});"
        f"DELETE FROM modern_player_quest_log WHERE player_id IN (SELECT chrid FROM character_info WHERE {ownership});"
        f"DELETE FROM modern_character_equipment WHERE chrid IN (SELECT chrid FROM character_info WHERE {ownership});"
        f"DELETE FROM modern_player_item WHERE player_id IN (SELECT chrid FROM character_info WHERE {ownership});"
        f"DELETE FROM modern_player_position WHERE player_id IN (SELECT chrid FROM character_info WHERE {ownership});"
        f"DELETE FROM modern_player_state WHERE player_id IN (SELECT chrid FROM character_info WHERE {ownership});"
        f"DELETE FROM character_info WHERE {ownership};"
        f"DELETE FROM modern_account_identity WHERE account_id IN ({sql_text(account)},{sql_text(observer_account)});"
        f"DELETE FROM chr_log_info WHERE id IN ({sql_text(account)},{sql_text(observer_account)})")
    dbtool_run(dbtool, env, quiet, 'exec', cleanup)


def collision_fixture(resources: Path) -> dict:
    source = resources / 'Resource/Map/10.ttb'
    raw = source.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if digest != '6b1cc9a83d79aa7f764e1d19446ee3aff6d9625f7d8a52b02ffa8be7e4b78f5b':
        raise RuntimeError('Map10 collision fixture digest changed; re-audit before updating fixture')
    width, height = struct.unpack_from('<ii', raw)
    if (width, height, len(raw)) != (1024, 1024, 2097160):
        raise RuntimeError('Map10 fixed tile layout changed')
    start_x, start_z = 25064, 25032  # Movement probe's accepted Move/Stop endpoint.
    candidates = []
    for z in range(max(0, start_z // 50 - 89), min(height, start_z // 50 + 90)):
        for x in range(max(0, start_x // 50 - 89), min(width, start_x // 50 + 90)):
            if not struct.unpack_from('<H', raw, 8 + 2 * (z * width + x))[0] & 1:
                continue
            px, pz = x * 50 + 25, z * 50 + 25
            distance_squared = (px - start_x) ** 2 + (pz - start_z) ** 2
            if 0 < distance_squared < 4500 ** 2:
                candidates.append((distance_squared, px, pz))
    if not candidates:
        raise RuntimeError('No blocked Map10 target inside the non-jump collision probe radius')
    distance_squared, x, z = min(candidates)
    return {'map': 10, 'sha256': digest, 'x': x, 'z': z,
            'fromX': start_x, 'fromZ': start_z, 'distanceSquared': distance_squared,
            'attribute': struct.unpack_from('<H', raw, 8 + 2 * ((z // 50) * width + x // 50))[0]}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--player', type=Path, required=True)
    parser.add_argument('--editor-test', action='store_true', help='Run Unity EditMode against the same real servers instead of the Player')
    parser.add_argument('--create-character', action='store_true', help='Start with an empty account and create through the real client protocol')
    parser.add_argument('--movement', action='store_true', help='Verify two native sessions with real movement broadcasts and correction')
    parser.add_argument('--trade', action='store_true', help='Verify real Map12 NPC 92 purchase and reconnect persistence in Editor')
    parser.add_argument('--equipment', action='store_true', help='Verify real Map10 unequip/equip and reconnect persistence in Editor')
    parser.add_argument('--item-use', action='store_true', help='Verify real Map10 consumable use and reconnect persistence in Editor')
    parser.add_argument('--sell', action='store_true', help='Verify real Map12 NPC 92 sale and reconnect persistence in Editor')
    parser.add_argument('--discard', action='store_true', help='Verify real Map10 item discard and reconnect persistence in Editor')
    parser.add_argument('--quest', action='store_true', help='Verify real Map10 quest acceptance and reconnect persistence in Editor')
    parser.add_argument('--quest-reward', action='store_true', help='Verify canonical quest 173 completion, reward and reconnect persistence in Editor')
    parser.add_argument('--quest-npc', action='store_true', help='Verify canonical Map10 NPC 572 advances quest 180 and persists')
    parser.add_argument('--combat-timeline', action='store_true', help='Verify authoritative skill release and hit drive the original monster hit animation in Player')
    parser.add_argument('--pickup-loop', action='store_true', help='Isolated SQLite Player kill/pickup/restart/relogin evidence; no reward overrides')
    parser.add_argument('--player-death', action='store_true', help='With --combat-timeline, verify server death and dead input rejection')
    parser.add_argument('--transfer', action='store_true', help='Verify canonical Map10 to Map2 transfer using two real MapServers')
    parser.add_argument('--backend', choices=('sqlite', 'mssql_odbc'), default='sqlite')
    parser.add_argument('--mssql-config-env', default='MXH_MSSQL_E2E',
                        help='Environment variable containing the MSSQL connection config; its value is never logged')
    args = parser.parse_args()
    if args.pickup_loop and (args.backend != 'sqlite' or any((args.editor_test, args.create_character,
            args.movement, args.trade, args.equipment, args.item_use, args.sell, args.discard, args.quest,
            args.quest_reward, args.quest_npc, args.combat_timeline, args.player_death, args.transfer))):
        parser.error('--pickup-loop requires a separate standalone SQLite fixture')
    if args.transfer and (not args.editor_test or args.trade or args.equipment or args.item_use or args.sell or args.discard or args.quest or args.movement or args.create_character):
        parser.error('--transfer requires --editor-test and an isolated transfer fixture')
    if args.trade and (not args.editor_test or args.equipment or args.item_use or args.sell or args.discard or args.movement or args.create_character):
        parser.error('--trade requires --editor-test and a separate fixture from movement/creation')
    if args.equipment and (not args.editor_test or args.item_use or args.sell or args.discard or args.movement or args.create_character or args.transfer):
        parser.error('--equipment requires --editor-test and a separate fixture')
    if args.item_use and (not args.editor_test or args.sell or args.discard or args.movement or args.create_character or args.transfer):
        parser.error('--item-use requires --editor-test and a separate fixture')
    if args.sell and (not args.editor_test or args.discard or args.movement or args.create_character or args.transfer):
        parser.error('--sell requires --editor-test and a separate fixture')
    if args.discard and (not args.editor_test or args.quest or args.movement or args.create_character or args.transfer):
        parser.error('--discard requires --editor-test and a separate fixture')
    if args.quest and (not args.editor_test or args.trade or args.equipment or args.item_use or args.sell or args.discard or args.movement or args.create_character or args.transfer):
        parser.error('--quest requires --editor-test and a separate fixture')
    if args.quest_reward and (not args.editor_test or args.trade or args.equipment or args.item_use or args.sell or args.discard or args.quest or args.movement or args.create_character or args.transfer):
        parser.error('--quest-reward requires --editor-test and a separate fixture')
    if args.quest_npc and (args.trade or args.equipment or args.item_use or args.sell or args.discard or args.quest or args.quest_reward or args.movement or args.create_character or args.transfer):
        parser.error('--quest-npc requires a separate fixture')
    if args.player_death and not args.combat_timeline:
        parser.error('--player-death requires --combat-timeline')
    if args.combat_timeline and (args.editor_test or args.trade or args.equipment or args.item_use or args.sell or args.discard or args.quest or args.quest_reward or args.quest_npc or args.movement or args.create_character or args.transfer):
        parser.error('--combat-timeline requires a standalone Player fixture')
    map_number = 12 if args.trade or args.sell else 10
    if args.movement and args.create_character:
        parser.error('Run movement and empty-account creation as separate isolated fixtures')
    repo = Path(__file__).resolve().parents[2]
    output = repo / 'modern/out/unity-remaster/three-server' / uuid.uuid4().hex
    output.mkdir(parents=True)
    return execute_reported(args, parser, repo, output, map_number)


def execute_reported(args, parser, repo: Path, output: Path, map_number: int) -> int:
    """One failure boundary includes setup/startup and the existing owned-process cleanup."""
    progress = {'stage': 'player-path'}
    try:
        player = args.player.resolve(strict=True)
        return run_fixture(args, parser, repo, player, output, map_number, progress)
    except Exception as error:
        # Do not serialize command lines, subprocess stdout/stderr, env or credentials.
        failure = {'runId': output.name, 'passed': False, 'humanAcceptance': False,
                   'surface': 'Editor' if args.editor_test else 'Player', 'backend': args.backend,
                   'output': str(output), 'failureStage': progress['stage'],
                   'fixtureAudit': str(output / 'pickup-fixture-audit.json') if (output / 'pickup-fixture-audit.json').exists() else None,
                   'server': progress.get('server'), 'errorType': type(error).__name__,
                   'timeoutSeconds': error.timeout if isinstance(error, subprocess.TimeoutExpired)
                       else (20 if isinstance(error, TimeoutError) and progress.get('server') else None),
                   'returnCode': getattr(error, 'returncode', None)}
        # Finite writes, no retries. Stdout remains machine-readable if disk writes fail.
        try:
            (output / 'failure-summary.json').write_text(json.dumps(failure, indent=2), encoding='utf-8')
            (output / 'three-server-summary.json').write_text(json.dumps(failure, indent=2), encoding='utf-8')
            if args.pickup_loop:
                path = output / 'pickup-loop-summary.json'
                details = json.loads(path.read_text(encoding='utf-8')) if path.exists() else {'phases': []}
                details.update(passed=False, humanAcceptance=False, failure=failure)
                path.write_text(json.dumps(details, indent=2), encoding='utf-8')
        except (OSError, ValueError) as write_error:
            failure['reportWriteError'] = type(write_error).__name__
        print(json.dumps(failure))
        return 1


def run_fixture(args, parser, repo: Path, player: Path, output: Path, map_number: int, progress: dict) -> int:
    if args.pickup_loop:
        progress['stage'] = 'combat-fixture-preflight'
        require_viable_pickup_fixture(repo, output)
    progress['stage'] = 'fixture-setup'
    database = output / 'fixture.db'
    dbtool = repo / 'modern/build/tools/MoxianDbTool/mxh_db_tool.exe'
    env = os.environ.copy()
    env.pop('MXH_SMOKE_TRANSFER', None)
    for key in ('MXH_SMOKE_CREATE_NAME', 'MXH_SMOKE_OBSERVER_USER', 'MXH_SMOKE_BLOCKED_X', 'MXH_SMOKE_BLOCKED_Z', 'MXH_SMOKE_TRADE', 'MXH_SMOKE_EQUIPMENT', 'MXH_SMOKE_ITEM_USE', 'MXH_SMOKE_SELL', 'MXH_SMOKE_DISCARD', 'MXH_SMOKE_QUEST', 'MXH_SMOKE_QUEST_REWARD', 'MXH_SMOKE_QUEST_NPC', 'MXH_SMOKE_COMBAT_TIMELINE', 'MXH_SMOKE_PLAYER_DEATH', 'MXH_SMOKE_PICKUP_LOOP', 'MXH_SMOKE_PICKUP_PHASE'):
        env.pop(key, None)
    if args.backend == 'sqlite':
        env['MXH_UNITY_SMOKE_DATABASE'] = f'backend=sqlite;path={database}'
    else:
        mssql_config = os.environ.get(args.mssql_config_env, '')
        normalized = mssql_config.lower().replace(' ', '')
        if 'backend=mssql_odbc' not in normalized or 'database=mxh_test' not in normalized:
            parser.error(f'--backend mssql_odbc requires {args.mssql_config_env} to select the dedicated mxh_test database')
        env['MXH_UNITY_SMOKE_DATABASE'] = mssql_config
    env['MXH_RUN_ID'] = output.name
    common = ['--backend', args.backend, '--db-env', 'MXH_UNITY_SMOKE_DATABASE']
    quiet = {'creationflags': subprocess.CREATE_NO_WINDOW} if os.name == 'nt' else {}
    progress['stage'] = 'database-migration'
    subprocess.run([str(dbtool), 'migrate', '--db-env', 'MXH_UNITY_SMOKE_DATABASE'], env=env, check=True, capture_output=True, timeout=30 if args.pickup_loop else None, **quiet)
    run_suffix = output.name[:8]
    account, password = f'ux_{run_suffix}', 'Mx1' + secrets.token_hex(6)
    progress['stage'] = 'account-registration'
    subprocess.run([str(dbtool), 'register', '--db-env', 'MXH_UNITY_SMOKE_DATABASE', account], input=password + '\n', text=True, env=env, check=True, capture_output=True, timeout=30 if args.pickup_loop else None, **quiet)
    progress['stage'] = 'fixture-seeding'
    observer_account = f'uo_{run_suffix}'
    if args.movement:
        subprocess.run([str(dbtool), 'register', '--db-env', 'MXH_UNITY_SMOKE_DATABASE', observer_account], input=password + '\n', text=True, env=env, check=True, capture_output=True, **quiet)
    identity = None
    fixture_name = f'UnitySmoke{run_suffix}'
    observer_name = f'UnityObserver{run_suffix}'
    create_name = f'UnityNew{run_suffix}'
    cleanup_registered = False
    if args.backend == 'mssql_odbc':
        atexit.register(cleanup_mssql_fixture, dbtool, env, quiet, account, observer_account,
                        fixture_name, observer_name)
        cleanup_registered = True
    if args.backend == 'sqlite':
      with sqlite3.connect(database) as db:
        identity = db.execute('SELECT user_idx FROM modern_account_identity WHERE account_id=?', (account,)).fetchone()
        if identity is None:
            # Same persistent identity table consumed by LoginServer; isolated fixture only.
            db.execute('INSERT INTO modern_account_identity(account_id,user_idx) VALUES(?,?)', (account, 1))
            identity = (1,)
        if not args.create_character:
            db.execute('INSERT INTO character_info(charname,chrid,userid,map_num,start_area) VALUES(?,?,?,?,?)', (fixture_name, 111, str(identity[0]), map_number, map_number))
        if args.trade:
            db.execute("INSERT INTO modern_player_state(player_id,money,updated_at) VALUES(111,100000000,CURRENT_TIMESTAMP)")
        if args.equipment:
            db.execute("INSERT INTO modern_player_item(player_id,container,slot,db_idx,item_idx,durability,rare_idx,quick_position,item_param) VALUES(111,1,1,9001,11000,100,0,65535,1)")
        if args.item_use:
            db.execute("INSERT INTO modern_player_item(player_id,container,slot,db_idx,item_idx,durability,rare_idx,quick_position,item_param) VALUES(111,0,0,9001,1,1,0,65535,1)")
        if args.sell:
            db.execute("INSERT INTO modern_player_state(player_id,money,updated_at) VALUES(111,1000,CURRENT_TIMESTAMP)")
            db.execute("INSERT INTO modern_player_item(player_id,container,slot,db_idx,item_idx,durability,rare_idx,quick_position,item_param) VALUES(111,0,0,9001,53343,1,0,65535,1)")
        if args.discard:
            db.execute("INSERT INTO modern_player_item(player_id,container,slot,db_idx,item_idx,durability,rare_idx,quick_position,item_param) VALUES(111,0,0,9001,53343,1,0,65535,1)")
        if args.quest_reward:
            db.execute("INSERT INTO modern_player_state(player_id,money,level,exp,updated_at) VALUES(111,0,48,0,CURRENT_TIMESTAMP)")
            db.execute("INSERT INTO modern_player_position VALUES(111,10,44178,13253,CURRENT_TIMESTAMP)")
            db.execute("INSERT INTO modern_player_quest_log(player_id,quest_id,state,accepted_time_ms,updated_at) VALUES(111,173,1,123456,CURRENT_TIMESTAMP)")
            db.executemany("INSERT INTO modern_player_quest_sub(player_id,quest_id,sub_index,kind,target_id,count,target_count) VALUES(111,173,?,?,?,?,?)",
                           ((0,4,38,1,1),(1,1,73,29,30),(2,4,38,1,1)))
        if args.quest_npc:
            db.execute("INSERT INTO modern_player_state(player_id,money,level,exp,updated_at) VALUES(111,0,50,0,CURRENT_TIMESTAMP)")
            db.execute("INSERT INTO modern_player_position VALUES(111,10,3500,46900,CURRENT_TIMESTAMP)")
            db.execute("INSERT INTO modern_player_quest_log(player_id,quest_id,state,accepted_time_ms,updated_at) VALUES(111,180,1,123456,CURRENT_TIMESTAMP)")
            db.execute("INSERT INTO modern_player_quest_sub(player_id,quest_id,sub_index,kind,target_id,count,target_count) VALUES(111,180,0,4,41,1,1)")
        if args.combat_timeline or args.pickup_loop:
            db.execute("INSERT INTO modern_player_state(player_id,money,level,exp,updated_at) VALUES(111,0,48,0,CURRENT_TIMESTAMP)")
            db.execute("INSERT INTO modern_player_position VALUES(111,10,44178,13253,CURRENT_TIMESTAMP)")
        if args.transfer:
            route_path = repo / 'modern/data/PlayDH/Resource/MapChange.bin'
            if hashlib.sha256(route_path.read_bytes()).hexdigest() != '66aca3ccca86469e4f8ec1f0dada451fd2422d5c4b0c2698408e56c0101b7048':
                raise RuntimeError('Canonical route fixture changed; audit Map10 exit before use')
            db.execute("INSERT INTO modern_player_state(player_id,money,updated_at) VALUES(111,4242,CURRENT_TIMESTAMP)")
            db.execute("INSERT INTO modern_player_position VALUES(111,10,46973,4198,CURRENT_TIMESTAMP)")
        if args.movement:
            observer_identity = db.execute('SELECT user_idx FROM modern_account_identity WHERE account_id=?', (observer_account,)).fetchone()
            if observer_identity is None:
                observer_index = db.execute('SELECT COALESCE(MAX(user_idx),0)+1 FROM modern_account_identity').fetchone()[0]
                db.execute('INSERT INTO modern_account_identity(account_id,user_idx) VALUES(?,?)', (observer_account, observer_index))
            else:
                observer_index = observer_identity[0]
            db.execute('INSERT INTO character_info(charname,chrid,userid,map_num,start_area) VALUES(?,?,?,?,?)', (observer_name, 222, str(observer_index), 10, 10))
    else:
        identity_rows = dbtool_rows(dbtool, env, quiet,
            f"SELECT user_idx FROM modern_account_identity WHERE account_id={sql_text(account)}")
        if not identity_rows:
            dbtool_run(dbtool, env, quiet, 'exec',
                f"INSERT INTO modern_account_identity(account_id,user_idx) SELECT {sql_text(account)},COALESCE(MAX(user_idx),0)+1 FROM modern_account_identity")
            identity_rows = dbtool_rows(dbtool, env, quiet,
                f"SELECT user_idx FROM modern_account_identity WHERE account_id={sql_text(account)}")
        if len(identity_rows) != 1:
            raise RuntimeError('Registered MSSQL smoke account has no unique modern identity')
        identity = (int(identity_rows[0][0]),)
        occupied = dbtool_rows(dbtool, env, quiet,
            "SELECT chrid,charname FROM character_info WHERE chrid IN (111,222)")
        if occupied:
            raise RuntimeError('Dedicated mxh_test fixture IDs 111/222 are occupied; no rows were changed')
        statements = []
        if not args.create_character:
            statements.append(f"INSERT INTO character_info(charname,chrid,userid,map_num,start_area) VALUES({sql_text(fixture_name)},111,{sql_text(str(identity[0]))},{map_number},{map_number})")
        if args.trade:
            statements.append("INSERT INTO modern_player_state(player_id,money,updated_at) VALUES(111,100000000,CURRENT_TIMESTAMP)")
        if args.equipment:
            statements.append("INSERT INTO modern_player_item(player_id,container,slot,db_idx,item_idx,durability,rare_idx,quick_position,item_param) VALUES(111,1,1,9001,11000,100,0,65535,1)")
        if args.item_use:
            statements.append("INSERT INTO modern_player_item(player_id,container,slot,db_idx,item_idx,durability,rare_idx,quick_position,item_param) VALUES(111,0,0,9001,1,1,0,65535,1)")
        if args.sell:
            statements.append("INSERT INTO modern_player_state(player_id,money,updated_at) VALUES(111,1000,CURRENT_TIMESTAMP)")
            statements.append("INSERT INTO modern_player_item(player_id,container,slot,db_idx,item_idx,durability,rare_idx,quick_position,item_param) VALUES(111,0,0,9001,53343,1,0,65535,1)")
        if args.discard:
            statements.append("INSERT INTO modern_player_item(player_id,container,slot,db_idx,item_idx,durability,rare_idx,quick_position,item_param) VALUES(111,0,0,9001,53343,1,0,65535,1)")
        if args.quest_reward:
            statements.extend((
                "INSERT INTO modern_player_state(player_id,money,level,exp,updated_at) VALUES(111,0,48,0,CURRENT_TIMESTAMP)",
                "INSERT INTO modern_player_position(player_id,map_num,pos_x,pos_z,updated_at) VALUES(111,10,44178,13253,CURRENT_TIMESTAMP)",
                "INSERT INTO modern_player_quest_log(player_id,quest_id,state,accepted_time_ms,updated_at) VALUES(111,173,1,123456,CURRENT_TIMESTAMP)",
                "INSERT INTO modern_player_quest_sub(player_id,quest_id,sub_index,kind,target_id,count,target_count) VALUES(111,173,0,4,38,1,1)",
                "INSERT INTO modern_player_quest_sub(player_id,quest_id,sub_index,kind,target_id,count,target_count) VALUES(111,173,1,1,73,29,30)",
                "INSERT INTO modern_player_quest_sub(player_id,quest_id,sub_index,kind,target_id,count,target_count) VALUES(111,173,2,4,38,1,1)"))
        if args.quest_npc:
            statements.extend((
                "INSERT INTO modern_player_state(player_id,money,level,exp,updated_at) VALUES(111,0,50,0,CURRENT_TIMESTAMP)",
                "INSERT INTO modern_player_position(player_id,map_num,pos_x,pos_z,updated_at) VALUES(111,10,3500,46900,CURRENT_TIMESTAMP)",
                "INSERT INTO modern_player_quest_log(player_id,quest_id,state,accepted_time_ms,updated_at) VALUES(111,180,1,123456,CURRENT_TIMESTAMP)",
                "INSERT INTO modern_player_quest_sub(player_id,quest_id,sub_index,kind,target_id,count,target_count) VALUES(111,180,0,4,41,1,1)"))
        if args.combat_timeline:
            statements.extend((
                "INSERT INTO modern_player_state(player_id,money,level,exp,updated_at) VALUES(111,0,48,0,CURRENT_TIMESTAMP)",
                "INSERT INTO modern_player_position(player_id,map_num,pos_x,pos_z,updated_at) VALUES(111,10,44178,13253,CURRENT_TIMESTAMP)"))
        if args.transfer:
            route_path = repo / 'modern/data/PlayDH/Resource/MapChange.bin'
            if hashlib.sha256(route_path.read_bytes()).hexdigest() != '66aca3ccca86469e4f8ec1f0dada451fd2422d5c4b0c2698408e56c0101b7048':
                raise RuntimeError('Canonical route fixture changed; audit Map10 exit before use')
            statements.extend(("INSERT INTO modern_player_state(player_id,money,updated_at) VALUES(111,4242,CURRENT_TIMESTAMP)",
                               "INSERT INTO modern_player_position(player_id,map_num,pos_x,pos_z,updated_at) VALUES(111,10,46973,4198,CURRENT_TIMESTAMP)"))
        if args.movement:
            observer_rows = dbtool_rows(dbtool, env, quiet,
                f"SELECT user_idx FROM modern_account_identity WHERE account_id={sql_text(observer_account)}")
            if not observer_rows:
                dbtool_run(dbtool, env, quiet, 'exec',
                    f"INSERT INTO modern_account_identity(account_id,user_idx) SELECT {sql_text(observer_account)},COALESCE(MAX(user_idx),0)+1 FROM modern_account_identity")
                observer_rows = dbtool_rows(dbtool, env, quiet,
                    f"SELECT user_idx FROM modern_account_identity WHERE account_id={sql_text(observer_account)}")
            if len(observer_rows) != 1:
                raise RuntimeError('Registered MSSQL observer account has no unique modern identity')
            statements.append(f"INSERT INTO character_info(charname,chrid,userid,map_num,start_area) VALUES({sql_text(observer_name)},222,{sql_text(observer_rows[0][0])},10,10)")
        if statements:
            dbtool_run(dbtool, env, quiet, 'exec', ';'.join(statements))
    sockets = [socket.socket() for _ in range(4 if args.transfer else 3)]
    try:
        for item in sockets: item.bind(('127.0.0.1', 0))
        reserved_ports = [item.getsockname()[1] for item in sockets]
        login_port, agent_port, map_port = reserved_ports[:3]
        target_port = reserved_ports[3] if args.transfer else 0
    finally:
        for item in sockets: item.close()
    resources = repo / 'modern/data/PlayDH'
    collision = collision_fixture(resources) if args.movement else None
    if collision:
        env['MXH_SMOKE_BLOCKED_X'] = str(collision['x'])
        env['MXH_SMOKE_BLOCKED_Z'] = str(collision['z'])
    tools = repo / 'modern/build/tools'
    specs = [
        ('map', tools / 'MoxianMapServer/mxh_map_server_CHINA.exe', map_port,
         ['--map', str(map_number), '--resource-root', str(resources), '--server-resource-root', str(resources / 'Resource/Server'), '--resource-profile', 'playdh-current']),
        ('agent', tools / 'MoxianAgentServer/mxh_agent_server_CHINA.exe', agent_port,
         ['--legacy', '--map-server', f'127.0.0.1:{map_port}', '--default-map', str(map_number)]),
        ('login', tools / 'MoxianLoginServer/mxh_login_server.exe', login_port,
         ['--legacy', '--agent-addr', '127.0.0.1', '--agent-port', str(agent_port)]),
    ]
    if args.transfer:
        specs[1][3].extend(['--map-server-map', f'2=127.0.0.1:{target_port}'])
        specs.insert(1, ('map-target', tools / 'MoxianMapServer/mxh_map_server_CHINA.exe', target_port,
            ['--map', '2', '--resource-root', str(resources), '--server-resource-root', str(resources / 'Resource/Server'), '--resource-profile', 'playdh-current']))
    processes, logs = [], []
    restart_count = 0
    def start_servers(suffix=''):
        for name, executable, port, extra in specs:
            progress['server'] = name
            log = (output / f'{name}{suffix}.log').open('w', encoding='utf-8')
            logs.append(log)
            # Current Agent MapClientHandler speaks legacy framing without HSEL.
            # Keep that existing internal link strictly on loopback. Client-facing
            # Login/Agent links use HSEL; this is not production transport acceptance.
            hsel = [] if name.startswith('map') else ['--use-hsel']
            process = subprocess.Popen([str(executable), '--port', str(port), '--bind-address', '127.0.0.1', *hsel, *common, *extra], cwd=output, env=env, stdout=log, stderr=subprocess.STDOUT, **quiet)
            processes.append(process)
            deadline = time.monotonic() + 20
            while True:
                if process.poll() is not None: raise RuntimeError(f'{name} exited before listening; inspect {name}.log')
                try:
                    with socket.create_connection(('127.0.0.1', port), timeout=0.2): break
                except OSError:
                    if time.monotonic() >= deadline: raise TimeoutError(f'{name} listen timeout')
                    time.sleep(0.1)
    def restart_servers():
        nonlocal restart_count
        if any(process.poll() is not None for process in processes):
            raise RuntimeError('Server exited before controlled pickup-loop restart')
        for process in reversed(processes):
            if process.poll() is None:
                process.terminate()
                try: process.wait(timeout=10)
                except subprocess.TimeoutExpired: process.kill(); process.wait(timeout=5)
        processes.clear()
        restart_count += 1
        start_servers(f'-restart-{restart_count}')
    try:
        progress['stage'] = 'initial-server-startup'
        start_servers()
        progress['stage'] = 'client-and-evidence'
        progress.pop('server', None)
        env['MXH_SMOKE_LOGIN_PORT'] = str(login_port)
        env['MXH_SMOKE_USER'], env['MXH_SMOKE_PASSWORD'] = account, password
        if args.create_character: env['MXH_SMOKE_CREATE_NAME'] = create_name
        if args.movement: env['MXH_SMOKE_OBSERVER_USER'] = observer_account
        if args.trade: env['MXH_SMOKE_TRADE'] = '1'
        if args.equipment: env['MXH_SMOKE_EQUIPMENT'] = '1'
        if args.item_use: env['MXH_SMOKE_ITEM_USE'] = '1'
        if args.sell: env['MXH_SMOKE_SELL'] = '1'
        if args.discard: env['MXH_SMOKE_DISCARD'] = '1'
        if args.quest: env['MXH_SMOKE_QUEST'] = '1'
        if args.quest_reward: env['MXH_SMOKE_QUEST_REWARD'] = '1'
        if args.quest_npc: env['MXH_SMOKE_QUEST_NPC'] = '1'
        if args.combat_timeline: env['MXH_SMOKE_COMBAT_TIMELINE'] = '1'
        if args.player_death: env['MXH_SMOKE_PLAYER_DEATH'] = '1'
        if args.transfer: env['MXH_SMOKE_TRANSFER'] = '1'
        pickup_loop = None
        if args.editor_test:
            cli = Path(os.environ['LOCALAPPDATA']) / 'Unity/bin/unity.exe'
            result_path = output / 'editor-tests.xml'
            completed = subprocess.run([str(cli), 'test', str(repo / 'unity/MoxiangClient'), '--mode', 'EditMode', '--filter', 'Moxiang.Tests', '--output', str(result_path), '--timeout', '300', '--format', 'json', '--non-interactive'], env=env, capture_output=True, text=True, timeout=360, **quiet)
            (output / 'editor-test-command.json').write_text(completed.stdout, encoding='utf-8')
            (output / 'editor-test-stderr.log').write_text(completed.stderr, encoding='utf-8')
            results = ET.parse(result_path).getroot() if result_path.exists() else None
            test_name = 'RealMerchantPurchasePersistsAcrossReconnect' if args.trade else 'RealThreeServerGameInAndReconnect'
            if args.equipment: test_name = 'RealEquipmentMovePersistsAcrossReconnect'
            if args.item_use: test_name = 'RealConsumableUsePersistsAcrossReconnect'
            if args.sell: test_name = 'RealMerchantSalePersistsAcrossReconnect'
            if args.discard: test_name = 'RealItemDiscardPersistsAcrossReconnect'
            if args.quest: test_name = 'RealQuestAcceptancePersistsAcrossReconnect'
            if args.quest_reward: test_name = 'RealQuestCompletionRewardAndReconnect'
            if args.quest_npc: test_name = 'RealQuestNpcTalkAndReconnect'
            if args.transfer: test_name = 'RealMap10ToMap2TransferAndReconnect'
            real_test = None if results is None else results.find(f".//test-case[@name='{test_name}']")
            passed = completed.returncode == 0 and results is not None and results.get('failed') == '0' and real_test is not None and real_test.get('result') == 'Passed'
            if args.movement:
                move_test = None if results is None else results.find(".//test-case[@name='RealTwoClientsObserveMoveStopAndRejectedJump']")
                passed = passed and move_test is not None and move_test.get('result') == 'Passed'
        elif args.pickup_loop:
            from unity_pickup_loop import run_pickup_loop
            pickup_loop = run_pickup_loop(player, output, env, database, restart_servers)
            passed = pickup_loop['passed']
        else:
            completed = subprocess.run([str(player), '--mxh-smoke-output', str(output), '-logFile', str(output / 'player.log'), '-screen-width', '1280', '-screen-height', '720', '-screen-fullscreen', '0'], cwd=player.parent, env=env, timeout=60)
            report_path = output / 'report.json'
            report = json.loads(report_path.read_text()) if report_path.exists() else {}
            passed = completed.returncode == 0 and report.get('gameInReached') and report.get('mapNumber') == 10
            # This isolated fixture has no shop rows. Validate source defaults
            # after native transport and managed decoding, not just receipt.
            passed = passed and report.get('shopAppearanceReceived') is True
            passed = passed and report.get('shopAppearanceAvatar') == [0] * 12 + [1] * 11
            passed = passed and report.get('shopAppearanceSkin') == [0] * 5
            if not args.create_character: passed = passed and report.get('playerId') == 111
            if args.movement:
                passed = passed and report.get('movementRequested') is True and report.get('movementPassed') is True
                passed = passed and report.get('movementProbeVersion') == 2 and report.get('collisionPassed') is True
            if args.quest_npc:
                passed = passed and report.get('questNpcRequested') is True
                passed = passed and report.get('questDialogueOpened') is True and report.get('questNpcPassed') is True
                passed = passed and report.get('questHeading') == '不知名的屍體'
                passed = passed and report.get('questBody') == '血教的移動?\n面目全非，幾乎無法辨識誰是誰。'
                passed = passed and report.get('questAction') == '搜身找尋名牌'
                passed = passed and report.get('questFeedback') == '任务步骤已更新'
            if args.combat_timeline:
                passed = passed and report.get('combatTimelineRequested') is True
                passed = passed and report.get('skillSubmitted') is True
                passed = passed and report.get('skillReleaseObserved') is True
                passed = passed and report.get('skillHitObserved') is True
                passed = passed and report.get('skillReleaseBeforeHit') is True
                passed = passed and report.get('skillReleaseCaster') == 111
                passed = passed and report.get('skillReleaseId') == 1
                passed = passed and report.get('skillObjectId', 0) > 0
                passed = passed and report.get('skillHitTarget') == 50023
                passed = passed and report.get('skillHitDamage', 0) > 0
                passed = passed and report.get('hitAnimationObserved') is True
                passed = passed and report.get('playerLifeObserved') is True
                passed = passed and report.get('playerHitObserved') is True
                passed = passed and report.get('playerLifeDelta', 0) < 0
                passed = passed and report.get('playerLifeAfter', 0) < report.get('playerLifeBefore', 0)
                passed = passed and report.get('snapshotLifeAtCapture', 0) < report.get('playerLifeBefore', 0)
                passed = passed and report.get('hudLifeObserved') is True
                if args.player_death:
                    passed = passed and all(report.get(field) is True for field in
                        ('deathRequested', 'deathObserved', 'deadMoveRejected', 'deadSkillRejected'))
                    passed = passed and report.get('snapshotLifeAtCapture') == 0
        creation = None
        if args.create_character:
            if args.backend == 'sqlite':
                with sqlite3.connect(database) as db:
                    rows = db.execute('SELECT chrid,charname,start_area FROM character_info WHERE userid=?', (str(identity[0]),)).fetchall()
                    equipment = [] if len(rows) != 1 else db.execute('SELECT slot,item_idx FROM modern_character_equipment WHERE chrid=? ORDER BY slot', (rows[0][0],)).fetchall()
            else:
                rows = [(int(row[0]), row[1], int(row[2])) for row in dbtool_rows(dbtool, env, quiet,
                    f"SELECT chrid,charname,start_area FROM character_info WHERE userid={sql_text(str(identity[0]))}")]
                equipment = [] if len(rows) != 1 else [(int(row[0]), int(row[1])) for row in dbtool_rows(dbtool, env, quiet,
                    f"SELECT slot,item_idx FROM modern_character_equipment WHERE chrid={rows[0][0]} ORDER BY slot")]
            creation = {'characters': rows, 'equipment': equipment}
            passed = passed and len(rows) == 1 and rows[0][1:] == (create_name, 17) and equipment == [(1, 11000), (2, 23000), (3, 27000)]
            if not args.editor_test: passed = passed and report.get('playerId') == rows[0][0] and report.get('characterCreated') is True
        summary = {'runId': output.name, 'passed': bool(passed), 'surface': 'Editor' if args.editor_test else 'Player', 'backend': args.backend, 'serverType': 'real modern executables', 'clientTransport': 'HSEL', 'internalTransport': 'legacy plaintext loopback', 'fixtureCharacter': not args.create_character, 'creation': creation, 'humanAcceptance': False, 'output': str(output)}
        if args.pickup_loop: summary['pickupLoop'] = pickup_loop
        summary['serverExitCodesBeforeCleanup'] = {spec[0]: process.poll() for spec, process in zip(specs, processes)}
        if args.pickup_loop and any(code is not None for code in summary['serverExitCodesBeforeCleanup'].values()):
            passed = False
            summary['passed'] = False
        summary['twoNativeSessionMovement'] = args.movement
        summary['collisionFixture'] = collision
        if args.transfer:
            if args.backend == 'sqlite':
                with sqlite3.connect(database) as db:
                    position = db.execute('SELECT map_num,pos_x,pos_z FROM modern_player_position WHERE player_id=111').fetchone()
                    character_map = db.execute('SELECT map_num FROM character_info WHERE chrid=111').fetchone()
                    money = db.execute('SELECT money FROM modern_player_state WHERE player_id=111').fetchone()
            else:
                position_rows = dbtool_rows(dbtool, env, quiet, 'SELECT map_num,pos_x,pos_z FROM modern_player_position WHERE player_id=111')
                character_rows = dbtool_rows(dbtool, env, quiet, 'SELECT map_num FROM character_info WHERE chrid=111')
                money_rows = dbtool_rows(dbtool, env, quiet, 'SELECT money FROM modern_player_state WHERE player_id=111')
                position = None if not position_rows else tuple(map(int, position_rows[0]))
                character_map = None if not character_rows else (int(character_rows[0][0]),)
                money = None if not money_rows else (int(money_rows[0][0]),)
            persisted = position == (2, 7211, 43329) and character_map == (2,) and money == (4242,)
            summary['transfer'] = {'source': 10, 'target': 2, 'position': position, 'characterMap': character_map,
                                   'money': money, 'persistencePassed': persisted}
            passed = passed and persisted
            summary['passed'] = bool(passed)
        if args.equipment:
            if args.backend == 'sqlite':
                with sqlite3.connect(database) as db:
                    items = db.execute('SELECT container,slot,db_idx,item_idx FROM modern_player_item WHERE player_id=111 ORDER BY container,slot').fetchall()
            else:
                items = [tuple(map(int, row)) for row in dbtool_rows(dbtool, env, quiet,
                    'SELECT container,slot,db_idx,item_idx FROM modern_player_item WHERE player_id=111 ORDER BY container,slot')]
            persisted = items == [(1, 1, 9001, 11000)]
            summary['equipment'] = {'item': 11000, 'databaseId': 9001, 'finalContainer': 1,
                                    'finalSlot': 1, 'items': items, 'persistencePassed': persisted}
            passed = passed and persisted
            summary['passed'] = bool(passed)
        if args.item_use:
            if args.backend == 'sqlite':
                with sqlite3.connect(database) as db:
                    items = db.execute('SELECT container,slot,db_idx,item_idx FROM modern_player_item WHERE player_id=111 ORDER BY container,slot').fetchall()
            else:
                items = [tuple(map(int, row)) for row in dbtool_rows(dbtool, env, quiet,
                    'SELECT container,slot,db_idx,item_idx FROM modern_player_item WHERE player_id=111 ORDER BY container,slot')]
            persisted = items == []
            summary['itemUse'] = {'item': 1, 'databaseId': 9001, 'items': items,
                                  'persistencePassed': persisted}
            passed = passed and persisted
            summary['passed'] = bool(passed)
        if args.sell:
            if args.backend == 'sqlite':
                with sqlite3.connect(database) as db:
                    money = db.execute('SELECT money FROM modern_player_state WHERE player_id=111').fetchone()
                    items = db.execute('SELECT item_idx FROM modern_player_item WHERE player_id=111 ORDER BY slot').fetchall()
            else:
                money_rows = dbtool_rows(dbtool, env, quiet, 'SELECT money FROM modern_player_state WHERE player_id=111')
                money = None if not money_rows else (int(money_rows[0][0]),)
                items = [(int(row[0]),) for row in dbtool_rows(dbtool, env, quiet,
                    'SELECT item_idx FROM modern_player_item WHERE player_id=111 ORDER BY slot')]
            persisted = money is not None and money[0] > 1000 and items == []
            summary['sell'] = {'map': 12, 'npc': 92, 'item': 53343, 'money': money,
                               'items': items, 'persistencePassed': persisted}
            passed = passed and persisted
            summary['passed'] = bool(passed)
        if args.discard:
            if args.backend == 'sqlite':
                with sqlite3.connect(database) as db:
                    items = db.execute('SELECT item_idx FROM modern_player_item WHERE player_id=111 ORDER BY slot').fetchall()
            else:
                items = [(int(row[0]),) for row in dbtool_rows(dbtool, env, quiet,
                    'SELECT item_idx FROM modern_player_item WHERE player_id=111 ORDER BY slot')]
            persisted = items == []
            summary['discard'] = {'map': 10, 'item': 53343, 'items': items, 'persistencePassed': persisted}
            passed = passed and persisted
            summary['passed'] = bool(passed)
        if args.trade:
            if args.backend == 'sqlite':
                with sqlite3.connect(database) as db:
                    money = db.execute('SELECT money FROM modern_player_state WHERE player_id=111').fetchone()
                    items = db.execute('SELECT item_idx FROM modern_player_item WHERE player_id=111 ORDER BY slot').fetchall()
            else:
                money_rows = dbtool_rows(dbtool, env, quiet, 'SELECT money FROM modern_player_state WHERE player_id=111')
                money = None if not money_rows else (int(money_rows[0][0]),)
                items = [(int(row[0]),) for row in dbtool_rows(dbtool, env, quiet,
                    'SELECT item_idx FROM modern_player_item WHERE player_id=111 ORDER BY slot')]
            persisted = money is not None and 0 <= money[0] < 100000000 and items == [(53343,)]
            summary['trade'] = {'map': 12, 'npc': 92, 'item': 53343, 'money': money, 'items': items,
                                'dealitemSha256': hashlib.sha256((resources / 'Resource/Dealitem.bin').read_bytes()).hexdigest(),
                                'persistencePassed': persisted}
            passed = passed and persisted
            summary['passed'] = bool(passed)
        if args.quest:
            if args.backend == 'sqlite':
                with sqlite3.connect(database) as db:
                    quests = db.execute('SELECT quest_id,state FROM modern_player_quest_log WHERE player_id=111 ORDER BY quest_id').fetchall()
            else:
                quests = [tuple(map(int, row)) for row in dbtool_rows(dbtool, env, quiet,
                    'SELECT quest_id,state FROM modern_player_quest_log WHERE player_id=111 ORDER BY quest_id')]
            persisted = quests == [(1, 1)]
            summary['quest'] = {'map': 10, 'questId': 1, 'rows': quests, 'persistencePassed': persisted}
            passed = passed and persisted
            summary['passed'] = bool(passed)
        if args.quest_reward:
            if args.backend == 'sqlite':
                with sqlite3.connect(database) as db:
                    quests = db.execute('SELECT quest_id,state FROM modern_player_quest_log WHERE player_id=111 ORDER BY quest_id').fetchall()
                    subs = db.execute('SELECT sub_index,kind,target_id,count,target_count FROM modern_player_quest_sub WHERE player_id=111 AND quest_id=173 ORDER BY sub_index').fetchall()
                    items = db.execute('SELECT item_idx,item_param FROM modern_player_item WHERE player_id=111 ORDER BY slot').fetchall()
                    progress = db.execute('SELECT level,exp,money FROM modern_player_state WHERE player_id=111').fetchone()
            else:
                quests = [tuple(map(int, row)) for row in dbtool_rows(dbtool, env, quiet,
                    'SELECT quest_id,state FROM modern_player_quest_log WHERE player_id=111 ORDER BY quest_id')]
                subs = [tuple(map(int, row)) for row in dbtool_rows(dbtool, env, quiet,
                    'SELECT sub_index,kind,target_id,count,target_count FROM modern_player_quest_sub WHERE player_id=111 AND quest_id=173 ORDER BY sub_index')]
                items = [tuple(map(int, row)) for row in dbtool_rows(dbtool, env, quiet,
                    'SELECT item_idx,item_param FROM modern_player_item WHERE player_id=111 ORDER BY slot')]
                progress_rows = dbtool_rows(dbtool, env, quiet,
                    'SELECT level,exp,money FROM modern_player_state WHERE player_id=111')
                progress = None if not progress_rows else tuple(map(int, progress_rows[0]))
            persisted = (quests == [(173, 3)] and
                         subs == [(0,4,38,1,1),(1,1,73,30,30),(2,4,38,1,1)] and
                         items == [(414,30)] and progress is not None and progress[0] >= 48 and progress[2] == 0)
            summary['questReward'] = {'map': 10, 'questId': 173, 'rows': quests, 'subs': subs,
                                      'items': items, 'progress': progress, 'persistencePassed': persisted}
            passed = passed and persisted
            summary['passed'] = bool(passed)
        if args.quest_npc:
            if args.backend == 'sqlite':
                with sqlite3.connect(database) as db:
                    quests = db.execute('SELECT quest_id,state FROM modern_player_quest_log WHERE player_id=111 ORDER BY quest_id').fetchall()
                    subs = db.execute('SELECT sub_index,kind,target_id,count,target_count FROM modern_player_quest_sub WHERE player_id=111 AND quest_id=180 ORDER BY sub_index').fetchall()
            else:
                quests = [tuple(map(int, row)) for row in dbtool_rows(dbtool, env, quiet,
                    'SELECT quest_id,state FROM modern_player_quest_log WHERE player_id=111 ORDER BY quest_id')]
                subs = [tuple(map(int, row)) for row in dbtool_rows(dbtool, env, quiet,
                    'SELECT sub_index,kind,target_id,count,target_count FROM modern_player_quest_sub WHERE player_id=111 AND quest_id=180 ORDER BY sub_index')]
            persisted = (quests == [(180, 1)] and
                         subs == [(0,4,41,1,1),(1,4,572,1,1),(2,4,41,0,1),(3,4,12,0,1)])
            summary['questNpc'] = {'map': 10, 'npcIndex': 572, 'questId': 180,
                                   'rows': quests, 'subs': subs, 'persistencePassed': persisted}
            passed = passed and persisted
            summary['passed'] = bool(passed)
        (output / 'three-server-summary.json').write_text(json.dumps(summary, indent=2))
        print(json.dumps(summary))
        return 0 if passed else 1
    finally:
        env.pop('MXH_SMOKE_PASSWORD', None)
        # Own Popen objects only: never stop by process name or shared PID manifests.
        for process in reversed(processes):
            if process.poll() is None:
                process.terminate()
                try: process.wait(timeout=10)
                except subprocess.TimeoutExpired: process.kill(); process.wait(timeout=5)
        for log in logs: log.close()
        if args.backend == 'mssql_odbc':
            cleanup_mssql_fixture(dbtool, env, quiet, account, observer_account, fixture_name, observer_name)
            if cleanup_registered:
                atexit.unregister(cleanup_mssql_fixture)


if __name__ == '__main__':
    raise SystemExit(main())
