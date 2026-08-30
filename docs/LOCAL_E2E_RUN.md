# Local end-to-end runbook

This runbook exercises the modern Login/Agent/Map chain with the canonical
`playdh-current` resource profile. It is an engineering E3/E4 check; it does
not replace GUI input or legacy-client visual review.

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
  & (Join-Path $build 'MoxianClientE2E/mxh_client_e2e.exe') --no-spawn --backend sqlite --db (Join-Path $runRoot 'game.db') --map-number 10 --timeout 30
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
& (Join-Path $build 'MoxianClientE2E/mxh_client_e2e.exe') --no-spawn --backend mssql_odbc --init-schema --db $env:MXH_MSSQL_E2E --map-number 10 --timeout 30
```

`MXH_MSSQL_E2E` is operator-supplied and is not persisted by the client. If it
is absent, report an environment skip rather than silently switching backend
or resource profile.

For the visible, operator-driven acceptance run, pass the same protected
connection configuration only through the current process environment, then
start the launcher-backed runner:

```powershell
$env:MXH_DATABASE_CONFIG = $env:MXH_MSSQL_E2E
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\run-human-acceptance.ps1 -Backend mssql_odbc
```

The runner starts the three modern services against MSSQL, opens the real
launcher and leaves login credentials and all mouse/keyboard actions to the
operator. It records only the backend name and environment-variable name in
its run metadata, never the connection string or password.

## Expected result

The run reports LoginAck, CharacterListAck, character creation/re-list, and a
Map10 GameInAck, ending with `all 5 protocol steps passed`. This is protocol
and live-state evidence only; GUI, GPU, audio, human-input, and legacy
comparison evidence belongs in `docs/VERIFICATION_MATRIX.md`.

## Related checks

- `ctest -C Debug --test-dir modern/build -R MoxianClientE2E`
- `scripts/run-human-acceptance.ps1 -Backend mssql_odbc` for the visible,
  operator-driven MSSQL path
- `docs/VERIFICATION_MATRIX.md` for commit/resource/evidence binding
