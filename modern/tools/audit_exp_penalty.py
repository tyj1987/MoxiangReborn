"""Recover the pinned PlayDH ExpPenalty candidate; never deploy gameplay data."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

EXPECTED_SHA256 = '7abcc8211e0bffa58ce7658883c79bf040e748a42568701b773e4ed3f0809419'
REFERENCE_SHA256 = 'f39c4d312788fbb66f7eecf798e33bb82a9e2f5a647b31e300cf2db19d396a19'


def recover(raw):
    if hashlib.sha256(raw).hexdigest() != EXPECTED_SHA256:
        raise ValueError('Unreviewed resource digest')
    if len(raw) < 25 or struct.unpack_from('<I', raw)[0] != len(raw):
        raise ValueError('Invalid size-prefixed container')
    body = raw[24:]
    alphabet = set(b'0123456789. \t\r\n')
    key = []
    for lane in range(8):
        candidates = [value for value in range(256)
                      if all((byte ^ value) in alphabet for byte in body[lane::8])]
        if len(candidates) != 1:
            raise ValueError('Non-unique numeric-table transform')
        key.append(candidates[0])
    decoded = bytes(byte ^ key[index % 8] for index, byte in enumerate(body))
    rows = []
    for line in decoded.decode('ascii').splitlines():
        if not line.strip():
            continue
        columns = line.split()
        if len(columns) != 3:
            raise ValueError('Expected level, fNow, fSave')
        level = int(columns[0])
        now, save = map(float, columns[1:])
        if level != len(rows) + 1 or not 0 <= save <= now <= 100:
            raise ValueError('Invalid level sequence or percentage')
        rows.append(dict(level=level, presentPercent=now, loginPercent=save))
    if len(rows) != 99:
        raise ValueError('Incomplete pinned table')
    return dict(sourceSha256=EXPECTED_SHA256, status='recovered-candidate-not-runtime-approved',
                transform='payload offset 24, unique numeric-alphabet XOR8',
                decodedSha256=hashlib.sha256(decoded).hexdigest(), rows=rows)


def verify_reference(report, raw):
    if hashlib.sha256(raw).hexdigest() != REFERENCE_SHA256:
        raise ValueError('Unreviewed reference digest')
    _, kind, size = struct.unpack_from('<III', raw)
    if not kind or size + 14 != len(raw):
        raise ValueError('Invalid reference MHFile header')
    encrypted = raw[13:-1]
    crc = (kind + sum(encrypted)) & 255
    if raw[12] != crc or raw[-1] != crc:
        raise ValueError('Reference CRC mismatch')
    decoded = bytes((value - index - (kind if index % kind == 0 else 0)) & 255
                    for index, value in enumerate(encrypted))
    if hashlib.sha256(decoded).hexdigest() != report['decodedSha256']:
        raise ValueError('Current and reference plaintext differ')
    report.update(status='plaintext-verified-against-legacy-resource',
                  referenceSha256=REFERENCE_SHA256, referenceCrc=crc)
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--reference', type=Path)
    args = parser.parse_args()
    report = recover(args.source.read_bytes())
    if args.reference:
        verify_reference(report, args.reference.read_bytes())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({key: value for key, value in report.items() if key != 'rows'}))
