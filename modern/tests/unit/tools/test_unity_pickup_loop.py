import importlib.util
import json
from pathlib import Path
import sqlite3
import subprocess
import tempfile
import unittest
from unittest.mock import patch

SPEC = importlib.util.spec_from_file_location('pickup_loop', Path(__file__).resolve().parents[3] / 'tools/unity_pickup_loop.py')
probe = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(probe)


def item(identity=1, position=0):
    return dict(zip(probe.FIELDS, (identity, 77, position, 100, 0, 65535, 1)))


def evidence():
    report = {'version': 1, 'phase': 'first', 'runId': 'test-run', 'passed': True,
              'gameIn': True, 'presentationReady': True, 'disconnected': True, 'error': '',
              'playerId': 111, 'mapNumber': 10, 'humanAcceptance': False, 'mouseInteraction': False,
              'before': [], 'after': [item()], 'dropId': 90000, 'itemId': 77, 'count': 1,
              'acquiredDatabaseId': 1}
    report.update({key: True for key in ('hit', 'zeroLife', 'dropObserved', 'pickupSubmitted', 'pickupAck', 'inventoryFresh')})
    events = []
    for kind, a0, a1, extra in ((34, 50023, 10, 1), (15, 50023, 0, 0), (17, 90000, 77, 0),
                                (18, 90000, 77, 0), (24, 111, 124, 121), (4, 0, 0, 0)):
        events.append(dict(type=kind, argument0=a0, argument1=a1, reserved0=extra,
                           sequence=len(events)+1, session=2, map=1, result='Ok'))
    return report, events


class PickupLoopEvidenceTests(unittest.TestCase):
    def test_valid_first_session_and_two_item_reconnect(self):
        report, events = evidence()
        probe.validate_phase(report, events, 'first', [], 'test-run')
        first = report['after']
        report.update(phase='second', before=first, after=first + [item(2, 1)], acquiredDatabaseId=2)
        probe.validate_phase(report, events, 'second', first, 'test-run')
        report.update(phase='verify', before=report['after'])
        probe.validate_phase(report, [events[-1]], 'verify', report['after'], 'test-run')

    def test_booleans_without_protocol_evidence_fail(self):
        report, _ = evidence()
        with self.assertRaises(ValueError): probe.validate_phase(report, [], 'first', [], 'test-run')

    def test_each_required_event_is_required(self):
        report, events = evidence()
        for index in range(len(events)):
            with self.subTest(index=index), self.assertRaises(ValueError):
                probe.validate_phase(report, events[:index]+events[index+1:], 'first', [], 'test-run')

    def test_stale_inventory_or_cross_generation_fails(self):
        for mutation in ('sequence', 'session', 'map'):
            report, events = evidence()
            events[-2][mutation] = 0
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                probe.validate_phase(report, events, 'first', [], 'test-run')

    def test_inventory_reuse_or_changed_prior_item_fails(self):
        report, events = evidence()
        old = item(1)
        report.update(phase='second', before=[old], after=[item(1), item(1, 1)])
        with self.assertRaises(ValueError): probe.validate_phase(report, events, 'second', [old], 'test-run')
        report['after'] = [item(2, 1)]
        with self.assertRaises(ValueError): probe.validate_phase(report, events, 'second', [old], 'test-run')

    def test_wrong_run_or_count_or_false_marker_fails(self):
        for key, value in (('runId', 'old-run'), ('count', 2), ('pickupAck', False), ('humanAcceptance', True)):
            report, events = evidence()
            report[key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                probe.validate_phase(report, events, 'first', [], 'test-run')

    def test_sqlite_read_only_snapshot_matches_all_item_fields(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'fixture.db'
            with sqlite3.connect(path) as db:
                db.execute('CREATE TABLE modern_player_item(player_id,db_idx,item_idx,slot,durability,rare_idx,quick_position,item_param,container)')
                db.execute('INSERT INTO modern_player_item VALUES(111,1,77,0,100,0,65535,1,0)')
            self.assertEqual(probe.read_persisted(path), probe.inventory([item()]))

    def test_player_timeout_writes_failure_summary_without_restart(self):
        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder)
            with patch.object(probe, 'read_persisted', return_value=[]), \
                 patch.object(probe.subprocess, 'run', side_effect=subprocess.TimeoutExpired('player', 240)):
                result = probe.run_pickup_loop(output/'player.exe', output, {'MXH_RUN_ID': 'test-run'}, output/'fixture.db',
                                               lambda: self.fail('must not restart after missing evidence'))
            self.assertFalse(result['passed'])
            self.assertIn('TimeoutExpired', result['error'])
            self.assertFalse(json.loads((output/'pickup-loop-summary.json').read_text())['passed'])

    def test_three_sessions_require_two_restarts_and_final_persisted_inventory(self):
        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder)
            persisted = []
            calls = []
            def launch(command, **kwargs):
                nonlocal persisted
                phase = kwargs['env']['MXH_SMOKE_PICKUP_PHASE']
                directory = Path(command[command.index('--mxh-smoke-output')+1])
                report, events = evidence()
                report.update(phase=phase, before=list(persisted))
                if phase != 'verify':
                    new = item(len(persisted)+1, len(persisted))
                    report.update(after=persisted+[new], acquiredDatabaseId=new['databaseId'])
                else: report['after'] = list(persisted)
                persisted = report['after']
                (directory/'pickup-report.json').write_text(json.dumps(report))
                (directory/'pickup-events.jsonl').write_text('\n'.join(json.dumps(e) for e in events))
                return subprocess.CompletedProcess(command, 0)
            with patch.object(probe.subprocess, 'run', side_effect=launch), \
                 patch.object(probe, 'read_persisted', side_effect=lambda _: probe.inventory(persisted)):
                result = probe.run_pickup_loop(output/'player.exe', output, {'MXH_RUN_ID': 'test-run'},
                                               output/'fixture.db', lambda: calls.append('restart'))
            self.assertTrue(result['passed'])
            self.assertEqual(calls, ['restart', 'restart'])
            self.assertEqual(len(result['phases']), 3)
            self.assertEqual(len(result['phases'][-1]['persisted']), 2)

    def test_cli_rejects_nonisolated_modes_before_launch(self):
        import sys
        script = Path(__file__).resolve().parents[3] / 'tools/unity_three_server_smoke.py'
        for other in (['--backend', 'mssql_odbc'], ['--editor-test'], ['--combat-timeline']):
            completed = subprocess.run([sys.executable, str(script), '--player', 'missing.exe', '--pickup-loop', *other],
                                       capture_output=True, text=True, timeout=5)
            self.assertEqual(completed.returncode, 2)
            self.assertIn('separate standalone SQLite fixture', completed.stderr)


if __name__ == '__main__':
    unittest.main()
