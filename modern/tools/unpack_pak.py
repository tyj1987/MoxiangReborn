#!/usr/bin/env python3
"""
unpack_pak.py — decompress 4Dyuchi .pak archives.

Uses the legacy sequential-walk approach (CoStorage::Initialize):
  1. Read PACK_FILE_HEADER (92 bytes).
  2. For each entry:
     a. Read FSFILE_HEADER (32 bytes).
     b. Verify current file position == dwFileDataOffset.
     c. Read name (dwFileNameLen + 1 bytes).
     d. Skip dwRealFileSize bytes.

The total layout of each entry is: 32 byte header + nameLen bytes + 1 NUL
+ realSize bytes of data. The legacy writer uses dwTotalSize to compute
dwFileDataOffset and the file pads to 4-byte alignment at the end.
"""

import os
import struct
import sys


def unpack(pak_path: str, out_dir: str) -> None:
    with open(pak_path, "rb") as f:
        pak_header = f.read(92)
        if len(pak_header) < 92:
            sys.exit(f"pak header too short: {pak_path}")
        version, n_items, flag = struct.unpack("<III", pak_header[:12])
        if version != 1:
            sys.exit(f"unknown pak version {version:#x} in {pak_path}")
        print(f"version={version} n_items={n_items} flag={flag}")

        # The legacy reader uses SetFilePointer to get the current position
        # and compares it to dwFileDataOffset. After reading the header (32)
        # + name (dwFileNameLen + 1) bytes + realSize bytes, the cursor
        # advances to the next entry. Some real files have corrupt
        # dwFileDataOffset (zeros) so we just walk via layout.
        os.makedirs(out_dir, exist_ok=True)
        cur = 92
        n_written = 0
        for i in range(n_items):
            f.seek(cur)
            hdr = f.read(32)
            if len(hdr) < 32:
                print(f"  !! fsfile header truncated at i={i} cur=0x{cur:x}")
                break
            (_total, real, name_len, _data_off, _f1, _f2, _f3, _f4) = (
                struct.unpack("<IIIIIIII", hdr))
            if name_len > 4096:
                print(f"  !! name_len={name_len} too big at i={i} cur=0x{cur:x}")
                break
            if real > 0xFFFFFFFF:
                print(f"  !! real={real} too big at i={i} cur=0x{cur:x}")
                break
            name = f.read(name_len + 1).split(b"\x00", 1)[0].decode(
                "latin-1", errors="replace")
            data = f.read(real)
            safe = f"{i:05d}_{real}.bin"
            target = os.path.join(out_dir, safe)
            with open(target, "wb") as out:
                out.write(data)
            n_written += 1
            cur = cur + 32 + (name_len + 1) + real
            pad = (-cur) & 3
            cur += pad
            if i < 3 or i % 500 == 0:
                print(f"  [{i+1}/{n_items}] {safe}  ({real} bytes, name={name[:40]!r})")
        print(f"wrote {n_written} entries")


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    pak_path = sys.argv[1]
    if len(sys.argv) >= 3:
        out_dir = sys.argv[2]
    else:
        stem = os.path.splitext(pak_path)[0]
        out_dir = stem + "_unpacked"
    print(f"unpacking {pak_path} -> {out_dir}")
    print(f"  size = {os.path.getsize(pak_path)} bytes")
    unpack(pak_path, out_dir)
    print(f"done. output -> {out_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
