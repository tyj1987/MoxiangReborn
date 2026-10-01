"""Independent ANM fixture checks time fields, scale orientation and mesh keys."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

binary = str(Path(sys.argv[1]).resolve())
header = struct.pack('<7I', 1, 160, 0, 59, 30, 1, 1).ljust(160, b'\0')
object_header = struct.pack('<5I', 7, 1, 1, 1, 1) + b'Bone'.ljust(128, b'\0') + b'\0' * 4
position = struct.pack('<II3f', 320, 2, 1, 2, 3)
rotation = struct.pack('<II4f', 480, 3, 0, 0, 0, 1)
scale = struct.pack('<II7f', 640, 4, 2, 3, 4, 0, 0, 0, 1)
mesh = struct.pack('<6I3f2f', 800, 5, 1, 1, 0, 0, 10, 20, 30, 0, 1)
body = object_header + position + rotation + scale + mesh
payload = header + struct.pack('<II', 0, len(body)) + body
with tempfile.TemporaryDirectory(prefix='mxh-motion-') as folder:
    root = Path(folder)
    source = root / 'motion.anm'
    source.write_bytes(payload)
    def run(path, digest=None):
        return subprocess.run([binary, str(source), str(path), digest or hashlib.sha256(source.read_bytes()).hexdigest(), 'mxh:motion-fixture'], capture_output=True)
    first, second = root / 'a.mxhmotion', root / 'b.mxhmotion'
    assert run(first).returncode == 0
    assert run(second).returncode == 0 and first.read_bytes() == second.read_bytes()
    doc = json.loads(first.read_text())
    assert [doc[key] for key in ('ticksPerFrame','firstFrame','lastFrame','frameSpeed','keyFrameStep')] == [160,0,59,30,1]
    assert doc['releaseReady'] is False
    track = doc['objects'][0]
    assert track['name'] == 'Bone' and track['index'] == 7
    assert track['positions'][0] == {'ticks':320,'frame':2,'value':{'x':1,'y':2,'z':3}}
    assert track['rotations'][0]['value'] == {'x':0,'y':0,'z':0,'w':1}
    assert track['scales'][0]['axis'] == {'x':0,'y':0,'z':0}
    assert track['scales'][0]['axisAngle'] == 1
    assert bytes(doc['sourceHeaderBytes']) == header
    assert track['scales'][0]['value'] == {'x':2,'y':3,'z':4}
    assert track['meshAnimationKeyCount'] == 1 and bytes(track['meshAnimationBytes']) == mesh
    assert run(first).returncode != 0
    bad = root / 'bad.mxhmotion'
    assert run(bad, '0' * 64).returncode != 0 and not bad.exists()
    source.write_bytes(payload[:-1])
    assert run(bad).returncode != 0 and not bad.exists()
    invalid = bytearray(payload)
    struct.pack_into('<f', invalid, 160 + 8 + 152 + 8, float('nan'))
    source.write_bytes(invalid)
    assert run(bad).returncode != 0 and not bad.exists()
print('ANM export PASS: timing, tracks, scale orientation, mesh data, deterministic and fail-closed')
