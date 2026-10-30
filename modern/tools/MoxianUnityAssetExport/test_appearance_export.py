import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

exe, root = map(Path, sys.argv[1:])
with tempfile.TemporaryDirectory() as temp:
    folder = Path(temp)
    output = folder / 'catalog.mxhappearance'
    subprocess.run([exe, root, output], check=True, capture_output=True)
    raw = output.read_bytes()
    catalog = json.loads(raw)
    assert catalog['schemaVersion'] == 1 and catalog['releaseReady'] is False
    assert len(catalog['sources']) == 7
    for source in catalog['sources']:
        original = (root / source['path']).read_bytes()
        assert source['bytes'] == len(original)
        assert source['sha256'] == hashlib.sha256(original).hexdigest()
    assert [entry['gender'] for entry in catalog['genders']] == [0, 1]
    assert [name.lower() for name in catalog['genders'][0]['faces']] == [f'm_face{i:02}.mod' for i in range(1, 6)]
    for gender in catalog['genders']:
        assert gender['baseObject'] and gender['models'] and gender['hairs']
    assert len(catalog['items']) == 9887
    assert len({item['itemId'] for item in catalog['items']}) == 9887
    second = folder / 'second.mxhappearance'
    subprocess.run([exe, root, second], check=True, capture_output=True)
    assert second.read_bytes() == raw
    assert subprocess.run([exe, root, output], capture_output=True).returncode != 0
    assert output.read_bytes() == raw
    assert subprocess.run([exe, folder / 'missing', folder / 'bad.mxhappearance'], capture_output=True).returncode != 0
    assert not (folder / 'bad.mxhappearance').exists()
