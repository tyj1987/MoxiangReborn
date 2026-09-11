# Unity movement migration evidence

2026-09-11. Partial implementation; not playable movement acceptance.

## Bridge contract

API `0x00010002` adds MOVE/STOP commands without changing the 128-byte command,
320-byte event or 992-byte snapshot layout. Arguments are unsigned 16-bit game
coordinates expressed in 32-bit fields; overflow is rejected before narrowing.
Only in-game commands with the current session and map generations are sent.
DX11 and Unity now share `ClientWire::make_move_message/parse_move_payload`.
The current modern wire is Category 8, OneTarget 13 / Stop 8, then four bytes
`x:u16 LE, z:u16 LE`. This is the existing modern protocol, not the complete
original start/target wire structure.

Event 7 means a local prediction was submitted, not that the server accepted it.
Event 8 applies an own-player server Correction. Event 9 preserves remote object
movement. Movement event argument0 is object ID; argument1 packs x in low 16 bits
and z in high 16 bits; reserved0 carries the MoveProtocol value. Corrections have
request ID zero because the legacy wire contains no command correlation ID.
Unity must cancel its pending predicted path on correction. The current scene
has no completed player controller or character rendering.

## Live integration defects discovered

The initial two-session test exposed duplicate movement: Map sent both a direct
reply and a fan-out to the same multiplexed Agent connection. It also exposed
Agent's incorrect broadcast treatment of Correction, which excluded its owner.
Map now sends normal movement once per Agent connection; Agent sends Correction
only to the owning character. Unit tests cover two players sharing one Agent
connection plus a second Agent connection, and owner-only correction routing.

Further live testing exposed a shared HSEL state problem. Both directions used
one evolving key schedule and mutable algorithm scratch. A deterministic test
that sends unequal bursts from both peers before receiving reproduced corruption
before the fix. Each HselStreamCipher now owns separate TX/RX streams initialized
from the same handshake and protects operations/initialization. Both net send
paths serialize key advancement with the corresponding wire send/queue insertion.
Tests cover all four transform types and four simultaneous senders exchanging
800 complete short/large frames, checking exact payloads and unique identities.
Public cipher signatures and original protocol headers are unchanged. Client,
native core and server binaries must be rebuilt and rolled back as a matched set;
this fixes modern transport behavior, not authenticated transport/security stubs.

## Verification runs

- x64 native/crypto suite: 40 passed, zero failed (4.72 seconds). This includes
  the crossed-burst red/green regression and real simultaneous TCP senders.
- Full x86 build succeeded; CTest: 12,456 registered, zero failed, six skipped
  (90.33 seconds). Log: `modern/out/unity-remaster/movement-hsel-ctest.log`.
  MSSQL and release/resource skips remain unverified.
- Real three-server Editor run `e76269ba960c4c1682af3ef2ed18aba4` passed the
  two-session movement test and the existing 100 reconnect test.
- Real standalone Player run `759f279828f049ceb0d85ec5b96bb9ab` passed the
  two-session move/stop/rejected-jump probe. Its inspected frame shows Map10 and
  corrected position 25064,25032. `report.json` keeps `gameplayAccepted=false`.
- Native DLL SHA-256 at these runs:
  `8D8A878EC36643F0F06E48EEDF5EAF97A9378B856FB465E3288DA45E3915B69C`.
- Post-fix Player creation regression `9cbc21fc885d4f7f8dbef1ff2034cac3` passed
  empty-account creation, GameIn and expected initial equipment persistence.

Run evidence is under `modern/out/unity-remaster/three-server/<run-id>/`.
These use isolated SQLite and loopback servers, HSEL on client-facing links and
the inherited plaintext Agent-to-Map link. Two native sessions in one host are
not two human players. No character animation, input-controller, collision,
position persistence or complete movement acceptance is established by them.

## Original movement and authoritative data still to migrate

Read-only review of `D:/MX/src/[Server]Map` located the original rules:
- `MapNetworkMsgParser.cpp:1162` Target and `:1258` OneTarget validate state,
  start/current distance, full segment collision and endpoint collision before
  starting movement. `:1492` Stop checks collision and valid stopping position.
- `CharMove.cpp:106` starts a speed/time-based movement; `:173` computes the
  authoritative position; `:73` checks stopping error. Original ordinary walk/run
  speeds are 200/400, stop tolerance 1000, and coordinate clamp 0..51100.
- `MHMap.cpp:27` maps map 10 to `Resource/Map/10.ttb`. `TileManager.cpp:304`
  loads two signed 32-bit dimensions followed by one WORD attribute per cell.
  `TileManager.h:16` defines a 50-unit fixed cell. Bit 0 is collision.

Do not confuse this server fixed-attribute format with the modern best-effort
texture-table abstraction despite their shared `.ttb` extension. Map10's actual
file `modern/data/PlayDH/Resource/Map/10.ttb` is 2,097,160 bytes: 1024x1024 WORDs
plus an eight-byte header. SHA-256:
`6B1CC9A83D79AA7F764E1D19446EE3AFF6D9625F7D8A52B02FFA8BE7E4B78F5B`.
451,667 cells contain 0 and 596,909 contain 1. Six read-only copies in canonical
PlayDH, D:/MX server/client/client-legacy/SWorking/backup match exactly.
These are existing authoritative source bytes, not generated replacement data.

### Fixed attributes integrated (2026-09-11)

`FixedTileMap` now loads bounded dimensions and exact WORD payload lengths,
retains all attribute bits, and rejects missing, truncated or trailing data.
MapServer requires its actual map file before listening, including development
fallback mode. Original resources remain unchanged. The original biased segment
scan is preserved; OneTarget endpoint blocking is checked separately because the scan can
omit the diagonal endpoint. Target and Stop retain the original segment-only
collision rule. Invalid world/grid coordinates fail closed.

Movement now requires exact four-byte payloads, a known player/runtime and the
player's physical Agent connection. Only Target, OneTarget and Stop are accepted
from clients. Missing tiles, blocked paths/endpoints and excessive steps return
an owner-only correction without changing state or broadcasting invalid moves.
Client Warp, Correction and Init cannot rewrite position. Map replacement is
refused while players are connected. The 5000-unit coarse bound now also applies
to Stop; it remains an interim defense, not original timed movement authority.

Validation: x86 full build passed; full `ctest -C Debug --test-dir modern/build
--output-on-failure -j 8 --quiet` returned 0. A focused 15-test run passed,
including five FixedTileMap tests and server rejection/ownership/endpoint tests.
Real SQLite/HSEL Unity Player two-session movement passed with run ID
`1b0dafffeb7647678a19c69c675348b1` after the read-only review corrected the
Target/Stop endpoint distinction. That Player run verifies normal movement and
excessive-step correction after integration; blocked endpoint rejection is
currently unit-tested, not yet a dedicated Player wall-crossing probe.

Independent scan coverage now includes 124,852 paths over a 7x7 grid: each single
blocked cell, all clear, checkerboard and all blocked; all start/end combinations
cover every direction and subcell offsets. A separate reference path builder
normalizes major/minor axes and resolves fixture collisions independently of
production helpers. Goldens lock the biased diagonal and strict decision tie.
All seven fixed-tile tests and the full CTest run pass.

The real-server smoke now audits Map10's exact SHA-256 and chooses blocked cell
center (26175,25425), WORD attribute 1, from accepted position (25064,25032).
Squared distance 1,388,770 is below the 5000-unit jump threshold. Both Editor and
Player probes require a correction back to the accepted position and observe the
second session for forbidden movement broadcasts. Player reports require probe
version 2 and a separate collisionPassed flag, so older binaries cannot satisfy
this new check. Fresh real SQLite/HSEL evidence:
- Editor: `f5d80dbe7bbb4b61bb651797623e6f7e`, all selected tests passed.
- Player: `d7193e33e2ce402daee4fda301e714fc`, report explicitly records
  movementProbeVersion=2, collisionPassed=true, position=(25064,25032).
- Player screenshot inspected at that run's `player.png`; it remains a terrain
  and connection validation view, without character/art-quality acceptance.

These are two native sessions with real servers, not two human players or full
collision certification across every map. The selected blocked target proves
rejection below the jump threshold; clear destinations across obstacles and
complete client input/path prediction are further runtime cases to add.

Remaining: original timed speed/state/stop rules, shared client prediction and
position persistence. HFL and STM
visual collision are not substitutes for authoritative tiles. These gaps remain
blockers to complete Map10/full-game acceptance.
