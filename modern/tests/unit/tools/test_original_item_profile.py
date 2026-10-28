"""The separate original Item profile must be derived from the pinned source."""

from __future__ import annotations

import hashlib
import importlib.util
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[4]
SCRIPT = ROOT / "modern/tools/generate_original_item_profile.py"
spec = importlib.util.spec_from_file_location("original_item_profile_generator", SCRIPT)
assert spec is not None and spec.loader is not None
generator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generator)


class OriginalItemProfileTests(unittest.TestCase):
    def test_fixed_source_generates_complete_committed_profile(self) -> None:
        source = generator.DEFAULT_SOURCE.read_bytes()
        self.assertEqual(hashlib.sha256(source).hexdigest(), generator.SOURCE_SHA256)
        entries = generator.extract_item_enum(source)
        self.assertEqual(len(entries), 252)
        values = dict(entries)
        self.assertEqual(values["MP_ITEM_BUY_SYN"], 19)
        self.assertEqual(values["MP_ITEM_MONSTER_OBTAIN_NOTIFY"], 32)
        self.assertEqual(values["MP_ITEM_PROTOCOL_MAX"], 256)
        self.assertNotIn("MP_ITEM_YY_MOVE_SYN", values)
        self.assertNotIn("MP_ITEM_DROP", values)
        self.assertEqual(generator.render_header(entries), generator.DEFAULT_OUTPUT.read_bytes())

    def test_comment_stripping_happens_before_enum_counting(self) -> None:
        source = b"""enum MP_PROTOCOL_ITEM {
            MP_ITEM_TOTALINFO_LOCAL,
            /* MP_ITEM_UNUSED_BLOCK, */
            // MP_ITEM_UNUSED_LINE,
            MP_ITEM_BUY_SYN = 19,
            MP_ITEM_MONSTER_OBTAIN_NOTIFY = 32,
            MP_ITEM_PROTOCOL_MAX = 256,
        };"""
        values = dict(generator.extract_item_enum(source))
        self.assertEqual(values["MP_ITEM_BUY_SYN"], 19)
        self.assertNotIn("MP_ITEM_UNUSED_BLOCK", values)
        self.assertNotIn("MP_ITEM_UNUSED_LINE", values)


if __name__ == "__main__":
    unittest.main()
