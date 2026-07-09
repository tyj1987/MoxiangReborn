#!/usr/bin/env python3
# -*- coding: utf-8 -*-
r"""
repack_titan_bin.py - Phase 7.5g (Bug C-34) re-encoder for PackingMan .bin files.

Background
----------
PackingMan's ``CMHFileEx::ConvertBin()`` encrypts the .bin payload with a
positional XOR keyed on the byte index and (optionally) the per-file ``dwType``:

    encoded[i] = (raw[i] + (char)i + (char)dwType if (i % dwType == 0)) & 0xFF
    crc        = (char)dwType + sum(encoded) & 0xFF
    file       = { header{dwVersion=20040308+type+size, dwType, dwFileSize},
                   crc1,
                   encoded[0..dwFileSize-1],
                   crc2 }

The runtime ``CMHFile::CheckCRC()`` inverts this in-place, leaving ``m_pData``
holding the raw bytes. ``GetStringInQuotation()`` then scans for the next
``"..."" pair.

The runtime server binary embeds a sentinel strcmp in ``Server.cpp``::

    if( strcmp( temp, "이 파일이 없으면 타이탄 업데이트 안돼요~" ) != 0 )

After Phase 7.5b the source tree is UTF-8 with ``/source-charset:utf-8``, so the
sentinel in the binary is the **UTF-8** byte sequence. Pre-Phase-7.5b
``SWorking/Resource/Server/TitanServer.bin`` was generated from a CP949
``.txt`` source and decrypts to the **EUC-KR** byte sequence — a different
set of bytes, so ``strcmp`` always fails. The smoke harness reports the
mismatch as ``sentinel mismatch (got '...')``.

This tool re-encodes any PackingMan .bin file so the *decoded* payload is the
exact byte sequence the runtime strcmp expects. For TitanServer.bin that means
encoding the **UTF-8** string ``"이 파일이 없으면 타이탄 업데이트 안돼요~"``.

Reference
---------
- Algorithm source of truth: ``墨香【源码】\[Tool]PackingMan\MHFileEx.cpp`` (ConvertBin / CheckCRC)
- Runtime decoder:           ``墨香【源码】\[Server]Map\MHFile.cpp`` (CheckCRC)
- Modern C++ port:           ``modern/src/mh_file_ex.cpp`` (encrypt_bin_payload / decrypt_bin_payload)
- Bug C-34 record:           ``docs/KNOWN_BUGS.md``
"""
from __future__ import annotations

import argparse
import os
import struct
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Final

# Module-private re import. The module docstring has many backslash-bracket
# sequences (e.g. ``\[Tool]PackingMan``) that would otherwise trip
# SyntaxWarning on Python 3.12+; we use raw here to silence it.

# The sentinel string the runtime strcmp() compares against. The leading and
# trailing ASCII double-quote are part of the protocol: GetStringInQuotation()
# skips bytes until the first '"', then reads until the matching '"' or LF.
SENTINEL_UTF8: Final[bytes] = (
    b'\x22'                                                 # "
    b'\xec\x9d\xb4'                                         # 이
    b'\x20'                                                 # space
    b'\xed\x8c\x8c'                                         # 파
    b'\xec\x9d\xbc'                                         # 일
    b'\xec\x9d\xb4'                                         # 이
    b'\x20'                                                 # space
    b'\xec\x97\x86'                                         # 없
    b'\xec\x9c\xbc'                                         # 으
    b'\xeb\xa9\xb4'                                         # 면
    b'\x20'                                                 # space
    b'\xed\x83\x80'                                         # 타
    b'\xec\x9d\xb4'                                         # 이
    b'\xed\x83\x84'                                         # 탄
    b'\x20'                                                 # space
    b'\xec\x97\x85'                                         # 업
    b'\xeb\x8d\xb0'                                         # 데
    b'\xec\x9d\xb4'                                         # 이
    b'\xed\x8a\xb8'                                         # 트
    b'\x20'                                                 # space
    b'\xec\x95\x88'                                         # 안
    b'\xeb\x8f\xbc'                                         # 돼
    b'\xec\x9a\x94'                                         # 요
    b'\x7e'                                                 # ~
    b'\x22'                                                 # "
)
assert len(SENTINEL_UTF8) == 59, f"expected 59 bytes, got {len(SENTINEL_UTF8)}"


@dataclass
class PackingManHeader:
    dw_version: int
    dw_type: int
    dw_file_size: int


def encode_payload(raw: bytes, dw_type: int) -> tuple[bytes, int]:
    """Run ``ConvertBin`` on ``raw`` and return ``(encoded, crc8)``.

    The CRC matches the legacy ``crc = (char)dwType + sum(encoded)`` formula.
    ``(char)i`` and ``(char)dwType`` are 8-bit signed truncations, which means
    for i/dwType >= 128 the additions wrap around — but for our 59-byte
    payload both values are always < 128 so this is a moot point.
    """
    if not (1 <= dw_type <= 256):
        raise ValueError(f"dw_type must be in 1..256, got {dw_type}")
    if dw_type > len(raw):
        # PackingMan's rand()%size+1 guarantees 1..size, so we enforce it too.
        raise ValueError(f"dw_type={dw_type} > payload size {len(raw)}")

    encoded = bytearray(len(raw))
    crc = dw_type & 0xFF
    for i, byte in enumerate(raw):
        v = byte + (i & 0xFF)
        if i % dw_type == 0:
            v += dw_type
        encoded[i] = v & 0xFF
        crc = (crc + encoded[i]) & 0xFF
    return bytes(encoded), crc


def decode_payload(encoded: bytes, dw_type: int) -> bytes:
    """Run ``CheckCRC`` in reverse — the runtime's exact algorithm."""
    if not (1 <= dw_type <= 256):
        raise ValueError(f"dw_type must be in 1..256, got {dw_type}")

    decoded = bytearray(len(encoded))
    for i, byte in enumerate(encoded):
        v = byte - (i & 0xFF)
        if i % dw_type == 0:
            v -= dw_type
        decoded[i] = v & 0xFF
    return bytes(decoded)


def build_bin(raw: bytes, dw_type: int) -> bytes:
    """Pack the 12-byte header + crc1 + payload + crc2 blob."""
    encoded, crc = encode_payload(raw, dw_type)
    dw_version = 20040308 + dw_type + len(raw)
    return (
        struct.pack('<III', dw_version, dw_type, len(raw))
        + struct.pack('B', crc)
        + encoded
        + struct.pack('B', crc)
    )


def parse_bin(blob: bytes) -> tuple[PackingManHeader, bytes, int, int]:
    """Inverse of build_bin — extract header, payload, crc1, crc2."""
    if len(blob) < 14:
        raise ValueError(f"file too small ({len(blob)} bytes) — minimum 14")
    dw_version, dw_type, dw_file_size = struct.unpack_from('<III', blob, 0)
    crc1 = blob[12]
    payload = bytes(blob[13:13 + dw_file_size])
    crc2 = blob[13 + dw_file_size]
    return (
        PackingManHeader(dw_version, dw_type, dw_file_size),
        payload,
        crc1,
        crc2,
    )


def verify_roundtrip(target: Path, expected_raw: bytes, dw_type: int) -> None:
    """Re-parse the freshly written .bin and confirm the runtime decoder
    reproduces ``expected_raw`` byte-for-byte."""
    blob = target.read_bytes()
    header, payload, crc1, crc2 = parse_bin(blob)

    print(f"  file size       : {len(blob)} bytes")
    print(f"  header.dwVersion: {header.dw_version}  (expected {20040308 + header.dw_type + header.dw_file_size})")
    print(f"  header.dwType   : {header.dw_type}")
    print(f"  header.dwFileSize: {header.dw_file_size}")
    print(f"  crc1=0x{crc1:02x}  crc2=0x{crc2:02x}  match={crc1 == crc2}")

    decoded = decode_payload(payload, header.dw_type)
    if decoded != expected_raw:
        raise SystemExit(
            f"ROUNDTRIP FAILED for {target}\n"
            f"  got:  {decoded!r}  hex={decoded.hex()}\n"
            f"  want: {expected_raw!r}  hex={expected_raw.hex()}"
        )

    # What the runtime's strcmp() will see.
    sentinel = decoded.decode('utf-8', errors='replace')
    print(f"  runtime strcmp sees: {sentinel!r}")
    print(f"  -> roundtrip OK")


# ---------------------------------------------------------------------------
# C-34 specific entry point
# ---------------------------------------------------------------------------

def fix_titan_server_bin(target: Path) -> None:
    """Re-encode the smoke-staging ``TitanServer.bin`` so it decrypts to the
    UTF-8 sentinel string the modern runtime expects.
    """
    # dwType 7 is a small deterministic pick (PackingMan used rand(), so any
    # 1..59 is valid). Keep it stable so the test can be reproduced.
    DW_TYPE = 7

    print(f"Phase 7.5g (C-34): repack {target}")
    print(f"  sentinel (UTF-8, {len(SENTINEL_UTF8)} bytes): {SENTINEL_UTF8!r}")
    print(f"  dwType: {DW_TYPE}")

    blob = build_bin(SENTINEL_UTF8, DW_TYPE)

    # Back up the old file in case we ever need to compare.
    backup = target.with_suffix(target.suffix + '.old_euc_kr')
    if target.exists() and not backup.exists():
        backup.write_bytes(target.read_bytes())
        print(f"  backup: {backup}")

    target.write_bytes(blob)
    print(f"  wrote : {target}  ({len(blob)} bytes)")

    verify_roundtrip(target, SENTINEL_UTF8, DW_TYPE)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Re-encode PackingMan .bin files (Phase 7.5g, Bug C-34).",
    )
    sub = parser.add_subparsers(dest="cmd", required=False)

    # C-34 specific
    sub.add_parser(
        "fix-titan",
        help="Re-encode SWorking/Resource/Server/TitanServer.bin to UTF-8.",
    )

    # Generic
    gen = sub.add_parser("encode", help="Encode a raw payload to .bin format.")
    gen.add_argument("--out", required=True, type=Path, help="Output .bin path.")
    gen.add_argument("--type", type=int, required=True, help="dwType (1..size).")
    gen.add_argument(
        "--encoding",
        default="utf-8",
        help="Source text encoding (default utf-8).",
    )
    gen.add_argument(
        "--text",
        help="Raw text to encode (use --text or --from-file, not both).",
    )
    gen.add_argument(
        "--from-file",
        type=Path,
        help="Read text from this file (auto-detected encoding by default).",
    )

    args = parser.parse_args(argv)

    if args.cmd is None or args.cmd == "fix-titan":
        target = Path(r"D:\smoke_test_full\Resource\Server\TitanServer.bin")
        fix_titan_server_bin(target)
        return 0

    if args.cmd == "encode":
        if args.text is not None:
            raw = args.text.encode(args.encoding)
        elif args.from_file is not None:
            raw = args.from_file.read_bytes()  # binary read; no transcoding
        else:
            parser.error("encode: provide --text or --from-file")
        blob = build_bin(raw, args.type)
        args.out.write_bytes(blob)
        print(f"wrote {args.out} ({len(blob)} bytes, dwType={args.type})")
        verify_roundtrip(args.out, raw, args.type)
        return 0

    parser.error(f"unknown subcommand: {args.cmd}")
    return 2


if __name__ == "__main__":
    sys.exit(main())
