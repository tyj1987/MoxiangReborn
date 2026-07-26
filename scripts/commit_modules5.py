#!/usr/bin/env python3
"""Commit traffic_log + commit_modules4 helper."""
import subprocess, os
from pathlib import Path
os.chdir(r'C:\moxiang')
def run(cmd):
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        print('FAIL:', cmd, r.stderr.strip()[:200])
        return False
    return r

# traffic_log
files = ['modern/include/mxh/server/traffic_log.hpp',
         'modern/src/server/traffic_log.cpp',
         'modern/tests/unit/server/traffic_log_test.cpp']
existing = [f for f in files if Path(f).exists()]
p = Path('modern/tests/unit/server/traffic_log_test.cpp')
n = p.read_text(encoding='utf-8', errors='replace').count('TEST(') if p.exists() else 0
run(['git', 'add'] + existing)
r = run(['git', 'commit', '-m', 'server: traffic_log 1:1 port (legacy [Server]Map/TrafficLog.h) + ' + str(n) + ' tests'])
print('COMMIT traffic_log' if r.returncode == 0 else 'FAIL traffic_log')

# commit_modules4
run(['git', 'add', 'scripts/commit_modules4.py'])
r = run(['git', 'commit', '-m', 'scripts: commit_modules4.py (final batch helper)'])
print('COMMIT commit_modules4.py' if r.returncode == 0 else 'FAIL commit_modules4.py')

# Final state
r = run(['git', 'status', '--porcelain'])
print('Remaining:')
print(r.stdout)
