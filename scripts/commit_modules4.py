#!/usr/bin/env python3
"""Commit the final 17 modified files + 1 untracked script."""
import subprocess
import os
from pathlib import Path

os.chdir(r'C:\moxiang')

def run(cmd, check=True):
    r = subprocess.run(cmd, capture_output=True, text=True)
    if check and r.returncode != 0:
        print('FAIL:', ' '.join(cmd[:6]), 'stderr:', r.stderr.strip()[:200])
        return False
    return r

# Define each commit (msg, files)
COMMITS = [
    ('build: register 8 CMakeLists updates (new ui + server + side-by-side + battle + hselshackshieldstub + legacywire + prototcolbyte + mssqlreal + client_e2e + attack_loop + resource_byte_level)',
     ['modern/CMakeLists.txt',
      'modern/src/CMakeLists.txt',
      'modern/src/game/CMakeLists.txt',
      'modern/src/ui/CMakeLists.txt',
      'modern/tests/unit/client/CMakeLists.txt',
      'modern/tests/unit/compat/CMakeLists.txt',
      'modern/tests/unit/db/CMakeLists.txt',
      'modern/tests/unit/game/CMakeLists.txt',
      'modern/tests/unit/proto/CMakeLists.txt',
      'modern/tests/unit/server/CMakeLists.txt',
      'modern/tests/unit/ui/CMakeLists.txt']),
    ('client: CInGameState MonsterAdd wire-format parser (legacy MAPOBJECT_TOTALINFO layout)',
     ['modern/src/client/CInGameState.hpp',
      'modern/src/client/CInGameState.cpp',
      'modern/tests/unit/client/cingame_state_test.cpp']),
    ('ui: skilloptioncleardlg minor touch-up + test extension',
     ['modern/src/ui/skilloptioncleardlg.hpp',
      'modern/tests/unit/ui/skilloptioncleardlg_test.cpp']),
    ('ui: cchatdialog test additional coverage',
     ['modern/tests/unit/ui/cchatdialog_test.cpp']),
    ('compat: mh_file_ex_utf8_test encoding assertion update',
     ['modern/tests/unit/mh_file_ex_utf8_test.cpp']),
    ('monitor: performance_monitor_test additional metric',
     ['modern/tests/unit/monitor/performance_monitor_test.cpp']),
    ('scripts: restore_databases_simple.ps1 -ComposeUp param + 3-DB restore path',
     ['scripts/restore_databases_simple.ps1']),
    ('scripts: commit_modules3.py (batch commit helper for tests + tools + scripts)',
     ['scripts/commit_modules3.py']),
]

count = 0
for msg, files in COMMITS:
    existing = [f for f in files if Path(f).exists()]
    if not existing:
        print('SKIP', msg[:60], ': no files')
        continue
    run(['git', 'add'] + existing)
    r = run(['git', 'commit', '-m', msg], check=False)
    if r.returncode == 0:
        print('COMMIT', msg[:80])
        count += 1
    else:
        print('FAIL', msg[:60], ':', r.stderr.strip()[:300])

print('\nTotal:', count, 'commits')
