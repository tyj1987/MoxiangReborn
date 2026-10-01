import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

REPO = Path(__file__).resolve().parents[4]
SPEC = importlib.util.spec_from_file_location('fixture_audit', REPO/'modern/tools/unity_pickup_fixture.py')
audit = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(audit)


class PickupFixtureTests(unittest.TestCase):
    def test_even_all_critical_no_miss_budget_cannot_kill_current_target(self):
        budget = audit.optimistic_damage_budget(10, 0, 480, 5436)
        self.assertEqual(budget['maximumCriticalDamage'], 1)
        self.assertEqual(budget['maximumAttempts'], 151)
        self.assertFalse(budget['canPossiblyKillWithinDeadline'])

    def test_real_sources_match_audit_and_emit_specific_blocker(self):
        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder)
            with self.assertRaisesRegex(RuntimeError, 'attribute-import-required'):
                audit.require_viable_pickup_fixture(REPO, output)
            report = json.loads((output/'pickup-fixture-audit.json').read_text())
            self.assertTrue(report['resourceHashesMatchAudit'])
            self.assertFalse(report['readyForPlayerRun'])
            self.assertIn('unknown', report['actor']['attributes'])
            self.assertNotIn('physicalAttack', report['actor'])

    def test_missing_or_changed_sources_require_reaudit(self):
        with tempfile.TemporaryDirectory() as folder:
            with self.assertRaisesRegex(RuntimeError, 'fixture-audit-source-mismatch'):
                audit.require_viable_pickup_fixture(Path(folder), Path(folder))
            report = json.loads((Path(folder)/'pickup-fixture-audit.json').read_text())
            self.assertFalse(report['resourceHashesMatchAudit'])


if __name__ == '__main__':
    unittest.main()
