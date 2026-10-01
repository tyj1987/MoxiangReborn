import unittest
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[3] / 'tools'))
from select_original_pak_hfl import extend


class SelectOriginalPakHflTest(unittest.TestCase):
    def source(self, source_id, source_type, kind='unknown'):
        return {'sourceId': source_id, 'sourceType': source_type, 'sha256': source_id * 64,
                'container': 'Map.pak', 'entryIndex': 7, 'provenance': {'kind': kind}}

    def test_selects_unique_pak_when_loose_candidates_are_only_placeholders(self):
        manifest = {'assets': [{'stableId': 'asset', 'logicalPath': 'Map/7.hfl',
            'sources': [self.source('p', 'pak'), self.source('l', 'loose', 'generated-placeholder')]}]}
        result, added = extend(manifest, {'selections': {}, 'attestations': {}})
        self.assertEqual(added, 1)
        self.assertEqual(result['selections']['asset'], 'p')
        self.assertEqual(result['attestations']['p']['classification'], 'verified-original')

    def test_retains_ambiguous_real_loose_or_multiple_pak_candidates(self):
        assets = [
            {'stableId': 'real-loose', 'logicalPath': 'Map/1.hfl', 'sources': [self.source('p', 'pak'), self.source('l', 'loose')]},
            {'stableId': 'two-pak', 'logicalPath': 'Map/2.hfl', 'sources': [self.source('a', 'pak'), self.source('b', 'pak')]},
        ]
        result, added = extend({'assets': assets}, {'selections': {}, 'attestations': {}})
        self.assertEqual(added, 0)
        self.assertEqual(result['selections'], {})

    def test_selects_pak_for_byte_identical_non_hfl_duplicate_only(self):
        pak = self.source('p', 'pak')
        loose = self.source('l', 'loose')
        loose['sha256'] = pak['sha256']
        duplicate = {'stableId': 'ttb', 'logicalPath': 'Map/12.ttb',
                     'requiresSelection': True, 'relation': 'duplicate', 'sources': [pak, loose]}
        conflict = {'stableId': 'bad', 'logicalPath': 'Map/13.ttb',
                    'requiresSelection': True, 'relation': 'conflict',
                    'sources': [self.source('a', 'pak'), self.source('b', 'loose')]}
        result, added = extend({'assets': [duplicate, conflict]}, {'selections': {}, 'attestations': {}})
        self.assertEqual(added, 1)
        self.assertEqual(result['selections'], {'ttb': 'p'})
        self.assertEqual(result['attestations'], {})


if __name__ == '__main__':
    unittest.main()
