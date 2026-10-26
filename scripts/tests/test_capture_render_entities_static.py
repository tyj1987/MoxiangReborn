"""Static check: every smoke-exit capture script enables entity rendering.

There is no PowerShell harness here. This reads scripts/*.ps1 and locks
the MXH_GUI_SMOKE_RENDER_ENTITIES default, the -NoEntities opt-out, and
the entities_enabled metadata field.
"""

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPTS = ROOT / "scripts"

_EXIT_ASSIGN = re.compile(r"MXH_GUI_SMOKE_EXIT['\"]?\]?\s*=\s*'1'")
_RENDER_ASSIGN = re.compile(r"MXH_GUI_SMOKE_RENDER_ENTITIES['\"]?\]?\s*=\s*'1'")
_FOLLOW_ONLY = re.compile(
    r"if\s*\(\s*\$FollowCamera\s*\)\s*\{\s*"
    r"\$env:MXH_GUI_SMOKE_RENDER_ENTITIES\s*=\s*'1'\s*\}"
)


def smoke_exit_scripts():
    found = []
    for path in sorted(SCRIPTS.glob("*.ps1")):
        text = path.read_text(encoding="utf-8")
        if _EXIT_ASSIGN.search(text):
            found.append(path)
    return found


class CaptureEntityDefaultTests(unittest.TestCase):
    def test_known_smoke_exit_scripts(self):
        names = [path.name for path in smoke_exit_scripts()]
        self.assertEqual(names, [
            "capture-gamein.ps1",
            "capture-state-frames.ps1",
            "gui-client-smoke.ps1",
        ])

    def test_each_smoke_exit_script_defaults_entities_on(self):
        offenders = []
        for path in smoke_exit_scripts():
            text = path.read_text(encoding="utf-8")
            if "[switch]$NoEntities" not in text:
                offenders.append(f"{path.name}: missing [switch]$NoEntities")
            if not _RENDER_ASSIGN.search(text):
                offenders.append(f"{path.name}: does not assign RENDER_ENTITIES='1'")
            if "if ($NoEntities)" not in text:
                offenders.append(f"{path.name}: missing -NoEntities opt-out branch")
            if "entities_enabled" not in text:
                offenders.append(f"{path.name}: does not record entities_enabled")
            if "capture-metadata.json" not in text and path.name != "capture-gamein.ps1":
                offenders.append(f"{path.name}: does not write capture-metadata.json")
            if path.name == "capture-gamein.ps1" and "capture-metadata.json" not in text:
                offenders.append("capture-gamein.ps1: does not write capture-metadata.json")
            if _FOLLOW_ONLY.search(text):
                offenders.append(
                    f"{path.name}: RENDER_ENTITIES still gated only on -FollowCamera"
                )
        self.assertEqual(offenders, [])

    def test_other_smoke_scripts_do_not_set_smoke_exit(self):
        # These launch a client or wrap a smoke, but they do not set
        # MXH_GUI_SMOKE_EXIT, so they do not hit the empty-entity path.
        innocent = [
            "commercial-smoke.ps1",
            "test-gpu-smoke.ps1",
            "visual-smoke.ps1",
        ]
        for name in innocent:
            text = (SCRIPTS / name).read_text(encoding="utf-8")
            self.assertIsNone(
                _EXIT_ASSIGN.search(text),
                f"{name} unexpectedly sets MXH_GUI_SMOKE_EXIT",
            )


if __name__ == "__main__":
    unittest.main()
