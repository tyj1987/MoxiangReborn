# Session Handoff — 2026-07-16

> Cross-session handoff doc. Read this at the start of your
> next session, before anything else. It documents what the
> 2026-07-15 → 2026-07-16 session did, what is open, and
> what to do next.

## TL;DR

- **HEAD**: `98e2375` (Phase 10.25 — CI helper scripts)
- **Test count**: 783/783 ctest PASS, ~12 sec wall, Debug config
- **Build**: 0 error, 102 vcxproj targets, 5/5 server matrix
- **Session duration**: ~8.2 hours (2026-07-15 17:37 → 2026-07-16 01:59)
- **Commits this session**: 38 (Phase 10.2 → Phase 10.25)
- **Net test growth**: 404 → 783 (+379 tests, +94%)

## What this session did

### Phase 10.4 (11 commits, 2026-07-15)
Infrastructure + 5 modern modules + 5 new test files:
- `.gitignore` expansion (Phase 10.4.0)
- `docs/DATABASE_SCHEMA.md` (Phase 10.4.1)
- `vcpkg.json` — gtest 1.14+ + sqlite3 3.45+ (Phase 10.4.2)
- `Dockerfile` + `docker-compose.yml` (Phase 10.4.3)
- `modern/scripts/` — 17 dev utilities (Phase 10.4.4)
- `scripts/` — SQL Server setup + DB restore (Phase 10.4.5)
- `deploy/` — operational subset (Phase 10.4.6)
- 5 modern modules — game / memory / monitor / util / iocp (Phase 10.4.7)
- `map_handler.cpp` — MapServer business code (Phase 10.4.8)
- 5 new test files + CMake wire-up (Phase 10.4.9)
- CHANGELOG.md + .gitignore housekeeping (Phase 10.4.10)

### Phase 10.5–10.7 (housekeeping)
- 167 files → 12 subdir archive (`modern/scratch/_archive_2026-07-15/`)
- CHANGELOG.md test count sync (430+ → 506/506)
- `MODERNIZATION_PLAN.md` §9 Phase 10 总结 added

### Phase 10.8–10.25 (test coverage expansion)
17 new test files / 277 new tests:
- **P10.8** memory_pool_test (11) + BufferPool capacity_ fix
- **P10.9** MSVC 19.44 init-lock deadlock fix in ObjectPool
- **P10.10** game_types_test (25) — item/monster/skill wire-format
- **P10.11** iocp.cpp enable + iocp_test (12) + 6 real error fixes
- **P10.12** protocol_test (26) — 12 protocol enums
- **P10.13** ttb_tile_table (11) + mlog (11)
- **P10.14** chr_motion_test (18)
- **P10.15** platform_test (19)
- **P10.16** message_test (21)
- **P10.17** server_handler_test (14) — Login/Agent/Map handlers
- **P10.18** mesh_flag_test (30) — render flag bitmask
- **P10.19** math_test (28) — VECTOR + MATRIX4 + helpers
- **P10.20** motion_flag_test (15)
- **P10.21** file_storage_typedef_test (13) — .pak wire-format
- **P10.22** chx_model_test (18) — .chx parser
- **P10.23** db_factory contract (5) — concrete-class identity
- **P10.24** CHANGELOG + MODERNIZATION_PLAN sync (wrap-up)
- **P10.25** CI helper scripts (test-count + CRLF guards)

## Coverage audit (all 28 hpp files covered)

All public headers in `modern/include/mxh/...` have at
least one test file. Excluded:
- `render/IFileStorage.hpp` — pure abstract interface
- `render/IRenderer.hpp` — pure abstract interface
- `render/render_typedef.hpp` — 33.5 KB, mostly typedefs

Wire-format pinning (sizes / offsets / mask values) covers
every binary on-disk format that legacy tools can still
write: `.pak`, `.chx`, `.chr`, `.ttb`, `.bsad`, `.bmhm`.

## Cross-project memory (agent permanent memory)

Two entries added this session that will help future
projects on different repos:

1. **MSVC 19.44 init-lock deadlock with gtest single-
   threaded test body** — symptom / root cause / 3 fix
   options (cheapest first) / 命中点.
2. **mavis-trash refuses reparse-point paths** — `D:\Moxian`
   is a reparse point to the CJK mirror; pass mirror path
   explicitly to mavis-trash.

## Carry-over limitations

### C-32: SQL Server runtime smoke
- code-layer ✅ (MssqlOdbcAdapter + factory + 9 tests)
- build-layer ✅ (5/5 server matrix compiles with
  `--backend mssql_odbc`)
- container-image ✅ (docker-compose.yml + Dockerfile +
  init.sh + config/{login,agent,map}/)
- runtime connection ❌ — host has no docker / podman /
  WSL2. **User decision required**: install Docker
  Desktop, or WSL2, or native SQL Server 2022 Express
  + Native Client ODBC. See `docs/KNOWN_BUGS.md` C-32.

### C-35: 4/5 Distribute locale builds
KOR/CHINA/JP/HK/TL: only CHINA builds clean. Other 4
fail with mfc71.lib missing + 4 anonymous enum
redefinitions. Shared-header refactor would break 1:1
contract, so not fixed. Documented.

### MSSQL ODBC test flake
`MssqlOdbcAdapter.ConnectToInvalidServerFails` flake on
busy machines (5s ODBC retry timeout spikes past 30s
ctest budget). Passes on retry.

## Open questions for next session

1. **C-32 user decision** — when will docker / WSL2 /
   native SQL Server be installed? Until then, the full
   SQL Server runtime smoke is env-blocked.
2. **C-35 locale builds** — accept the 1/5 build rate,
   or refactor shared headers (which would break 1:1)?
3. **Phase 11 follow-ups** — the CHANGELOG says Phase 11
   is done, but the user's earlier `MODERNIZATION_PLAN`
   lists Phase 11 as "continuous iteration" with no end
   date. Verify the actual current state vs the plan.

## Quickstart for next session

```bash
# 1. Verify HEAD
git -C D:\Moxian log --oneline -1    # expect: 98e2375 ...

# 2. Verify build / test baseline
cmake --build D:\Moxian\modern\build --config Debug
ctest -C Debug --test-dir D:\Moxian\modern\build --timeout 30
# expect: 783/783 PASS, ~12 sec

# 3. Verify CI guards work
python D:\Moxian\modern\scripts\ci_test_count_guard.py \
    --build-dir D:\Moxian\modern\build --min-tests 700
python D:\Moxian\modern\scripts\ci_line_ending_guard.py
# both expect: exit 0

# 4. Check git status (any uncommitted work?)
git -C D:\Moxian status --short
```

## Files the next session should know about

| Path | What it is |
|------|------------|
| `AGENTS.md` | Project-wide AI agent guide (read first) |
| `MODERNIZATION_PLAN.md` | 12-phase roadmap, §9 has Phase 10 summary |
| `CHANGELOG.md` | 0.13.0 entry covers Phase 10 series |
| `docs/KNOWN_BUGS.md` | C-32, C-35 carry-over limitations |
| `modern/scripts/ci_test_count_guard.py` | CI guard (P10.25) |
| `modern/scripts/ci_line_ending_guard.py` | CI guard (P10.25) |
| `modern/scratch/project_status_2026-07-16.html` | Visual status report |
| `D:\Moxian\.gitignore` | 200+ entries, including `.github/`, `modern/scratch/` |

## Gotchas hit this session (already in memory)

These are in the agent's permanent memory now, but listed
here for completeness:

- `D:\Moxian` is a reparse point to the CJK mirror.
  Use `Get-ChildItem -LiteralPath` for CJK paths. Use
  mirror path for `mavis-trash`.
- PowerShell 5.1 `Add-Content -Encoding UTF8` writes BOM.
  Strip with Python before `git add`.
- `std::span<const uint8_t>{...}` brace-init doesn't
  compile C++20 — use `std::vector<uint8_t>`.
- `Set-Location` on CJK path sometimes fails — use
  `& git -C "D:\Moxian" ...` instead.
- ODBC header order: `<windows.h>` MUST come before
  `<sql.h>`. Do NOT set `WIN32_LEAN_AND_MEAN`.
- MSVC 19.44 `std::mutex` lazy init + gtest single-threaded
  test body → deadlock. Pre-warm mutex in ctor or skip
  dtor cleanup.

## Session log details

For the full commit-by-commit log:

```bash
git -C D:\Moxian log --oneline bafc20d..HEAD
```

38 commits, all merge into main. No PRs opened (no remote
configured).
