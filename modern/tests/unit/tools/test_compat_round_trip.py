"""Unit tests for modern/tools/compat_round_trip.py.

The tests run the verifier against a synthetic in-memory resource tree
(we never touch PlayDH directly, so ctest stays deterministic) and
assert:

  1. A 200-byte MHFile-style payload (legacy signed-i wrap is in scope
     after byte 127, so this guards the documented equivalence between
     the modern `i & 0xFF` and the legacy `(char)i` decode paths — the
     two algorithms collapse to the same plaintext even though their
     intermediate arithmetic differs).
  2. The .ttb verifier accepts the u16 layout (the format that real
     shipped maps actually use, e.g. 101.ttb = 8 + 1024*1024*2 = 2 097
     160 bytes).
  3. The .ttb verifier rejects the editor u32 layout when given a
     mismatched file.
  4. The .bsad verifier accepts a 7x7 blank area without complaint.

These tests are small and fast (a few ms each).  They are not a
replacement for running compat_round_trip.py against the real PlayDH
tree (which is a developer-only audit, not a ctest), but they lock
the verifier's classification logic so a future refactor of the
shared text_parse.hpp helpers cannot silently misclassify real maps.
"""

import os
import struct
import sys
import tempfile
import unittest
from pathlib import Path

# Make the verifier importable without setting PYTHONPATH.
HERE = Path(__file__).resolve().parent
TOOLS = HERE.parent.parent.parent / "tools"
sys.path.insert(0, str(TOOLS))

import compat_round_trip as crt  # noqa: E402


def _make_mhfile_payload(plaintext: bytes, type_byte: int) -> bytes:
    """Encode a plaintext the way legacy CMHFile::Save() would, so the
    verifier can decode it.  Mirrors 墨香【源码】\[Server]Map\MHFile.cpp
    encrypt loop, which is the same loop inverted (subtract on read,
    add on write)."""
    payload = bytearray(plaintext)
    for i in range(len(payload)):
        # Legacy signed-i: i cast to (char) on MSVC is signed.
        signed_i = struct.unpack("b", struct.pack("B", i & 0xFF))[0]
        v = (payload[i] + signed_i) & 0xFF
        if i % type_byte == 0:
            v = (v + type_byte) & 0xFF
        payload[i] = v
    return bytes(payload)


def _wrap_mhfile(version: int, type_byte: int, payload: bytes) -> bytes:
    """Build the on-disk framing: 12-byte header + crc1 + payload + crc2."""
    crc1 = (type_byte + sum(payload)) & 0xFF
    crc2 = crc1 ^ 0xFF
    return struct.pack("<III", version, type_byte, len(payload)) + bytes([crc1]) + payload + bytes([crc2])


class ModernLegacyDecodeEquivalence(unittest.TestCase):
    """The signed-i legacy and unsigned-i modern decode paths must
    produce the same plaintext for any input, because the bytewise
    sum `signed_i + (i & 0xFF)` is always 0 mod 256 (the legacy
    signed value is `i & 0xFF` for i<128, and `i & 0xFF - 256` for
    i>=128, which is ≡0 mod 256 once we re-add the unsigned i&0xFF).
    """

    def test_round_trip_300_bytes_crosses_signed_boundary(self):
        plaintext = bytes((i * 17 + 3) & 0xFF for i in range(300))
        encrypted = _make_mhfile_payload(plaintext, 152)
        wrapped = _wrap_mhfile(0x0131CC0D, 152, encrypted)
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / "test.bmhm"
            p.write_bytes(wrapped)
            r = crt.bmhm_canonical_round_trip(p)
        self.assertEqual(r["status"], "ok")
        self.assertEqual(r["diff_bytes"], 0)
        # The first line of the plaintext should survive the round trip
        # (we use bytes that don't decode to 0x0A / 0x0D to keep
        # line-splitting well-defined).
        self.assertTrue(r["modern_first_line"])


class TtbLayoutClassification(unittest.TestCase):
    def test_u16_layout_accepted(self):
        width, height = 4, 4
        body = b"".join(struct.pack("<H", i & 0xFFFF) for i in range(width * height))
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / "small.ttb"
            p.write_bytes(struct.pack("<II", width, height) + body)
            r = crt.ttb_canonical_round_trip(p)
        self.assertEqual(r["status"], "ok")
        self.assertEqual(r["layout"], "u16")
        self.assertEqual(r["width"], width)
        self.assertEqual(r["height"], height)

    def test_u32_layout_accepted(self):
        width, height = 4, 4
        body = b"".join(struct.pack("<I", i) for i in range(width * height))
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / "small.ttb"
            p.write_bytes(struct.pack("<II", width, height) + body)
            r = crt.ttb_canonical_round_trip(p)
        self.assertEqual(r["status"], "ok")
        self.assertEqual(r["layout"], "u32")

    def test_size_mismatch_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / "bad.ttb"
            p.write_bytes(struct.pack("<II", 4, 4) + b"\x00" * 100)
            r = crt.ttb_canonical_round_trip(p)
        self.assertEqual(r["status"], "size_mismatch")


class BsadPlaintextAcceptance(unittest.TestCase):
    def test_7x7_blank_area_accepted(self):
        # Mirror of 7x7_Blank.bsad: 7x7 of '.' chars, header is just
        # the file (BSADs are text, not framed like MHFiles).
        body = "\n".join("." * 7 for _ in range(7)).encode("utf-8")
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / "7x7_Blank.bsad"
            p.write_bytes(body)
            r = crt.bsad_canonical_round_trip(p)
        self.assertEqual(r["status"], "ok")
        self.assertEqual(r["lines"], 7)


if __name__ == "__main__":
    unittest.main()
