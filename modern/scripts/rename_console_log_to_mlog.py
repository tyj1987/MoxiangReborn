#!/usr/bin/env python3
"""Phase 12.x: Rename active g_Console.LOG( -> g_Console.MLOG( in Map/ files.

C-1 (Phase 7.5o) renamed CConsole::LOG to CConsole::MLOG but only scanned
[Lib]MHConsole/ + client-side Console.h direct includes. C-36 (Phase 12.x
commit 53026a9) caught [CC]ServerModule/ callers and fixed Distribute
Debug_<LOCALE> 5/5. This script extends the same fix to [Server]Map/ files
where the same regression produces 28 C2039 errors per locale target.

Idempotent: skip lines already containing g_Console.MLOG(.
Skip line-comment positions (//-prefixed lines) AND block-comment positions
(inside /* ... */). Inactive calls stay LOG for the historical record (they
don't compile under C-1 anyway, but the diff is kept minimal — same as the
C-36 fix entry's "注释掉的 // g_Console.LOG(...) 不动" rule).
"""
import os
import re

MAP_DIR = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\[Server]Map"
FILES = [
    "MapItemDrop.cpp", "Server.cpp", "RegenManager.cpp",
    "MapDBMsgParser.cpp", "MapNetworkMsgParser.cpp",
    "ServerSystem.cpp", "UserTable.cpp",
]

for fn in FILES:
    path = os.path.join(MAP_DIR, fn)
    with open(path, "r", encoding="utf-8") as f:
        lines = f.readlines()
    in_block = False
    active_renamed = 0
    skipped_line_comment = 0
    skipped_block_comment = 0
    new_lines = []
    for line in lines:
        if re.match(r"^\s*/\*", line):
            in_block = True
        if in_block:
            if "g_Console.LOG(" in line and "g_Console.MLOG(" not in line:
                skipped_block_comment += 1
            new_lines.append(line)
            if re.search(r"\*/", line):
                in_block = False
            continue
        # not in block comment
        stripped = line.lstrip()
        if stripped.startswith("//"):
            if "g_Console.LOG(" in line and "g_Console.MLOG(" not in line:
                skipped_line_comment += 1
            new_lines.append(line)
            continue
        # active line: do the rename
        if "g_Console.LOG(" in line and "g_Console.MLOG(" not in line:
            new_line = line.replace("g_Console.LOG(", "g_Console.MLOG(")
            new_lines.append(new_line)
            active_renamed += 1
        else:
            new_lines.append(line)
    if active_renamed > 0 or skipped_line_comment > 0 or skipped_block_comment > 0:
        with open(path, "w", encoding="utf-8", newline="") as f:
            f.writelines(new_lines)
    print(f"{fn}: active_renamed={active_renamed}, skipped //comment={skipped_line_comment}, skipped /* */={skipped_block_comment}")
