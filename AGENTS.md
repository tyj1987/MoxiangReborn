# AGENTS.md

Moxian (DarkStory) — 2003-2010 Korean 2D MMORPG, modernized 1:1 port. Two coexisting trees:
- `墨香【源码】/` — legacy MSVC 6/2003-era source, **READ-ONLY reference**
- `modern/` — active C++20 rewrite (this is where work happens)

## Setup commands

- Configure: `cmake -S modern -B modern/build -G "NMake Makefiles"` (x86; vcvars32.bat first)
- Build:     `pwsh -File scripts/build-modern.ps1` (vcvars32 + nmake)
- Test:      `ctest -C Debug --test-dir modern/build` (12013 tests)
- Smoke:     `pwsh -File scripts/gui-client-smoke.ps1` (6 state frames: connect/login/charselect/charmake/gameloading/gamein)
- Resources: `modern/data/PlayDH/` (built from `墨香【源码配套资源】/PlayDH/`)
- Server:    `pwsh -File scripts/start_modern.ps1 -Mode start` (Login+Agent+Map+SQLite/MSSQL)

## Project layout

- `墨香【源码】/` — legacy 2003-era code (read-only)
- `墨香【源码配套资源】/` — PlayDH resource tree (read-only)
- `modern/src/` — modern C++20 source
- `modern/include/` — public headers (mxh/*)
- `modern/tests/unit/` — gtest unit tests
- `modern/tools/` — MoxianClient, MoxianClientE2E
- `scripts/` — PowerShell build/smoke/deploy/verify scripts
- `docs/` — long-form documentation
- `deploy/` — production deployment scripts + Docker

## 4 不可破坏约束 (do not violate)

1. **资源字节 1:1** — `.bin` / `.pak` / `.tif` / `.dds` / `.tga` / `.bmhm` / `.ttb` / `.chl` / `.chx` / `.chr` / `.mon` / `.bsad` must read byte-for-byte identical to the legacy originals
2. **modern 网络闭环** — `[CC]Header/Protocol.h` + `CommonStruct.h` are read-only references; modern client/server must interoperate (Login+Agent+Map), but old↔new is not required
3. **玩法 / 数值 1:1** — exp curves, damage formulas, drop rates, Boss spawn, shop prices, MurimNet PvP are immutable
4. **HSEL / HackShield / nProtect 接口签名** — implementations may be replaced (stubs OK), but the signatures must stay

## Code style

- **modern** — C++20 strict, camelCase methods, `m_` members, `g_` globals, `std::unique_ptr` for ownership, `#pragma once` headers
- **legacy (read-only)** — Hungarian notation, MFC, MSVC 6.0 compatibility; do not touch
- Headers: separate .hpp declarations from .cpp bodies for non-trivial types
- Commit: 1 commit = 1 sub-task (imperative subject, e.g. `dialog: G2 verify M-R4.1+`)

## Testing instructions

- Unit tests live in `modern/tests/unit/<module>/` as `*_test.cpp` (gtest hand-rolled main, no gtest_main)
- Run a single target: `ctest -C Debug --test-dir modern/build -R <pattern>`
- Visual verify: `python scripts/verify-g4-ssim.py` (G4 SSIM ≥ 0.95, 800x600 1:1 vs legacy baseline)
- All new code paths need a test before commit

## Security

- `.env` and secrets are in `.gitignore`; never commit
- HSEL hardware dog + HackShield + nProtect interfaces must keep their public signatures even when stubbed

## Reference

- Long-form knowledge preserved in `AGENTS.md.bak.1787145351` (pre-init handbook) — recoverable via `git show HEAD:AGENTS.md`
- Session bootstrap: `pwsh -File scripts/session-bootstrap.ps1` (read-only, audit working tree)
