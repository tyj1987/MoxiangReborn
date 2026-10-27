"""SWorking .bin reader.

Default reads follow the mainline C++ reader, including the R-17 layout
``[uint32 file_size == total size][payload]`` with positional type 0.
That path is not refused.

The installed client container uses the same first field. It is refused
only when the caller passes profile ``playdh-current`` (function argument
or ``--profile playdh-current``). The refusal does not strip the header
or decode the body. ``sworking-2008-reference`` matches the default.
"""

from __future__ import annotations

import argparse
import struct
import sys
from dataclasses import dataclass
from pathlib import Path


MAX_FILE_SIZE = 256 * 1024 * 1024
PLAYDH_CURRENT = "playdh-current"
SWORKING_REFERENCE = "sworking-2008-reference"
KNOWN_PROFILES = (PLAYDH_CURRENT, SWORKING_REFERENCE)


class ObfuscatedBinContainerError(ValueError):
    """Installed-profile refusal. The message contains ``obfuscated container``."""


class UnknownResourceProfileError(ValueError):
    """``profile`` is not a known resource profile id."""


@dataclass
class MhBin:
    version: int
    type: int
    file_size: int
    data: bytes


def _obfuscated_message(name: str, size: int) -> str:
    shown = Path(name).name if name else "(unnamed)"
    if not shown:
        shown = "(unnamed)"
    return f"ERROR [bin] obfuscated container: {shown} (bytes={size})"


def installed_client_container_shape(data: bytes) -> bool:
    """True when ``playdh-current`` must refuse the buffer.

    The check is ``len >= 20`` and the first little-endian uint32 equals
    the length. It is not used by the default reader. R-17 uses the same
    first field and is still decoded there.
    """
    if len(data) < 20 or len(data) > 0xFFFFFFFF:
        return False
    (declared,) = struct.unpack_from("<I", data, 0)
    return declared == len(data)


def decrypt_bin_payload(encrypted: bytes, type_: int) -> bytes:
    """Classic MHFileEx positional transform. Not a container unwrap."""
    out = bytearray(encrypted)
    for index, value in enumerate(out):
        decoded = (value - (index & 0xFF)) & 0xFF
        if type_ != 0 and index % type_ == 0:
            decoded = (decoded - type_) & 0xFF
        out[index] = decoded
    return bytes(out)


def encrypt_bin_payload(raw: bytes, type_: int) -> bytes:
    """Inverse of ``decrypt_bin_payload`` for plain SWorking fixtures."""
    out = bytearray(raw)
    for index, value in enumerate(out):
        encoded = (value + (index & 0xFF)) & 0xFF
        if type_ != 0 and index % type_ == 0:
            encoded = (encoded + type_) & 0xFF
        out[index] = encoded
    return bytes(out)


def read_mh_bin_bytes(
    data: bytes,
    name: str = "(unnamed)",
    profile: str | None = None,
) -> MhBin:
    """Read one .bin buffer.

    ``profile`` None or ``sworking-2008-reference`` uses mainline semantics,
    including R-17 for any buffer of at least 5 bytes whose first uint32
    equals the length. ``playdh-current`` refuses that shape at 20 bytes
    or more and does not decode it. Any other profile raises
    ``UnknownResourceProfileError``.
    """
    if profile not in (None, "", SWORKING_REFERENCE, PLAYDH_CURRENT):
        raise UnknownResourceProfileError(f"unknown resource profile: {profile}")
    if profile == PLAYDH_CURRENT and installed_client_container_shape(data):
        raise ObfuscatedBinContainerError(_obfuscated_message(name, len(data)))

    if len(data) >= 5:
        (declared,) = struct.unpack_from("<I", data, 0)
        if declared == len(data):
            payload = data[4:]
            return MhBin(
                version=0x00000001,
                type=0,
                file_size=len(payload),
                data=decrypt_bin_payload(payload, 0),
            )

    if len(data) < 14:
        raise ValueError(f"invalid header: {name} ({len(data)} bytes)")

    version, type_, file_size = struct.unpack_from("<III", data, 0)
    if file_size > MAX_FILE_SIZE:
        raise ValueError(f"invalid header: {name}")

    if len(data) < 14 + file_size:
        if len(data) >= 12 + file_size:
            payload = data[12:12 + file_size]
            return MhBin(version, type_, file_size, decrypt_bin_payload(payload, type_))
        raise ValueError(f"invalid header: {name}")

    payload = data[13:13 + file_size]
    return MhBin(version, type_, file_size, decrypt_bin_payload(payload, type_))


def read_mh_bin(path: Path | str, profile: str | None = None) -> MhBin:
    """Read one .bin file from disk. See ``read_mh_bin_bytes``."""
    file_path = Path(path)
    return read_mh_bin_bytes(file_path.read_bytes(), file_path.name, profile)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Read one SWorking .bin file.")
    parser.add_argument("path", type=Path)
    parser.add_argument(
        "--profile",
        default=None,
        help="Resource profile. Default and sworking-2008-reference keep R-17. "
        "playdh-current refuses the installed container.",
    )
    args = parser.parse_args(argv)
    try:
        parsed = read_mh_bin(args.path, args.profile)
    except ObfuscatedBinContainerError as exc:
        print(str(exc), file=sys.stderr)
        return 1
    except (OSError, ValueError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    print(
        f"version={parsed.version} type={parsed.type} file_size={parsed.file_size}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
