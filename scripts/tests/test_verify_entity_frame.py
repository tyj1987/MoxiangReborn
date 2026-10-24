"""Entity-frame gate: disabled rendering and a zero drawn count must fail."""

import importlib.util
import io
import json
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "scripts" / "verify-entity-frame.py"


def load_module():
    spec = importlib.util.spec_from_file_location("verify_entity_frame", SCRIPT)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


PASSING_PIXELS = {
    "player-upright-silhouette": True,
    "player-skin": True,
    "monster-dark-body": True,
    "monster-red-markings": True,
}


class DrawnCulledParseTests(unittest.TestCase):
    def setUp(self):
        self.mod = load_module()

    def test_absent_log_has_no_counts(self):
        self.assertIsNone(self.mod.parse_drawn_culled(""))
        self.assertIsNone(self.mod.parse_drawn_culled(
            "mxh_client: entity loaded=3 failed=0 placeholders=0 monsters=228 npcs=2 players=1\n"
        ))

    def test_split_lines_are_not_a_per_frame_record(self):
        text = "drawn=4\nculled=1\n"
        self.assertIsNone(self.mod.parse_drawn_culled(text))

    def test_last_same_line_pair_wins(self):
        text = "\n".join([
            "mxh_client: entity frame drawn=0 culled=10",
            "noise drawn=1 culled=1",
            "mxh_client: entity frame drawn=12 culled=3",
        ])
        self.assertEqual(self.mod.parse_drawn_culled(text), (12, 3))

    def test_slash_and_culled_first_forms(self):
        self.assertEqual(self.mod.parse_drawn_culled("drawn/culled=0/4"), (0, 4))
        self.assertEqual(self.mod.parse_drawn_culled("culled=8 drawn=2"), (2, 8))


class EntityGateTests(unittest.TestCase):
    def setUp(self):
        self.mod = load_module()

    def test_non_map_frame_is_not_applicable(self):
        status, _ = self.mod.evaluate_entity_gate("state-login.tga", None, "")
        self.assertEqual(status, "not-applicable")

    def test_disabled_capture_fails_even_if_pixels_would_pass(self):
        meta = {"entities_enabled": False, "map_number": 10}
        code, lines = self.mod.decide(
            "map10.tga", meta, "entity frame drawn=20 culled=1", PASSING_PIXELS)
        self.assertEqual(code, 1)
        self.assertIn("FAIL: entity rendering disabled", lines)
        self.assertFalse(any("PASS" in line for line in lines))
        self.assertNotIn("no conclusion", lines)

    def test_zero_drawn_fails(self):
        meta = {"entities_enabled": True}
        status, message = self.mod.evaluate_entity_gate(
            "state-gamein.tga", meta, "drawn=0 culled=228")
        self.assertEqual(status, "fail")
        self.assertIn("entity drawn count is zero", message)

    def test_missing_log_is_no_conclusion_not_pass(self):
        meta = {"entities_enabled": True, "in_game_map": True}
        code, lines = self.mod.decide("map10.tga", meta, "", PASSING_PIXELS)
        self.assertEqual(code, 3)
        self.assertEqual(lines[0], "no conclusion")
        self.assertFalse(any("PASS" in line for line in lines))

    def test_missing_metadata_and_log_is_no_conclusion(self):
        status, message = self.mod.evaluate_entity_gate("state-gamein.tga", None, "")
        self.assertEqual(status, "no-conclusion")
        self.assertEqual(message, "no conclusion")

    def test_positive_drawn_count_can_pass_with_pixels(self):
        meta = {"entities_enabled": True}
        code, lines = self.mod.decide(
            "map10.tga", meta, "mxh_client: entity frame drawn=4 culled=9",
            PASSING_PIXELS)
        self.assertEqual(code, 0)
        self.assertTrue(any(line.startswith("PASS:") for line in lines))
        self.assertIn("entity drawn=4 culled=9", lines)

    def test_pixel_failure_beats_missing_log(self):
        pixels = dict(PASSING_PIXELS)
        pixels["monster-dark-body"] = False
        code, lines = self.mod.decide("map10.tga", {"entities_enabled": True}, "", pixels)
        self.assertEqual(code, 1)
        self.assertIn("FAIL: monster-dark-body", lines)
        self.assertFalse(any(line == "no conclusion" or line.startswith("PASS") for line in lines))

    def test_named_metadata_claims_a_custom_frame(self):
        meta = {"in_game_map": True, "frame": "custom-gamein.tga", "entities_enabled": False}
        status, _ = self.mod.evaluate_entity_gate("custom-gamein.tga", meta, "")
        self.assertEqual(status, "fail")
        other, _ = self.mod.evaluate_entity_gate("state-login.tga", meta, "")
        self.assertEqual(other, "not-applicable")

    def test_string_false_counts_as_disabled(self):
        status, message = self.mod.evaluate_entity_gate(
            "map10.tga", {"entities_enabled": "false"}, "drawn=3 culled=0")
        self.assertEqual(status, "fail")
        self.assertIn("disabled", message)


class DiscoveryTests(unittest.TestCase):
    def setUp(self):
        self.mod = load_module()

    def test_parent_result_and_sibling_log(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            evidence = root / "evidence"
            evidence.mkdir()
            frame = evidence / "state-gamein.tga"
            frame.write_bytes(b"not-a-tga")
            (root / "result.json").write_text(json.dumps({
                "map_number": 10,
                "entities_enabled": False,
            }), encoding="utf-8")
            logs = root / "logs"
            logs.mkdir()
            (logs / "client.stderr.log").write_text(
                "mxh_client: entity frame drawn=2 culled=9\n", encoding="utf-8")
            meta = self.mod.load_metadata(frame, None)
            self.assertIs(meta["entities_enabled"], False)
            self.assertIn("drawn=2", self.mod.load_logs(frame, None))
            status, message = self.mod.evaluate_entity_gate(frame.name, meta, "")
            self.assertEqual(status, "fail")
            self.assertIn("disabled", message)

    def test_capture_metadata_is_preferred_over_result(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            frame = root / "map10.tga"
            frame.write_bytes(b"x")
            (root / "result.json").write_text(
                json.dumps({"entities_enabled": False}), encoding="utf-8")
            (root / "capture-metadata.json").write_text(
                json.dumps({"entities_enabled": True, "frame": "map10.tga"}),
                encoding="utf-8")
            meta = self.mod.load_metadata(frame, None)
            self.assertIs(meta["entities_enabled"], True)


class MainTests(unittest.TestCase):
    def setUp(self):
        self.mod = load_module()

    def _run(self, frame, extra=None):
        argv = ["verify-entity-frame.py", str(frame)]
        if extra:
            argv.extend(extra)
        buffer = io.StringIO()
        with redirect_stdout(buffer):
            code = self.mod.main(argv)
        return code, buffer.getvalue()

    def test_disabled_metadata_exits_nonzero_without_pass(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            frame = root / "map10.tga"
            frame.write_bytes(b"not-an-image")
            (root / "capture-metadata.json").write_text(json.dumps({
                "in_game_map": True,
                "map_number": 10,
                "frame": "map10.tga",
                "entities_enabled": False,
            }), encoding="utf-8")
            code, out = self._run(frame)
        self.assertEqual(code, 1)
        self.assertIn("FAIL: entity rendering disabled", out)
        self.assertNotIn("PASS", out)
        self.assertNotIn("no conclusion", out)

    def test_zero_drawn_log_exits_nonzero(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            frame = root / "state-gamein.tga"
            frame.write_bytes(b"not-an-image")
            log = root / "client.err.log"
            log.write_text("drawn/culled=0/15\n", encoding="utf-8")
            code, out = self._run(frame, ["--log", str(log)])
        self.assertEqual(code, 1)
        self.assertIn("FAIL: entity drawn count is zero", out)
        self.assertNotIn("PASS", out)

    def test_pixels_that_would_pass_stay_inconclusive_without_log(self):
        self.mod.read_pixel_checks = lambda frame: (dict(PASSING_PIXELS), None)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            frame = root / "map10.tga"
            frame.write_bytes(b"unused")
            (root / "capture-metadata.json").write_text(
                json.dumps({"entities_enabled": True, "frame": "map10.tga"}),
                encoding="utf-8")
            code, out = self._run(frame)
        self.assertEqual(code, 3)
        self.assertTrue(out.splitlines()[0] == "no conclusion")
        self.assertNotIn("PASS", out)

    def test_usage(self):
        buffer = io.StringIO()
        with redirect_stdout(buffer):
            code = self.mod.main(["verify-entity-frame.py"])
        self.assertEqual(code, 2)
        self.assertIn("usage:", buffer.getvalue())


if __name__ == "__main__":
    unittest.main()
