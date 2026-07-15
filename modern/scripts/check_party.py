"""Test Party.cpp encoding."""
import os
path = os.path.join('墨香【源码】', '[Server]Map', 'Party.cpp')
with open(path, 'rb') as f:
    data = f.read()

# Find line 132
lines_so_far = 0
line_start = None
for i, b in enumerate(data):
    if b == 0x0A:
        lines_so_far += 1
        if lines_so_far == 131:
            line_start = i + 1
            break

end = line_start
while end < len(data) and data[end] != 0x0A:
    end += 1
line = data[line_start:end]

# Try different encodings
print("Encoding tests for line 132:")
print(f"  Length: {len(line)}")
print(f"  hex: {line.hex()}")

# UTF-8
try:
    print(f"  UTF-8: {line.decode('utf-8')!r}")
except UnicodeDecodeError as e:
    print(f"  UTF-8 fails: {e}")

# CP949 (EUC-KR)
try:
    print(f"  CP949: {line.decode('cp949')!r}")
except UnicodeDecodeError as e:
    print(f"  CP949 fails: {e}")

# Check BOM
print(f"\nFile BOM (first 3 bytes): {data[:3].hex()}")
print(f"UTF-8 BOM: {data[:3] == b'\\xef\\xbb\\xbf'}")

# Try cp949 first
try:
    full = data.decode('cp949')
    print(f"Full file as CP949: OK ({len(full)} chars)")
except UnicodeDecodeError as e:
    print(f"Full file CP949 fails: {e}")

# Try UTF-8
try:
    full = data.decode('utf-8')
    print(f"Full file as UTF-8: OK ({len(full)} chars)")
except UnicodeDecodeError as e:
    print(f"Full file UTF-8 fails: {e}")