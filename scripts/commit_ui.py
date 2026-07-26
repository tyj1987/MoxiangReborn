#!/usr/bin/env python3
"""Commit each untracked UI dialog as a separate commit."""
import subprocess
import os
from pathlib import Path

os.chdir(r'C:\moxiang')

# Collect untracked files in modern/src/ui/ and modern/tests/unit/ui/
untracked = subprocess.run(['git', 'status', '--porcelain'], capture_output=True, text=True).stdout
ui_files = []
test_files = []
for line in untracked.splitlines():
    if line.startswith('??'):
        path = line[3:]
        if path.startswith('modern/src/ui/'):
            ui_files.append(path)
        elif path.startswith('modern/tests/unit/ui/'):
            test_files.append(path)

print('UI source files:', len(ui_files))
print('UI test files:', len(test_files))

# Extract unique dialog names
def dialog_name(path):
    fn = path.split('/')[-1]
    if fn.endswith('_test.cpp'):
        return fn[:-len('_test.cpp')]
    return fn.rsplit('.', 1)[0]

dialogs = set()
for p in ui_files + test_files:
    dialogs.add(dialog_name(p))
print('Unique dialogs:', len(dialogs))

def run(cmd, check=True):
    r = subprocess.run(cmd, capture_output=True, text=True)
    if check and r.returncode != 0:
        print('FAIL:', ' '.join(cmd[:5]), 'stderr:', r.stderr.strip()[:200])
        return False
    return r

def get_test_count(dialog):
    p = Path('modern/tests/unit/ui/' + dialog + '_test.cpp')
    if not p.exists():
        return 0
    text = p.read_text(encoding='utf-8', errors='replace')
    return text.count('TEST(')

count = 0
for dialog in sorted(dialogs):
    candidates = [
        'modern/src/ui/' + dialog + '.cpp',
        'modern/src/ui/' + dialog + '.hpp',
        'modern/tests/unit/ui/' + dialog + '_test.cpp',
    ]
    files = [c for c in candidates if Path(c).exists()]
    if not files:
        continue
    n = get_test_count(dialog)
    run(['git', 'add'] + files)
    msg = 'ui: ' + dialog + ' 1:1 port + ' + str(n) + ' tests'
    r = run(['git', 'commit', '-m', msg], check=False)
    if r.returncode == 0:
        print('COMMIT', dialog, '(' + str(n) + ' tests)')
        count += 1
    else:
        print('SKIP', dialog, ':', r.stderr.strip()[:200])

print('\nTotal UI commits:', count)
