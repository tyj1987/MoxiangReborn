#!/usr/bin/env python3
"""Pixel gate for the deterministic player/monster smoke frame.

An in-game Map frame (``state-gamein.tga``, ``map<N>.tga``, or metadata
that names this file with ``in_game_map`` true) must not pass when the
capture disabled entity rendering, or when a per-frame drawn count is
zero.

The renderer does not emit a per-frame drawn/culled line yet. Until it
does, a missing line is inconclusive: this script prints ``no conclusion``
and exits non-zero. It does not report PASS. Accepted line shapes, last
match on a single line wins::

    mxh_client: entity frame drawn=12 culled=3
    drawn/culled=12/3
    culled=3 drawn=12
"""

import json
import re
import sys
from pathlib import Path


_DRAWN_CULLED = re.compile(
    r"""
    (?:
        drawn\s*/\s*culled\s*[=:]?\s*(?P<d1>\d+)\s*/\s*(?P<c1>\d+)
      | \bdrawn\s*[=:]\s*(?P<d2>\d+)\b.{0,80}?\bculled\s*[=:]\s*(?P<c2>\d+)\b
      | \bculled\s*[=:]\s*(?P<c3>\d+)\b.{0,80}?\bdrawn\s*[=:]\s*(?P<d3>\d+)\b
    )
    """,
    re.IGNORECASE | re.VERBOSE,
)
_LOG_NAMES = (
    "client.stderr.log",
    "client.err.log",
    "client.stdout.log",
    "client.out.log",
)
_META_NAMES = ("capture-metadata.json", "result.json")
_USAGE = "usage: verify-entity-frame.py FRAME.tga [--metadata FILE] [--log FILE]"


def parse_drawn_culled(text):
    """Return the last ``(drawn, culled)`` pair, or None if no line exists."""
    if not text:
        return None
    found = None
    for line in text.splitlines():
        for match in _DRAWN_CULLED.finditer(line):
            if match.group("d1") is not None:
                found = (int(match.group("d1")), int(match.group("c1")))
            elif match.group("d2") is not None:
                found = (int(match.group("d2")), int(match.group("c2")))
            else:
                found = (int(match.group("d3")), int(match.group("c3")))
    return found


def _as_bool(value):
    if isinstance(value, bool):
        return value
    if isinstance(value, int) and value in (0, 1):
        return bool(value)
    if isinstance(value, str):
        lowered = value.strip().lower()
        if lowered in ("1", "true", "yes"):
            return True
        if lowered in ("0", "false", "no", ""):
            return False
    return None


def entities_enabled_from_meta(meta):
    """True/False when the capture recorded the flag, else None."""
    if not isinstance(meta, dict) or "entities_enabled" not in meta:
        return None
    return _as_bool(meta.get("entities_enabled"))


def claims_in_game_map(frame_name, meta):
    """Whether this file is presented as an in-game Map capture."""
    name = Path(str(frame_name)).name.lower()
    if name == "state-gamein.tga" or re.fullmatch(r"map\d+\.tga", name):
        return True
    if isinstance(meta, dict) and _as_bool(meta.get("in_game_map")) is True:
        named = Path(str(meta.get("frame") or meta.get("frame_name") or "")).name.lower()
        if named and named == name:
            return True
    return False


def evaluate_entity_gate(frame_name, meta, log_text):
    """Return ``(status, message)``.

    status is ``fail``, ``no-conclusion``, ``pass``, or ``not-applicable``.
    ``pass`` only means the entity-count gate is satisfied; pixel checks
    still decide the frame. A missing drawn/culled line is never ``pass``.
    """
    if not claims_in_game_map(frame_name, meta):
        return "not-applicable", "not an in-game Map frame"
    if entities_enabled_from_meta(meta) is False:
        return "fail", "FAIL: entity rendering disabled"
    counts = parse_drawn_culled(log_text or "")
    if counts is None:
        return "no-conclusion", "no conclusion"
    drawn, culled = counts
    if drawn == 0:
        return "fail", f"FAIL: entity drawn count is zero (culled={culled})"
    return "pass", f"entity drawn={drawn} culled={culled}"


def decide(frame_name, meta, log_text, pixel_results):
    """Combine the entity gate with pixel results.

    Returns ``(exit_code, lines)``. Exit 0 only when the frame may be
    reported as PASS. Exit 3 is ``no conclusion`` (log line absent) and
    is not a pass. Pixel failures stay exit 1 and do not print PASS.
    """
    status, message = evaluate_entity_gate(frame_name, meta, log_text)
    if status == "fail":
        return 1, [message]
    if pixel_results:
        failed = [name for name, ok in pixel_results.items() if not ok]
        if failed:
            return 1, [f"FAIL: {name}" for name in failed]
    if status == "no-conclusion":
        return 3, ["no conclusion", "per-frame drawn/culled log line absent"]
    lines = []
    if pixel_results:
        lines.extend(f"PASS: {name}" for name, ok in pixel_results.items() if ok)
    if status == "pass":
        lines.append(message)
    return 0, lines


def _search_dirs(frame):
    dirs = []
    current = frame.parent
    for _ in range(3):
        if current not in dirs:
            dirs.append(current)
        logs = current / "logs"
        if logs not in dirs:
            dirs.append(logs)
        if current.parent == current:
            break
        current = current.parent
    return dirs


def load_metadata(frame, explicit):
    """Load capture metadata. ``explicit`` is required when given."""
    if explicit is not None:
        path = Path(explicit)
        data = json.loads(path.read_text(encoding="utf-8-sig"))
        if not isinstance(data, dict):
            raise ValueError(f"metadata must be a JSON object: {path}")
        return data
    for directory in _search_dirs(frame):
        for name in _META_NAMES:
            path = directory / name
            if not path.is_file():
                continue
            try:
                data = json.loads(path.read_text(encoding="utf-8-sig"))
            except (OSError, json.JSONDecodeError):
                continue
            if isinstance(data, dict):
                return data
    return None


def load_logs(frame, explicit):
    """Read an explicit log, or concatenate nearby client logs."""
    if explicit is not None:
        return Path(explicit).read_text(encoding="utf-8", errors="replace")
    chunks = []
    seen = set()
    for directory in _search_dirs(frame):
        for name in _LOG_NAMES:
            path = directory / name
            if not path.is_file() or path in seen:
                continue
            seen.add(path)
            chunks.append(path.read_text(encoding="utf-8", errors="replace"))
    return "\n".join(chunks)


def pixel_checks(image):
    """Return ``(checks, error)``. ``error`` is set for a short frame."""
    if image.width < 800 or image.height < 600:
        return None, f"FAIL: unexpected dimensions {image.size}"
    sx, sy = image.width / 800.0, image.height / 600.0

    def crop(rect):
        return image.crop(tuple(int(round(v * (sx if i % 2 == 0 else sy)))
                           for i, v in enumerate(rect)))

    # The first deterministic spawn is east of LoginPoint 2012 and projects
    # into this region with the legacy 30-degree GameIn camera.
    # Keep a generous world-space crop around the first projected spawn. The
    # exact screen footprint shifts by a few pixels when the interaction smoke
    # opens inventory/skill UI before the settled frame; expanding the crop
    # avoids treating that valid projection shift as a missing material.
    monster_crop = crop((450, 180, 700, 450))
    pixels = list(monster_crop.get_flattened_data())
    dark = sum(1 for r, g, b in pixels if r < 70 and g < 70 and b < 55)
    red = sum(1 for r, g, b in pixels if r > 45 and r > g * 1.5 and r > b * 1.35)
    # The player is camera-centred. These three independent colour groups lock
    # a visible upright silhouette, skin and the original blue waist detail;
    # the terrain-only baseline stays well below all three thresholds.
    player = list(crop((375, 260, 425, 375)).get_flattened_data())
    player_dark = sum(1 for r, g, b in player if r < 75 and g < 75 and b < 75)
    player_skin = sum(1 for r, g, b in player if r > 65 and r > g * 1.2 and g > b * 1.02)
    checks = {
        "player-upright-silhouette": player_dark >= 1000,
        "player-skin": player_skin >= 80,
        "monster-dark-body": dark >= 300,
        "monster-red-markings": red >= 10,
    }
    return checks, None


def read_pixel_checks(frame):
    """Decode ``frame``. Returns ``(checks, error)``.

    ``error`` is ``pillow-missing`` when Pillow is not installed, a
    ``FAIL:`` string for a short or unreadable image, or None.
    """
    try:
        from PIL import Image
    except ImportError:
        return None, "pillow-missing"
    try:
        image = Image.open(frame).convert("RGB")
    except (OSError, ValueError) as exc:
        return None, f"FAIL: cannot read frame: {exc}"
    return pixel_checks(image)


def parse_args(argv):
    if len(argv) < 2:
        return None
    frame = None
    metadata = None
    log = None
    index = 1
    while index < len(argv):
        arg = argv[index]
        if arg in ("--metadata", "--log"):
            index += 1
            if index >= len(argv):
                return None
            if arg == "--metadata":
                metadata = Path(argv[index])
            else:
                log = Path(argv[index])
        elif arg.startswith("-"):
            return None
        elif frame is None:
            frame = Path(arg)
        else:
            return None
        index += 1
    if frame is None:
        return None
    return frame, metadata, log


def main(argv=None):
    argv = list(sys.argv if argv is None else argv)
    parsed = parse_args(argv)
    if parsed is None:
        print(_USAGE)
        return 2
    frame, metadata_path, log_path = parsed
    if not frame.is_file():
        print(f"FAIL: frame not found: {frame}")
        return 1
    try:
        meta = load_metadata(frame, metadata_path)
        log_text = load_logs(frame, log_path)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"FAIL: {exc}")
        return 1
    status, message = evaluate_entity_gate(frame.name, meta, log_text)
    if status == "fail":
        print(message)
        return 1
    checks, pixel_error = read_pixel_checks(frame)
    if pixel_error == "pillow-missing":
        if status == "no-conclusion":
            print("no conclusion")
            print("per-frame drawn/culled log line absent")
            return 3
        print("FAIL: Pillow is required to inspect the frame")
        return 1
    if pixel_error:
        print(pixel_error)
        return 1
    code, lines = decide(frame.name, meta, log_text, checks)
    for line in lines:
        print(line)
    return code


if __name__ == "__main__":
    raise SystemExit(main())
