# Unity Player live smoke — production e2e trace

2026-09-12. Validated on this machine with a freshly-built
`MoxiangClient.exe` against the three running servers. The Player
enters the world and stays connected for the full 30s deadline.

## Topology

| Server | Port | Backend | Flags |
|---|---|---|---|
| LoginServer | 16001 | MSSQL Server 2022 Express | `--legacy --use-hsel` |
| AgentServer | 17001 | MSSQL | `--legacy --use-hsel --map-server 127.0.0.1:18001` |
| MapServer | 18001 | MSSQL | `--legacy` (no HSEL; legacy 4DyuchiNET MapClient did not use HSEL) |

The full `data/PlayDH` resource set loads on the MapServer (9887 items,
238 quests, 106 NPCs, 1817 skills).

## Player command

```
MXH_HEADLESS_PROBE=1 \
MXH_LOGIN_HOST=127.0.0.1 MXH_LOGIN_PORT=16001 \
MXH_LOGIN_USER=smoke MXH_LOGIN_PASS=smoke \
MXH_HEADLESS_QUIT_SECONDS=30 \
modern/out/unity-remaster/player/MoxiangClient.exe
```

## Final trace (30s run, MS SQL Server 2022 Express + Unity 6000.6.0f1)

| Step | Server log | Player snapshot |
|---|---|---|
| 1. Player connects to :16001 | `[Login] client connected from 127.0.0.1:51267` | `LoginConnecting` |
| 2. HSEL handshake | `[Login] legacy: sent DistConnectSuccess auth_key=1000` | `LoginConnecting` |
| 3. Player sends RequestLogin | `[Login] legacy: auth_key=1000 id='smoke'` | `LoginConnecting` |
| 4. Auth OK | `[Login] legacy: auth OK for 'smoke', sending ACK (127.0.0.1:17001)` | `LoginConnecting` |
| 5. Agent TCP connect | `[Agent] client connected from 127.0.0.1:51268` | `AgentConnecting` |
| 6. Agent HSEL handshake | `[Agent] legacy: sent AgentConnectSuccess auth_key=93031` | `AwaitCharacterList` |
| 7. CharacterList returned | `[Agent] legacy: found 1 character(s)` | `CharacterListReady` |
| 8. Player selects charid=1001 | `[Agent] CHARACTERSELECT_ACK chrid=1001 map=12 name='smoke'` | `AwaitGameIn` |
| 9. GameIn forwarded to Map | `[Map] GAMEIN_SYN from player=1001 payload=16B` | `AwaitGameIn` |
| 10. **Player in world** | `[Map] stats map=12 players=1` | **`InGame` (map=12, x=27189, z=27361)** |
| 11. Stable in-game | (continues printing snapshots) | `InGame` for full 30s deadline |

The MapServer's periodic `[Map] stats map=12 players=1 timed_movement=0
draining=no` is the canonical evidence: a real Player session is live
on this MapServer instance, tracked in its runtime for the full
deadline.

## Why this works

- The modern native core (`mxh_unity_core`) was rebuilt into the
  Player with `ApiVersion=0x00010003`.
- LoginServer in legacy mode with HSEL matches the Player's
  `useHsel=true` default. The legacy `[Net/2B size][8B MSGBASE]`
  framing is what the modern core emits (`use_legacy_framing=true`
  in `NativeClientCore::connect`).
- AgentServer in legacy mode with HSEL matches the Player→Agent leg.
- MapServer in legacy mode **without** HSEL matches the
  original-4DyuchiNET MapClient convention. The fix in
  `3cf44a67` was to mirror `--use-hsel` onto the Agent's outbound
  `TcpClient.use_encryption` so Agent→MapServer stays consistent
  with whatever mode MapServer was started in.
- The `smoke` account was seeded into `chr_log_info` (login) and
  `character_info` (game) in `mxh_test` on the live MSSQL instance.

## What this proves

1. Unity Editor builds a Development Windows x64 Player
   (`RemasterSetup.BuildDevelopment`).
2. `MoxiangClient.exe` launches, instantiates the modern native
   core, and runs the modern state machine without crashing.
3. TCP+HSEL handshake with the LoginServer completes; auth
   succeeds against a real MSSQL account.
4. TCP+HSEL handshake with the AgentServer completes;
   CharacterListAck returns 1 character; CharacterSelectAck
   confirms the selected charid.
5. The Agent forwards GameInSyn to the MapServer; the MapServer
   replies with GameInAck + HERO_TOTALINFO; the modern core
   transitions to `InGame`.
6. The MapServer's periodic stats report `players=1`, proving the
   Player is bound to the world runtime, not just a logged-in
   socket. Stable for the full 30s deadline.

## Operators can now

- Watch the Player's `MXH_HEADLESS_PROBE: in-game reached` line as a
  CI gate for the full modern handshake.
- Add a `last_movement` field to `MapHandler::Stats` so the periodic
  dump also reports whether the player is actually moving.
- Drive the rest of the in-game flow (chat, move, combat) once the
  visual client is wired. The modern native core has the wire paths
  ready; only the visual + input side is missing.
