#!/usr/bin/env python3
"""
Register IConsole.dll as a COM server.
CLSID_ULTRA_TCONSOLE: {4845A47E-F4E1-45cd-8561-C7B947BBA936}
CLSID_4DyuchiCONSOLE: {DB6175A7-1DD7-40f2-BDFB-D5964CFA8157}
"""
import winreg
import os
import sys

def register_com_dll(dll_path, clsid, description):
    """Register a COM DLL in the Windows registry."""
    dll_path = os.path.abspath(dll_path)
    if not os.path.exists(dll_path):
        print(f"ERROR: DLL not found: {dll_path}")
        return False

    # Create CLSID key
    clsid_key_path = f"CLSID\\{clsid}"
    try:
        # Create CLSID\{guid} key
        with winreg.CreateKeyEx(winreg.HKEY_CLASSES_ROOT, clsid_key_path, 0, winreg.KEY_ALL_ACCESS) as key:
            winreg.SetValueEx(key, "", 0, winreg.REG_SZ, description)
            print(f"  Created: HKCR\\{clsid_key_path}")

        # Create CLSID\{guid}\InprocServer32
        inproc_path = f"{clsid_key_path}\\InprocServer32"
        with winreg.CreateKeyEx(winreg.HKEY_CLASSES_ROOT, inproc_path, 0, winreg.KEY_ALL_ACCESS) as key:
            winreg.SetValueEx(key, "", 0, winreg.REG_SZ, dll_path)
            winreg.SetValueEx(key, "ThreadingModel", 0, winreg.REG_SZ, "Both")
            print(f"  Created: HKCR\\{inproc_path}")

        # Create CLSID\{guid}\LocalServer32
        local_path = f"{clsid_key_path}\\LocalServer32"
        with winreg.CreateKeyEx(winreg.HKEY_CLASSES_ROOT, local_path, 0, winreg.KEY_ALL_ACCESS) as key:
            winreg.SetValueEx(key, "", 0, winreg.REG_SZ, dll_path)
            print(f"  Created: HKCR\\{local_path}")

        print(f"  SUCCESS: Registered {description}")
        return True
    except PermissionError:
        print(f"  ERROR: Permission denied. Need admin rights.")
        return False
    except Exception as e:
        print(f"  ERROR: {e}")
        return False

def main():
    dll_dir = r"d:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\SWorking"
    iconsole_dll = os.path.join(dll_dir, "IConsole.dll")

    if not os.path.exists(iconsole_dll):
        print(f"IConsole.dll not found at: {iconsole_dll}")
        sys.exit(1)

    print(f"Registering COM servers from: {iconsole_dll}")
    print()

    # Register CLSID_ULTRA_TCONSOLE
    ok1 = register_com_dll(
        iconsole_dll,
        "{4845A47E-F4E1-45cd-8561-C7B947BBA936}",
        "4DyuchiUltraTConsole"
    )

    # Register CLSID_4DyuchiCONSOLE
    ok2 = register_com_dll(
        iconsole_dll,
        "{DB6175A7-1DD7-40f2-BDFB-D5964CFA8157}",
        "4DyuchiConsole"
    )

    print()
    if ok1 and ok2:
        print("All COM servers registered successfully!")
    else:
        print("Some registrations failed. Try running as Administrator.")
        print("Alternative: Run 'regsvr32 IConsole.dll' as Administrator")

if __name__ == '__main__':
    main()
