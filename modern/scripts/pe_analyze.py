import struct, sys

path = sys.argv[1] if len(sys.argv) > 1 else r'd:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\SWorking\DistributeServer.exe'
with open(path, 'rb') as f:
    data = f.read()

pe_off = struct.unpack_from('<I', data, 60)[0]
print("DOS signature: 0x{:04X}".format(struct.unpack_from('<H', data, 0)[0]))
print("PE signature: 0x{:08X}".format(struct.unpack_from('<I', data, pe_off)[0]))

num_sections = struct.unpack_from('<H', data, pe_off + 6)[0]
opt_hdr_size = struct.unpack_from('<H', data, pe_off + 20)[0]
opt_hdr_off = pe_off + 24

magic = struct.unpack_from('<H', data, opt_hdr_off)[0]
pe_type = "PE32" if magic == 0x10b else "PE32+"
print("PE magic: 0x{:04X} ({})".format(magic, pe_type))

# Subsystem
subsystem = struct.unpack_from('<H', data, opt_hdr_off + 68)[0]
subsystem_names = {1:'NATIVE', 2:'WINDOWS_GUI', 3:'WINDOWS_CUI', 5:'OS2_CUI', 7:'POSIX_CUI', 9:'WINDOWS_CE_GUI', 10:'EFI_APPLICATION'}
print("Subsystem: {} ({})".format(subsystem, subsystem_names.get(subsystem, "UNKNOWN")))

# Stack/Heap sizes
stack_reserve = struct.unpack_from('<I', data, opt_hdr_off + 72)[0]
stack_commit = struct.unpack_from('<I', data, opt_hdr_off + 76)[0]
heap_reserve = struct.unpack_from('<I', data, opt_hdr_off + 80)[0]
heap_commit = struct.unpack_from('<I', data, opt_hdr_off + 84)[0]
print("Stack reserve: {} (0x{:X})".format(stack_reserve, stack_reserve))
print("Stack commit: {} (0x{:X})".format(stack_commit, stack_commit))
print("Heap reserve: {} (0x{:X})".format(heap_reserve, heap_reserve))
print("Heap commit: {} (0x{:X})".format(heap_commit, heap_commit))

# Load config directory
if magic == 0x10b:
    lc_rva = struct.unpack_from('<I', data, opt_hdr_off + 208)[0]
    lc_size = struct.unpack_from('<I', data, opt_hdr_off + 212)[0]
else:
    lc_rva = struct.unpack_from('<I', data, opt_hdr_off + 240)[0]
    lc_size = struct.unpack_from('<I', data, opt_hdr_off + 244)[0]
print("Load Config RVA: 0x{:X}, Size: {}".format(lc_rva, lc_size))

# Security cookie
if magic == 0x10b:
    sec_cookie = struct.unpack_from('<I', data, opt_hdr_off + 232)[0]
    print("Security Cookie RVA: 0x{:X}".format(sec_cookie))

# Check for CFG
if magic == 0x10b:
    cfg_flags = struct.unpack_from('<I', data, opt_hdr_off + 260)[0]
else:
    cfg_flags = struct.unpack_from('<I', data, opt_hdr_off + 260)[0]
print("CFG Flags: 0x{:X}".format(cfg_flags))
if cfg_flags & 0x100:
    print("  -> Control Flow Guard (CFG) is ENABLED!")

# Data directories
if magic == 0x10b:
    dd_off = opt_hdr_off + 96
else:
    dd_off = opt_hdr_off + 112

dir_names = ['EXPORT', 'IMPORT', 'RESOURCE', 'EXCEPTION', 'SECURITY', 'BASERELOC',
             'DEBUG', 'ARCHITECTURE', 'GLOBALPTR', 'TLS', 'LOAD_CONFIG',
             'BOUND_IMPORT', 'IAT', 'DELAY_IMPORT', 'COM_DESCRIPTOR', 'RESERVED']

print("\nData Directories:")
for i, name in enumerate(dir_names):
    rva = struct.unpack_from('<I', data, dd_off + i*8)[0]
    size = struct.unpack_from('<I', data, dd_off + i*8 + 4)[0]
    if rva != 0 or size != 0:
        print("  [{}] RVA=0x{:X} Size=0x{:X}".format(name, rva, size))

# Sections
sections_off = opt_hdr_off + opt_hdr_size
print("\nSections:")
for i in range(num_sections):
    s_off = sections_off + i * 40
    name = data[s_off:s_off+8].rstrip(b'\x00').decode('ascii', errors='replace')
    vsize = struct.unpack_from('<I', data, s_off + 8)[0]
    vrva = struct.unpack_from('<I', data, s_off + 12)[0]
    rawsize = struct.unpack_from('<I', data, s_off + 16)[0]
    rawoff = struct.unpack_from('<I', data, s_off + 20)[0]
    chars = struct.unpack_from('<I', data, s_off + 36)[0]
    print("  {:10s} RVA=0x{:06X} VSize=0x{:06X} RawOff=0x{:06X} RawSize=0x{:06X} Chars=0x{:08X}".format(
        name, vrva, vsize, rawoff, rawsize, chars))

# Check linker version
linker_major = struct.unpack_from('<B', data, opt_hdr_off + 2)[0]
linker_minor = struct.unpack_from('<B', data, opt_hdr_off + 3)[0]
print("\nLinker version: {}.{}".format(linker_major, linker_minor))

# Check timestamp
timestamp = struct.unpack_from('<I', data, pe_off + 4)[0]
import datetime
try:
    dt = datetime.datetime.utcfromtimestamp(timestamp)
    print("Timestamp: {} UTC".format(dt))
except:
    print("Timestamp: 0x{:X}".format(timestamp))
