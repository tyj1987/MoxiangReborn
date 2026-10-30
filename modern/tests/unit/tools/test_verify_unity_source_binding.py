import json
import tempfile
import unittest
from pathlib import Path

import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / 'tools'))
from verify_unity_source_binding import verify


class VerifyUnitySourceBindingTest(unittest.TestCase):
    def fixtures(self, root: Path):
        source = {'sourceId': 'mxh:source', 'sha256': 'a' * 64, 'sourceType': 'pak',
                  'provenance': {'kind': 'verified-original'}}
        manifest = {'profileId': 'fixture', 'assets': [{
            'stableId': 'mxh:asset', 'logicalPath': 'Map/10.hfl',
            'selectedSourceId': 'mxh:source', 'sources': [source]}]}
        descriptor = {'sourceId': 'mxh:source', 'sourceSha256': 'a' * 64}
        manifest_path, descriptor_path = root / 'manifest.json', root / 'Map10.mxhasset'
        manifest_path.write_text(json.dumps(manifest), encoding='utf-8')
        descriptor_path.write_text(json.dumps(descriptor), encoding='utf-8')
        return manifest_path, descriptor_path

    def test_accepts_selected_verified_original_with_matching_hash(self):
        with tempfile.TemporaryDirectory() as directory:
            result = verify(*self.fixtures(Path(directory)))
            self.assertTrue(result['passed'])
            self.assertEqual(result['logicalPath'], 'Map/10.hfl')

    def test_rejects_unselected_hash_mismatch_and_unverified_hfl(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            manifest_path, descriptor_path = self.fixtures(root)
            manifest = json.loads(manifest_path.read_text())
            descriptor = json.loads(descriptor_path.read_text())
            manifest['assets'][0]['selectedSourceId'] = None
            manifest_path.write_text(json.dumps(manifest))
            with self.assertRaisesRegex(ValueError, 'not explicitly selected'):
                verify(manifest_path, descriptor_path)
            manifest['assets'][0]['selectedSourceId'] = 'mxh:source'
            descriptor['sourceSha256'] = 'b' * 64
            manifest_path.write_text(json.dumps(manifest))
            descriptor_path.write_text(json.dumps(descriptor))
            with self.assertRaisesRegex(ValueError, 'does not match'):
                verify(manifest_path, descriptor_path)
            descriptor['sourceSha256'] = 'a' * 64
            manifest['assets'][0]['sources'][0]['provenance']['kind'] = 'unknown'
            manifest_path.write_text(json.dumps(manifest))
            descriptor_path.write_text(json.dumps(descriptor))
            with self.assertRaisesRegex(ValueError, 'verified-original'):
                verify(manifest_path, descriptor_path)


if __name__ == '__main__':
    unittest.main()
