"""Create a valid TitanServer.bin file for MapServer.

The .bin format used by CMHFile::OpenBin():
  - MHFILE_HEADER (12 bytes): dwVersion(4) + dwType(4) + dwFileSize(4)
  - crc1 (1 byte)
  - data (dwFileSize bytes, encoded)
  - crc2 (1 byte)

Encoding (from CheckCRC):
  for i in range(filesize):
      data[i] -= i
      if i % dwType == 0:
          data[i] -= dwType

The decoded data must contain: "이 파일이 없으면 타이탄 업데이트 안돼요~"
(with surrounding quotes)
"""
import struct
import os

SWORKING = r"D:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\SWorking\Resource\Server"

# The plaintext content (with quotes, as GetStringInQuotation expects)
# EUC-KR encoded Korean text
plaintext = '"이 파일이 없으면 타이탄 업데이트 안돼요~"'
data = plaintext.encode('euc-kr')
filesize = len(data)

print(f"Plaintext: {plaintext}")
print(f"EUC-KR bytes: {filesize}")
print(f"Hex: {data.hex()}")

# Header: dwVersion = 20040308 + dwType + filesize (from CheckHeader)
dwType = 35  # From the original file analysis
dwVersion = 20040308 + dwType + filesize

print(f"dwVersion: {dwVersion}")
print(f"dwType: {dwType}")
print(f"dwFileSize: {filesize}")

# Encode the data (reverse of CheckCRC decode)
encoded = bytearray(data)
for i in range(filesize):
    if i % dwType == 0:
        encoded[i] = (encoded[i] + dwType) & 0xFF
    encoded[i] = (encoded[i] + i) & 0xFF

# Calculate CRC (crc = sum of encoded data type char, but simplified)
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

decoded_text = verify.decode('euc-kr')
print(f"Verification decoded: {decoded_text}")
print(f"Match: {decoded_text == plaintext}")
