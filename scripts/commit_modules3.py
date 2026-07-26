#!/usr/bin/env python3
"""Commit the remaining test + tool + script files."""
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

def get_test_count(test_path):
    p = Path(test_path)
    if not p.exists():
        return 0
    text = p.read_text(encoding='utf-8', errors='replace')
    return text.count('TEST(')

# Define each commit
COMMITS = [
    # (msg, [files])
    ('crypto: hsel_class_test 1:1 ABI preservation + ' + str(get_test_count('modern/tests/unit/hsel_class_test.cpp')) + ' tests',
     ['modern/tests/unit/hsel_class_test.cpp']),
    ('crypto: hackshield_stub_test 1:1 ABI preservation + ' + str(get_test_count('modern/tests/unit/hackshield_stub_test.cpp')) + ' tests',
     ['modern/tests/unit/hackshield_stub_test.cpp']),
    ('compat: resource_byte_level_test (T1) + reference manifest + reference test (T1 baseline)',
     ['modern/tests/unit/compat/resource_byte_level_test.cpp',
      'modern/tests/unit/compat/resource_reference_manifest.json',
      'modern/tests/unit/compat/resource_reference_test.cpp']),
    ('game: attack_loop_test (E2E attack) + ' + str(get_test_count('modern/tests/unit/game/attack_loop_test.cpp')) + ' tests',
     ['modern/tests/unit/game/attack_loop_test.cpp']),
    ('db: mssql_real_e2e_test (SQL Server real E2E) + ' + str(get_test_count('modern/tests/unit/db/mssql_real_e2e_test.cpp')) + ' tests',
     ['modern/tests/unit/db/mssql_real_e2e_test.cpp']),
    ('client: client_e2e_flow_test (6-state lifecycle E2E) + ' + str(get_test_count('modern/tests/unit/client/client_e2e_flow_test.cpp')) + ' tests',
     ['modern/tests/unit/client/client_e2e_flow_test.cpp']),
    ('tools: MoxianSideBySide packet + capture + replay + CMakeLists (T2/T3 harness)',
     ['modern/tools/MoxianSideBySide/CMakeLists.txt',
      'modern/tools/MoxianSideBySide/packet.cpp',
      'modern/tools/MoxianSideBySide/packet.hpp',
      'modern/tools/MoxianSideBySide/capture',
      'modern/tools/MoxianSideBySide/replay/replay.hpp']),
    ('docs: PLAN_2026Q3.md master 12-week plan (T1+T2+T3 -> 1.0 tag)',
     ['docs/PLAN_2026Q3.md']),
    ('scripts: commit_modules + commit_ui + commit_modules2 + gen_plan (working tree hygiene helpers)',
     ['scripts/commit_modules.py',
      'scripts/commit_modules2.py',
      'scripts/commit_ui.py',
      'scripts/gen_plan.py']),
    ('tests: legacy e2e fixture directory placeholder',
     ['tests/']),
]

count = 0
for msg, files in COMMITS:
    # Only add files that exist
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
        print('FAIL', msg[:60], ':', r.stderr.strip()[:200])

print('\nTotal:', count, 'commits')
