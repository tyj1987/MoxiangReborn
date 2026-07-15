"""Decode serverlist.msl (XOR encrypted with key 'yunhozzang!#&')"""
import struct
import sys
import os

key = b'yunhozzang!#&'
ENTRY_SIZE = 46  # sizeof(SERVERINFO) with #pragma pack(push,1)

def xor_decode(data, key):
    decoded = bytearray(len(data))
    kp = 0
    for i in range(len(data)):
        decoded[i] = data[i] ^ key[kp]
        kp = (kp + 1) % len(key)
    return decoded

def decode_serverlist(filepath):
    with open(filepath, 'rb') as f:
        data = f.read()
    decoded = xor_decode(data, key)

    server_set_num = struct.unpack('<i', decoded[:4])[0]
    
    kind_names = {1: 'DISTRIBUTE', 2: 'AGENT', 3: 'MAP', 4: 'MONITOR', 5: 'BUDDYAUTH'}
    
    results = []
    results.append(f"File: {os.path.basename(filepath)}")
    results.append(f"Server Set Num: {server_set_num}")
    results.append(f"File size: {len(data)} bytes, Entries: {(len(data)-4)//ENTRY_SIZE}")
    results.append("")
    
    offset = 4
    entry_num = 0
    while offset + ENTRY_SIZE <= len(decoded):
        chunk = decoded[offset:offset+ENTRY_SIZE]
        wServerKind = struct.unpack('<H', chunk[0:2])[0]
        raw_ip_s = chunk[2:18]
        raw_ip_u = chunk[18:34]
        null_pos_s = raw_ip_s.find(0)
        if null_pos_s < 0: null_pos_s = 16
        szIPForServer = raw_ip_s[:null_pos_s].decode('ascii', errors='replace')
        null_pos_u = raw_ip_u.find(0)
        if null_pos_u < 0: null_pos_u = 16
        szIPForUser = raw_ip_u[:null_pos_u].decode('ascii', errors='replace')
        wPortForServer = struct.unpack('<H', chunk[34:36])[0]
        wPortForUser = struct.unpack('<H', chunk[36:38])[0]
        wServerNum = struct.unpack('<H', chunk[38:40])[0]
        
        kind_name = kind_names.get(wServerKind, f'?{wServerKind}')
        results.append(f"[{entry_num:2d}] Kind={kind_name:10s}({wServerKind:2d}) Num={wServerNum:3d} Svr={szIPForServer:16s}:{wPortForServer:5d} Usr={szIPForUser:16s}:{wPortForUser:5d}")
        
        offset += ENTRY_SIZE
        entry_num += 1
    
    return "\n".join(results)

if __name__ == "__main__":
    base = r"d:\墨香全套源代码（源码+资源+客户端+服务端+教程)"
    files = [
        os.path.join(base, r"墨香【源码】\SWorking\ServerSet\1\serverlist.msl"),
        os.path.join(base, r"deploy\server\ServerSet\1\serverlist.msl"),
    ]
    
    output = []
    for f in files:
        if os.path.exists(f):
            output.append(decode_serverlist(f))
            output.append("")
            output.append("=" * 80)
            output.append("")
    
    result = "\n".join(output)
    
    # Write to file
    outfile = os.path.join(base, r"modern\scratch\serverlist_decoded.txt")
    os.makedirs(os.path.dirname(outfile), exist_ok=True)
    with open(outfile, 'w', encoding='utf-8') as f:
        f.write(result)
    
    print(f"Written to {outfile}")
