import importlib.util
from pathlib import Path
import unittest


MODULE_PATH = Path(__file__).parents[3] / "tools" / "export_unity_quest_interactions.py"
SPEC = importlib.util.spec_from_file_location("export_unity_quest_interactions", MODULE_PATH)
EXPORTER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(EXPORTER)


class VerifiedPageRecoveryTests(unittest.TestCase):
    EXPECTED_HASHES = (
        "93d964a93be393a8a83fb9a84d80883b27821a593d180a30b365516df1702a52",
        "1c443241334c73f3b028f979e7fb09ae8b48c7021ec0243437fe4e2405eb2d45",
        "29d8540ed9a90d2a6061807580a90c195faa27e93031bd8560740359ee37e3c5",
    )

    def setUp(self):
        self.messages = {8363: "first", 8366: "second", 8370: "third"}
        self.hypertext = {480: "take", 481: "flower", 482: "water"}

    def test_exact_source_hashes_recover_three_quest_pages(self):
        pages = {}

        EXPORTER.apply_verified_page_recoveries(
            pages, self.messages, self.hypertext, self.EXPECTED_HASHES
        )

        self.assertEqual({(264, 16), (264, 19), (264, 23)}, set(pages))
        self.assertTrue(all(page["pageSource"] == "RecoveredContiguousNpcMessageSequence"
                            for page in pages.values()))
        self.assertEqual([480], [option["textId"] for option in pages[(264, 16)]["options"]])

    def test_any_source_hash_change_disables_recovery(self):
        for index in range(3):
            with self.subTest(index=index):
                hashes = list(self.EXPECTED_HASHES)
                hashes[index] = "0" * 64
                pages = {}

                EXPORTER.apply_verified_page_recoveries(
                    pages, self.messages, self.hypertext, tuple(hashes)
                )

                self.assertEqual({}, pages)

    def test_missing_paired_payload_does_not_create_partial_page(self):
        messages = dict(self.messages)
        messages.pop(8366)
        pages = {}

        EXPORTER.apply_verified_page_recoveries(
            pages, messages, self.hypertext, self.EXPECTED_HASHES
        )

        self.assertEqual({(264, 16), (264, 23)}, set(pages))
        self.assertNotIn((264, 19), pages)


if __name__ == "__main__":
    unittest.main(verbosity=2)
