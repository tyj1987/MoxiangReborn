# Clean-Machine Deployment

> Status: 2026-08-11. ROADMAP M6-A. 1.0 release-readiness gate.

## Purpose

`scripts/clean-deploy.ps1` takes a blank Windows box (Windows Server 2022 or
Windows 10/11) with only PowerShell and Git installed, and bootstraps it into a
fully built + smoke-verified modern server in one command.

This is the §5.E "clean machine deployment" gate from ROADMAP.

## Quick Start

```powershell
PS> powershell -ExecutionPolicy Bypass -File C:\moxiang\scripts\clean-deploy.ps1 -InstallPrereqs
```

After ~15 minutes (depending on VS Build Tools download size), the machine will have:
- Modern stack built into `C:\moxiang\modern\build\`
- 11,863 / 11,863 unit tests passing
- PlayDH junction pointing at `墨香【源码配套资源】\PlayDH`
- commercial-smoke verified (Login/Agent/Map + GUI client + BGM)

## Steps Performed

| # | Step | Description |
|---|------|-------------|
| 1 | preflight | admin check, OS, RAM, disk |
| 2 | prereq detect | VS2022, cmake, git, sqlcmd, SqlLocalDB, ODBC 18, VC++ Redist |
| 3 | prereq install | (only with `-InstallPrereqs`) chocolatey + direct download for ODBC 18 + VS Build Tools 2022 |
| 4 | PlayDH junction | `modern/data/PlayDH` -> `<RepoRoot>\墨香【源码配套资源】\PlayDH` |
| 5 | build modern | invokes `cmake --build modern/build --config <Config>` |
| 6 | run ctest | `ctest -C <Config> --test-dir modern/build --output-on-failure` |
| 7 | commercial smoke | invokes `scripts/commercial-smoke.ps1` |

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

