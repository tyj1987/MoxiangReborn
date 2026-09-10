"""Exercise the real native exporter with tiny independently encoded HFLs."""
import hashlib
import json
import struct
import subprocess
import sys
import tempfile
from pathlib import Path


def hfl(heights):
    descriptor = bytearray(108)
    struct.pack_into('<f', descriptor, 16, 100.0)
    struct.pack_into('<I', descriptor, 44, 1)
    struct.pack_into('<IIff', descriptor, 52, 2, 2, 100.0, 100.0)
    struct.pack_into('<I', descriptor, 88, 1)
    struct.pack_into('<II', descriptor, 96, 1, 1)
    texture = struct.pack('<H', 0) + b'ground.dds'.ljust(128, b'\0')
    return struct.pack('<I', 1) + descriptor + struct.pack('<4f', *heights) + struct.pack('<I', 1) + texture + struct.pack('<IIH', 1, 1, 0)


def main():
    binary = str(Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix='mxh-export-') as folder:
        root = Path(folder)
        source = root / 'tiny.hfl'
        payload = hfl([0.0, 2.0, 4.0, 6.0])
        source.write_bytes(payload)
        digest = hashlib.sha256(payload).hexdigest()
        def run(output, checksum=digest):
            return subprocess.run([binary, str(source), str(output), checksum, 'mxh:fixture'], capture_output=True, text=True)
        a, b = root / 'a' / 'tiny.mxhasset', root / 'b' / 'tiny.mxhasset'
        assert run(a).returncode == 0
        assert run(b).returncode == 0
        assert a.read_bytes() == b.read_bytes(), 'Conversion must be deterministic'
        doc = json.loads(a.read_text())
        heights = a.with_suffix('.mxhheight').read_bytes()
        assert struct.unpack('<4f', heights) == (0.0, 2.0, 4.0, 6.0)
        assert hashlib.sha256(heights).hexdigest() == doc['heightSha256']
        assert doc['releaseReady'] is False and doc['sourceSha256'] == digest
        assert run(a).returncode != 0, 'Existing derived file must not be overwritten'
        invalid = root / 'bad.mxhasset'
        assert run(invalid, '0' * 64).returncode != 0 and not invalid.exists()
        source.write_bytes(hfl([0.0, float('nan'), 4.0, 6.0]))
        assert run(invalid, hashlib.sha256(source.read_bytes()).hexdigest()).returncode != 0
        assert not invalid.exists() and not invalid.with_suffix('.mxhheight').exists()
        source.write_bytes(payload[:120])
        assert run(invalid, hashlib.sha256(source.read_bytes()).hexdigest()).returncode != 0
    print('Native export contract: deterministic, hash-bound, no overwrite, finite and complete geometry PASS')


if __name__ == '__main__':
    main()
