import json
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / 'tools'))
from unity_terrain_palette import build
from unity_resource_audit import AuditError
from test_unity_resource_audit import audit, create_profile, COMMIT


class PalettePlanTests(unittest.TestCase):
    def test_explicit_sources_produce_matching_receipt_and_refuse_overwrite(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp); root = base / 'source'; root.mkdir(); create_profile(root)
            document = audit.build_manifest(root, 'fixture', COMMIT)
            manifest = base / 'manifest.json'; audit._write_manifest(manifest, root, document)
            source = next(s for a in document['assets'] for s in a['sources'] if s['sourceType'] == 'pak')
            plan = base / 'plan.json'
            plan.write_text(json.dumps(dict(schemaVersion=1, releaseReady=False, entries=[dict(slot=0,
                file='tile.mxhdds', sourceId=source['sourceId'], sha256=source['sha256'])])))
            self.assertEqual(build(manifest, root, plan, base / 'output'), 1)
            self.assertEqual(audit._sha256_file(base / 'output/tile.mxhdds'), source['sha256'])
            receipt = json.loads((base / 'output/palette.json').read_text())
            self.assertFalse(receipt['releaseReady'])
            self.assertEqual(receipt['planSha256'], audit._sha256_file(plan))
            with self.assertRaises(AuditError): build(manifest, root, plan, base / 'output')

    def test_never_creates_output_inside_source(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            with self.assertRaises(AuditError):
                build(root / 'unused', root, root / 'unused-plan', root / 'must-not-exist')
            self.assertFalse((root / 'must-not-exist').exists())

    def test_invalid_plan_does_not_write_any_assets(self):
        with tempfile.TemporaryDirectory() as temp:
            base = Path(temp); root = base / 'source'; root.mkdir()
            for entries in ([{'slot': 0, 'file': '../escape.mxhdds'}],
                            [{'slot': 0, 'file': 'a.mxhdds'}, {'slot': 0, 'file': 'b.mxhdds'}],
                            [{'slot': -1, 'file': 'a.mxhdds'}]):
                plan = base / 'plan.json'
                plan.write_text(json.dumps(dict(schemaVersion=1, releaseReady=False, entries=entries)))
                with self.assertRaises(AuditError): build(base / 'unused', root, plan, base / 'output')
                self.assertFalse((base / 'output').exists())


if __name__ == '__main__': unittest.main()
