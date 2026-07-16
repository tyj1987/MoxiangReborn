#!/usr/bin/env python3
"""Count active g_Console.LOG( in Map/ cpp files (skip // and /* */ comments)."""
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
    active_lines = []
    for i, line in enumerate(lines, 1):
        # crude block-comment tracking
        if re.match(r"^\s*/\*", line):
            in_block = True
        if re.search(r"\*/", line):
            in_block = False
        if in_block:
            continue
        # skip line comments
        stripped = line.lstrip()
        if stripped.startswith("//"):
            continue
        if "g_Console.LOG(" in line:
            active_lines.append(i)
    if active_lines:
        print(f"{fn}: {len(active_lines)} active g_Console.LOG( calls at lines {active_lines}")
