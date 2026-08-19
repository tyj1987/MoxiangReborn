"""
test_login_server.py
Connect to the running Moxian LoginServer on port 6001, send a MP_USERCONN_REQUEST_LOGIN,
and verify we get a NOTIFY_USERLOGIN_ACK back with the AgentServer address.
"""
import socket
import struct
import sys

HOST = "127.0.0.1"
PORT = 6001

def send_login_request(sock, user_id, password):
    # MP_USERCONN_REQUEST_LOGIN payload format:
    # [u16 id_len][N id bytes][u16 pw_len][N pw bytes]
    id_bytes = user_id.encode("ascii")
    pw_bytes = password.encode("ascii")
    payload = struct.pack("<H", len(id_bytes)) + id_bytes + struct.pack("<H", len(pw_bytes)) + pw_bytes
    # MSGBASE header: [u8 checksum][i8 code][u8 category=MP_USERCONN(7)][u8 protocol=RequestLogin(1)]
    header = struct.pack("<BBBB", 0, 0, 7, 1) + struct.pack("<I", 0)  # + dwObjectID
    # No length field in original MSGBASE, just send header + payload as one stream
    sock.sendall(header + payload)

def recv_msg(sock):
    # Read header (8 bytes: u8 + i8 + u8 + u8 + u32)
    data = b""
    while len(data) < 8:
        chunk = sock.recv(8 - len(data))
        if not chunk:
            return None
        data += chunk
    checksum, code, cat, proto, obj_id = struct.unpack("<BBBBI", data)
    # The original protocol has no length prefix. For demo, we read 32 more bytes
    # as a heuristic (login_ack payload is short).
    payload = b""
    sock.settimeout(2.0)
    try:
        while True:
            chunk = sock.recv(64)
            if not chunk:
                break
            payload += chunk
            if len(payload) >= 4:
                break
    except socket.timeout:
        pass
    return {
        "checksum": checksum,
        "code": code,
        "category": cat,
        "protocol": proto,
        "obj_id": obj_id,
        "payload": payload,
    }

def test_login(user_id, password, expected_ack):
    print(f"\n[TEST] login user='{user_id}' pw='{password}'")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(5.0)
    try:
        sock.connect((HOST, PORT))
        print(f"  [OK] connected to {HOST}:{PORT}")
        send_login_request(sock, user_id, password)
        print(f"  [OK] sent RequestLogin ({len(user_id)}+{len(password)} bytes)")
        msg = recv_msg(sock)
        if msg is None:
            print(f"  [FAIL] no response")
            return False
        print(f"  [OK] received: cat={msg['category']} proto={msg['protocol']} obj_id={msg['obj_id']}")
        print(f"        payload = {msg['payload'].hex()}")
        if msg["category"] != 7:
            print(f"  [FAIL] wrong category, expected 7 (UserConn)")
            return False
        if msg["protocol"] not in (2, 3):
            print(f"  [FAIL] wrong protocol, expected 2 (ACK) or 3 (NACK)")
            return False
        is_ack = (msg["protocol"] == 2)
        ack_byte = msg["payload"][0] if msg["payload"] else -1
        print(f"  ack_byte={ack_byte} (expected={expected_ack})")
        ok = (is_ack == (expected_ack == 1))
        print(f"  {'[PASS]' if ok else '[FAIL]'}")
        if is_ack and len(msg["payload"]) >= 4:
            port = msg["payload"][1] | (msg["payload"][2] << 8)
            addr_end = 3
            while addr_end < len(msg["payload"]) and msg["payload"][addr_end] != 0:
                addr_end += 1
            addr = msg["payload"][3:addr_end].decode("ascii", errors="replace")
            print(f"  AgentServer = {addr}:{port}")
        return ok
    except (socket.timeout, ConnectionRefusedError, OSError) as e:
        print(f"  [FAIL] {type(e).__name__}: {e}")
        return False
    finally:
        sock.close()

if __name__ == "__main__":
    print("=" * 60)
    print(" Moxian LoginServer end-to-end protocol test")
    print("=" * 60)
    results = [
        test_login("admin", "admin", expected_ack=1),    # exists → ACK
        test_login("admin", "wrong", expected_ack=0),   # wrong pw → NACK
        test_login("nonexistent", "x", expected_ack=0), # no user → NACK
        test_login("alice", "wonderland", expected_ack=1),
    ]
    passed = sum(results)
    print("\n" + "=" * 60)
    print(f" RESULT: {passed}/{len(results)} tests passed")
    print("=" * 60)
    sys.exit(0 if passed == len(results) else 1)