"""Hash-pinned real MOD contract; source pack remains read-only."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

binary, pack = Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve()
digest = 'b4e235f83554531f0a54ceff553807f77fc649d91afd6e86fa864a8cea43326e'
with pack.open('rb') as stream:
    stream.seek(92601570)
    payload = stream.read(91691)
assert hashlib.sha256(payload).hexdigest() == digest, 'Pinned body source changed'
with tempfile.TemporaryDirectory(prefix='mxh-model-export-') as folder:
    root = Path(folder)
    source = root / 'body.mod'
    source.write_bytes(payload)
    def run(output, checksum=digest):
        return subprocess.run([str(binary), str(source), str(output), checksum, 'mxh:body-fixture'], capture_output=True)
    first, second = root / 'a.mxhmodel', root / 'b.mxhmodel'
    assert run(first).returncode == 0
    assert run(second).returncode == 0
    assert first.read_bytes() == second.read_bytes(), 'Non-deterministic export'
    doc = json.loads(first.read_text())
    assert doc['sourceSha256'] == digest and doc['releaseReady'] is False
    assert doc['coordinateSpace'] == 'legacy-unconverted'
    assert len(doc['bones']) == 96 and len(doc['meshes']) == 1
    mesh = doc['meshes'][0]
    assert len(mesh['positions']) == len(mesh['normals']) == len(mesh['texcoords']) == len(mesh['physique']) == 355
    assert sum(len(vertex['influences']) for vertex in mesh['physique']) == 365
    bone_ids = {bone['index'] for bone in doc['bones']}
    assert len(bone_ids) == 96
    for bone in doc['bones']:
        assert len(bone['transform']) == 16
        assert bone['parentIndex'] == 0xffffffff or bone['parentIndex'] in bone_ids
    for vertex in mesh['physique']:
        assert all(influence['boneIndex'] in bone_ids for influence in vertex['influences'])
        assert abs(sum(influence['weight'] for influence in vertex['influences']) - 1) < 0.001
    assert {material['textureName'] for material in doc['materials']} == {'m_nude.tga'}
    assert run(first).returncode != 0 and first.read_bytes() == second.read_bytes()
    bad = root / 'bad.mxhmodel'
    assert run(bad, '0' * 64).returncode != 0 and not bad.exists()
    source.write_bytes(payload[:28])
    assert run(bad, hashlib.sha256(source.read_bytes()).hexdigest()).returncode != 0 and not bad.exists()
print('MOD contract PASS: deterministic geometry/skin/bones, source hash, no overwrite, malformed rejection')
