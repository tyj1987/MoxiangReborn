#!/usr/bin/env python3
"""
verify_servers_e2e.py — Phase B end-to-end smoke test.

Boots the three modern servers (mxh_login_server, mxh_agent_server,
mxh_map_server) with SQLite backends, simulates a legacy Moxian
client through Distribute → Agent → Map, and reports PASS/FAIL.

What it covers (Phase B.1-B.6 E2E):
  * LoginServer listens on :6001 + sends DistConnectSuccess on connect.
  * Client sends RequestLogin (proto=1) with id+password; server
    validates against the SQLite chr_log_info table and replies
    NotifyUserLoginAck (proto=2) carrying the AgentServer address.
  * Client connects to AgentServer :7001 and sends
    CharacterListSyn (proto=9); server replies CharacterListAck
    (proto=12) with a (possibly empty) character list from the DB.
  * Client connects to MapServer :8001 and sends GameInSyn
    (proto=28); server replies GameInAck (proto=29) with a
    SEND_HERO_TOTALINFO payload (~3000 bytes).

The test is non-destructive: it writes its DBs to a temp directory
under modern/scratch/ and removes them on success. The servers are
spawned as subprocesses and killed on exit.

Usage:
  python modern/scripts/verify_servers_e2e.py [--build-dir BUILD_DIR]
                                              [--keep-on-fail]

Exit codes:
  0  — all 4 protocol steps passed
  1  — server failed to start within the timeout
  2  — protocol step failed (printed which)
  3  — usage error
"""

import argparse
import os
import shutil
import signal
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path
from typing import Optional, Tuple


# ---------------------------------------------------------------------------
# Protocol constants (must match modern/include/mxh/proto/protocol.hpp)
# ---------------------------------------------------------------------------
CATEGORY_USERCONN = 7   # legacy MP_USERCONN category id
# UserConnProtocol sub-protocols
DIST_CONNECT_SUCCESS     = 0   # D → C on connect (DistributeServer)
AGENT_CONNECT_SUCCESS    = 8   # A → C on connect (AgentServer)
REQUEST_LOGIN            = 1
NOTIFY_USER_LOGIN_ACK    = 2
CHARACTER_LIST_SYN       = 9
CHARACTER_LIST_ACK       = 12
GAME_IN_SYN              = 28
GAME_IN_ACK              = 29

# LoginServer defaults from modern/tools/MoxianLoginServer/main.cpp.
LOGIN_HOST = "127.0.0.1"
LOGIN_PORT = 6001
# AgentServer defaults
AGENT_PORT = 7001
# MapServer defaults
MAP_PORT = 8001

# Server readiness wait (seconds)
WAIT_FOR_PORT_TIMEOUT = 10.0
WAIT_FOR_PORT_POLL = 0.1


# ---------------------------------------------------------------------------
# Legacy 4DyuchiNET protocol message framing
#
# The 3 modern servers run with --legacy (4DyuchiNET compat), so wire format is:
#   [2B length LE u16] [8B MSGBASE: 1B checksum | 1B code | 1B category |
#                       1B protocol | 4B object_id LE] [payload]
# Length prefix covers the 8B MSGBASE header + payload (NOT itself).
# MSGBASE is byte-compatible with the original 墨香 client (MSGBASE.h).
# ---------------------------------------------------------------------------
def build_message(category: int, protocol: int, object_id: int = 0,
                  payload: bytes = b"", checksum: int = 0,
                  code: int = 0) -> bytes:
    """Return a legacy-framed message: [2B len LE][8B MSGBASE][payload].

    MSGBASE layout (8 bytes, packed, no padding):
      [0]  checksum  (u8)
      [1]  code      (u8)
      [2]  category  (u8)
      [3]  protocol  (u8)
      [4..7] object_id (u32 LE)
    """
    msb = struct.pack("<BBBB", checksum, code, category, protocol) + \
          struct.pack("<I", object_id)
    assert len(msb) == 8, f"MSGBASE must be 8 bytes, got {len(msb)}"
    body = msb + payload
    return struct.pack("<H", len(body)) + body


def recv_exact(sock: socket.socket, n: int, timeout: float = 3.0) -> bytes:
    sock.settimeout(timeout)
    buf = b""
    while len(buf) < n:
        chunk = sock.recv(n - len(buf))
        if not chunk:
            raise ConnectionError(
                f"peer closed after {len(buf)}/{n} bytes")
        buf += chunk
    return buf


def recv_message(sock: socket.socket, timeout: float = 3.0) -> Tuple[int, int, int, bytes]:
    """Receive one legacy-framed message; return (cat, proto, obj, payload).

    Note: the 1B checksum and 1B code are not exposed (clients typically
    ignore them and the modern handlers don't read them back either).
    """
    length_prefix = recv_exact(sock, 2, timeout)
    (msg_len,) = struct.unpack("<H", length_prefix)
    body = recv_exact(sock, msg_len, timeout)
    if len(body) < 8:
        raise ConnectionError(f"legacy frame body too short: {len(body)}")
    # 8B MSGBASE: [cksum:1][code:1][cat:1][proto:1][obj_id:4 LE]
    # category and protocol are single bytes (u8); object_id is u32 LE.
    category = body[2]
    protocol = body[3]
    (object_id,) = struct.unpack("<I", body[4:8])
    payload = body[8:]
    return category, protocol, object_id, payload


# ---------------------------------------------------------------------------
# Process management
# ---------------------------------------------------------------------------
class ServerProc:
    def __init__(self, name: str, cmd: list, cwd: Optional[str] = None):
        self.name = name
        self.cmd = cmd
        self.cwd = cwd
        self.proc: Optional[subprocess.Popen] = None
        self.log_path: Optional[Path] = None

    def start(self, log_dir: Path) -> None:
        self.log_path = log_dir / f"{self.name}.log"
        log_fh = open(self.log_path, "wb")
        # On Windows, CREATE_NEW_PROCESS_GROUP lets us kill the
        # process tree cleanly.  Popen.close_fds ensures no fd leak.
        creationflags = 0
        if os.name == "nt":
            creationflags = subprocess.CREATE_NEW_PROCESS_GROUP
        self.proc = subprocess.Popen(
            self.cmd,
            cwd=self.cwd,
            stdout=log_fh,
            stderr=subprocess.STDOUT,
            stdin=subprocess.DEVNULL,
            close_fds=True,
            creationflags=creationflags,
        )

    def kill(self) -> None:
        if self.proc is None or self.proc.poll() is not None:
            return
        try:
            if os.name == "nt":
                # CTRL_BREAK_EVENT is the polite kill that gives the
                # process a chance to clean up.  Fallback to terminate().
                self.proc.send_signal(signal.CTRL_BREAK_EVENT)
                try:
                    self.proc.wait(timeout=2.0)
                    return
                except subprocess.TimeoutExpired:
                    pass
            self.proc.terminate()
            try:
                self.proc.wait(timeout=2.0)
            except subprocess.TimeoutExpired:
                self.proc.kill()
        except Exception as exc:
            print(f"  [warn] kill {self.name} failed: {exc}",
                  file=sys.stderr)


def wait_for_port(host: str, port: int, timeout: float) -> bool:
    """Poll TCP connect until success or timeout."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            with socket.create_connection((host, port), timeout=0.5):
                return True
        except OSError:
            time.sleep(WAIT_FOR_PORT_POLL)
    return False


# ---------------------------------------------------------------------------
# E2E test protocol
# ---------------------------------------------------------------------------
def step_distribute(host: str, port: int, user_id: str, password: str) \
        -> Tuple[str, int, int, int]:
    """
    Connect to LoginServer, expect DistConnectSuccess on connect,
    send RequestLogin, expect NotifyUserLoginAck with agent_addr/port
    and user_idx.

    Legacy 4DyuchiNET request format (login_handler.cpp:handle_legacy_login):
        payload = [AuthKey: u32 LE] [id: char[17]] [pw: char[17]]
        total   = 38 bytes (4 + 17 + 17)

    LoginAck payload (login_handler.cpp:make_login_ack legacy path, 23 bytes):
        [agentip: char[16]] [agentport: u16 LE] [userIdx: u32 LE]
        [cbUserLevel: u8]
    """
    print(f"  [1/3] Distribute connect {host}:{port} ...")
    s = socket.create_connection((host, port), timeout=3.0)
    cat, proto, obj, payload = recv_message(s)
    if (cat, proto) != (CATEGORY_USERCONN, DIST_CONNECT_SUCCESS):
        s.close()
        raise RuntimeError(
            f"expected (UserConn, DistConnectSuccess), got "
            f"(cat={cat}, proto={proto})")
    auth_key = obj
    print(f"        got DistConnectSuccess auth_key={auth_key}")

    # Build legacy RequestLogin payload: [AuthKey:4B][id:17B][pw:17B] = 38 bytes.
    pl = bytearray(38)
    struct.pack_into("<I", pl, 0, auth_key)
    # 17-byte null-padded id and pw
    id_b = user_id.encode("utf-8")[:17]
    pw_b = password.encode("utf-8")[:17]
    pl[4:4 + len(id_b)] = id_b
    pl[21:21 + len(pw_b)] = pw_b
    s.sendall(build_message(CATEGORY_USERCONN, REQUEST_LOGIN, 0, bytes(pl)))
    print(f"        sent RequestLogin id={user_id} auth_key={auth_key} (38B payload)")

    cat, proto, obj, payload = recv_message(s)
    s.close()
    if (cat, proto) != (CATEGORY_USERCONN, NOTIFY_USER_LOGIN_ACK):
        raise RuntimeError(
            f"expected (UserConn, NotifyUserLoginAck), got "
            f"(cat={cat}, proto={proto}, obj={obj}, payload={payload!r})")
    if len(payload) < 23:
        raise RuntimeError(
            f"LoginAck payload too short: {len(payload)} bytes (need >= 23)")

    # Parse 23-byte legacy ack:
    #   [0..16]  agentip (16 bytes, null-padded)
    #   [16..18] agentport (u16 LE)
    #   [18..22] userIdx   (u32 LE)
    #   [22]     cbUserLevel
    agent_addr_bytes = payload[0:16]
    agent_addr = agent_addr_bytes.split(b"\x00", 1)[0].decode("utf-8", "replace")
    agent_port = struct.unpack("<H", payload[16:18])[0]
    user_idx = struct.unpack("<I", payload[18:22])[0]
    user_level = payload[22]
    print(f"        got LoginAck agent={agent_addr}:{agent_port} "
          f"user_idx={user_idx} level={user_level}")
    return agent_addr, agent_port, user_idx, auth_key


def step_agent(host: str, port: int, user_id: str, user_idx: int,
               dist_auth_key: int) -> int:
    """
    Connect to AgentServer, send CharacterListSyn, expect CharacterListAck.

    Legacy CharacterListSyn payload (agent_handler.cpp:526-538):
        [user_id: u32 LE] [dist_auth_key: u32 LE]   = 8 bytes

    Returns the user_id (which is also the player_id for MapServer).
    Note: GameInSyn via Agent requires a prior CharacterSelectSyn
    (proto=16) to set conn_char_ids_; that's an Agent↔Map forwarding
    test (Phase 9), exercised separately. Here we only verify the
    AgentServer's character-list reply works against the live DB.
    """
    print(f"  [2/3] Agent connect {host}:{port} ...")
    s = socket.create_connection((host, port), timeout=3.0)
    cat, proto, obj, payload = recv_message(s)
    # AgentServer uses AgentConnectSuccess (proto=8) for its connect ack,
    # not DistConnectSuccess (proto=0) like DistributeServer.
    if (cat, proto) != (CATEGORY_USERCONN, AGENT_CONNECT_SUCCESS):
        s.close()
        raise RuntimeError(
            f"expected (UserConn, AgentConnectSuccess) for agent, got "
            f"(cat={cat}, proto={proto})")
    agent_auth_key = obj
    print(f"        got AgentConnectSuccess auth_key={agent_auth_key}")

    # Send CharacterListSyn: [user_id:4B LE][dist_auth_key:4B LE] = 8 bytes
    pl = struct.pack("<II", user_idx, dist_auth_key)
    s.sendall(build_message(CATEGORY_USERCONN, CHARACTER_LIST_SYN, user_idx, pl))
    print(f"        sent CharacterListSyn user_idx={user_idx}")

    cat, proto, obj, payload = recv_message(s)
    s.close()
    if (cat, proto) != (CATEGORY_USERCONN, CHARACTER_LIST_ACK):
        raise RuntimeError(
            f"expected (UserConn, CharacterListAck), got "
            f"(cat={cat}, proto={proto}, obj={obj})")
    print(f"        got CharacterListAck payload={len(payload)} bytes")
    return user_idx


def step_map(host: str, port: int, player_id: int) -> None:
    """
    Connect to MapServer directly, send GameInSyn, expect GameInAck with
    SEND_HERO_TOTALINFO payload (>= 1000 bytes).

    This bypasses the AgentServer's Phase 9 forwarding (which requires
    CharacterSelectSyn first). MapServer reads MSGBASE.object_id as
    player_id and replies with default CharData even when chrid is not
    in character_info.

    Legacy GameInSyn payload: empty (server reads player_id from
    MSGBASE.object_id; payload is reserved for legacy channel/level
    fields which the modern server ignores for stub mode).
    """
    print(f"  [3/3] Map connect {host}:{port} ...")
    s = socket.create_connection((host, port), timeout=3.0)
    # MapServer's on_connect does NOT push a DistConnectSuccess (unlike
    # DistributeServer/AgentServer); it just logs and waits for the
    # client to send GameInSyn. Skip the recv and go straight to send.
    print(f"        connected (MapServer doesn't push connect-ack)")

    # GameInSyn with MSGBASE.object_id = player_id, empty payload.
    s.sendall(build_message(CATEGORY_USERCONN, GAME_IN_SYN, player_id, b""))
    print(f"        sent GameInSyn player_id={player_id}")

    cat, proto, obj, payload = recv_message(s)
    s.close()
    if (cat, proto) != (CATEGORY_USERCONN, GAME_IN_ACK):
        raise RuntimeError(
            f"expected (UserConn, GameInAck), got "
            f"(cat={cat}, proto={proto}, obj={obj})")
    if len(payload) < 1000:
        raise RuntimeError(
            f"GameInAck payload suspiciously small ({len(payload)} bytes); "
            f"expected >= 1000 for SEND_HERO_TOTALINFO")
    print(f"        got GameInAck payload={len(payload)} bytes "
          f"(>= 1000 -> SEND_HERO_TOTALINFO present)")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--build-dir", default=None,
                        help="Path to cmake build dir (auto-detected if omitted)")
    parser.add_argument("--keep-on-fail", action="store_true",
                        help="Keep scratch/ on failure for forensic inspection")
    args = parser.parse_args()

    # Auto-detect build dir from the script's own location.
    # Script lives in modern/scripts/, build dir is modern/build/.
    script_dir = Path(__file__).resolve().parent
    if args.build_dir is None:
        # Default: ../build (modern/build — the standard cmake output dir)
        candidate = (script_dir / ".." / "build").resolve()
        if (candidate / "tools" / "MoxianLoginServer" / "Debug").is_dir():
            args.build_dir = candidate
        else:
            # Fall back to Release
            args.build_dir = candidate
    build_dir = Path(args.build_dir).resolve()
    tools_dir = build_dir / "tools"
    cfg_dir_name = "Debug" if (tools_dir / "MoxianLoginServer" / "Debug").is_dir() else "Release"
    cfg_dir = tools_dir / "MoxianLoginServer" / cfg_dir_name
    if not cfg_dir.is_dir():
        print(f"  [fatal] build tools not found at {cfg_dir}", file=sys.stderr)
        print(f"          re-run with --build-dir pointing at the cmake build root",
              file=sys.stderr)
        return 3
    login_exe   = cfg_dir / "mxh_login_server.exe"
    agent_exe   = tools_dir / "MoxianAgentServer"  / cfg_dir_name / "mxh_agent_server_CHINA.exe"
    map_exe     = tools_dir / "MoxianMapServer"    / cfg_dir_name / "mxh_map_server_CHINA.exe"
    for p in (login_exe, agent_exe, map_exe):
        if not p.is_file():
            print(f"  [fatal] missing {p}", file=sys.stderr)
            return 3

    # Scratch dir for DBs and server logs.
    scratch = (script_dir / ".." / "scratch" / "e2e_servers").resolve()
    if scratch.exists():
        shutil.rmtree(scratch, ignore_errors=True)
    scratch.mkdir(parents=True, exist_ok=True)
    log_dir = scratch / "logs"
    log_dir.mkdir(exist_ok=True)
    db_login = scratch / "login.db"
    db_agent = scratch / "agent.db"
    db_map   = scratch / "map.db"

    # Spawn servers.  LoginServer uses --init-schema so the test user is
    # already in chr_log_info; the seed user is id="test" pw="test".
    servers = [
        ServerProc("login",
                    [str(login_exe),
                     "--port", str(LOGIN_PORT),
                     "--db", str(db_login),
                     "--agent-addr", "127.0.0.1",
                     "--agent-port", str(AGENT_PORT),
                     "--init-schema",
                     "--legacy"]),
        ServerProc("agent",
                    [str(agent_exe),
                     "--port", str(AGENT_PORT),
                     "--db", str(db_agent),
                     "--legacy",
                     "--map-server", f"127.0.0.1:{MAP_PORT}"]),
        ServerProc("map",
                    [str(map_exe),
                     "--port", str(MAP_PORT),
                     "--map", "12",  # default 12 = village map
                     "--db", str(db_map),
                     "--legacy"]),
    ]
    print(f"  [boot] starting 3 servers from {cfg_dir_name} ...")
    for s in servers:
        s.start(log_dir)
        print(f"        started {s.name} (pid={s.proc.pid})")

    try:
        # Wait for each port to come up.
        port_for = {"login": LOGIN_PORT, "agent": AGENT_PORT, "map": MAP_PORT}
        for s in servers:
            if not wait_for_port("127.0.0.1", port_for[s.name], WAIT_FOR_PORT_TIMEOUT):
                raise RuntimeError(
                    f"{s.name} did not start listening on "
                    f":{port_for[s.name]} within {WAIT_FOR_PORT_TIMEOUT}s; "
                    f"see {s.log_path}")
            print(f"        {s.name} listening on :{port_for[s.name]}")

        # 1. Distribute login
        agent_addr, agent_port, user_idx, dist_auth_key = step_distribute(
            LOGIN_HOST, LOGIN_PORT, "test", "test")
        if agent_port != AGENT_PORT:
            print(f"  [warn] login ack agent port {agent_port} != expected {AGENT_PORT}",
                  file=sys.stderr)

        # 2. Agent character list (basic DB roundtrip; full Agent→Map
        #    forwarding needs CharacterSelectSyn which is a separate
        #    test in the Agent↔Map forwarding smoke).
        player_id = step_agent(agent_addr, agent_port, "test", user_idx,
                                dist_auth_key)

        # 3. MapServer GameInSyn → GameInAck (direct, bypassing Agent).
        step_map("127.0.0.1", MAP_PORT, player_id)

        print("\n  [OK] Phase B e2e: all 3 protocol steps passed.")
        return 0

    except Exception as exc:
        print(f"\n  [FAIL] {exc}", file=sys.stderr)
        print(f"        server logs in: {log_dir}", file=sys.stderr)
        return 2

    finally:
        for s in reversed(servers):
            s.kill()
        if (not args.keep_on_fail) and (scratch.exists()) and (sys.exc_info() == (None, None, None)):
            # Only auto-clean on success; on failure keep for debugging.
            shutil.rmtree(scratch, ignore_errors=True)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(1)
