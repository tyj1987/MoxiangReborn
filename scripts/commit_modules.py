#!/usr/bin/env python3
"""Commit each untracked server module as a separate commit."""
import subprocess
import os
from pathlib import Path

os.chdir(r'C:\moxiang')

MODULES = [
    'bobusang_manager',
    'change_item_manager',
    'character_calc_manager',
    'cquest_base',
    'distributer',
    'distribute_strategy',
    'exchange_manager',
    'friend_manager',
    'guild_manager',
    'help_request_manager',
    'item_drop',
    'item_limit_manager',
    'item_manager',
    'looting_manager',
    'monster',
    'party_manager',
    'party_war_mgr',
    'pet_manager',
    'player_state',
    'quest_group',
    'quest_map_mgr',
    'quest_regen_mgr',
    'quest_updater',
    'regen_manager',
    'street_stall_manager',
    'survival_mode_manager',
    'titan_manager',
    'agent_db_msg_parser',
    'agent_network_msg_parser',
]

def run(cmd, check=True):
    r = subprocess.run(cmd, capture_output=True, text=True)
    if check and r.returncode != 0:
        print('FAIL:', ' '.join(cmd))
        print('  stderr:', r.stderr.strip()[:300])
        return False
    return r

r = run(['git', 'log', '--oneline', '-1'], check=False)
print('Current HEAD:', r.stdout.strip())

def get_test_count(module):
    p = Path('modern/tests/unit/server/' + module + '_test.cpp')
    if not p.exists():
        return 0
    text = p.read_text(encoding='utf-8', errors='replace')
    return text.count('TEST(')

count = 0
for module in MODULES:
    hpp = Path('modern/include/mxh/server/' + module + '.hpp')
    cpp = Path('modern/src/server/' + module + '.cpp')
    test = Path('modern/tests/unit/server/' + module + '_test.cpp')
    files = [str(f) for f in [hpp, cpp, test] if f.exists()]
    if not files:
        print('SKIP', module, ': no files')
        continue
    n = get_test_count(module)
    run(['git', 'add'] + files)
    msg = 'server: ' + module + ' 1:1 port + ' + str(n) + ' tests'
    r = run(['git', 'commit', '-m', msg], check=False)
    if r.returncode == 0:
        print('COMMIT', module, '(' + str(n) + ' tests)')
        count += 1
    else:
        print('SKIP', module, '(commit failed)')

print('\nTotal commits:', count)
r = run(['git', 'log', '--oneline', '-15'], check=False)
print('Recent commits:')
print(r.stdout)
