import hashlib
import tempfile
import unittest
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[3] / 'tools'))
from build_unity_overlay_manifest import build, stable_id


class BuildUnityOverlayManifestTest(unittest.TestCase):
    def test_stable_id_is_case_insensitive(self):
        self.assertEqual(stable_id('Resource/Map/10.HFL'), stable_id('resource/map/10.hfl'))

    def test_rejects_incomplete_overlay(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path = root / 'Resource/Map/1.hfl'
            path.parent.mkdir(parents=True)
            path.write_bytes(b'placeholder')
            report = {'extras': [{'path': 'Resource/Map/1.hfl', 'bytes': 11,
                                  'sha256': hashlib.sha256(b'placeholder').hexdigest()}]}
            with self.assertRaisesRegex(ValueError, 'expected 241'):
                build(report, root)


if __name__ == '__main__':
    unittest.main()
