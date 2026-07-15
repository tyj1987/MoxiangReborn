"""Detect encoding of all source files."""
import os
import sys

# Find all .h and .cpp files in [Server]Map and cross-dir
roots = [
    os.path.join('墨香【源码】', '[Server]Map'),
    os.path.join('墨香【源码】', '[CC]Header'),
    os.path.join('墨香【源码】', '[CC]Ability'),
    os.path.join('墨香【源码】', '[CC]BattleSystem'),
    os.path.join('墨香【源码】', '[CC]Quest'),
    os.path.join('墨香【源码】', '[CC]ServerModule'),
    os.path.join('墨香【源码】', '[CC]Skill'),
    os.path.join('墨香【源码】', '[CC]Suryun'),
]

encodings = {}
for root in roots:
    if not os.path.isdir(root):
        continue
    for f in os.listdir(root):
        if not (f.endswith('.h') or f.endswith('.cpp')):
            continue
        path = os.path.join(root, f)
        if not os.path.isfile(path):
            continue
        try:
            with open(path, 'rb') as fh:
                data = fh.read()
            # Try UTF-8 first
            try:
                data.decode('utf-8')
                enc = 'utf-8'
            except UnicodeDecodeError:
                # Try cp949
                try:
                    data.decode('cp949')
                    enc = 'cp949'
                except UnicodeDecodeError:
                    enc = 'other'
            # Categorize utf-8: clean vs replacement chars
            if enc == 'utf-8':
                if b'\xef\xbf\xbd' in data:
                    enc = 'utf-8(repl)'
            encodings[path] = enc
        except Exception as e:
            encodings[path] = f'error:{e}'

# Summary
from collections import Counter
counts = Counter(encodings.values())
print("Encoding summary:")
for enc, count in sorted(counts.items(), key=lambda x: -x[1]):
    print(f"  {enc}: {count}")

# List cp949 files (which break under utf-8)
print("\nCP949 files (would break under /source-charset:utf-8):")
for path, enc in sorted(encodings.items()):
    if enc == 'cp949':
        print(f"  {path}")

print("\nUTF-8 files (clean, would work under utf-8):")
utf8_clean = [p for p, e in encodings.items() if e == 'utf-8']
print(f"  count: {len(utf8_clean)}")

print("\nUTF-8 with replacement chars (would work but Korean comments corrupted):")
utf8_repl = [p for p, e in encodings.items() if e == 'utf-8(repl)']
print(f"  count: {len(utf8_repl)}")