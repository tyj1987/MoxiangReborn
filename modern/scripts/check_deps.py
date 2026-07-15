import pefile
import sys

path = sys.argv[1]
pe = pefile.PE(path)
print("Machine:", hex(pe.FILE_HEADER.Machine))
print("Subsystem:", pe.OPTIONAL_HEADER.Subsystem)
print()
print("Imports:")
if hasattr(pe, 'DIRECTORY_ENTRY_IMPORT'):
    for entry in pe.DIRECTORY_ENTRY_IMPORT:
        name = entry.dll.decode('ascii', errors='replace')
        funcs = [f.name.decode('ascii', errors='replace') if f.name else str(f.ordinal) for f in entry.imports]
        print("  %s: %d functions" % (name, len(funcs)))
else:
    print("  (none)")
