import subprocess
import tempfile
import unittest
from pathlib import Path


CONTRACT = 'mxh-map-writer-v2:item-containers-0-5,shop-skin-5,used-items,starter-equipment-atomic'


class WriterContractTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        modern = Path(__file__).resolve().parents[3]
        cls.dbtool = modern / 'build/tools/MoxianDbTool/mxh_db_tool.exe'
        cls.mapserver = modern / 'build/tools/MoxianMapServer/mxh_map_server_CHINA.exe'

    def db(self, command, config, *extra):
        return subprocess.run([self.dbtool, command, '--db', config, *extra],
                              text=True, capture_output=True)

    def test_mapserver_and_migrated_database_report_exact_same_contract(self):
        reported = subprocess.run([self.mapserver, '--print-writer-contract'],
                                  text=True, capture_output=True, check=True)
        self.assertEqual(reported.stdout.strip(), CONTRACT)
        with tempfile.TemporaryDirectory() as directory:
            config = f'backend=sqlite;path={Path(directory) / "contract.db"}'
            self.assertNotEqual(self.db('verify-writer-contract', config).returncode, 0)
            self.assertEqual(self.db('migrate', config).returncode, 0)
            verified = self.db('verify-writer-contract', config)
            self.assertEqual(verified.returncode, 0, verified.stderr)
            self.assertEqual(verified.stdout.strip(), CONTRACT)

    def test_missing_required_writer_table_fails_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            config = f'backend=sqlite;path={Path(directory) / "contract.db"}'
            self.assertEqual(self.db('migrate', config).returncode, 0)
            self.assertEqual(self.db('exec', config, 'DROP TABLE modern_character_equipment').returncode, 0)
            rejected = self.db('verify-writer-contract', config)
            self.assertNotEqual(rejected.returncode, 0)
            self.assertIn('writer contract probe failed', rejected.stderr)


if __name__ == '__main__':
    unittest.main()
