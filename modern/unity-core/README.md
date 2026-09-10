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
selection and returns `MXH_UNITY_UNSUPPORTED` for every other command.

The state machine advances only after valid server acknowledgements:

`Login connect -> LoginAck -> Agent connect -> CharacterListAck ->
CharacterSelectAck -> GameInAck`

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
Events and selection commands carry session and map generations so Unity can
discard late events and the core can reject stale UI commands.
