"""Isolated loopback-only real server + Unity Player smoke; never production acceptance."""
from __future__ import annotations
import argparse
import json
import os
import secrets
import socket
import sqlite3
import subprocess
import time
import uuid
import xml.etree.ElementTree as ET
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--player', type=Path, required=True)
    parser.add_argument('--editor-test', action='store_true', help='Run Unity EditMode against the same real servers instead of the Player')
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[2]
    player = args.player.resolve(strict=True)
    output = repo / 'modern/out/unity-remaster/three-server' / uuid.uuid4().hex
    output.mkdir(parents=True)
    database = output / 'fixture.db'
    dbtool = repo / 'modern/build/tools/MoxianDbTool/mxh_db_tool.exe'
    env = os.environ.copy()
    env['MXH_UNITY_SMOKE_DATABASE'] = f'backend=sqlite;path={database}'
    env['MXH_RUN_ID'] = output.name
    common = ['--backend', 'sqlite', '--db-env', 'MXH_UNITY_SMOKE_DATABASE']
    quiet = {'creationflags': subprocess.CREATE_NO_WINDOW} if os.name == 'nt' else {}
    subprocess.run([str(dbtool), 'migrate', '--db-env', 'MXH_UNITY_SMOKE_DATABASE'], env=env, check=True, capture_output=True, **quiet)
    account, password = 'unity_smoke', 'Mx1' + secrets.token_hex(6)
    subprocess.run([str(dbtool), 'register', '--db-env', 'MXH_UNITY_SMOKE_DATABASE', account], input=password + '\n', text=True, env=env, check=True, capture_output=True, **quiet)
    with sqlite3.connect(database) as db:
        identity = db.execute('SELECT user_idx FROM modern_account_identity WHERE account_id=?', (account,)).fetchone()
        if identity is None:
            # Same persistent identity table consumed by LoginServer; isolated fixture only.
            db.execute('INSERT INTO modern_account_identity(account_id,user_idx) VALUES(?,?)', (account, 1))
            identity = (1,)
        db.execute('INSERT INTO character_info(charname,chrid,userid,map_num,start_area) VALUES(?,?,?,?,?)', ('UnitySmoke', 111, str(identity[0]), 10, 10))
    sockets = [socket.socket() for _ in range(3)]
    try:
        for item in sockets: item.bind(('127.0.0.1', 0))
        login_port, agent_port, map_port = [item.getsockname()[1] for item in sockets]
    finally:
        for item in sockets: item.close()
    resources = repo / 'modern/data/PlayDH'
    tools = repo / 'modern/build/tools'
    specs = [
        ('map', tools / 'MoxianMapServer/mxh_map_server_CHINA.exe', map_port,
         ['--map', '10', '--resource-root', str(resources), '--server-resource-root', str(resources / 'Resource/Server'), '--resource-profile', 'playdh-current']),
        ('agent', tools / 'MoxianAgentServer/mxh_agent_server_CHINA.exe', agent_port,
         ['--legacy', '--map-server', f'127.0.0.1:{map_port}', '--default-map', '10']),
        ('login', tools / 'MoxianLoginServer/mxh_login_server.exe', login_port,
         ['--legacy', '--agent-addr', '127.0.0.1', '--agent-port', str(agent_port)]),
    ]
    processes, logs = [], []
    try:
        for name, executable, port, extra in specs:
            log = (output / f'{name}.log').open('w', encoding='utf-8')
            logs.append(log)
            # Current Agent MapClientHandler speaks legacy framing without HSEL.
            # Keep that existing internal link strictly on loopback. Client-facing
            # Login/Agent links use HSEL; this is not production transport acceptance.
            hsel = [] if name == 'map' else ['--use-hsel']
            process = subprocess.Popen([str(executable), '--port', str(port), '--bind-address', '127.0.0.1', *hsel, *common, *extra], cwd=output, env=env, stdout=log, stderr=subprocess.STDOUT, **quiet)
            processes.append(process)
            deadline = time.monotonic() + 20
            while True:
                if process.poll() is not None: raise RuntimeError(f'{name} exited before listening; inspect {name}.log')
                try:
                    with socket.create_connection(('127.0.0.1', port), timeout=0.2): break
                except OSError:
                    if time.monotonic() >= deadline: raise RuntimeError(f'{name} listen timeout')
                    time.sleep(0.1)
        env['MXH_SMOKE_LOGIN_PORT'] = str(login_port)
        env['MXH_SMOKE_USER'], env['MXH_SMOKE_PASSWORD'] = account, password
        if args.editor_test:
            cli = Path(os.environ['LOCALAPPDATA']) / 'Unity/bin/unity.exe'
            result_path = output / 'editor-tests.xml'
            completed = subprocess.run([str(cli), 'test', str(repo / 'unity/MoxiangClient'), '--mode', 'EditMode', '--filter', 'Moxiang.Tests', '--output', str(result_path), '--timeout', '300', '--format', 'json', '--non-interactive'], env=env, capture_output=True, text=True, timeout=360, **quiet)
            (output / 'editor-test-command.json').write_text(completed.stdout, encoding='utf-8')
            results = ET.parse(result_path).getroot() if result_path.exists() else None
            real_test = None if results is None else results.find(".//test-case[@name='RealThreeServerGameInAndReconnect']")
            passed = completed.returncode == 0 and results is not None and results.get('failed') == '0' and real_test is not None and real_test.get('result') == 'Passed'
        else:
            completed = subprocess.run([str(player), '--mxh-smoke-output', str(output), '-logFile', str(output / 'player.log'), '-screen-width', '1280', '-screen-height', '720', '-screen-fullscreen', '0'], cwd=player.parent, env=env, timeout=60)
            report_path = output / 'report.json'
            report = json.loads(report_path.read_text()) if report_path.exists() else {}
            passed = completed.returncode == 0 and report.get('gameInReached') and report.get('playerId') == 111 and report.get('mapNumber') == 10
        summary = {'runId': output.name, 'passed': bool(passed), 'surface': 'Editor' if args.editor_test else 'Player', 'backend': 'sqlite', 'serverType': 'real modern executables', 'clientTransport': 'HSEL', 'internalTransport': 'legacy plaintext loopback', 'fixtureCharacter': True, 'humanAcceptance': False, 'output': str(output)}
        summary['serverExitCodesBeforeCleanup'] = {spec[0]: process.poll() for spec, process in zip(specs, processes)}
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


if __name__ == '__main__':
    raise SystemExit(main())
