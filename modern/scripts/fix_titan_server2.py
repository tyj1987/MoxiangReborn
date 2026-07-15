"""Fix TitanServer.bin file for DistributeServer and AgentServer.
Directly constructs the correct binary format without Python string encoding issues."""
import struct
import os

SWORKING = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\SWorking\Resource\Server"

# The plaintext content in EUC-KR encoding (pre-computed)
# "이 파일이 없으면 타이탄 업데이트 안돼요~" with quotes
# Each Korean char in EUC-KR is 2 bytes
data = bytes([
    0x22,                           # opening quote "
    0xC0, 0xCC,                     # 이
    0x20,                           # space
    0xC6, 0xC4, 0xC0, 0xCF,        # 파일
    0xC0, 0xCC,                     # 이
    0x20,                           # space
    0xBE, 0xF8,                     # 없
    0xC0, 0xB8, 0xB8, 0xE9,        # 으면
    0x20,                           # space
    0xC5, 0xB8, 0xC0, 0xCC,        # 타이
    0xC5, 0xBA,                     # 탄
    0x20,                           # space
    0xBE, 0xF7, 0xB5, 0xA5,        # 업데
    0xC0, 0xCC, 0xC6, 0xAE,        # 이트
    0x20,                           # space
    0xBE, 0xC8,                     # 안
    0xB5, 0xC5,                     # 돼
    0xBF, 0xE4,                     # 요
    0x7E,                           # ~
    0x22,                           # closing quote "
])
filesize = len(data)

print(f"EUC-KR bytes: {filesize}")
print(f"Hex: {data.hex()}")

# Header: dwVersion = 20040308 + dwType + filesize (from CheckHeader)
dwType = 35  # From the original file analysis
dwVersion = 20040308 + dwType + filesize

print(f"dwVersion: {dwVersion}")
print(f"dwType: {dwType}")
print(f"dwFileSize: {filesize}")

# Encode the data (reverse of CheckCRC decode)
# CheckCRC decode:
#   for i in range(filesize):
#       crc += m_pData[i]
#       m_pData[i] -= (char)i
#       if i % dwType == 0:
#           m_pData[i] -= (char)dwType
# So to encode (reverse order of operations):
#   for i in range(filesize):
#       if i % dwType == 0:
#           encoded[i] += dwType
#       encoded[i] += i
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

print(f"\nOutput size: {len(output)} bytes")
print(f"Header hex: {header.hex()}")
print(f"CRC: 0x{crc:02X}")

# Write file
outpath = os.path.join(SWORKING, "TitanServer.bin")
with open(outpath, 'wb') as f:
    f.write(output)
print(f"\nWritten to: {outpath}")

# Verify by decoding
verify = bytearray(encoded)
for i in range(filesize):
    verify[i] = (verify[i] - i) & 0xFF
    if i % dwType == 0:
        verify[i] = (verify[i] - dwType) & 0xFF

print(f"Verification hex: {verify.hex()}")
print(f"Match: {verify == data}")
