"""SWorking .bin reader.

Installed Resource .bin files can use the same length-prefixed container
family as obfuscated .map entries. ``read_mh_bin`` detects that container
and refuses it. The header is not stripped and the body is not decoded.
No key is stored or derived.

Detection rule (same as ``mxh::compat::is_obfuscated_bin_container``):

* the buffer is at least 20 bytes and fits in a uint32
* the first little-endian uint32 equals the buffer size
* the buffer is not a plausible plain SWorking .bin

A plain SWorking file is plausible when the classic header's ``file_size``
fits, either as header + crc + payload + crc or as header + payload. A
plain file whose version field happens to equal the file size stays on
that path, including a file shorter than 20 bytes. Buffers of 5..19 bytes
are too small for the container. The historical positional type-0 read
remains only when such a buffer is not a plausible plain SWorking .bin.
"""

from __future__ import annotations

import argparse
import struct
import sys
from dataclasses import dataclass
from pathlib import Path


MAX_FILE_SIZE = 256 * 1024 * 1024
CONTAINER_HEADER_BYTES = 20


class ObfuscatedBinContainerError(ValueError):
    """The bytes are an installed Resource .bin obfuscated container."""


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


def plausible_plain_sworking_bin(data: bytes) -> bool:
    """True when the classic MHFileEx header's payload length fits.

    This is the same gate the C++ reader uses. It does not transform bytes.
    """
    if len(data) < 12:
        return False
    _version, _type, file_size = struct.unpack_from("<III", data, 0)
    if file_size > MAX_FILE_SIZE:
        return False
    return len(data) >= 14 + file_size or len(data) >= 12 + file_size


def is_obfuscated_bin_container(data: bytes) -> bool:
    """True for an installed Resource .bin obfuscated container.

    Shape check only. See the module docstring for the rule.
    """
    if len(data) < CONTAINER_HEADER_BYTES or len(data) > 0xFFFFFFFF:
        return False
    (declared,) = struct.unpack_from("<I", data, 0)
    if declared != len(data):
        return False
    return not plausible_plain_sworking_bin(data)


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


def read_mh_bin_bytes(data: bytes, name: str = "(unnamed)") -> MhBin:
    """Read one .bin buffer.

    Raises ``ObfuscatedBinContainerError`` (a ``ValueError``) whose message
    contains ``obfuscated container`` when the buffer is that container.
    """
    if is_obfuscated_bin_container(data):
        raise ObfuscatedBinContainerError(_obfuscated_message(name, len(data)))

    if 5 <= len(data) < CONTAINER_HEADER_BYTES:
        (declared,) = struct.unpack_from("<I", data, 0)
        if declared == len(data) and not plausible_plain_sworking_bin(data):
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


def read_mh_bin(path: Path | str) -> MhBin:
    """Read one .bin file from disk. See ``read_mh_bin_bytes``."""
    file_path = Path(path)
    return read_mh_bin_bytes(file_path.read_bytes(), file_path.name)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Read one SWorking .bin file.")
    parser.add_argument("path", type=Path)
    args = parser.parse_args(argv)
    try:
        parsed = read_mh_bin(args.path)
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
