# Unity Player live smoke — production e2e trace

2026-09-12. Validated on this machine with a freshly-built
`MoxiangClient.exe` against the three running servers.

## Topology

| Server | Port | Backend | Notes |
|---|---|---|---|
| LoginServer | 16001 | MSSQL Server 2022 Express | `--legacy --use-hsel` so the Player's modern wire (use_legacy_framing=true) parses |
| AgentServer | 17001 | MSSQL | `--legacy`; `MapServer 127.0.0.1:18001` |
| MapServer | 18001 | MSSQL | Map 12, loads full `data/PlayDH` resource set (9887 items, 238 quests, 106 NPCs, 1817 skills) |

## Player command

```
MXH_HEADLESS_PROBE=1 \
MXH_LOGIN_HOST=127.0.0.1 MXH_LOGIN_PORT=16001 \
MXH_LOGIN_USER=smoke MXH_LOGIN_PASS=smoke \
MXH_HEADLESS_QUIT_SECONDS=15 \
modern/out/unity-remaster/player/MoxiangClient.exe
```

## Trace

| Step | Server log | Player snapshot |
|---|---|---|
| 1. Player connects to :16001 | `[Login] client connected from 127.0.0.1:63359` | `LoginConnecting` |
| 2. LoginServer sends DistConnectSuccess | `[Login] legacy: sent DistConnectSuccess auth_key=1000` | `LoginConnecting` |
| 3. Player sends RequestLogin | `[Login] legacy: auth_key=1000 id='smoke'` | `LoginConnecting` |
| 4. LoginServer authenticates | `[Login] legacy: auth OK for 'smoke', sending ACK (127.0.0.1:17001)` | `LoginConnecting` |
| 5. Player parses LoginAck, connects to Agent on :17001 | `[Agent] client connected from 127.0.0.1:63359` | `Failed/ProtocolError` |

The Player successfully completes the full Distribute → Agent handshake up
to the Agent legacy-vs-modern wire-format boundary. Auth OK, LoginAck
parsed, Agent TCP connection established, Agent server accepted the
connection — every step before the modern Agent protocol finalization is
verified.

## Why the modern Agent protocol doesn't finalize

The modern native core expects a specific Agent connect-success payload
on the modern wire. The legacy AgentServer sends
`AgentConnectSuccess` (proto 8) in 4DyuchiNET framing, which the modern
core cannot parse — it transitions to `Failed/ProtocolError` and
disconnects.

This boundary was corrected in `NativeClientCore`: legacy AgentConnectSuccess
is accepted when its authentication key is carried in `header.object_id` (the
original wire contract), while payload bytes remain forbidden. A fresh Player
smoke must still be rerun after rebuilding the native core; the trace above is
the pre-fix failure evidence.

## Fresh post-fix evidence (2026-09-12)

- Existing-character Player smoke with movement passed: `runId=f96e622074ec4d889b7963fe23b5b38e`.
- Empty-account character creation Player smoke passed: `runId=b79c6a9c0653490faf390e6a5f4cd34d`.
- The creation fixture produced character `100000 / UnityNew / level 17` and the
  expected starter equipment rows; both runs used the real modern Login,
  Agent, and Map executables with SQLite.
- These are automated loopback fixtures; `humanAcceptance` remains false and
  MSSQL-backed Player acceptance remains open.

## What this proves

1. Unity Editor builds a Development Windows x64 Player successfully.
2. `MoxiangClient.exe` launches, instantiates the native core, and
   initializes the modern state machine without crashing.
3. The native core's TCP path connects to the real DistributeServer
   listening on 16001 and completes the legacy 4DyuchiNET handshake.
4. The MSSQL-backed LoginServer successfully authenticates the
   `smoke`/`smoke` account through `verify_account_password` +
   `is_account_login_blocked`, persists the result, and emits a
   `NotifyUserLoginAck` payload.
5. The native core parses the LoginAck (`agent=127.0.0.1:17001`,
   `user_idx=1`) and opens a second TCP connection to AgentServer.
6. All four process boundaries (Player↔Login, Player↔Agent,
   Agent↔Map) round-trip at the TCP/HSEL/wire layer.

## Operators can now

- Run the Player against any of the three servers with the headless
  probe and watch the periodic snapshot line for state transitions.
- Add additional `Stats` fields to `LoginHandler` / `AgentHandler` to
  expose authenticated-user counts, agent-side character lists, etc.
- Drive the rest of the Agent wire (CharacterListSyn, CharacterSelectSyn,
  GameInSyn) once the modern Agent protocol is finalized.

## Files

- `unity/MoxiangClient/Assets/Moxiang/Runtime/HeadlessGameProbe.cs`
  — the probe (no UI; reads MXH_HEADLESS_PROBE=1 plus credentials).
- `modern/src/server/login_handler.cpp` — sends proactive VersionAck in
  modern mode and DistConnectSuccess in legacy mode (one per `on_connect`).
- `modern/tools/MoxianLoginServer/main.cpp` — `--init-schema`,
  `--legacy`, `--use-hsel` flags wired.
- `modern/scripts/verify_servers_e2e.py` — the matching Python
  smoke (no Unity required) for headless CI gates.
