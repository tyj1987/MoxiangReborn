#!/usr/bin/env python3
"""Exact-byte, no-fuzz patch for an explicitly supplied isolated Unity copy."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import tempfile

HERE = Path(__file__).resolve().parent

def sha(data):
    return hashlib.sha256(data).hexdigest()

def patched_files(originals, patch):
    lines = patch.splitlines(keepends=True)
    result, i = {}, 0
    while i < len(lines):
        if not lines[i].startswith('--- a/') or i + 1 >= len(lines):
            raise ValueError('Unexpected patch header')
        path = lines[i][6:].rstrip('\n')
        if lines[i + 1] != '+++ b/' + path + '\n' or path not in originals or path in result:
            raise ValueError('Unknown or duplicate patch target')
        source = originals[path].decode('utf-8').splitlines(keepends=True)
        output, cursor = [], 0
        i += 2
        while i < len(lines) and not lines[i].startswith('--- '):
            match = re.fullmatch(r'@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@\n', lines[i])
            if not match:
                raise ValueError('Invalid hunk')
            start, old_count, new_start, new_count = [int(v) if v is not None else 1 for v in match.groups()]
            start -= 1
            if start < cursor or start > len(source):
                raise ValueError('Hunk position mismatch')
            output.extend(source[cursor:start])
            cursor, consumed, produced = start, 0, 0
            if len(output) != new_start - 1:
                raise ValueError('Output position mismatch')
            i += 1
            while i < len(lines) and lines[i][:1] in (' ', '+', '-') and not lines[i].startswith('--- '):
                kind, line = lines[i][0], lines[i][1:]
                if kind in (' ', '-'):
                    if cursor >= len(source) or source[cursor] != line:
                        raise ValueError('Exact context mismatch')
                    cursor += 1
                    consumed += 1
                if kind in (' ', '+'):
                    output.append(line)
                    produced += 1
                i += 1
            if consumed != old_count or produced != new_count:
                raise ValueError('Hunk length mismatch')
        output.extend(source[cursor:])
        result[path] = ''.join(output).encode('utf-8')
    if set(result) != set(originals):
        raise ValueError('Patch does not cover exactly the manifest')
    return result

def atomic_write(path, data):
    fd, temporary = tempfile.mkstemp(prefix='.effekseer-', dir=path.parent)
    try:
        with os.fdopen(fd, 'wb') as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)

def inspect(project):
    project = Path(project).absolute()
    if project.resolve() != project or not (project / 'ProjectSettings/ProjectVersion.txt').is_file():
        raise ValueError('Expected a real isolated Unity project, not a linked path')
    # This utility must never target this repository's canonical Unity tree.
    canonical = HERE.parents[2] / 'unity/MoxiangClient'
    if project == canonical.resolve():
        raise ValueError('Canonical game project refused; use an isolated copy')
    manifest = json.loads((HERE / 'manifest.json').read_text())
    originals = {}
    states = []
    for relative, hashes in manifest['files'].items():
        path = project / relative
        if path.resolve() != path or not path.is_file():
            raise ValueError('Missing or linked vendor file: ' + relative)
        data = path.read_bytes()
        digest = sha(data)
        if digest == hashes['original_sha256']:
            states.append('original')
        elif digest == hashes['patched_sha256']:
            states.append('patched')
        else:
            raise ValueError('Unrecognized exact SHA256: ' + relative + ' ' + digest)
        originals[relative] = data
    if len(set(states)) != 1:
        raise ValueError('Mixed original/patched state; inspect and restore the saved pair')
    return project, manifest, originals, states[0]

def apply(project, backup, write=False):
    project, manifest, originals, state = inspect(project)
    if state == 'patched':
        return 'already patched (exact SHA256); no writes'
    output = patched_files(originals, (HERE / 'lifecycle.patch').read_text(encoding='utf-8'))
    for relative, data in output.items():
        if sha(data) != manifest['files'][relative]['patched_sha256']:
            raise ValueError('Patched output SHA256 mismatch: ' + relative)
    if not write:
        return 'verified originals and patch outputs; dry run only'
    if backup is None:
        raise ValueError('--apply requires --backup-dir')
    backup = Path(backup).absolute()
    if backup.resolve() != backup or backup.exists() or backup == project or project in backup.parents:
        raise ValueError('Backup must be a new, unlinked directory outside the Unity project')
    backup.mkdir(parents=True, exist_ok=False)
    for relative, data in originals.items():
        target = backup / Path(relative).name
        target.write_bytes(data)
        if target.read_bytes() != data:
            raise OSError('Backup verification failed')
    (backup / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    written = []
    try:
        for relative, data in output.items():
            path = project / relative
            if path.read_bytes() != originals[relative]:
                raise ValueError('Vendor file changed during apply; stop the isolated Editor first')
            atomic_write(path, data)
            written.append(relative)
        for relative, data in output.items():
            if (project / relative).read_bytes() != data:
                raise OSError('Post-write verification failed')
    except Exception:
        for relative in written:
            atomic_write(project / relative, originals[relative])
        raise
    return 'applied exact patch; originals retained at ' + str(backup)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--isolated-project', required=True)
    parser.add_argument('--backup-dir')
    parser.add_argument('--apply', action='store_true')
    args = parser.parse_args()
    try:
        print(apply(args.isolated_project, args.backup_dir, args.apply))
    except (OSError, ValueError) as error:
        parser.exit(2, 'REFUSED: ' + str(error) + '\n')

if __name__ == '__main__':
    main()
