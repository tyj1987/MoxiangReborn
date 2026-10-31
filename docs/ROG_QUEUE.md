# ROG queue

Checks in this list are not CI passes. A skip is not a pass. Each item
reads the original client tree read-only from `D:\MX` on ROG. Do not copy
those bytes into git, logs, or this document.

## Non-LFS pak archives removed from the tree

`modern/data/PlayDH/npc.pak`, `Pet.pak`, and `Titan.pak` are not in git.
CI covers them with synthetic archives only
(`PackFileHeader.SyntheticNpcPetTitanArchivesRoundTrip` and
`modern/tests/unit/tools/test_repack_titan_bin.py`).

Run these on ROG, read-only, against `D:\MX`:

1. `PackFile.ParsesAllRealPakFiles` — open `Effect.pak`, `Character.pak`,
   `Map.pak`, `monster.pak`, `npc.pak`, `Pet.pak`, and `Titan.pak` from the
   original client directory. The repo checkout prints
   `SKIP real <name> absent: ROG-only` and skips. That skip is not a pass.
2. `scripts/verify_real_resources.py` — list `npc.pak` (422 entries),
   `Pet.pak` (362), and `Titan.pak` (294) from `D:\MX`. When the repo copy
   is absent the script prints `SKIP real <name> absent: ROG-only` and does
   not count the skip as a pass.
3. `scripts/verify_algorithms.ps1` — the same three names. Absent repo files
   print `SKIP real <name> absent: ROG-only`. Do not treat that skip as a pass.
4. `modern/tools/compat_round_trip.py` — developer audit of `.bmhm`, `.ttb`,
   and `.bsad` under `D:\MX`. The ctest module
   `modern/tests/unit/tools/test_compat_round_trip.py` is synthetic and stays
   required. The real-tree run is this queue item, not a CI pass.
5. `modern/tools/repack_titan_bin.py` — the required test encodes a synthetic
   payload. It does not open `Titan.pak`. Confirming the original `Titan.pak`
   still opens is item 1, read-only from `D:\MX`.
