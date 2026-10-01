import argparse
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import sqlite3
import subprocess
import tempfile
import unittest
from unittest.mock import Mock, patch

SPEC = importlib.util.spec_from_file_location('smoke', Path(__file__).resolve().parents[3] / 'tools/unity_three_server_smoke.py')
smoke = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(smoke)


class SmokeFailureReportTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.output = self.root / 'isolated-run'
        self.output.mkdir()
        player = self.root / 'Player.exe'
        player.touch()
        self.args = argparse.Namespace(player=player, backend='sqlite', pickup_loop=True,
            **{key: False for key in ('editor_test', 'create_character', 'movement', 'trade', 'equipment',
                'item_use', 'sell', 'discard', 'quest', 'quest_reward', 'quest_npc', 'combat_timeline', 'player_death', 'transfer')})
        # These tests isolate later setup failures, after a reviewed fixture gate.
        gate = patch.object(smoke, 'require_viable_pickup_fixture')
        gate.start()
        self.addCleanup(gate.stop)

    def run_reported(self):
        stdout = io.StringIO()
        with contextlib.redirect_stdout(stdout):
            code = smoke.execute_reported(self.args, argparse.ArgumentParser(), self.root, self.output, 10)
        self.assertEqual(code, 1)
        report = json.loads((self.output/'three-server-summary.json').read_text())
        self.assertFalse(report['passed'])
        self.assertEqual(report, json.loads((self.output/'failure-summary.json').read_text()))
        self.assertEqual(report, json.loads(stdout.getvalue()))
        pickup = json.loads((self.output/'pickup-loop-summary.json').read_text())
        self.assertFalse(pickup['passed'])
        self.assertEqual(pickup['failure'], report)
        self.assertEqual(pickup['phases'], [])
        return report

    def test_migration_timeout_writes_failure_before_any_server_start(self):
        error = subprocess.TimeoutExpired(['private-command'], 30, output='secret-output', stderr='secret-stderr')
        with patch.object(smoke.subprocess, 'run', side_effect=error) as run, \
             patch.object(smoke.subprocess, 'Popen') as popen:
            report = self.run_reported()
        self.assertEqual(report['failureStage'], 'database-migration')
        self.assertEqual(report['errorType'], 'TimeoutExpired')
        self.assertEqual(report['timeoutSeconds'], 30)
        self.assertEqual(run.call_args.kwargs['timeout'], 30)
        popen.assert_not_called()
        self.assertNotIn('secret', json.dumps(report))
        self.assertNotIn('private-command', json.dumps(report))

    def test_registration_failure_writes_machine_readable_return_code(self):
        error = subprocess.CalledProcessError(7, ['private-command'], stderr='secret-stderr')
        with patch.object(smoke.subprocess, 'run', side_effect=[subprocess.CompletedProcess([], 0), error]), \
             patch.object(smoke.subprocess, 'Popen') as popen:
            report = self.run_reported()
        self.assertEqual(report['failureStage'], 'account-registration')
        self.assertEqual(report['returnCode'], 7)
        popen.assert_not_called()

    def test_initial_listen_timeout_reports_failure_and_cleans_only_owned_process(self):
        def db_setup(command, **kwargs):
            if command[1] == 'migrate':
                with sqlite3.connect(self.output/'fixture.db') as db:
                    db.executescript('''
                        CREATE TABLE modern_account_identity(account_id,user_idx);
                        CREATE TABLE character_info(charname,chrid,userid,map_num,start_area);
                        CREATE TABLE modern_player_state(player_id,money,level,exp,updated_at);
                        CREATE TABLE modern_player_position(player_id,map_num,pos_x,pos_z,updated_at);
                    ''')
            return subprocess.CompletedProcess(command, 0)
        owned = Mock()
        owned.poll.return_value = None
        owned.wait.return_value = 0
        port_socket = Mock()
        port_socket.getsockname.return_value = ('127.0.0.1', 12000)
        with patch.object(smoke.subprocess, 'run', side_effect=db_setup), \
             patch.object(smoke.subprocess, 'Popen', return_value=owned) as popen, \
             patch.object(smoke.socket, 'socket', return_value=port_socket), \
             patch.object(smoke.socket, 'create_connection', side_effect=OSError('not listening')), \
             patch.object(smoke.time, 'monotonic', side_effect=[0, 21]):
            report = self.run_reported()
        self.assertEqual(report['failureStage'], 'initial-server-startup')
        self.assertEqual(report['server'], 'map')
        self.assertEqual(report['errorType'], 'TimeoutError')
        self.assertEqual(report['timeoutSeconds'], 20)
        popen.assert_called_once()
        owned.terminate.assert_called_once_with()
        owned.wait.assert_called_once_with(timeout=10)
        owned.kill.assert_not_called()


if __name__ == '__main__':
    unittest.main()
