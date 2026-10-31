"""Synthetic round-trip for modern/tools/repack_titan_bin.py.

The encoder is exercised on bytes built in this file. It does not open
the PlayDH tree. A real-archive read is ROG-only; see docs/ROG_QUEUE.md.
"""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

TOOLS = Path(__file__).resolve().parents[3] / "tools"
sys.path.insert(0, str(TOOLS))

import repack_titan_bin as repack  # noqa: E402


class RepackSynthetic(unittest.TestCase):
    def test_ascii_payload_round_trip(self) -> None:
        raw = b"SYNTH-npc-pet-titan"
        dw_type = 3
        encoded, crc = repack.encode_payload(raw, dw_type)
        self.assertEqual(repack.decode_payload(encoded, dw_type), raw)
        blob = repack.build_bin(raw, dw_type)
        header, payload, crc1, crc2 = repack.parse_bin(blob)
        self.assertEqual(header.dw_type, dw_type)
        self.assertEqual(header.dw_file_size, len(raw))
        self.assertEqual(header.dw_version, 20040308 + dw_type + len(raw))
        self.assertEqual(crc1, crc)
        self.assertEqual(crc1, crc2)
        self.assertEqual(repack.decode_payload(payload, header.dw_type), raw)

    def test_in_source_sentinel_round_trip(self) -> None:
        raw = repack.SENTINEL_UTF8
        dw_type = 7
        encoded, _crc = repack.encode_payload(raw, dw_type)
        self.assertEqual(repack.decode_payload(encoded, dw_type), raw)
        blob = repack.build_bin(raw, dw_type)
        header, payload, crc1, crc2 = repack.parse_bin(blob)
        self.assertEqual(crc1, crc2)
        self.assertEqual(repack.decode_payload(payload, header.dw_type), raw)


if __name__ == "__main__":
    unittest.main()
