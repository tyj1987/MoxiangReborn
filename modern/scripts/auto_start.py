#!/usr/bin/env python3
"""
Auto-start Moxian servers and client, auto-dismiss MessageBox dialogs.
"""
import subprocess
import time
import ctypes
import ctypes.wintypes
import os
import sys
import threading

SW_DIR = r'd:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\SWorking'
CLIENT_DIR = r'd:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码配套资源】\PlayDH'

WM_CLOSE = 0x0010
WM_COMMAND = 0x0111
IDOK = 1

EnumWindows = ctypes.windll.user32.EnumWindows
GetWindowTextW = ctypes.windll.user32.GetWindowTextW
GetWindowThreadProcessId = ctypes.windll.user32.GetWindowThreadProcessId
PostMessageW = ctypes.windll.user32.PostMessageW
SendMessageW = ctypes.windll.user32.SendMessageW
IsWindowVisible = ctypes.windll.user32.IsWindowVisible
FindWindowW = ctypes.windll.user32.FindWindowW

def dismiss_dialogs():
    """Find and click OK on any visible MessageBox dialogs."""
    results = []

    @ctypes.WINFUNCTYPE(ctypes.wintypes.BOOL, ctypes.wintypes.HWND, ctypes.wintypes.LPARAM)
    def enum_callback(hwnd, lparam):
        if not IsWindowVisible(hwnd):
            return True
        buf = ctypes.create_unicode_buffer(256)
        GetWindowTextW(hwnd, buf, 256)
        title = buf.value
        if any(kw in title for kw in ['Console', 'Error', 'Fail', 'Initialize', 'error']):
            results.append(hwnd)
        return True

    EnumWindows(enum_callback, 0)
    for hwnd in results:
        try:
            SendMessageW(hwnd, WM_COMMAND, IDOK, 0)
            print(f'  Dismissed dialog: hwnd={hwnd}')
        except:
            try:
                PostMessageW(hwnd, WM_CLOSE, 0, 0)
            except:
                pass

def dismiss_loop():
    """Background thread that continuously dismisses dialogs."""
    for i in range(120):  # Run for 2 minutes
        dismiss_dialogs()
        time.sleep(1)

def main():
    print("=== Moxian Auto-Start ===")
    print("1. Starting DistributeServer...")
    dist = subprocess.Popen(
        [os.path.join(SW_DIR, "DistributeServer.exe")],
        cwd=SW_DIR,
        creationflags=subprocess.CREATE_NO_WINDOW
    )

    print("2. Waiting 5s then starting AgentServer...")
    time.sleep(5)
    agent = subprocess.Popen(
        [os.path.join(SW_DIR, "AgentServer.exe")],
        cwd=SW_DIR,
        creationflags=subprocess.CREATE_NO_WINDOW
    )

    print("3. Waiting 5s then starting MapServer...")
    time.sleep(5)
    mapsv = subprocess.Popen(
        [os.path.join(SW_DIR, "MapServer.exe"), "17"],
        cwd=SW_DIR,
        creationflags=subprocess.CREATE_NO_WINDOW
    )

    # Start dialog dismisser in background
    print("4. Starting dialog auto-dismisser...")
    dismisser = threading.Thread(target=dismiss_loop, daemon=True)
    dismisser.start()

    print("5. Waiting 10s for servers to initialize...")
    time.sleep(10)

    print("6. Starting client...")
    client = subprocess.Popen(
        [os.path.join(CLIENT_DIR, "MHClient-Connect.exe"), "anrgideoqcn"],
        cwd=CLIENT_DIR,
    )

    print("7. Waiting 5s for client to start...")
    time.sleep(5)

    # Final check
    import socket
    for name, port in [("DistributeServer", 16001), ("AgentServer", 17001), ("MapServer", 18001)]:
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(1)
            result = s.connect_ex(('127.0.0.1', port))
            s.close()
            status = "LISTENING" if result == 0 else f"NOT LISTENING (errno={result})"
        except Exception as e:
            status = f"ERROR: {e}"
        print(f"  {name} port {port}: {status}")

    print("\nDone! Servers and client should be running.")
    print("Press Ctrl+C to keep running, or wait for dismisser to finish.")

    try:
        while True:
            time.sleep(5)
            dismiss_dialogs()
    except KeyboardInterrupt:
        print("Exiting...")

if __name__ == '__main__':
    main()
