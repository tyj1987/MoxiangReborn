"""Synthetic tests for profile-selected .bin reads.

Fixtures are built in memory. No original game bytes are read or written.
Default and sworking-2008-reference keep the R-17 positional read.
playdh-current refuses that shape at 20 bytes or more.
"""

from __future__ import annotations

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


def synthetic_r17(payload: bytes) -> bytes:
    """``[uint32 total size][payload]``. Not a game file."""
    total = 4 + len(payload)
    return struct.pack("<I", total) + payload


def plain_sworking_bin(payload: bytes, type_: int = 3, version: int | None = None) -> bytes:
    """Classic header + payload, no CRC bytes, matching write_mh_bin."""
    encrypted = mh_bin.encrypt_bin_payload(payload, type_)
    blob_size = 12 + len(encrypted)
    if version is None:
        version = 20040308 + type_ + len(payload)
    return struct.pack("<III", version, type_, len(payload)) + encrypted


class ProfileSelectedBinTests(unittest.TestCase):
    def test_default_parses_r17_shape(self):
        payload = bytes(range(24))
        data = synthetic_r17(payload)
        self.assertGreaterEqual(len(data), 20)
        self.assertTrue(mh_bin.installed_client_container_shape(data))
        parsed = mh_bin.read_mh_bin_bytes(data, "r17.bin")
        self.assertEqual(parsed.type, 0)
        self.assertEqual(parsed.file_size, len(payload))
        self.assertEqual(parsed.data, mh_bin.decrypt_bin_payload(payload, 0))

    def test_sworking_profile_parses_r17_shape(self):
        payload = b"sworking-r17-payload"
        data = synthetic_r17(payload)
        parsed = mh_bin.read_mh_bin_bytes(
            data, "r17.bin", profile="sworking-2008-reference"
        )
        default = mh_bin.read_mh_bin_bytes(data, "r17.bin")
        self.assertEqual(parsed, default)
        self.assertEqual(parsed.data, mh_bin.decrypt_bin_payload(payload, 0))

    def test_playdh_current_refuses_same_r17_bytes(self):
        payload = bytes(range(24))
        data = synthetic_r17(payload)
        with self.assertRaises(mh_bin.ObfuscatedBinContainerError) as caught:
            mh_bin.read_mh_bin_bytes(data, "synthetic.bin", profile="playdh-current")
        message = str(caught.exception)
        self.assertIn("obfuscated container", message)
        self.assertIn("synthetic.bin", message)
        self.assertIn(f"bytes={len(data)}", message)

    def test_playdh_current_refuses_exactly_20_bytes(self):
        data = synthetic_r17(b"0123456789abcdef")
        self.assertEqual(len(data), 20)
        with self.assertRaisesRegex(
            mh_bin.ObfuscatedBinContainerError, "obfuscated container"
        ):
            mh_bin.read_mh_bin_bytes(data, "edge.bin", profile="playdh-current")

    def test_short_r17_stays_on_default_under_playdh_current(self):
        payload = bytes(range(15))
        data = synthetic_r17(payload)
        self.assertEqual(len(data), 19)
        self.assertFalse(mh_bin.installed_client_container_shape(data))
        for profile in (None, "playdh-current", "sworking-2008-reference"):
            parsed = mh_bin.read_mh_bin_bytes(data, "tiny.bin", profile=profile)
            self.assertEqual(parsed.type, 0)
            self.assertEqual(parsed.data, mh_bin.decrypt_bin_payload(payload, 0))

    def test_plain_classic_ok_under_both_profiles(self):
        payload = b"Hello Moxian"
        data = plain_sworking_bin(payload, type_=7)
        self.assertFalse(mh_bin.installed_client_container_shape(data))
        for profile in (None, "playdh-current", "sworking-2008-reference"):
            parsed = mh_bin.read_mh_bin_bytes(data, "plain.bin", profile=profile)
            self.assertEqual(parsed.type, 7)
            self.assertEqual(parsed.file_size, len(payload))
            self.assertEqual(parsed.data, payload)

    def test_version_equals_size_stays_on_r17_by_default(self):
        payload = b"plain-size-match"
        type_ = 4
        encrypted = mh_bin.encrypt_bin_payload(payload, type_)
        total = 14 + len(payload)
        data = (
            struct.pack("<III", total, type_, len(payload))
            + bytes([0x11])
            + encrypted
            + bytes([0x22])
        )
        self.assertGreaterEqual(len(data), 20)
        self.assertEqual(struct.unpack_from("<I", data, 0)[0], len(data))
        parsed = mh_bin.read_mh_bin_bytes(data, "coincidence.bin")
        self.assertEqual(parsed.type, 0)
        self.assertEqual(parsed.file_size, len(data) - 4)
        self.assertEqual(parsed.data, mh_bin.decrypt_bin_payload(data[4:], 0))
        reference = mh_bin.read_mh_bin_bytes(
            data, "coincidence.bin", profile="sworking-2008-reference"
        )
        self.assertEqual(reference, parsed)
        with self.assertRaisesRegex(
            mh_bin.ObfuscatedBinContainerError, "obfuscated container"
        ):
            mh_bin.read_mh_bin_bytes(
                data, "coincidence.bin", profile="playdh-current"
            )

    def test_unknown_profile_is_rejected(self):
        data = plain_sworking_bin(b"x", type_=1)
        with self.assertRaises(mh_bin.UnknownResourceProfileError):
            mh_bin.read_mh_bin_bytes(data, "plain.bin", profile="installed")

    def test_cli_default_reads_r17_shape(self):
        data = synthetic_r17(b"cli-r17-body-bytes!!")
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "synthetic.bin"
            path.write_bytes(data)
            result = subprocess.run(
                [sys.executable, "-B", str(_TOOLS_DIR / "mh_bin.py"), str(path)],
                capture_output=True,
                text=True,
                check=False,
            )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("type=0", result.stdout)
        self.assertNotIn("obfuscated container", result.stderr)

    def test_cli_playdh_current_refuses_r17_shape(self):
        data = synthetic_r17(b"cli-r17-body-bytes!!")
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "synthetic.bin"
            path.write_bytes(data)
            result = subprocess.run(
                [
                    sys.executable,
                    "-B",
                    str(_TOOLS_DIR / "mh_bin.py"),
                    "--profile",
                    "playdh-current",
                    str(path),
                ],
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
                [
                    sys.executable,
                    "-B",
                    str(_TOOLS_DIR / "mh_bin.py"),
                    "--profile",
                    "sworking-2008-reference",
                    str(path),
                ],
                capture_output=True,
                text=True,
                check=False,
            )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("type=1", result.stdout)
        self.assertNotIn("obfuscated container", result.stderr)


if __name__ == "__main__":
    unittest.main()
