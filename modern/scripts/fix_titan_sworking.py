#!/usr/bin/env python3
"""Fix TitanServer.bin in SWorking to use UTF-8 sentinel."""
import struct
import sys
from pathlib import Path

SENTINEL_UTF8 = (
    b'\x22'
    b'\xec\x9d\xb4'
    b'\x20'
    b'\xed\x8c\x8c'
    b'\xec\x9d\xbc'
    b'\xec\x9d\xb4'
    b'\x20'
    b'\xec\x97\x86'
    b'\xec\x9c\xbc'
    b'\xeb\xa9\xb4'
    b'\x20'
    b'\xed\x83\x80'
    b'\xec\x9d\xb4'
    b'\xed\x83\x84'
    b'\x20'
    b'\xec\x97\x85'
    b'\xeb\x8d\xb0'
    b'\xec\x9d\xb4'
    b'\xed\x8a\xb8'
    b'\x20'
    b'\xec\x95\x88'
    b'\xeb\x8f\xbc'
    b'\xec\x9a\x94'
    b'\x7e'
    b'\x22'
)
assert len(SENTINEL_UTF8) == 59

def encode_payload(raw, dw_type):
    encoded = bytearray(len(raw))
    crc = dw_type & 0xFF
    for i, byte in enumerate(raw):
        v = byte + (i & 0xFF)
        if i % dw_type == 0:
            v += dw_type
        encoded[i] = v & 0xFF
        crc = (crc + encoded[i]) & 0xFF
    return bytes(encoded), crc

def build_bin(raw, dw_type):
    encoded, crc = encode_payload(raw, dw_type)
    dw_version = 20040308 + dw_type + len(raw)
    return (
        struct.pack('<III', dw_version, dw_type, len(raw))
        + struct.pack('B', crc)
        + encoded
        + struct.pack('B', crc)
    )

def decode_payload(encoded, dw_type):
    decoded = bytearray(len(encoded))
    for i, byte in enumerate(encoded):
        v = byte - (i & 0xFF)
        if i % dw_type == 0:
            v -= dw_type
        decoded[i] = v & 0xFF
    return bytes(decoded)

target = Path(r"d:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\SWorking\Resource\Server\TitanServer.bin")
DW_TYPE = 7

# Backup
backup = target.with_suffix('.old_euc_kr')
if target.exists() and not backup.exists():
    backup.write_bytes(target.read_bytes())
    print(f"Backed up to {backup}")

# Build
blob = build_bin(SENTINEL_UTF8, DW_TYPE)
target.write_bytes(blob)
print(f"Wrote {len(blob)} bytes (sentinel={len(SENTINEL_UTF8)} bytes, dwType={DW_TYPE})")

# Verify
header_bytes = struct.unpack_from('<III', blob, 0)
crc1 = blob[12]
payload = blob[13:13 + header_bytes[2]]
crc2 = blob[13 + header_bytes[2]]
decoded = decode_payload(payload, DW_TYPE)
print(f"Header: dwVersion={header_bytes[0]}, dwType={header_bytes[1]}, dwFileSize={header_bytes[2]}")
print(f"CRC: crc1=0x{crc1:02x}, crc2=0x{crc2:02x}, match={crc1==crc2}")
print(f"Decoded match sentinel: {decoded == SENTINEL_UTF8}")
print(f"Decoded hex: {decoded.hex()}")
print("SUCCESS" if decoded == SENTINEL_UTF8 else "FAILED")
