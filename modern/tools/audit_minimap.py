#!/usr/bin/env python3
"""
audit_minimap.py -- one-shot health check of the PlayDH minimap resources.

Reports:
  - real minimap DDS coverage (mini_<N>.dds + mini_<N>_ful.dds)
  - per-map presence table (which maps have which variants)
  - minimap.bin path-index files (Minimap<N>.bin, encrypted path index)
  - image_minimap_path.bin (the central path index)
  - optional --check-fold flag that runs the same audit that
    polish_assets.py --generate-minimap uses, so the report is the
    exact input that drives the placeholder synthesizer

Output is written to stdout as a markdown report -- safe to redirect
to ``playdh_minimap_audit.md`` for archive.

Usage:
  python audit_minimap.py --playdh modern/data/PlayDH
  python audit_minimap.py --playdh modern/data/PlayDH --check-fold
  python audit_minimap.py --playdh modern/data/PlayDH --json    # machine-readable
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

# Mirror the same parser as modern/tools/polish_assets.py so the
# audit and the synthesizer agree on what counts as a minimap.
def _parse_mini_filename(stem: str) -> int | None:
    if not stem.lower().startswith("mini_"):
        return None
    rest = stem[5:]
    if rest.endswith("_ful"):
        rest = rest[:-4]
    if not rest or not rest.isdigit():
        return None
    return int(rest)


def scan_minimap_dds(dds_dir: Path) -> dict[int, dict[str, bool]]:
    """Return {map_id: {'plain': bool, 'ful': bool}}."""
    out: dict[int, dict[str, bool]] = {}
    if not dds_dir.is_dir():
        return out
    for entry in dds_dir.iterdir():
        if not entry.is_file() or entry.suffix.lower() != ".dds":
            continue
        stem = entry.stem
        is_ful = stem.lower().endswith("_ful")
        base = stem[:-4] if is_ful else stem
        mid = _parse_mini_filename(base)
        if mid is None:
            continue
        rec = out.setdefault(mid, {"plain": False, "ful": False})
        rec["ful" if is_ful else "plain"] = True
    return out


def scan_minimap_bins(bin_dir: Path) -> list[Path]:
    """Return all Minimap<N>.bin path-index files."""
    if not bin_dir.is_dir():
        return []
    return sorted(bin_dir.glob("Minimap*.bin"))


def scan_central_path_index(playdh: Path) -> dict[str, int | str]:
    """Report on image_minimap_path.bin if it exists."""
    out: dict[str, int | str] = {"exists": False}
    p = playdh / "Image" / "image_minimap_path.bin"
    if p.is_file():
        data = p.read_bytes()
        out["exists"] = True
        out["size"] = len(data)
        # 4-byte LE magic (legacy family) — 0x0131xxxx
        if len(data) >= 4:
            magic = int.from_bytes(data[:4], "little")
            out["magic"] = f"0x{magic:08x}"
    return out


def _check_fold_consistency(playdh: Path, known_map_ids: list[int]) -> dict[str, int]:
    """Run the same scan that polish_assets --generate-minimap uses
    (with no --force) and report the missing/synthesize counts.
    """
    mm_dir = playdh / "Image" / "MiniMap"
    existing = set(scan_minimap_dds(mm_dir).keys())
    known = set(known_map_ids)
    missing = sorted(known - existing)
    return {
        "known": len(known),
        "existing": len(existing),
        "missing": len(missing),
        "to_synthesize": 2 * len(missing),
    }


def build_report(playdh: Path, known_map_ids: list[int], check_fold: bool) -> str:
    dds_dir = playdh / "Image" / "MiniMap"
    dds = scan_minimap_dds(dds_dir)
    bins = scan_minimap_bins(dds_dir)
    central = scan_central_path_index(playdh)

    known_set = set(known_map_ids)
    in_dds = set(dds.keys())
    in_dds_known = in_dds & known_set
    extra_dds = sorted(in_dds - known_set)
    missing_dds = sorted(known_set - in_dds)

    plain_count = sum(1 for r in dds.values() if r.get("plain"))
    ful_count = sum(1 for r in dds.values() if r.get("ful"))
    both = sum(1 for r in dds.values() if r.get("plain") and r.get("ful"))
    plain_only = sum(1 for r in dds.values() if r.get("plain") and not r.get("ful"))
    ful_only = sum(1 for r in dds.values() if r.get("ful") and not r.get("plain"))

    lines: list[str] = []
    lines.append("# PlayDH minimap audit\n")
    lines.append(f"- playdh root: `{playdh}`")
    lines.append(f"- known map ids: {len(known_set)}")
    lines.append(f"- DDS scans:")
    lines.append(f"  - real `mini_<N>.dds`: {plain_count}")
    lines.append(f"  - real `mini_<N>_ful.dds`: {ful_count}")
    lines.append(f"  - both variants present: {both}")
    lines.append(f"  - plain only: {plain_only}")
    lines.append(f"  - ful only: {ful_only}")
    lines.append(f"  - extra DDS outside known map ids: {len(extra_dds)}")
    lines.append(f"- `Minimap<N>.bin` (encrypted path index): {len(bins)}")
    if central["exists"]:
        lines.append(f"- `image_minimap_path.bin`: exists, size={central['size']} bytes, magic={central.get('magic')}")
    else:
        lines.append(f"- `image_minimap_path.bin`: missing")

    if missing_dds:
        lines.append("\n## Missing minimap DDS (known map id, no DDS on disk)\n")
        for mid in missing_dds:
            lines.append(f"- map {mid}")
    if extra_dds:
        lines.append("\n## Extra minimap DDS (not in known map ids)\n")
        for mid in extra_dds[:20]:
            lines.append(f"- map {mid}")
        if len(extra_dds) > 20:
            lines.append(f"- ... and {len(extra_dds) - 20} more")

    if check_fold:
        fold = _check_fold_consistency(playdh, known_map_ids)
        lines.append("\n## Fold-mode dry run (what `polish_assets --generate-minimap` would do)\n")
        lines.append(f"- known map ids: {fold['known']}")
        lines.append(f"- existing real DDS: {fold['existing']}")
        lines.append(f"- missing (would synthesize): {fold['missing']}")
        lines.append(f"- DDS files to write: {fold['to_synthesize']}")

    lines.append("\n## Health verdict\n")
    if not missing_dds and both == len(known_set):
        lines.append("- PASS: every known map id has both `mini_<N>.dds` and `mini_<N>_ful.dds`.")
    else:
        lines.append("- INCOMPLETE: see missing/extra sections above.")
    if central["exists"]:
        lines.append("- `image_minimap_path.bin` present (encrypted path index loaded by client).")
    return "\n".join(lines) + "\n"


def build_json(playdh: Path, known_map_ids: list[int], check_fold: bool) -> str:
    dds = scan_minimap_dds(playdh / "Image" / "MiniMap")
    bins = scan_minimap_bins(playdh / "Image" / "MiniMap")
    central = scan_central_path_index(playdh)
    payload = {
        "playdh": str(playdh),
        "known_map_ids_count": len(known_map_ids),
        "real_dds": dds,
        "real_dds_count": len(dds),
        "minimap_bins_count": len(bins),
        "central_path_index": central,
    }
    if check_fold:
        payload["fold_dry_run"] = _check_fold_consistency(playdh, known_map_ids)
    return json.dumps(payload, indent=2, sort_keys=True) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--playdh", default="modern/data/PlayDH",
                    help="Path to the PlayDH/ root (default: modern/data/PlayDH)")
    ap.add_argument("--check-fold", action="store_true",
                    help="Also report what `polish_assets --generate-minimap` would do")
    ap.add_argument("--json", action="store_true",
                    help="Emit machine-readable JSON instead of markdown")
    args = ap.parse_args()

    playdh = Path(args.playdh)
    if not playdh.is_dir():
        print(f"[audit] not a directory: {playdh}", file=sys.stderr)
        return 2

    # Re-use the canonical map-id list from polish_assets so the audit
    # and the placeholder generator agree.
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    try:
        import polish_assets  # type: ignore
        known = polish_assets._known_map_ids()[:65]
    except Exception:
        # Fallback: 0..65 if polish_assets can't be imported.
        known = list(range(66))

    if args.json:
        sys.stdout.write(build_json(playdh, known, args.check_fold))
    else:
        sys.stdout.write(build_report(playdh, known, args.check_fold))
    return 0


if __name__ == "__main__":
    sys.exit(main())
