#!/usr/bin/env python3
"""Analyze PE headers of Moxian server binaries to diagnose crash."""
import struct
import sys
import os

def rva_to_offset(sections, rva):
    """Convert RVA to file offset using section headers."""
    for vaddr, vsize, raw_ptr, raw_size in sections:
        if vaddr <= rva < vaddr + vsize:
            return raw_ptr + (rva - vaddr)
    return None

def analyze_pe(path):
    print(f"\n{'='*60}")
    print(f"Analyzing: {os.path.basename(path)}")
    print(f"Size: {os.path.getsize(path)} bytes")
    print(f"{'='*60}")

    with open(path, 'rb') as f:
        data = f.read()

    # DOS header
    e_lfanew = struct.unpack_from('<I', data, 0x3C)[0]

    # PE signature
    pe_sig = data[e_lfanew:e_lfanew+4]
    if pe_sig != b'PE\x00\x00':
        print(f"ERROR: Not a PE file (signature: {pe_sig})")
        return

    # COFF header
    coff = e_lfanew + 4
    machine = struct.unpack_from('<H', data, coff)[0]
    num_sections = struct.unpack_from('<H', data, coff+2)[0]
    optional_header_size = struct.unpack_from('<H', data, coff+16)[0]
    file_chars = struct.unpack_from('<H', data, coff+18)[0]

    print(f"\n--- COFF Header ---")
    print(f"Machine: 0x{machine:04X} ({'x86' if machine == 0x14C else 'x64' if machine == 0x8664 else '?'})")
    print(f"Sections: {num_sections}")
    print(f"File Characteristics: 0x{file_chars:04X}")
    if file_chars & 0x0002: print("  EXECUTABLE_IMAGE")
    if file_chars & 0x0020: print("  LARGE_ADDRESS_AWARE")
    if file_chars & 0x0100: print("  32BIT_MACHINE")

    # Optional header
    opt = coff + 20
    magic = struct.unpack_from('<H', data, opt)[0]
    linker_major = struct.unpack_from('<B', data, opt+2)[0]
    linker_minor = struct.unpack_from('<B', data, opt+3)[0]
    entry_point = struct.unpack_from('<I', data, opt+16)[0]
    image_base = struct.unpack_from('<I', data, opt+28)[0]
    section_align = struct.unpack_from('<I', data, opt+32)[0]
    file_align = struct.unpack_from('<I', data, opt+36)[0]
    os_major = struct.unpack_from('<H', data, opt+40)[0]
    os_minor = struct.unpack_from('<H', data, opt+42)[0]
    subsystem = struct.unpack_from('<H', data, opt+68)[0]
    dll_chars = struct.unpack_from('<H', data, opt+46)[0]
    stack_reserve = struct.unpack_from('<I', data, opt+72)[0]
    stack_commit = struct.unpack_from('<I', data, opt+76)[0]
    heap_reserve = struct.unpack_from('<I', data, opt+80)[0]
    heap_commit = struct.unpack_from('<I', data, opt+84)[0]
    num_data_dirs = struct.unpack_from('<I', data, opt+92)[0]

    print(f"\n--- Optional Header ---")
    print(f"Linker: {linker_major}.{linker_minor}")
    print(f"Entry Point: 0x{entry_point:08X}")
    print(f"Image Base: 0x{image_base:08X}")
    print(f"Section Alignment: 0x{section_align:X}")
    print(f"File Alignment: 0x{file_align:X}")
    print(f"OS Version: {os_major}.{os_minor}")
    print(f"Subsystem: {subsystem} ({'GUI' if subsystem == 2 else 'Console' if subsystem == 3 else '?'})")
    print(f"Stack: reserve=0x{stack_reserve:X} commit=0x{stack_commit:X}")
    print(f"Heap: reserve=0x{heap_reserve:X} commit=0x{heap_commit:X}")
    print(f"Data Directories: {num_data_dirs}")

    print(f"\n--- DLL Characteristics: 0x{dll_chars:04X} ---")
    flags = {
        0x0020: 'HIGH_ENTROPY_VA',
        0x0040: 'DYNAMIC_BASE (ASLR)',
        0x0080: 'FORCE_INTEGRITY_CHECKS',
        0x0100: 'NX_COMPAT (DEP)',
        0x0200: 'NO_ISOLATION',
        0x0400: 'NO_SEH',
        0x0800: 'NO_BIND',
        0x1000: 'APPCONTAINER',
        0x2000: 'WDM_DRIVER',
        0x4000: 'GUARD_CF',
        0x8000: 'TERMINAL_SERVER_AWARE',
    }
    for bit, name in sorted(flags.items()):
        if dll_chars & bit:
            print(f"  {name}")

    # Parse section headers
    sections_start = opt + optional_header_size
    sections = []
    print(f"\n--- Sections ---")
    for s in range(num_sections):
        sec_off = sections_start + s * 40
        name = data[sec_off:sec_off+8].rstrip(b'\x00').decode('ascii', errors='replace')
        vsize = struct.unpack_from('<I', data, sec_off+8)[0]
        vaddr = struct.unpack_from('<I', data, sec_off+12)[0]
        raw_size = struct.unpack_from('<I', data, sec_off+16)[0]
        raw_ptr = struct.unpack_from('<I', data, sec_off+20)[0]
        flags = struct.unpack_from('<I', data, sec_off+36)[0]
        sections.append((vaddr, vsize, raw_ptr, raw_size))
        print(f"  {name:8s} VA=0x{vaddr:08X} VSize=0x{vsize:08X} Raw=0x{raw_ptr:08X} RawSize=0x{raw_size:08X} Flags=0x{flags:08X}")

    # Parse data directories
    dd_start = opt + 96
    dd_names = ['EXPORT', 'IMPORT', 'RESOURCE', 'EXCEPTION', 'SECURITY',
                'BASERELOC', 'DEBUG', 'ARCHITECTURE', 'GLOBALPTR', 'TLS',
                'LOAD_CONFIG', 'BOUND_IMPORT', 'IAT', 'DELAY_IMPORT', 'COM_DESCRIPTOR']
    print(f"\n--- Data Directories ---")
    dirs = {}
    for i in range(min(num_data_dirs, len(dd_names))):
        rva = struct.unpack_from('<I', data, dd_start + i*8)[0]
        size = struct.unpack_from('<I', data, dd_start + i*8 + 4)[0]
        dirs[dd_names[i]] = (rva, size)
        if rva > 0:
            print(f"  [{i:2d}] {dd_names[i]:20s} RVA=0x{rva:08X} Size=0x{size:08X}")

    # TLS Directory
    if 'TLS' in dirs and dirs['TLS'][0] > 0:
        tls_rva, tls_size = dirs['TLS']
        tls_off = rva_to_offset(sections, tls_rva)
        if tls_off:
            print(f"\n--- TLS Directory ---")
            start_addr = struct.unpack_from('<I', data, tls_off)[0]
            end_addr = struct.unpack_from('<I', data, tls_off+4)[0]
            addr_index = struct.unpack_from('<I', data, tls_off+8)[0]
            callbacks = struct.unpack_from('<I', data, tls_off+12)[0]
            zero_fill = struct.unpack_from('<I', data, tls_off+16)[0]
            characteristics = struct.unpack_from('<I', data, tls_off+20)[0]
            print(f"  Start Address: 0x{start_addr:08X}")
            print(f"  End Address: 0x{end_addr:08X}")
            print(f"  Address of Index: 0x{addr_index:08X}")
            print(f"  Callbacks: 0x{callbacks:08X}")
            print(f"  Zero Fill Size: {zero_fill}")
            print(f"  Characteristics: 0x{characteristics:08X}")

            # Read TLS callbacks
            if callbacks > 0:
                cb_off = rva_to_offset(sections, callbacks)
                if cb_off:
                    print(f"\n  TLS Callbacks (file offset 0x{cb_off:08X}):")
                    idx = 0
                    while True:
                        cb_addr = struct.unpack_from('<I', data, cb_off + idx*4)[0]
                        if cb_addr == 0:
                            break
                        print(f"    [{idx}] Callback RVA: 0x{cb_addr:08X} (file offset: 0x{rva_to_offset(sections, cb_addr) or 0:08X})")
                        idx += 1
                        if idx > 20:
                            print(f"    ... (more)")
                            break

    # Load Config Directory
    if 'LOAD_CONFIG' in dirs and dirs['LOAD_CONFIG'][0] > 0:
        lc_rva, lc_size = dirs['LOAD_CONFIG']
        lc_off = rva_to_offset(sections, lc_rva)
        if lc_off:
            print(f"\n--- Load Config Directory ---")
            lc_struct_size = struct.unpack_from('<I', data, lc_off)[0]
            print(f"  Struct Size: {lc_struct_size}")
            if lc_struct_size >= 64:
                guard_cf_flags = struct.unpack_from('<I', data, lc_off+60)[0]
                print(f"  Guard CF Flags: 0x{guard_cf_flags:08X}")
                if guard_cf_flags & 0x0001: print("    CF_FUNCTION_TABLE_PRESENT")
                if guard_cf_flags & 0x0002: print("    CF_INSTRUMENTED")
                if guard_cf_flags & 0x0004: print("    CF_FUNCTION_TABLE_FOR_EXPORTS_ONLY")
                if guard_cf_flags & 0x0008: print("    CF_FUNCTION_TABLE_FOR_DELAYLOAD_IMPORTS_ONLY")
                if guard_cf_flags & 0x0010: print("    CF_DISPATCH_FUNCTION_TABLE")
                if guard_cf_flags & 0x100: print("    RETURN_FLOW_GRAPH")
            if lc_struct_size >= 72:
                guard_cf_check_function = struct.unpack_from('<I', data, lc_off+64)[0]
                guard_cf_dispatch_function = struct.unpack_from('<I', data, lc_off+68)[0]
                print(f"  Guard CF Check Function: 0x{guard_cf_check_function:08X}")
                print(f"  Guard CF Dispatch Function: 0x{guard_cf_dispatch_function:08X}")
            if lc_struct_size >= 80:
                guard_rf_failure_routine = struct.unpack_from('<I', data, lc_off+72)[0]
                print(f"  Guard RF Failure Routine: 0x{guard_rf_failure_routine:08X}")

    # Security Cookie
    print(f"\n--- Security Cookie (.data section) ---")
    # The security cookie is typically at the end of the .data section
    # In MSVC, __security_cookie is a global variable
    # Let's search for common patterns
    data_rva = None
    for vaddr, vsize, raw_ptr, raw_size in sections:
        name_idx = 0
        # Find .data section
        sec_off = sections_start + name_idx * 40
        sec_name = data[sec_off:sec_off+8]
        if b'.data' in sec_name:
            data_rva = vaddr
            break

    print(f"  Note: Security cookie is validated during CRT startup (__security_check_cookie)")

    # Import table analysis
    if 'IMPORT' in dirs and dirs['IMPORT'][0] > 0:
        imp_rva = dirs['IMPORT'][0]
        imp_off = rva_to_offset(sections, imp_rva)
        if imp_off:
            print(f"\n--- Import Thunks (key APIs) ---")
            idx = 0
            while True:
                ilt_rva = struct.unpack_from('<I', data, imp_off + idx*20)[0]
                if ilt_rva == 0:
                    break
                name_rva = struct.unpack_from('<I', data, imp_off + idx*20 + 12)[0]
                if name_rva > 0:
                    name_off = rva_to_offset(sections, name_rva)
                    if name_off:
                        dll_name = data[name_off:name_off+64].split(b'\x00')[0].decode('ascii', errors='replace')
                        print(f"  DLL: {dll_name}")
                idx += 1
                if idx > 30:
                    break

    print()

if __name__ == '__main__':
    base = r'd:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\SWorking'
    files = [
        os.path.join(base, 'DistributeServer.exe'),
        os.path.join(base, 'AgentServer.exe'),
        os.path.join(base, 'MapServer.exe'),
    ]
    for f in files:
        if os.path.exists(f):
            analyze_pe(f)
