# Timed movement wire format v1 — not yet activated

`modern/include/mxh/proto/movement_wire.hpp` defines a bounded modern extension
for the coordinated server/native-core/Unity cutover. Original protocol headers,
existing enum values and four-byte position messages remain unchanged. No packet
dispatcher, client command or server permission currently activates this format.
Do not describe codec tests as an operational timed-movement network test.

All integers are unsigned little endian. Floats are explicit IEEE754 binary32
little endian; native C++ struct layout, alignment and pointer size are never
serialized. Unknown versions, kinds, nonzero reserved bits, truncation and trailing
bytes are rejected. Every route is bounded to 15 points, matching original
MAX_CHARTARGETPOSBUF_SIZE and the shared MovementTimeline.

## Client command: 24 + 4N bytes

| Offset | Field | Encoding |
|---:|---|---|
| 0 | Magic | ASCII `MXMC` |
| 4 | Version | u8, 1 |
| 5 | Kind | u8, Route=1, Stop=2 |
| 6 | Point count N | u8, 1..15; Stop requires exactly 1 |
| 7 | Reserved | u8, zero |
| 8 | Epoch | u64, nonzero |
| 16 | Command sequence | u64, nonzero |
| 24 | Points | N pairs of x:u16, z:u16 |

Maximum encoded command size is 84 bytes. Route points are target intents, never
client-selected speed or starting position. Stop's single point is a position
claim to be checked against the materialized server position and original Stop
tolerance. Each component is 0..51199; collision and reachability are separate
authoritative checks, not properties of this codec.

## Server state: 52 + 4N bytes

| Offset | Field | Encoding |
|---:|---|---|
| 0 | Magic | ASCII `MXMS` |
| 4 | Version | u8, 1 |
| 5 | Kind | u8, Started=1, Stopped=2, Corrected=3, Snapshot=4 |
| 6 | Remaining point count N | u8, 0..15 |
| 7 | Reserved | u8, zero |
| 8 | Epoch | u64, nonzero |
| 16 | Related command sequence | u64; only Snapshot permits zero |
| 24 | State sequence | u64, nonzero |
| 32 | Server observation time | u64 milliseconds; zero is valid |
| 40 | Current x | f32, finite 0..51100 |
| 44 | Current z | f32, finite 0..51100 |
| 48 | Current speed | f32, finite |
| 52 | Remaining points | N pairs of x:u16, z:u16 |

Maximum encoded state size is 112 bytes. Started requires at least one remaining
point and positive speed. Stopped/Corrected require zero points and zero speed.
Snapshot permits either stationary or moving state, with positive speed exactly
when remaining points exist. Remaining point coordinates use the same bounds as
commands. The different current-position bound preserves the original 51100 clamp.

## Required integration contracts

The server must issue a fresh nonzero epoch for each player map-entry session.
Clients must not create their own accepted epoch. Epoch is an identity discriminator,
not a secret or authentication token: MapServer still binds player ID and transport
ownership, then validates epoch and strictly increasing command sequence under the
same state lock used for movement. A malformed, stale or duplicate request must
not replace a route or reset its start clock; normal materialization of an already
accepted route may continue. Sequence overflow requires a fresh session;
it must not silently wrap. Decoder validity alone grants no permission to move.

Client consumers must track state sequence per entity/epoch and reject late old
epochs and non-increasing state sequences. A fresh epoch is accepted only through
the authorized map-entry/entity lifecycle, not from any arbitrary state packet.
Clock synchronization must interpret server observation time; it is not the local
Unity clock. Newly entering observers need a current snapshot and remaining route.

Before activation, implement capability negotiation and explicit modern packet
routing. Timed clients must not be allowed to fall back to legacy instantaneous
position reports within the same session. Authority replies must reach the owner
as well as appropriate observers, while rejection/correction routing stays scoped.
MapServer validates all segments against functional tiles and chooses current
mode/status speed; the native core and Unity must both interpolate the resulting
state rather than assigning the final waypoint immediately. These are pending.

## Verification scope

Six tests compile against both x86 and x64: exact-byte command/state goldens,
every truncated prefix, extra bytes, wrong direction/version/reserved fields,
15-point limit, invalid point coordinates, invalid state combinations, NaN/Inf,
large uint64 clocks/sequences and fractional position round trips. Independent
read-only review found no out-of-bounds or schema-consistency issue. Actual
server dispatch, negotiation, old-epoch suppression and Player movement are not
proved by these codec tests.

Final verification: x64 core build and CTest passed 66/66; x86 full build,
six focused MovementWire tests and full CTest passed (exit 0). No Player or
Editor route run was performed for this inactive codec-only increment.
