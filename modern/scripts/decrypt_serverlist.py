#!/usr/bin/env python3
"""
Decrypt/Encrypt serverlist.msl
Algorithm from ServerListManager.cpp CSimpleCodec:
  Decode: pchBufOut[n] -= m_CodecKeybuf[m_CurCodecKeyPos]
  Encode: tempBuf[n] += m_CodecKeybuf[m_CurCodecKeyPos]
Key: "yunhozzang!#&" (wraps around)
File format: [int ServerSetNum][SERVERINFO...]
SERVERINFO (pack,1): WORD kind(2) + char[16] + char[16] + WORD port(2) + WORD port(2) + WORD num(2) + DWORD conn(4) + WORD cnt(2) = 46 bytes
"""
import struct
import sys
import os

KEY = b"yunhozzang!#&"
SERVERINFO_SIZE = 46

def xor_codec(data, mode='decode'):
    """Apply CSimpleCodec cipher"""
    result = bytearray(len(data))
    key_pos = 0
    for i in range(len(data)):
        if key_pos == len(KEY):
            key_pos = 0
        if mode == 'decode':
            result[i] = (data[i] - KEY[key_pos]) & 0xFF
        else:
            result[i] = (data[i] + KEY[key_pos]) & 0xFF
        key_pos += 1
    return bytes(result)

def parse_serverinfo(data, offset):
    """Parse SERVERINFO struct"""
    # WORD wServerKind(2) + char[16] + char[16] + WORD(2) + WORD(2) + WORD(2) + DWORD(4) + WORD(2)
    fmt = '<H16s16sHHIH'  # pack(1) but individual fields have natural alignment... actually pack(1)
    # With pragma pack(1): H H16s H16s H H H I H
    # Let's use explicit offsets
    kind = struct.unpack_from('<H', data, offset)[0]
    ip_server = data[offset+2:offset+18].split(b'\x00')[0].decode('ascii', errors='replace')
    ip_user = data[offset+18:offset+34].split(b'\x00')[0].decode('ascii', errors='replace')
    port_server = struct.unpack_from('<H', data, offset+34)[0]
    port_user = struct.unpack_from('<H', data, offset+36)[0]
    server_num = struct.unpack_from('<H', data, offset+38)[0]
    conn_index = struct.unpack_from('<I', data, offset+40)[0]
    agent_cnt = struct.unpack_from('<H', data, offset+44)[0]
    return {
        'kind': kind,
        'ip_server': ip_server,
        'ip_user': ip_user,
        'port_server': port_server,
        'port_user': port_user,
        'server_num': server_num,
        'conn_index': conn_index,
        'agent_cnt': agent_cnt,
        'raw': bytearray(data[offset:offset+SERVERINFO_SIZE])
    }

def pack_serverinfo(info):
    """Pack SERVERINFO back to bytes"""
    raw = bytearray(SERVERINFO_SIZE)
    struct.pack_into('<H', raw, 0, info['kind'])
    raw[2:18] = info['ip_server'].encode('ascii').ljust(16, b'\x00')[:16]
    raw[18:34] = info['ip_user'].encode('ascii').ljust(16, b'\x00')[:16]
    struct.pack_into('<H', raw, 34, info['port_server'])
    struct.pack_into('<H', raw, 36, info['port_user'])
    struct.pack_into('<H', raw, 38, info['server_num'])
    struct.pack_into('<I', raw, 40, info['conn_index'])
    struct.pack_into('<H', raw, 44, info['agent_cnt'])
    return raw

SERVER_KINDS = {
    1: 'DISTRIBUTE_SERVER',
    2: 'AGENT_SERVER',
    3: 'MAP_SERVER',
    4: 'MONITOR_SERVER',
    5: 'BUDDYAUTH_SERVER',
}

def main():
    # Use absolute path to avoid encoding issues
    msl_path = r"d:\墨香全套源代码（源码+资源+客户端+服务端+教程）\墨香【源码】\SWorking\ServerSet\1\serverlist.msl"
    
    with open(msl_path, 'rb') as f:
        raw = f.read()
    
    print(f"File size: {len(raw)} bytes")
    
    # Decrypt
    decrypted = xor_codec(raw, 'decode')
    
    # Parse header
    server_set_num = struct.unpack_from('<i', decrypted, 0)[0]
    print(f"ServerSetNum: {server_set_num}")
    
    # Parse SERVERINFO entries
    offset = 4  # skip server_set_num
    entries = []
    while offset + SERVERINFO_SIZE <= len(decrypted):
        info = parse_serverinfo(decrypted, offset)
        if info['port_server'] == 0 and info['ip_server'] == '':
            break
        entries.append(info)
        offset += SERVERINFO_SIZE
    
    print(f"\nFound {len(entries)} server entries:")
    for i, e in enumerate(entries):
        kind_name = SERVER_KINDS.get(e['kind'], f'UNKNOWN({e["kind"]})')
        print(f"  [{i}] Kind={kind_name} ({e['kind']})")
        print(f"      IP(Server)={e['ip_server']}:{e['port_server']}")
        print(f"      IP(User)={e['ip_user']}:{e['port_user']}")
        print(f"      ServerNum={e['server_num']} ConnIdx={e['conn_index']} AgentCnt={e['agent_cnt']}")
    
    # Backup
    backup_path = msl_path + '.bak'
    if not os.path.exists(backup_path):
        with open(backup_path, 'wb') as f:
            f.write(raw)
        print(f"\nBackup saved: {backup_path}")
    
    # Replace IPs
    OLD_IP = "61.191.190.61"
    NEW_IP = "127.0.0.1"
    modified = False
    for e in entries:
        if OLD_IP in (e['ip_server'], e['ip_user']):
            print(f"\nReplacing {OLD_IP} -> {NEW_IP} in server kind {e['kind']} ({SERVER_KINDS.get(e['kind'], '?')})")
            if e['ip_server'] == OLD_IP:
                e['ip_server'] = NEW_IP
                modified = True
            if e['ip_user'] == OLD_IP:
                e['ip_user'] = NEW_IP
                modified = True
    
    if not modified:
        print(f"\nNo occurrences of {OLD_IP} found. Checking all IPs...")
        all_ips = set()
        for e in entries:
            all_ips.add(e['ip_server'])
            all_ips.add(e['ip_user'])
        print(f"All IPs in file: {all_ips}")
        print("Will replace ALL non-localhost IPs with 127.0.0.1")
        for e in entries:
            if e['ip_server'] not in ('127.0.0.1', 'localhost', ''):
                print(f"  Replacing server IP: {e['ip_server']} -> {NEW_IP} (kind={e['kind']})")
                e['ip_server'] = NEW_IP
                modified = True
            if e['ip_user'] not in ('127.0.0.1', 'localhost', ''):
                print(f"  Replacing user IP: {e['ip_user']} -> {NEW_IP} (kind={e['kind']})")
                e['ip_user'] = NEW_IP
                modified = True
    
    if modified:
        # Rebuild encrypted data
        new_data = struct.pack('<i', server_set_num)
        for e in entries:
            new_data += pack_serverinfo(e)
        
        # Encrypt
        encrypted = xor_codec(new_data, 'encode')
        
        # Write
        with open(msl_path, 'wb') as f:
            f.write(encrypted)
        print(f"\nFile re-encrypted with new IPs: {msl_path}")
        
        # Verify by re-reading
        with open(msl_path, 'rb') as f:
            verify_raw = f.read()
        verify_dec = xor_codec(verify_raw, 'decode')
        verify_num = struct.unpack_from('<i', verify_dec, 0)[0]
        print(f"Verification - ServerSetNum: {verify_num}")
        voff = 4
        while voff + SERVERINFO_SIZE <= len(verify_dec):
            ve = parse_serverinfo(verify_dec, voff)
            if ve['port_server'] == 0 and ve['ip_server'] == '':
                break
            kind_name = SERVER_KINDS.get(ve['kind'], f'UNKNOWN({ve["kind"]})')
            print(f"  {kind_name}: Server={ve['ip_server']}:{ve['port_server']} User={ve['ip_user']}:{ve['port_user']}")
            voff += SERVERINFO_SIZE
    else:
        print("\nNo modifications needed.")

if __name__ == '__main__':
    main()
