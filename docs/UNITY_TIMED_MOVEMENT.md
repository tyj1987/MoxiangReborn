# Timed movement migration — implementation in progress

## Current code and evidence boundary

`modern/include/mxh/game/movement_timeline.hpp` and its `.cpp` provide a
render-independent single-segment timeline, compiled in both the x86 game library
and the independent x64 Unity core. This is not yet activated in MapHandler or
client prediction. Existing Player movement probes still exercise the earlier
four-byte position behavior. Do not claim runtime speed authority from these
unit tests or from the collision probe.

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

## Required coordinated runtime cutover

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
The skill path also retains a PlayerInfo pointer after unlocking; copy the needed
combat/position state before releasing the lock during this refactor.

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
Like the timeline, this resolver is not yet activated in MapHandler; complete
authoritative input loading and coordinated server/client semantics remain open.
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
