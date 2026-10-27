"""Synthetic tests for the installed Resource .bin container refusal.

Fixtures are built in memory. No original game bytes are read or written.
"""

from __future__ import annotations

import random
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

_THIS_DIR = Path(__file__).resolve().parent
_TOOLS_DIR = _THIS_DIR.parents[2] / "tools"
if str(_TOOLS_DIR) not in sys.path:
    sys.path.insert(0, str(_TOOLS_DIR))

import mh_bin  # noqa: E402


def synthetic_obfuscated_bin(body_len: int = 32, seed: int = 1) -> bytes:
    """Length prefix + 16 filler bytes + constructed body. Not a game file."""
    rng = random.Random(seed)
    body = bytes(rng.randrange(256) for _ in range(body_len))
    # 0xA5 makes the classic file_size field implausible (above the 256 MB cap).
    filler = bytes([0xA5]) * 16
    total = 4 + len(filler) + len(body)
    return struct.pack("<I", total) + filler + body


def plain_sworking_bin(payload: bytes, type_: int = 3, version: int | None = None) -> bytes:
    """Classic header + payload, no CRC bytes, matching write_mh_bin."""
    encrypted = mh_bin.encrypt_bin_payload(payload, type_)
    blob_size = 12 + len(encrypted)
    if version is None:
        version = 20040308 + type_ + len(payload)
    return struct.pack("<III", version, type_, len(payload)) + encrypted


class ObfuscatedBinContainerTests(unittest.TestCase):
    def test_synthetic_container_is_refused(self):
        data = synthetic_obfuscated_bin()
        self.assertGreaterEqual(len(data), 20)
        self.assertEqual(struct.unpack_from("<I", data, 0)[0], len(data))
        self.assertTrue(mh_bin.is_obfuscated_bin_container(data))
        with self.assertRaises(mh_bin.ObfuscatedBinContainerError) as caught:
            mh_bin.read_mh_bin_bytes(data, "synthetic.bin")
        self.assertIn("obfuscated container", str(caught.exception))
        self.assertIn("synthetic.bin", str(caught.exception))
        self.assertIn(f"bytes={len(data)}", str(caught.exception))

    def test_container_of_exactly_20_bytes_is_refused(self):
        data = synthetic_obfuscated_bin(body_len=0, seed=2)
        self.assertEqual(len(data), 20)
        with self.assertRaisesRegex(mh_bin.ObfuscatedBinContainerError, "obfuscated container"):
            mh_bin.read_mh_bin_bytes(data, "edge.bin")

    def test_short_size_prefix_is_not_the_container(self):
        payload = bytes(range(15))
        data = struct.pack("<I", 19) + payload
        self.assertEqual(len(data), 19)
        self.assertFalse(mh_bin.is_obfuscated_bin_container(data))
        parsed = mh_bin.read_mh_bin_bytes(data, "tiny.bin")
        self.assertEqual(parsed.type, 0)
        self.assertEqual(parsed.data, mh_bin.decrypt_bin_payload(payload, 0))

    def test_short_plain_file_whose_version_equals_size_is_not_rejected(self):
        payload = b"Hi"
        type_ = 1
        encrypted = mh_bin.encrypt_bin_payload(payload, type_)
        total = 14 + len(payload)
        self.assertLess(total, 20)
        data = (
            struct.pack("<III", total, type_, len(payload))
            + bytes([0x11])
            + encrypted
            + bytes([0x22])
        )
        self.assertEqual(struct.unpack_from("<I", data, 0)[0], len(data))
        self.assertFalse(mh_bin.is_obfuscated_bin_container(data))
        parsed = mh_bin.read_mh_bin_bytes(data, "short-plain.bin")
        self.assertEqual(parsed.type, type_)
        self.assertEqual(parsed.data, payload)

    def test_plain_sworking_bin_still_parses(self):
        payload = b"Hello Moxian"
        data = plain_sworking_bin(payload, type_=7)
        self.assertFalse(mh_bin.is_obfuscated_bin_container(data))
        parsed = mh_bin.read_mh_bin_bytes(data, "plain.bin")
        self.assertEqual(parsed.type, 7)
        self.assertEqual(parsed.file_size, len(payload))
        self.assertEqual(parsed.data, payload)

    def test_plain_file_whose_version_equals_size_is_not_rejected(self):
        payload = b"plain-size-match"
        type_ = 4
        encrypted = mh_bin.encrypt_bin_payload(payload, type_)
        total = 14 + len(payload)
        # version == file size, but the classic header still fits.
        data = struct.pack("<III", total, type_, len(payload)) + bytes([0x11]) + encrypted + bytes([0x22])
        self.assertEqual(struct.unpack_from("<I", data, 0)[0], len(data))
        self.assertFalse(mh_bin.is_obfuscated_bin_container(data))
        parsed = mh_bin.read_mh_bin_bytes(data, "coincidence.bin")
        self.assertEqual(parsed.version, total)
        self.assertEqual(parsed.type, type_)
        self.assertEqual(parsed.data, payload)

    def test_length_mismatch_is_not_reported_as_container(self):
        data = bytearray(synthetic_obfuscated_bin())
        data[0] = 1
        data = bytes(data)
        self.assertFalse(mh_bin.is_obfuscated_bin_container(data))
        with self.assertRaises(ValueError) as caught:
            mh_bin.read_mh_bin_bytes(data, "broken.bin")
        self.assertNotIsInstance(caught.exception, mh_bin.ObfuscatedBinContainerError)
        self.assertNotIn("obfuscated container", str(caught.exception))

    def test_cli_refuses_synthetic_container_with_nonzero_exit(self):
        data = synthetic_obfuscated_bin(body_len=24, seed=9)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "synthetic.bin"
            path.write_bytes(data)
            result = subprocess.run(
                [sys.executable, "-B", str(_TOOLS_DIR / "mh_bin.py"), str(path)],
                capture_output=True,
                text=True,
                check=False,
            )
        self.assertEqual(result.returncode, 1)
        self.assertIn("obfuscated container", result.stderr)
        self.assertEqual(result.stdout, "")

    def test_cli_reads_plain_bin(self):
        payload = b"cli-plain"
        data = plain_sworking_bin(payload, type_=1)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "plain.bin"
            path.write_bytes(data)
            result = subprocess.run(
                [sys.executable, "-B", str(_TOOLS_DIR / "mh_bin.py"), str(path)],
                capture_output=True,
                text=True,
                check=False,
            )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("type=1", result.stdout)
        self.assertNotIn("obfuscated container", result.stderr)


if __name__ == "__main__":
    unittest.main()
