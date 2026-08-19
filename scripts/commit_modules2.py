#!/usr/bin/env python3
"""Commit the remaining 4 untracked modules (hsel_class, hackshield_stub, legacy_wire, battle)."""
import subprocess
import os
from pathlib import Path

os.chdir(r'C:\moxiang')

# Each tuple: (module_name, hpp_path, cpp_path, test_path)
MODULES = [
    ('crypto', 'hsel_class',
     'modern/include/mxh/crypto/hsel_class.hpp',
     'modern/src/crypto/hsel_class.cpp',
     None),  # No new test file (existing crypto_test.cpp covers it)
    ('crypto', 'hackshield_stub',
     'modern/include/mxh/crypto/hackshield_stub.hpp',
     'modern/src/crypto/hackshield_stub.cpp',
     None),
    ('proto', 'legacy_wire',
     'modern/include/mxh/proto/legacy_wire.hpp',
     None,
     'modern/tests/unit/proto/legacy_wire_test.cpp'),
    ('proto', 'protocol_byte_level',
     None,
     None,
     'modern/tests/unit/proto/protocol_byte_level_test.cpp'),
    ('game', 'battle',
     'modern/include/mxh/game/battle.hpp',
     'modern/src/game/battle.cpp',
     'modern/tests/unit/game/battle_runtime_test.cpp'),
]

def run(cmd, check=True):
    r = subprocess.run(cmd, capture_output=True, text=True)
    if check and r.returncode != 0:
        print('FAIL:', ' '.join(cmd[:5]), 'stderr:', r.stderr.strip()[:200])
        return False
    return r

def get_test_count(test_path):
    p = Path(test_path)
    if not p.exists():
        return 0
    text = p.read_text(encoding='utf-8', errors='replace')
    return text.count('TEST(')

count = 0
for ns, name, hpp, cpp, test in MODULES:
    files = []
    if hpp and Path(hpp).exists():
        files.append(hpp)
    if cpp and Path(cpp).exists():
        files.append(cpp)
    if test and Path(test).exists():
        files.append(test)
    if not files:
        print('SKIP', name, ': no files')
        continue
    n = get_test_count(test) if test else 0
    msg = ns + ': ' + name
    if hpp and cpp and test:
        msg += ' 1:1 port + ' + str(n) + ' tests'
    elif hpp and cpp:
        msg += ' ABI preservation header'
    elif test:
        msg += ' test + ' + str(n) + ' cases'
    run(['git', 'add'] + files)
    r = run(['git', 'commit', '-m', msg], check=False)
    if r.returncode == 0:
        print('COMMIT', name, '(' + str(len(files)) + ' files)')
        count += 1
    else:
        print('FAIL', name, ':', r.stderr.strip()[:200])

print('\nTotal:', count, 'commits')
