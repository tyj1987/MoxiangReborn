# Moxian Unity native core

This standalone target builds the Windows x64 native client core without the
legacy Win32 window, DirectX renderer, or UI libraries.

```bat
call C:\BuildTools\VC\Auxiliary\Build\vcvars64.bat
cmake -S modern\unity-core -B out\unity-core-x64-rel -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build out\unity-core-x64-rel
ctest --test-dir out\unity-core-x64-rel --output-on-failure
```

The public ABI is `modern/include/mxh/unity/unity_client.h`. All structures are
packed fixed-width POD values with explicit sizes. Unity owns no native pointer;
it holds a `uint64_t` handle and polls events/snapshots from its main thread.
`submit_command` accepts semantic commands only. Version 1 supports character
creation and selection and returns `MXH_UNITY_UNSUPPORTED` for every other
command. Character creation takes a UTF-8 name plus sex/hair/face and
cloth/boot/weapon option indices. The native core owns the original CHINA
option-value mapping and emits the 59-byte legacy request; Unity never builds a
packet or supplies raw item IDs.

The state machine advances only after valid server acknowledgements:

`Login connect -> LoginAck -> Agent connect -> CharacterListAck ->
CharacterSelectAck -> GameInAck`

From `CharacterListReady`, creation enters `AwaitCharacterCreate`. A valid
`CharacterMakeNack` produces a non-terminal `CHARACTER_CREATE/REJECTED` event
and returns to the ready list. Success is reported only when the server's
authoritative refreshed `CharacterListAck` contains exactly one new ID with the
requested name; an optional empty `CharacterMakeAck` is only an intermediate
acknowledgement.

The Agent TCP/HSEL session remains alive from character selection through
GameIn. A protocol error, HSEL error, acknowledgement timeout, or event queue
overflow closes the active connections and publishes `FAILED`. Passwords are
zeroed after `RequestLogin` is sent and on every shutdown/failure path.

HSEL only encrypts/obfuscates the legacy stream; it does not authenticate the
server or protect the handshake from an active MITM. Production release remains
blocked until an authenticated transport and server-identity policy are designed,
implemented, and verified end to end.

`timeout_ms` is a per-stage deadline: Login TCP connect, Login ACK, Agent TCP
connect, Agent ACK/list, selection ACK, and GameIn ACK each start with the full
configured duration.

Unity owns lifecycle calls on its polling thread. `TcpClient` receive-thread
callbacks enqueue network events and perform only transport-local HSEL setup;
they never call `tick`, `disconnect`, or `destroy`. Consequently core shutdown
closes the socket and joins the receive thread from the owner thread, never from
the thread being joined. This ownership rule is part of the native integration
contract.

Character names crossing the ABI are UTF-8. The default rejects invalid UTF-8;
the CP949 and CP936 connect flags explicitly transcode legacy database bytes.
Creation performs the inverse conversion without best-fit substitutions and
rejects control characters, unrepresentable names, and names outside the
original 4-16 encoded-byte limit; it never truncates a code point or wire name.
Events and selection commands carry session and map generations so Unity can
discard late events and the core can reject stale UI commands.

API 0x00010002 additionally submits MOVE/STOP in legacy game coordinates and
publishes predicted submissions, own corrections and remote movement events.
Normal movement has no owner ACK; a successful send is not server acceptance.
See `docs/UNITY_MOVEMENT_STATUS.md` for the exact event layout and the inherited
server collision/time/state gaps. HSEL now uses independent TX/RX key schedules;
client/core/server builds must be updated or rolled back together.
