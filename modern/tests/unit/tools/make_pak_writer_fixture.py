#!/usr/bin/env python3
"""Build the synthetic pak read by pak_writer_packfile_test.cpp.

    make_pak_writer_fixture.py <out.pak>

Payloads are LCG bytes (x = x * 1103515245 + 12345 mod 2^32, byte =
(x >> 16) & 0xff, seed = entry size).  The C++ test regenerates them, so
no data file is checked in.  Keep FIXTURE in sync with the C++ table.
"""

from __future__ import annotations

import hashlib
import shutil
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "tools"))
import pak_writer  # noqa: E402

FIXTURE = (("b_tree.MOD", 257), ("A.stm", 1024), ("z.tif", 1), ("map/c.hfl", 33))


def lcg_bytes(size: int) -> bytes:
    x, out = size, bytearray()
    for _ in range(size):
        x = (x * 1103515245 + 12345) & 0xFFFFFFFF
        out.append((x >> 16) & 0xFF)
    return bytes(out)


def main(argv: list[str]) -> int:
    out = Path(argv[1])
    with tempfile.TemporaryDirectory() as tmp:
        rows = []
        for name, size in FIXTURE:
            data = lcg_bytes(size)
            path = Path(tmp) / "in" / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
            rows.append({"name": name, "sha256": hashlib.sha256(data).hexdigest()})
        # pak_writer refuses repo/build trees; write outside, then copy.
        built = Path(tmp) / "out" / "fixture.pak"
        pak_writer.write_pak({"entries": rows}, Path(tmp) / "in", built)
        out.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(built, out)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
