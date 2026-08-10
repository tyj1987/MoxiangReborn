#!/usr/bin/env python3
"""Regression test for audit_resource_coverage.py plaintext detection.

Mirrors the is_plaintext_ini() logic in modern/tools/audit_resource_coverage.py
so we can pin the contract without spinning up the full audit script.  If
this test breaks, the audit gate will start mis-flagging legacy plaintext
.ini-style .bin files (e.g. Ini/GameDesc.bin = *DISPWIDTH 1024 ...) as
"fail" and the T1 resource coverage will drop below 100%.

Run:
    python modern/tests/unit/compat/audit_resource_coverage_plaintext_test.py
"""
import os
import sys
import tempfile
import unittest


# Mirror of modern/tools/audit_resource_coverage.py:is_plaintext_ini.
# Keep the two in sync.
def is_plaintext_ini(path, sniff_bytes=256):
    try:
        with open(path, "rb") as f:
            head = f.read(sniff_bytes)
    except OSError:
        return False
    if not head:
        return False
    if b"\x00" in head:
        return False  # NUL => binary
    printable = set(range(0x09, 0x0D)) | set(range(0x20, 0x7F))
    for b in head:
        if b not in printable:
            return False
    return True


class PlaintextDetectionTest(unittest.TestCase):

    def _write(self, suffix, content):
        fd, name = tempfile.mkstemp(suffix=suffix)
        with os.fdopen(fd, "wb") as f:
            if isinstance(content, str):
                f.write(content.encode("utf-8"))
            else:
                f.write(content)
        self.addCleanup(os.unlink, name)
        return name

    def test_legacy_ini_bin_is_plaintext(self):
        # Ini/GameDesc.bin in real PlayDH is exactly this content.
        path = self._write(
            ".bin",
            "*DISPWIDTH 1024\n*DISPHEIGHT 768\n*BPS 32\n*WINDOWTITLE \"Sky Online\"\n",
        )
        self.assertTrue(is_plaintext_ini(path))

    def test_real_resource_bin_is_not_plaintext(self):
        # A real binary .bin resource has NUL bytes (e.g. length-prefixed
        # strings, packed records).
        path = self._write(
            ".bin",
            b"\x00\x01\x02\x03\x04\x00some string\x00\x00\x00\xff",
        )
        self.assertFalse(is_plaintext_ini(path))

    def test_empty_file_is_not_plaintext(self):
        path = self._write(".bin", b"")
        self.assertFalse(is_plaintext_ini(path))

    def test_high_bit_byte_is_not_plaintext(self):
        # EUC-KR / Shift-JIS bytes have the high bit set (>= 0x80).
        # Legacy Korean client uses EUC-KR for some string tables.
        path = self._write(".bin", b"\xb0\xa1\xb0\xa2\xb0\xa3")
        self.assertFalse(is_plaintext_ini(path))

    def test_missing_path_is_not_plaintext(self):
        self.assertFalse(is_plaintext_ini(r"C:\does\not\exist.bin"))


if __name__ == "__main__":
    # Wire into ctest if launched from the test runner; standalone
    # python invocation also works for development.
    sys.exit(unittest.main(verbosity=2).exitcode)
