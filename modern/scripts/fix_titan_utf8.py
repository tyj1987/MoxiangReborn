"""Fix TitanServer.bin file with UTF-8 encoding (matches compiler /execution-charset:utf-8)."""
import struct
import os

SWORKING = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\SWorking\Resource\Server"

# The plaintext content (with quotes, as GetStringInQuotation expects)
# UTF-8 encoded Korean text (matches compiler /execution-charset:utf-8)
plaintext = '"이 파일이 없으면 타이탄 업데이트 안돼요~"'
data = plaintext.encode('utf-8')
filesize = len(data)

print(f"UTF-8 bytes: {filesize}")
print(f"Hex: {data.hex()}")

# Header: dwVersion = 20040308 + dwType + filesize
dwType = 35
dwVersion = 20040308 + dwType + filesize

# Encode the data
encoded = bytearray(data)
for i in range(filesize):
    if i % dwType == 0:
        encoded[i] = (encoded[i] + dwType) & 0xFF
    encoded[i] = (encoded[i] + i) & 0xFF

# Calculate CRC
crc = dwType & 0xFF
for b in encoded:
    crc = (crc + b) & 0xFF

# Build the file
header = struct.pack('<III', dwVersion, dwType, filesize)
output = header + bytes([crc]) + bytes(encoded) + bytes([crc])

# Write file
outpath = os.path.join(SWORKING, "TitanServer.bin")
with open(outpath, 'wb') as f:
    f.write(output)
print(f"Written to: {outpath} ({len(output)} bytes)")

# Verify
verify = bytearray(encoded)
for i in range(filesize):
    verify[i] = (verify[i] - i) & 0xFF
    if i % dwType == 0:
        verify[i] = (verify[i] - dwType) & 0xFF
print(f"Match: {verify == data}")
