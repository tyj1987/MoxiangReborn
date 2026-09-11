# Timed movement migration — implementation in progress

## Current code and evidence boundary

`modern/include/mxh/game/movement_timeline.hpp` and its `.cpp` provide a
render-independent movement timeline, compiled in both the x86 game library
and the independent x64 Unity core. MapHandler now owns and materializes each
player's timeline, but network commands do not yet start timed trajectories and
client prediction is not yet switched. Existing Player movement probes still exercise the earlier
four-byte position behavior. Do not claim runtime speed authority from these
unit tests or from the collision probe.

The server now centralizes position reset/materialization under `players_mu_`,
synchronizing PlayerInfo and actor.state. Network entry, monster tick and disconnect
materialize players before work; snapshots materialize the requested player.
GameIn, legacy position updates and the test setter reset trajectories. Dead or
inactive actors freeze at the last materialized position instead of adding travel
after the state change. Test-only clock/trajectory hooks prove interpolation,
legacy Stop cancellation and a real PickupSyn range transition: rejection at
600 units, acceptance at the existing 500-unit threshold, without a snapshot call
to refresh state first. No network flag or production shortcut enables those hooks.

The current entry hook updates every player's position per message. Its performance
must be measured before large-population acceptance. Test clock callbacks run under
the player mutex and must not reenter MapHandler.

The skill path now snapshots caster values and validates internal session tokens
before MP reservation and player damage/healing. No PlayerInfo pointer escapes
the mutex. Monster damage validates the caster token while holding both player
and monster locks; follow-up quest/experience mutations also check that identity.
MP is read from runtime vitals and synchronized with the combat cache after the
atomic deduction. This fixes a reproduced 55-versus-50 initial MP mismatch.
Instant skill removal releases the skill mutex before broadcasting, allowing
the send callback to reenter skill handling without self-deadlock.

These changes protect the state mutations covered here. They do not establish
global session-tagged delivery: result routing, persistence and other subsystems
still use player IDs in several later steps. Cooldown/learned-skill checks,
original skill range semantics and production timed movement remain separate
unfinished requirements. The regression fixtures use explicitly available
development skills, not evidence of the complete original skill catalog.

Skill lifetime validation: all three focused tests passed, covering caster and
target replacement on the same connection for attack/heal, monster commit after
caster replacement, nested casting during remove broadcasts, foreign connection
rejection and insufficient runtime MP. The MP test first failed with 40 instead
of the expected 45, exposing the stale combat cache; the reentry test also first
failed with resource-deadlock after exposing broadcast_except's player lock.
The latter now snapshots connections under lock and invokes the sender after
unlocking. Full x86 build and full CTest passed (exit 0). Real Player/SQLite/HSEL
three-server movement and collision regression passed in run
`4573f176ed15407d9153e0358af27bf2`; it validates shared-path regression, not
in-Player skill visuals, MSSQL, or human gameplay acceptance.

Validation of position centralization: full x86 build and full CTest passed
(exit 0); focused position/pickup regression tests passed. Real independent
Player against the three local servers passed movement and collision regression,
run `aa6440115db94b7da4e25aad6b45af5c`. This uses SQLite and the existing four-byte
movement protocol; it does not prove timed client movement or MSSQL behavior.

Source: `D:/MX/src/[Server]Map/CharMove.cpp:106-234` constructs direction from
distance/speed and evaluates position using server milliseconds. Arrival uses
strict `estimate < elapsed`; at exactly the endpoint time, moving remains true
until a subsequent evaluation. `:74-104` accepts a Stop claim within 1000 units
of current server position. `:341-355` halts and corrects a rejected Stop.
World output is clamped to 0..51100. The component retains those rules, while
using uint64 clocks, rejecting nonfinite/overflowing input and preventing stale
timestamps from rewinding position.

The ten deterministic tests cover run speed 400, walk 200, diagonal normalization,
mid-segment retargeting, 1000 same-time commands without distance accumulation,
strict arrival, Stop at 999/1000/1000.25 units, zero speed/distance, original
clamping, timestamps across DWORD wrap, invalid inputs and derived float overflow.
Additional checks cover sub-millisecond arrival, very large clock values, stale
Stop timestamps and reset versus materialized position. Speeds are supplied by
the caller; these tests do not establish full character
speed selection for lightness skills, equipment or Titan modes.

### Bounded route evaluation

`start_route` now accepts 1..15 waypoints, matching
`D:/MX/src/[CC]Header/CommonGameDefine.h:2180`. The original
`CharMove.cpp:194-219` starts the next segment at the current observation time;
it does not carry elapsed overshoot into later segments. At most one waypoint
transition happens per distinct timestamp, including repeated zero-length points.
At exactly the estimated arrival time the current segment is still active.

The recursive same-timestamp `CalcPositionEx` called by `StartMoveEx` clamps the
next start position before building its direction. The edge fixture reaches a
51199 waypoint, starts the next segment at 51100, and reaches 51050 after another
500 ms at speed 100. Clamping only the returned position would produce a different
route. Input is copied before materialization so a remaining-route span may alias
the same object. Invalid size/coordinates or invalid derived motion never replace
the existing route. Reset, Stop, halt and zero speed clear all queued waypoints.

Five route tests extend the ten existing timeline tests. Final x64 core CTest is
60/60 passed; x86 focused timeline/server-position regression is 17/17 passed.
Full x86 build and full CTest also passed (exit 0). Independent read-only source
review verified transition timing and identified the clamp-order detail covered
by the final edge fixture. No Editor/Player route test is claimed for this step.
This is shared math, not an enabled network route. The current route API uses a
caller-supplied constant speed; original StartMoveEx queries GetMoveSpeed at each
segment start. Authoritative mode/status refresh at segment transitions must be
connected before claiming dynamic-speed route parity. Versioned payloads,
client/server activation and visible Player interpolation remain pending.

## Required coordinated runtime cutover

The bounded versioned codec is implemented and specified in
`UNITY_MOVEMENT_WIRE.md`. It is not yet dispatched or negotiated; wire-level
epoch/sequence fields are not a substitute for session validation at the caller.

The current four-byte OneTarget may express a target command, but both clients
and the server currently treat it as a position update. Switching only the server
would leave local and remote views teleporting while gameplay uses interpolation.
The cutover must include:

1. OneTarget creates a target intent, with server-selected speed and a server
   start position. Stop validates against the current interpolated point. Preserve
   original multi-waypoint Target through a bounded, versioned payload; one x/z
   pair cannot represent its route. Keep original reference headers untouched.
2. Runtime movement state belongs in PlayerRuntime. Under `players_mu_`, one
   materialization helper synchronizes actor position and PlayerInfo position.
   Capture one injectable monotonic timestamp per handler/tick operation.
3. Route all position readers through the helper before distance comparisons or
   serialization. Use copied snapshots across lock boundaries. Current anchors:

| Operation | Current source anchor in map_handler.cpp |
|---|---|
| Runtime snapshot | player_runtime_snapshot, approximately line 414 |
| Spawn/reset | GameIn writes at 1361 and 1404; test setter around 478 |
| CharacterAdd serialization | 2217 |
| Pickup distance | 2315 |
| NPC interaction/sell/buy/speech | 2552, 2792, 3441 |
| Monster aggro/chase | 3327, 3348 |
| Skill target/caster range | 4023, 4048 |
| GameOut and disconnect | 1287, 1189 |

Line anchors are from the 6fe4ddf5 checkout and will move during extraction.
The previously noted unlocked skill PlayerInfo pointers have been replaced with
value snapshots and session checks; see the current evidence and remaining
cross-session delivery/persistence limits above.

4. Reset/correction/spawn clear prior trajectories. GameOut/disconnect/transfer
   must materialize the final point before persistence or ownership handoff.
   Existing logout persists items/money/quests but not position. Map transfer
   routing is in Agent and needs an explicit old-Map final-position contract.
5. DX11 `CInGameState::move_to_screen` and `send_move` currently snap to target;
   keyboard movement uses 220 units/s and periodic current-position reports.
   Replace those semantics alongside Unity core prediction and remote movement
   events. Accepted target, authoritative position and predicted display position
   must be distinguishable. Keep the C ABI bounded/versioned if expanding events.

## Authoritative speed and state work still required

Original Player.cpp:2911-3003 depends on MoveMode, KyungGong index/level, ability,
avatar/shop bonuses and Titan state. Plain walk/run are 200/400; setting every
mode to 400 would change gameplay. Modern does not yet have a complete equivalent
speed resolver or all its PlayerRuntime inputs.

Reusable sources include avatar KyunggongSpeed (`avatar_item_option.hpp`,
`server/avatar_calc.hpp`), shop KyungGongSpeed (`shop_item_option.hpp`,
`server/calc_shop_item_option.cpp`) and TitanStats::moveSpeed. These must be loaded
and tied to authoritative state, not trusted from a client movement packet.
Movement also needs Active/alive/initialized state checks and original state
transitions. Missing active-mode inputs must be visible as an incomplete mode;
they cannot silently select the plain-run speed.

Runtime acceptance remains open: two observers see continuous motion; attacks,
skills, NPCs and pickup use the same time-derived position; Stop tolerances,
delayed/reordered commands, death and mode changes match source behavior; final
positions survive SQLite/MSSQL logout, reconnect and map transfer. The full-game
and visual-quality plan remains unchanged.

## Exact speed-source follow-up

Direct read of Player.cpp:2938-2997 distinguishes these branches:
- Non-Titan lightness: skill speed + ability Kyunggong + avatar KyunggongSpeed
  + shop KyungGongSpeed. Missing skill information returns zero in original.
- Non-Titan ordinary movement: Run=400, otherwise Walk=200.
- Titan ordinary movement: Run uses current Titan stats MoveSpeed; Walk=300.
- Titan lightness: grade KyungGongSpeed index 1 for skill 2602, 2 for 2604,
  otherwise 0, plus avatar and shop bonuses (no ability bonus in this branch).
  Missing grade information explicitly returns TITAN_WALKSPEED=300 in original.

Constants verified at CommonGameDefine.h:936-939. Preserve explicit original
fallbacks; distinguish a verified source-defined missing-grade case from a
not-yet-loaded modern runtime dependency. Also inspect the base GetMoveSpeed
wrapper and movement state effects before treating DoGetMoveSpeed as the final
authoritative speed.

The wrapper is now located: Object.h:209-217 uses GET_STATUS over the ordered
status list; Status.h:12 computes `(Ori + Up) - Down`.
SkillObjectAttachUnit_MoveSpeed.cpp:42-47 assigns each nonzero percentage to its
direction's accumulator, replacing prior Up or Down. It neither sums percentages
nor compounds the already modified speed. Zero leaves the previous accumulator.

`game/player_move_speed.hpp/.cpp` now ports these branches and ordered effects
into both native architectures. Five tests cover plain modes, all lightness
bonuses, Titan grade selection/fallback, unresolved inputs, ordered status
overwrites, negative source results and invalid floating-point data. An explicit
special_resources_resolved flag prevents not-yet-loaded data from masquerading
as the original verified missing-resource cases. Caller must supply the original
ordered active status list. Negative source formula results are preserved; the
time module refuses negative speed, so runtime policy must report that outcome.
This speed resolver is not yet activated in MapHandler; complete
authoritative input loading and coordinated server/client semantics remain open.

### Actual lightness resource now loaded by MapServer

The original `KyungGongManager.cpp:32-53` loads `Resource/KyungGongInfo.bin`;
`KyungGongInfo.cpp:22-34` reads nine tokens in order: id, name, MP cost, move
type, speed, change time, start/ongoing/end effects. This is packed text, not a
fixed-width binary record. `KyungGongCatalog` now parses decoded MHFile data into
typed records, preserves source-name bytes and maps the original effect token
`-1` to WORD 65535. It rejects truncation, duplicate ids, extra incomplete records,
numeric suffixes/overflow, nonfinite or negative speed, embedded NUL and oversized
input. Zero speed remains expressible because original CharMove.cpp:110-115
explicitly ends movement for that value; parsed data is not movement permission.

MapServer startup requires the actual catalog before listening. Decode/parse
failure does not replace an existing catalog, and replacement is refused while
players are connected. The canonical 208-byte source is unchanged:
SHA-256 `AF3DBC8AB600325ED171AF68506B33ADBB97CF41FA78BE253E1E3BCDB6C1064E`.

| ID | Speed | MP cost | Move type | Change time |
|---|---:|---:|---:|---:|
| 2600 | 600 | 5 | 1 | 2500 |
| 2601 | 750 | 7 | 1 | 2500 |
| 2602 | 900 | 10 | 2 | 2000 |
| 2603 | 1050 | 13 | 2 | 1500 |
| 2604 | 1200 | 15 | 3 | 1000 |

All five start effects are 65535. The three catalog tests and MapHandler load/
session test pass. Real Player/SQLite/HSEL three-server regression
`4d537f92f9ac429eb34c29127c883dc8` passed after mandatory catalog loading,
including the existing movement/collision probe. That run proves startup and
existing behavior, not activation of lightness or timed authority.
Final full x86 build and CTest passed (exit 0), including the explicit original
zero-speed representation test. Original resource bytes were not changed.

Remaining mode work: Move proto 3/4 (Walk/Run) and proto 5 (KyungGongSyn) are
currently ignored by MapHandler. Original proto 5 uses MSG_DWORD2 with KG id in
dwData2, not the four-byte coordinate message. CharMove.cpp:287-323 checks ability
201/204/207, learned level, AbilityInfo and character level, then updates mode and
MP timing; these gates and the MP consumption loop still need migration before
using the catalog to enable lightness. Category 23 is a grade notification, not
the Move-mode request. Position materialization and client semantic cutover
listed above remain required.
Titan ordinary Walk uses constant 300 without requiring the resource-ready flag;
only resource-dependent branches require it. Final x64 core tests: 55/55 passed,
including all five speed tests. Read-only source review found no remaining
branch-formula discrepancy after rechecking the resource-ready gate.
Final x86 full build and full CTest also passed (exit 0). Neither the new speed
resolver nor the timeline is staged into a claimed runtime-authority release.

Validation for this increment: final x64 build/core CTest 50/50 passed; final
x86 full build passed; full x86 CTest with `--output-on-failure -j 8 --quiet`
returned 0. Independent read-only review found no blocking timeline math issue.
No Player build is claimed to exercise this inactive module.
