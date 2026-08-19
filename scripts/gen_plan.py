#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Generate the master 12-week plan to docs/PLAN_2026Q3.md (Chinese)."""
from pathlib import Path

plan = '''PLACEHOLDER'''

Path('docs/PLAN_2026Q3.md').write_text(plan, encoding='utf-8')
print('OK', len(plan), 'chars')
