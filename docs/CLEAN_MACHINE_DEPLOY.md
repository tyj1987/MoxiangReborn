# Clean-Machine Deployment

> Status: executable deployment runbook. It is not evidence that the current
> client is RC-ready; use `docs/VERIFICATION_MATRIX.md` for the current gates.

## Purpose

`scripts/clean-deploy.ps1` takes a blank Windows box (Windows Server 2022 or
Windows 10/11) with only PowerShell and Git installed, and bootstraps it into a
configured modern build environment and disposable server run in one command.

This is the §5.E "clean machine deployment" gate from ROADMAP.

## Quick Start

```powershell
PS> powershell -ExecutionPolicy Bypass -File C:\moxiang\scripts\clean-deploy.ps1 -InstallPrereqs
```

The resulting machine can configure and build the repository using its own
checkout-relative paths. Test, resource-profile, GUI and clean-machine results
must be recorded separately with commit and evidence identifiers; this runbook
does not manufacture or imply those results.

## Steps Performed

| # | Step | Description |
|---|------|-------------|
| 1 | preflight | admin check, OS, RAM, disk |
| 2 | prereq detect | VS2022, cmake, git, sqlcmd, SqlLocalDB, ODBC 18, VC++ Redist |
| 3 | prereq install | (only with `-InstallPrereqs`) chocolatey + direct download for ODBC 18 + VS Build Tools 2022 |
| 4 | PlayDH junction | `modern/data/PlayDH` -> `<RepoRoot>\墨香【源码配套资源】\PlayDH` |

`deploy/scripts/deploy_client.ps1` and `deploy/scripts/deploy_server.ps1` do not read that junction or `modern/data/PlayDH` on their own. They take the PlayDH tree from `-ResourceDir` or the environment variable `MXH_PLAYDH_ROOT`. If neither is set, the script prints `MXH_PLAYDH_ROOT is not set and no --resource-root/-ResourceDir was given` and exits 1. The in-game client and `MoxianLauncher` use the same rule with `--resource-root` or `MXH_PLAYDH_ROOT`. The client package bat `启动游戏.bat` passes `--resource-root "%~dp0data\PlayDH"` so a deployed tree does not need the environment variable. `deploy_server.ps1` does not generate a client bat. `scripts/clean-deploy.ps1` still requires its own `-PlayDhRoot` and does not default to the repo tree.
| 5 | build modern | invokes `cmake --build modern/build --config <Config>` |
| 6 | run ctest | `ctest -C <Config> --test-dir modern/build --output-on-failure` |
| 7 | commercial smoke | invokes `scripts/commercial-smoke.ps1` (includes portal HTTP smoke) |
| 8 | portal smoke | hits `/api/healthz` + `/api/status` + `/` on localhost:8080 |

All steps are idempotent and safe to re-run.

## Parameters

| Parameter | Description |
|-----------|-------------|
| `-DryRun` | Print every step that would run, do not actually mutate |
| `-InstallPrereqs` | Install missing prereqs via Chocolatey (choco) |
| `-SkipTests` | Skip ctest (faster bootstrap verification) |
| `-SkipSmoke` | Skip commercial-smoke |
| `-SkipGui` | Forward -SkipGui to commercial-smoke |
| `-Config` | CMake config (Debug or Release, default Debug) |
| `-RepoRoot` | Override repo root |
| `-Force` | Rebuild from scratch (reset PlayDH junction) |

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | success |
| 1 | preflight failure |
| 2 | prereq missing (rerun with -InstallPrereqs) |
| 3 | build failed |
| 4 | ctest failed |
| 5 | commercial-smoke failed |

## Prerequisite Installation

With `-InstallPrereqs`, the script installs missing tools via chocolatey:
- `choco install vcredist2022 -y`
- `choco install cmake -y --installargs ADD_CMAKE_TO_PATH=System`
- `choco install git -y`
- `choco install sql-server-express -y` (SQL Server 2022 Express, includes LocalDB)
- direct MSI download for `msodbcsql18` (ODBC Driver 18 for SQL Server)
- direct download of `vs_buildtools.exe` for VS 2022 Build Tools (C++ workload)

All installs run silently and require Administrator elevation.

## Item protocol gate

`scripts/clean-deploy.ps1` reads `protocol.manifest` beside `mxh_map_server_CHINA.exe` and `mxh_client.exe` before it starts commercial smoke. `deploy/scripts/start_modern.ps1` does the same before start or restart, using the selected locale's map server. `scripts/gui-client-smoke.ps1` and `scripts/visual-smoke.ps1` check before they start a map server and client together.

CMake builds a host tool that prints the compiled Item protocol as `Name=Value` lines in `MXH_ITEM_PROTOCOL_EACH` order. The hash script normalizes that list to LF and writes `item_protocol_sha256=<hex>` into `protocol.manifest` next to each of those binaries, plus `mxh/proto/protocol_hash.hpp` (`kItemProtocolSha256`). The hex is not stored in git. Comment and whitespace edits in the header do not change it. A count of zero, or a count other than `kItemProtocolCount`, fails the build.

A match prints `PROTOCOL_MATCH item=<hash>`. A missing manifest or a different value prints `PROTOCOL_MISMATCH item=<map_hash>/<client_hash>` (`missing` for that side) and the script exits non-zero without starting a process. There is no Linux map-and-client starter. `scripts/protocol_manifest_gate.py` is the check the tests call.

## Next Step After Deployment

Manual start of the server stack:

```powershell
# Legacy stack (pre-built binaries in 墨香【源码】\SWorking):
PS> scripts\start-server.ps1 -Mode start

# OR modern stack (mxh_* binaries from modern\build\bin):
PS> modern\build\bin\Debug\mxh_login_server.exe &
PS> modern\build\bin\Debug\mxh_agent_server_CHINA.exe &
PS> modern\build\bin\Debug\mxh_map_server_CHINA.exe &
```

## Non-Admin Caveats

The script gracefully degrades when not run as Administrator:
- Warns instead of fails on preflight admin check
- Warns when RAM/disk detection requires admin (skips strict check)
- `-InstallPrereqs` will fail without elevation (some installers need it)

For production deployment, always run with Administrator elevation.

## Local Verification (2026-08-18)

Run on the developer's local Windows 11 machine:

```powershell
PS> powershell -File C:\moxiang\scripts\clean-deploy.ps1 -DryRun -SkipTests -SkipSmoke
# Expected exit 0; prints every step without mutating.

PS> powershell -File C:\moxiang\scripts\clean-deploy.ps1 -SkipSmoke
# Expected exit 0; full build + 11,922 / 11,922 ctest PASS, ~120s.

PS> powershell -File C:\moxiang\scripts\clean-deploy.ps1 -InstallPrereqs
# Requires Administrator elevation. Expected exit 0 (~15 min).
```

After the gateway is started, the portal smoke step should hit:

```powershell
PS> curl http://127.0.0.1:8080/api/healthz
{"status":"ok","version":"1.0.0","uptime_seconds":0}
```

## Portal-Specific Notes

The `scripts/commercial-smoke.ps1` step also runs:

```powershell
PS> powershell -File deploy\portal\smoke-ecs.ps1 -PublicUrl http://127.0.0.1:8080/portal
```

Verifying:
- `/api/healthz` -> 200
- `/api/status` -> 200 (Login/Agent/Map up once game servers are running)
- `/` -> 200 (SPA fallback)

If `PORTAL_JWT_SECRET` is unset, the portal exits with code 6 — see
`docs/PORTAL_DEPLOY.md` for the secret bootstrap procedure.

## Release scripts

`deploy/scripts/start_release.ps1`, `deploy/scripts/rollback_release.ps1`, and
`deploy/scripts/healthcheck_release.ps1` prepare a release directory. They do
not copy artifacts, do not contact a host by themselves, and do not deploy
anything. `deploy/scripts/start_current_vm112.ps1` and
`deploy/scripts/start_original_vm112.ps1` are unchanged.

`-PublicHost` is required and has no default. The database connection is the
existing process variable `MXH_DATABASE_CONFIG`. These scripts do not define a
new variable and do not store secrets.

Default listeners are the modern stack: Login `16001`, Agent `17001`, Map
`18001`, map 12 on `18002`, and map 2 on `18003`. Map 17 listens on `18004`
only when `-EnableMap17` is set. `start_release.ps1` refuses to start when any
target port is already listening and prints the owner PID. It writes
`run-state.json` in the release directory with each PID and the executable
SHA-256.

`rollback_release.ps1` stops only PIDs listed in that file whose image path is
under `-ReleaseDir`. It does not stop a process by name. The image path must
match the recorded executable, the SHA-256 must match, and the process start
time must fall in the window described in `deploy/VM112_R2_STAGE.md`. It then
starts `-PreviousReleaseDir` on the same ports. `-WhatIf` and `-DryRun` print
the actions and do not stop or start processes.

`healthcheck_release.ps1` prints `HEALTH OK` or `HEALTH FAIL` for listening
ports, recorded processes, `PROTOCOL_MATCH` when a log contains it, and an
optional portal `/api/healthz` when `-PortalUrl` is set. Any FAIL line exits 1.

### Prerequisites before any real start

Do not point these scripts at VM112, or at any other machine, until all of the
following are true:

1. Pull request #114 is merged. That change generates the client ServerPort from the modern login port (`deploy_client.ps1 -LoginPort` default 16001). This is a hard prerequisite.
2. Pull request #109 is merged. That change requires an explicit PlayDH root for the remaining entry points. This is a hard prerequisite.
3. The agreed merge order has finished: #110 (done), then #117, #107, #114, #116, and #109. This pull request stays draft until those are merged.
4. ROG smoke and soak have passed.
5. Yongjun has made the repository private, or has accepted that it stays public.
6. VM112 staging rehearsal is confirmed by the owner. This item is pending that confirmation.
