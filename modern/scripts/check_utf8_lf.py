"""Check if CommonStruct.h has 0x0A inside UTF-8 sequences."""
import os

path = os.path.join('墨香【源码】', '[CC]Header', 'CommonStruct.h')
with open(path, 'rb') as f:
    data = f.read()

# Find bytes that look like UTF-8 continuation/start bytes (0x80-0xBF or 0xC0-0xFF)
# Check if any of these are 0x0A
issues = []
for i, b in enumerate(data):
    if b == 0x0A:
        # Check if this 0x0A is inside a UTF-8 sequence
        # Look back for a UTF-8 lead byte
        for j in range(i - 1, max(0, i - 4), -1):
            bj = data[j]
            if 0xC0 <= bj <= 0xDF:  # 2-byte UTF-8 lead, j+1 should be continuation
                if j + 1 == i:
                    issues.append(('inside 2-byte UTF-8', i, j))
                break
            elif 0xE0 <= bj <= 0xEF:  # 3-byte lead
                if j + 1 == i or j + 2 == i:
                    issues.append(('inside 3-byte UTF-8', i, j))
                break
            elif 0xF0 <= bj <= 0xF7:  # 4-byte lead
                if j + 1 == i or j + 2 == i or j + 3 == i:
                    issues.append(('inside 4-byte UTF-8', i, j))
                break
            elif bj < 0x80:
                break  # ASCII, not part of UTF-8 sequence
        # else: this 0x0A is real newline

print(f"Found {len(issues)} newline bytes inside UTF-8 sequences")
for issue in issues[:10]:
    print(f"  {issue}")
print("Done.")