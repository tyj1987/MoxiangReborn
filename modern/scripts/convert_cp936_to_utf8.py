"""Convert the 3 cp936-only files in [Server]Map to UTF-8 so they
decode cleanly under MSVC's /source-charset:utf-8 (or cp949 — both).

Files (3):
  - 墨香【源码】/[Server]Map/Npc.cpp
  - 墨香【源码】/[Server]Map/StreetStall.h
  - 墨香【源码】/[Server]Map/TacticObject.h

These 3 files contain Chinese (cp936/GBK) characters in COMMENTS, not in
game-logic. The legacy build's /source-charset was the system codepage
(GBK = 936 on Chinese Windows dev machine), but the build host runs
with Korean codepage preference — these 3 files only decode in cp936.
Transcoding them to UTF-8 makes them universally readable.

Bytes that don't decode in cp936 are replaced with U+FFFD.
"""
import os

ROOT = r'D:\墨香全套源代码（源码+资源+客户端+服务端+教程）'
FILES = [
    '墨香【源码】/[Server]Map/Npc.cpp',
    '墨香【源码】/[Server]Map/StreetStall.h',
    '墨香【源码】/[Server]Map/TacticObject.h',
]

for rel in FILES:
    p = os.path.join(ROOT, rel)
    with open(p, 'rb') as f:
        data = f.read()
    # First check: is it pure ASCII?
    try:
        data.decode('ascii')
        print(f'{rel}: pure ASCII, skipping')
        continue
    except UnicodeDecodeError:
        pass
    # Try cp949 (Korean) — the dominant source encoding
    try:
        data.decode('cp949')
        print(f'{rel}: cp949 OK, no transcoding needed')
        continue
    except UnicodeDecodeError:
        pass
    # Try cp936 (Chinese GBK)
    try:
        text = data.decode('cp936')
        strict = True
    except UnicodeDecodeError as e:
        text = data.decode('cp936', errors='replace')
        strict = False
    if not strict:
        print(f'{rel}: cp936 had errors (replaced with U+FFFD)')
    # Write back as UTF-8 no BOM
    new = text.encode('utf-8')
    with open(p, 'wb') as f:
        f.write(new)
    print(f'  -> wrote {len(new)} bytes (UTF-8 no BOM, was {len(data)} bytes)')