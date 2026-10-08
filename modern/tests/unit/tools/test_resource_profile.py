"""An exact resource baseline must reject extra files and linked input trees."""
import hashlib
import importlib.util
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[4] / 'scripts/verify-resource-profile.py'
spec = importlib.util.spec_from_file_location('resource_profile', SCRIPT)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class ExactResourceProfileTests(unittest.TestCase):
    def test_full_hash_alone_does_not_hide_extra_file_under_exact_mode(self):
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory)
            root = base / 'resources'
            root.mkdir()
            (root / 'canonical.bin').write_bytes(b'original')
            manifest = base / 'profile.json'
            manifest.write_text(json.dumps({'schemaVersion': 1, 'profileId': 'fixture', 'files': [
                {'path': 'canonical.bin', 'bytes': 8, 'sha256': hashlib.sha256(b'original').hexdigest()}
            ]}))
            command = [sys.executable, '-B', str(SCRIPT), str(manifest), '--root', str(root),
                       '--profile-id', 'fixture', '--full', '--exact']
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue(json.loads(result.stdout)['exactFileSet'])
            (root / 'generated-placeholder.bin').write_bytes(b'not canonical')
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(result.returncode, 1)
            self.assertIn('generated-placeholder.bin', result.stderr)

    def test_duplicate_manifest_entry_cannot_mask_missing_coverage(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'a.bin').write_bytes(b'a')
            with self.assertRaisesRegex(ValueError, 'duplicate'):
                module.verify_exact_file_set(root, [{'path': 'a.bin'}, {'path': 'a.bin'}])

    def test_linked_directory_is_rejected_without_following_it(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / 'resources'
            outside = Path(directory) / 'outside'
            root.mkdir()
            outside.mkdir()
            (outside / 'a.bin').write_bytes(b'a')
            try:
                (root / 'linked').symlink_to(outside, target_is_directory=True)
            except OSError as exc:
                self.skipTest('Host cannot create a test directory link: ' + str(exc))
            with self.assertRaisesRegex(ValueError, 'linked resource path'):
                module.verify_exact_file_set(root, [{'path': 'linked/a.bin'}])


if __name__ == '__main__':
    unittest.main()
