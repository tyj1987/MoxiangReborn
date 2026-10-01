import importlib.util
import json
from pathlib import Path
import sqlite3
import subprocess
import tempfile
import unittest
from unittest.mock import patch

TOOLS = Path(__file__).resolve().parents[3]/'tools'
SPEC = importlib.util.spec_from_file_location('prepare', TOOLS/'unity_pickup_prepare.py')
prepare = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(prepare)


class PreparationTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.output = self.root/'three-server'/('a'*32)
        self.output.mkdir(parents=True)
        self.database = self.output/'fixture.db'
        self.account = 'ux_aaaaaaaa'
        with sqlite3.connect(self.database) as db:
            db.executescript('''
                CREATE TABLE modern_account_identity(account_id,user_idx);
                INSERT INTO modern_account_identity VALUES('ux_aaaaaaaa',1);
                CREATE TABLE character_info(chrid,userid,level);
                CREATE TABLE modern_character_equipment(chrid,slot,item_idx);
                CREATE TABLE modern_player_item(player_id,container,slot,db_idx,item_idx,item_param);
                CREATE TABLE modern_item_grant(grant_id INTEGER PRIMARY KEY,idempotency_key UNIQUE,
                    character_id,item_id,item_count,status,created_by,reason);
            ''')

    def character(self):
        with sqlite3.connect(self.database) as db:
            db.execute("INSERT INTO character_info VALUES(100000,'1',1)")

    def test_grant_only_queues_request_without_inventory_or_stat_edits(self):
        self.character()
        grant = prepare.queue_starter_weapon(self.database, self.output, self.account, 100000)
        with sqlite3.connect(self.database) as db:
            self.assertEqual(db.execute('SELECT item_id,item_count,status FROM modern_item_grant WHERE grant_id=?', (grant,)).fetchone(), (11000,1,'pending'))
            self.assertEqual(db.execute('SELECT COUNT(*) FROM modern_player_item').fetchone()[0], 0)
            self.assertEqual(db.execute('SELECT level FROM character_info').fetchone()[0], 1)

    def test_refuses_arbitrary_database_path_or_foreign_account(self):
        for database, account in ((self.root/'production.db', self.account), (self.database, 'other')):
            with self.assertRaises(ValueError): prepare.verify_owned_fixture(database, self.output, account)
        self.assertFalse((self.root/'production.db').exists())

    def test_refuses_shared_accounts_and_preexisting_characters(self):
        self.character()
        with self.assertRaises(ValueError): prepare.verify_owned_fixture(self.database, self.output, self.account)
        with sqlite3.connect(self.database) as db: db.execute("INSERT INTO modern_account_identity VALUES('foreign',2)")
        with self.assertRaises(ValueError): prepare.queue_starter_weapon(self.database, self.output, self.account, 100000)

    def test_refuses_existing_inventory_and_repeated_grants(self):
        self.character()
        prepare.queue_starter_weapon(self.database, self.output, self.account, 100000)
        with self.assertRaises(ValueError): prepare.queue_starter_weapon(self.database, self.output, self.account, 100000)
        with sqlite3.connect(self.database) as db:
            db.execute('DELETE FROM modern_item_grant')
            db.execute('INSERT INTO modern_player_item VALUES(100000,0,0,1,77,1)')
        with self.assertRaises(ValueError): prepare.queue_starter_weapon(self.database, self.output, self.account, 100000)

    def test_player_timeout_produces_failed_preparation(self):
        with patch.object(prepare, 'verify_resource_inputs', return_value={}), \
             patch.object(prepare.subprocess, 'run', side_effect=subprocess.TimeoutExpired('player',75)):
            result = prepare.run_preparation(self.root/'Player.exe', self.output, {'MXH_RUN_ID': self.output.name}, self.database, self.account, self.root)
        self.assertFalse(result['passed'])
        self.assertFalse(result['preparationPassed'])
        self.assertIn('TimeoutExpired', result['error'])
        self.assertTrue((self.output/'pickup-preparation-summary.json').exists())

    def test_real_protocol_steps_are_required_but_do_not_imply_combat_success(self):
        def player(command, **kwargs):
            phase = kwargs['env']['MXH_SMOKE_PICKUP_PHASE']
            directory = Path(command[command.index('--mxh-smoke-output')+1])
            worn = []
            if phase == 'prepare-create':
                self.character()
                with sqlite3.connect(self.database) as db:
                    db.executemany('INSERT INTO modern_character_equipment VALUES(100000,?,?)', [(1,11000),(2,23000),(3,27000)])
            else:
                identity = int(kwargs['env']['MXH_SMOKE_PREPARE_ITEM'])
                if phase == 'prepare-equip':
                    # Only the fake server in this unit test performs claim/persistence.
                    with sqlite3.connect(self.database) as db:
                        db.execute("UPDATE modern_item_grant SET status='claimed'")
                        db.execute('INSERT INTO modern_player_item VALUES(100000,1,1,?,11000,1)', (identity,))
                worn = [dict(databaseId=identity, position=81, itemId=11000, itemParameter=1)]
            report = dict(phase=phase, runId=self.output.name, passed=True, gameIn=True, presentationReady=True,
                disconnected=True, playerId=100000, mapNumber=10, characterCreated=phase=='prepare-create',
                equipmentAck=phase=='prepare-equip', after=worn, visibleMonsterIds=[50000], visibleMonsterKinds=[105])
            (directory/'pickup-report.json').write_text(json.dumps(report))
            events = [dict(type=4, result='Ok'), dict(type=30, result='Ok')]
            (directory/'pickup-events.jsonl').write_text('\n'.join(json.dumps(e) for e in events))
            return subprocess.CompletedProcess(command,0)
        with patch.object(prepare, 'verify_resource_inputs', return_value={}), patch.object(prepare.subprocess, 'run', side_effect=player):
            result = prepare.run_preparation(self.root/'Player.exe', self.output, {'MXH_RUN_ID': self.output.name}, self.database, self.account, self.root)
        self.assertTrue(result['preparationPassed'])
        self.assertFalse(result['passed'])
        self.assertFalse(result['combatReady'])
        self.assertIsNone(result['selectedTarget'])
        self.assertEqual(len(result['phases']), 3)

    def test_cli_refuses_mssql_and_mixed_modes(self):
        import sys
        for args in (['--backend','mssql_odbc'], ['--pickup-loop'], ['--combat-timeline']):
            result = subprocess.run([sys.executable, str(TOOLS/'unity_three_server_smoke.py'), '--player','none.exe',
                '--prepare-pickup-input', *args], capture_output=True, text=True, timeout=5)
            self.assertEqual(result.returncode, 2)


if __name__ == '__main__': unittest.main()
