# Local end-to-end runbook

This runbook exercises the modern Login/Agent/Map chain with the canonical
`playdh-current` resource profile. It is an engineering E3/E4 check; it does
not replace GUI input or legacy-client visual review.

The local SQLite/MSSQL commands below are isolated engineering fixtures. The
current human acceptance route connects the local DX11 client to the PVE stack
at `192.168.2.244:16001/17001/18001`.

## SQLite (disposable local run)

Build the four binaries with `scripts/build-modern.bat`, then run from the
repository root (or pass explicit executable paths):

```powershell
$runRoot = Join-Path ([IO.Path]::GetTempPath()) ('moxian-e2e-' + [guid]::NewGuid())
New-Item -ItemType Directory -Path $runRoot | Out-Null
$build = Join-Path (Get-Location) 'modern/build/tools'
$login = Start-Process (Join-Path $build 'MoxianLoginServer/mxh_login_server.exe') -ArgumentList '--port','16001','--backend','sqlite','--db',(Join-Path $runRoot 'game.db'),'--agent-addr','127.0.0.1','--agent-port','17001','--init-schema','--legacy' -PassThru
$agent = Start-Process (Join-Path $build 'MoxianAgentServer/mxh_agent_server_CHINA.exe') -ArgumentList '--port','17001','--backend','sqlite','--db',(Join-Path $runRoot 'game.db'),'--legacy','--map-server','127.0.0.1:18001' -PassThru
$map = Start-Process (Join-Path $build 'MoxianMapServer/mxh_map_server_CHINA.exe') -ArgumentList '--port','18001','--backend','sqlite','--db',(Join-Path $runRoot 'game.db'),'--map','10','--legacy' -PassThru
try {
  & (Join-Path $build 'MoxianClientE2E/mxh_client_e2e.exe') --no-spawn --login-host 127.0.0.1 --backend sqlite --db (Join-Path $runRoot 'game.db') --map-number 10 --timeout 30
  if ($LASTEXITCODE -ne 0) { throw "E2E failed with exit code $LASTEXITCODE" }
} finally {
  foreach ($p in @($login,$agent,$map)) { if ($p -and -not $p.HasExited) { Stop-Process -Id $p.Id -Force } }
  Remove-Item -LiteralPath $runRoot -Recurse -Force -ErrorAction SilentlyContinue
}
```

The normal CTest entry starts and tears down its own isolated servers. Use the
manual form only when inspecting a run interactively.

## MSSQL

Provide the connection string through a protected environment/secret store;
never put credentials in a command line, tracked file, or log. Then run:

```powershell
& (Join-Path $build 'MoxianClientE2E/mxh_client_e2e.exe') --no-spawn --login-host 192.168.2.244 --backend mssql_odbc --init-schema --db $env:MXH_MSSQL_E2E --map-number 10 --timeout 30
```

`MXH_MSSQL_E2E` is operator-supplied and is not persisted by the client. If it
is absent, report an environment skip rather than silently switching backend
or resource profile.

For visible, operator-driven acceptance, confirm the PVE stack is healthy and
that LoginServer advertises Agent address `192.168.2.244`, then start the
local launcher-backed client:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\run-human-acceptance.ps1 -PlanOnly
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\run-human-acceptance.ps1
```

The runner opens the real launcher and leaves login credentials and all
mouse/keyboard actions to the operator. It neither starts local services nor
reads a local database connection string.

## Soak flags (24h ops client)

`mxh_client_e2e` can stay in a map, watch for another character, dump
state, and abort the socket. Reconnect is a new process with
`--reuse-character` (a full login). The tool does not resume a session.

`--reuse-character NAME` selects that character and never creates one.
A missing name prints `CLIENT_E2E CHARACTER_MISSING <name>` and exits 2.
`--hold-seconds N` stays connected N seconds after map entry, sends
UserConn ConnectionCheck every 10 seconds, and answers ConnectionCheck
with ConnectionCheckOk, then logs out (exit 0). `--expect-peer NAME`
requires a CharacterAdd for that name (`CLIENT_E2E PEER_SEEN`) within
`--expect-peer-timeout` seconds (default 30) or exits 2.
`--state-dump PATH` writes JSON of the entry and exit snapshots:
position (`x`, `y`, `map`; `y` is the ground axis), `hp`, `mp`,
`shield`, `money`, and inventory sorted by slot (`slot`, `item_index`,
`count`). Object keys are in a fixed order. `--drop-socket-after N`
sets SO_LINGER `{l_onoff=1, l_linger=0}` and closes the socket. It does
not call shutdown and does not send logout. Stdout contains
`CLIENT_E2E DROPPED rst` and the process exits 4.
`--report-peer-gone` requires `--expect-peer` and is off by default.
After `CLIENT_E2E PEER_SEEN <name>`, one later line is printed when that
character leaves the remote-player list (the client handles this in
`CInGameState` ObjectRemove): `CLIENT_E2E PEER_GONE <name> char_id=<id> t=<ms>`.
A `--state-dump` written with the flag set includes `peer_gone_ms`
(`null` when the peer never left). The password env stays
`MXH_SMOKE_PASSWORD`.

Ports are `--login-port`, `--agent-port`, and `--map-port` (or
`MXH_E2E_PORT_BASE`). The ROG set is 26001 / 27001 / 28001. Pass them
on the command line. Linux CI runs `mxh_e2e_soak_tests` (parsing, JSON,
linger seam) with synthetic data only. A live server check is a Windows
run; the three servers are Winsock processes and Linux CI does not
start them.

```powershell
$e2e = Join-Path $build 'MoxianClientE2E/mxh_client_e2e.exe'
$common = @(
  '--no-spawn','--login-host','127.0.0.1',
  '--login-port','26001','--agent-port','27001','--map-port','28001',
  '--account','mxh_e2e','--password-env','MXH_SMOKE_PASSWORD','--timeout','30')
& $e2e @common --character-name soakA --hold-seconds 20 --state-dump .\soakA.json
& $e2e @common --character-name soakB --expect-peer soakA --hold-seconds 20
& $e2e @common --reuse-character soakA --drop-socket-after 5
if ($LASTEXITCODE -ne 4) { throw "drop path exit $LASTEXITCODE" }
& $e2e @common --reuse-character soakA --hold-seconds 10 --state-dump .\soakA-relog.json
```

The chain must already be listening on those ports. The password lives
only in `MXH_SMOKE_PASSWORD`. `--password-env MXH_SMOKE_PASSWORD` reads
that variable and does not print it.

## Expected result

The run reports LoginAck, CharacterListAck, character creation/re-list, and a
Map10 GameInAck, ending with `all 5 protocol steps passed`. This is protocol
and live-state evidence only; GUI, GPU, audio, human-input, and legacy
comparison evidence belongs in `docs/VERIFICATION_MATRIX.md`.

## Related checks

- `ctest -C Debug --test-dir modern/build -R MoxianClientE2E`
- `scripts/run-human-acceptance.ps1 -PlanOnly` for the PVE endpoint plan
- `docs/VERIFICATION_MATRIX.md` for commit/resource/evidence binding
