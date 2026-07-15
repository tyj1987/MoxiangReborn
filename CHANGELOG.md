# Changelog — Moxian-Reborn

> All notable changes to the Moxian-Reborn modernization project.
> Format: [Keep a Changelog](https://keepachangelog.com/)

## [0.13.0] - 2026-07-16

### Phase 10 series: Test coverage expansion (Phase 10.4 — Phase 10.23) ✅

This release expands test coverage across the modern runtime
to 783/783 ctest PASS (~12 sec wall). Every public header in
`modern/include/mxh/...` is now covered by at least one
test file, with wire-format pinning (sizes / offsets /
mask values) for every binary on-disk format that legacy
tools can still write.

**Added (commit-by-commit, in order)**

- **Phase 10.4** (11 commits) — Infrastructure + 5 new modules
  + 5 test files: `DATABASE_SCHEMA.md`, `vcpkg.json`,
  `Dockerfile`, `deploy/`, plus `MoxianPacker`, `MoxianGMTool`,
  `MoxianMapEditor`, `MoxianAutoPatcher` + 17 dev utilities.
- **Phase 10.5 / 10.6 / 10.7** — `modern/scratch/` archival
  (167 files → 12 subdir), CHANGELOG test-count sync to
  506/506, `MODERNIZATION_PLAN.md` §9 Phase 10 总结 added.
- **Phase 10.8** — `memory_pool_test.cpp` (11 tests, 5 initially
  DISABLED) + `BufferPool::capacity_` fix for the lazy-allocate
  path.
- **Phase 10.9** — MSVC 19.44 init-lock deadlock fix in
  `mxh::memory::ObjectPool`. Re-enables the 5 DISABLED tests.
  Trade-off: `~ObjectPool()` is now a no-op (small memory leak
  for long-lived processes) to avoid the lazy `std::mutex`
  init deadlock that single-threaded gtest test bodies
  trigger. Documented in the header.
- **Phase 10.10** — `game_types_test.cpp` (25 tests) — wire-
  format pinning for `item_types`, `monster_types`,
  `skill_types`. Collateral findings: `is_empty_slot` returns
  true if EITHER field is zero; `NpcRegen` is 43 bytes (header
  comment said 44).
- **Phase 10.11** — `iocp.cpp` enabled + `iocp_test.cpp`
  (12 tests). Fixed 6 real errors: missing `<mswsock.h>`,
  `sockaddr_in` → `sockaddr_storage` zero-init copy, private
  `process_send_queue` → public, `Mswsock.lib` link, `mxh_net`
  PUBLIC-link `mxh_monitor`.
- **Phase 10.12** — `protocol_test.cpp` (26 tests) — wire-
  format pinning for 12 protocol enums. Collateral: `Category
  ::Npc=37`, `Monster=35` (different from the original guess).
- **Phase 10.13** — `ttb_tile_table_test.cpp` (11 tests) +
  `mlog_test.cpp` (11 tests). Collateral: `LogLevel` underlying
  type is `int` (not `uint8_t`); parser 4-byte input returns
  empty (size<8 early-exit).
- **Phase 10.14** — `chr_motion_test.cpp` (18 tests).
  Collateral: `is_chr` uses `fps < 240` strict less-than;
  `std::span` brace-init doesn't compile C++20.
- **Phase 10.15** — `platform_test.cpp` (19 tests). Collateral:
  `sockaddr_to_string` returns `""` (not `"unknown"`) on null /
  zero-length; needs `SocketGuard` for Winsock init.
- **Phase 10.16** — `message_test.cpp` (21 tests) — covers
  `MsgHeader` (8B), `MsgRoot` (4B), `Message+total_size`,
  `ConnectionId`, `NetError+to_string`, `ServerConfig` /
  `ClientConfig` defaults.
- **Phase 10.17** — `server_handler_test.cpp` (14 tests) — 3
  handlers (Login / Agent / Map) with `MockDbAdapter`. First
  attempt failed (MapHandler ctor needs 3-arg + use_legacy
  framing=true; MockDbAdapter private members); reverted and
  re-landed correctly.
- **Phase 10.18** — `mesh_flag_test.cpp` (30 tests) — render
  flag bitmask. Pinning the legacy wire-format quirk where
  `RENDER_ZPRIORITY_MASK_INVERSE = 0x80ffffff` includes bit 31
  (Z-write flag), so `(flag & ZPRIORITY_INVERSE) | new_prio`
  preserves the Z-write bit unchanged.
- **Phase 10.19** — `math_test.cpp` (28 tests) — VECTOR2/3/4 +
  MATRIX4 + 6 matrix helpers. Collateral: `MatrixLookAtLH` is
  right-handed in the original's convention (target ends up at
  +Z in view space, not -Z as the hpp comment claims); cross-
  product f×up produces a left-handed view.
- **Phase 10.20** — `motion_flag_test.cpp` (15 tests) — motion
  flag bitmask round-trips for KEYFRAME / VERTEX / UV.
- **Phase 10.21** — `file_storage_typedef_test.cpp` (13 tests)
  — wire-format pinning for `FSFILE_HEADER` (32B),
  `FSFILE_ATOM_INFO` (268B), `FSPACK_FILE_INFO` (272B).
  Collateral: the hpp doesn't include the header that defines
  `_MAX_PATH`; the test `#define _MAX_PATH 260` before
  including the hpp.
- **Phase 10.22** — `chx_model_test.cpp` (18 tests) — .chx
  character model parser (32B header + is_chx / parse / load).
  Pins the skeleton-parser contract (header + raw populated,
  vertices / indices empty with TODO(Phase 1.3) for the
  per-table decode).
- **Phase 10.23** — `db_adapter_test.cpp` augmented with 5
  new factory contract tests: concrete-class type identity
  (SqliteAdapter / MssqlOdbcAdapter), case-sensitivity of
  backend names, `is_connected() == false` pin, MSSQL alias
  routing.

**Test count**
- 449 (start of session, 2026-07-15) → 506 (Phase 10.6 sync)
  → 783 (Phase 10.23 end)
- Wall time: ~12 sec for the full ctest run
- Build: 0 error, Debug config

**Cross-project memory entries (agent memory, will help
future projects on different repos)**
- MSVC 19.44 init-lock deadlock with gtest single-threaded
  test body (root cause + 3 fix options).
- `mavis-trash` refuses reparse-point paths; go through mirror.

**Changed**
- `.gitignore`: added `!modern/tests/unit/log/` allow-list and
  several Phase 10 scratch entries.
- `MODERNIZATION_PLAN.md`: §9 Phase 10 总结 added (Phase 10.7).

**Known limitations (carry-over)**
- C-32: host has no Docker / podman / WSL2 — full SQL Server
  runtime smoke is env-blocked. Documented in
  `docs/KNOWN_BUGS.md`.
- C-35: 4/5 Distribute `Debug_<LOCALE>` targets fail (mfc71.lib
  + 4 anonymous enum redefinitions). Shared-header refactor
  would break 1:1 contract.
- `MssqlOdbcAdapter.ConnectToInvalidServerFails` flake on busy
  machines (5s ODBC retry timeout spikes past 30s ctest
  budget). Passes on retry.

## [0.12.0] - 2026-07-10

### Phase 11.2: Protocol Documentation Generator ✅

**Added**
- `MoxianProtocolDoc`: Protocol documentation generator
  - Parse Protocol.h and extract MP_CATEGORY enums
  - Extract MP_PROTOCOL_* enums and values
  - Generate Markdown documentation
  - Generate JSON protocol schema
  - Summary statistics (124 categories, 64 protocol enums, 3458 protocols)

### Phase 12: Continuous Iteration ✅

**Completed**
- 12.1 Feedback collection / bug fixes / performance tuning ✅
- 12.2 Community building / documentation improvement ✅
- 12.3 Client modernization (DX11 + modern UI) ✅
- 12.4 Server performance optimization (IOCP + memory pool) ✅

**Added**
- IOCP-based high-performance network layer
- Memory pool for object and buffer management
- Performance monitoring system
- Memory and network benchmarks

## [0.11.0] - 2026-07-10

### Phase 9.3: Docker Containerization ✅

**Added**
- `Dockerfile`: Multi-stage build for Windows containers
- `docker-compose.yml`: Full stack deployment (Login + Agent + Map + MSSQL)
- `.dockerignore`: Exclude legacy source and build artifacts
- `docker/init-db/init.sh`: Database initialization script
- `docker/config/`: Server configuration templates

### Phase 10: Tool Chain Modernization ✅

**Added**
- `MoxianPacker`: Modern CLI tool for PAK archive management
  - Pack files into .pak archives
  - Extract files from .pak archives
  - List .pak contents
  - Verify .pak integrity
  - CRC32 checksum verification

- `MoxianGMTool`: Modern GM management tool
  - HTTP REST API server
  - Player management (ban, mute, kick, teleport)
  - Item management (give, remove, search)
  - Server monitoring (status, player count, performance)
  - Chat moderation (logs, filters)
  - Event management (create, schedule, monitor)

- `MoxianMapEditor`: Modern map editor
  - Load and view .bmhm map files
  - Edit tile properties
  - Place and manage map objects
  - Export to text format
  - Create new maps

- `MoxianAutoPatcher`: Modern auto-update tool
  - Check for updates via HTTPS
  - Download patches with progress
  - Apply binary diffs (bsdiff/bspatch)
  - Verify file integrity (SHA-256)
  - Rollback on failure
  - Pack files into .pak archives
  - Extract files from .pak archives
  - List .pak contents
  - Verify .pak integrity
  - CRC32 checksum verification

## [0.10.0] - 2026-07-10

### Phase 9: Cross-Platform Support (Partial) ✅

**Added**
- `platform.hpp`: Platform detection macros (Windows/Linux/macOS)
- `platform.hpp`: Socket type aliases and helper functions
- `platform.hpp`: Cross-platform socket address helpers
- `platform.hpp`: Thread ID and filesystem abstractions
- `socket.hpp/cpp`: RAII socket wrapper with:
  - Non-blocking I/O support
  - TCP_NODELAY and SO_REUSEADDR options
  - Address resolution and connection
  - Thread-safe send/receive operations
  - Custom error codes (SocketErrc)
- `socket_test.cpp`: 20 socket tests (address, create, bind, listen, connect, echo server)

**Changed**
- `src/CMakeLists.txt`: Added socket.cpp to mxh_net library
- `tests/unit/net/CMakeLists.txt`: Added socket_test.cpp

## [0.9.0] - 2026-07-10

### Phase 5: Rendering Engine Modernization ✅

**Added**
- `IRenderer.hpp`: 1:1 port of 4Dyuchi IRenderer interface (75 methods)
- `IFileStorage.hpp`: 1:1 port of file storage interface (27 methods)
- `render_typedef.hpp`: Binary-compatible structures with original DX8 engine
- DX11 backend: Device, SwapChain, RenderTarget, default state objects
- HeightField system: CreateHeightField, height field objects
- Material system: CreateMaterial, CreateMaterialSet
- Mesh system: IDIMeshObject, IDIHFieldObject, IDIImmMeshObject
- Font system: IDIFontObject implementation
- Sprite system: IDISpriteObject implementation
- Texture loader: TGA, DDS, BC1/BC3/BC4/BC5 encoders
- Effect shaders: IEffectShader implementation
- Motion cache: per-motion VB/IB tracking
- Deferred renderer: SetRTLight, InitializeRenderTarget
- 141 render tests

### Phase 6: UI System Modernization ✅

**Added**
- `cWindow` base class with DX11 rendering backend
- `cButton`, `cCheckBox`, `cEditBox`, `cTextBox` controls
- `cImage`, `cListCtrl` advanced controls
- `cWindowManager`: top-most dispatch, modal, defer-destroy
- `cMsgBox`: modal 4-type dialog box
- `cDialog`: window management, findWindowById, alpha, positioning
- `cDivideBox`: split-pane container
- Legacy compatibility: WE_* events, cbWindowFunc bridge
- `mxh_ui_smoke`: headless UI integration test
- 254 UI tests

## [0.8.0] - 2026-07-10

### Phase 8: Performance Optimization ✅

**Added**
- `ThreadPool` (Phase 8.1): General-purpose thread pool with `std::counting_semaphore`
- `ObjectPool<T>` (Phase 8.2): Generic object pool for reducing heap allocations
- `compress.hpp` (Phase 8.3): RLE compression for large payloads (threshold: 128 bytes)
- `util_test.cpp`: 17 tests covering ThreadPool, ObjectPool, and Compression

### Phase 7: Build System Completion ✅

**Added**
- `vcpkg.json`: Dependency manifest (gtest + sqlite3)
- `.github/workflows/ci.yml`: GitHub Actions CI/CD pipeline
  - Windows 2022 + MSVC 2022
  - Debug/Release matrix build
  - Automatic test execution
- `net_benchmark.cpp`: TCP throughput & latency benchmark

**Changed**
- `tests/CMakeLists.txt`: 3-mode dependency resolution (vcpkg → vendored → FetchContent)

## [0.7.0] - 2026-07-10

### Phase 4: Network Layer Modernization ✅

**Added**
- Protocol versioning (Phase 4.3):
  - `kProtocolVersion=1`, `kMinProtocolVersion=0`
  - `VersionRejectReason` enum
  - `UserConnProtocol` enum (CheckVersion/NotifyVersionAck/NotifyVersionNack)
  - `LoginHandler::handle_version_check()` version negotiation
- Encryption middleware (Phase 4.4):
  - `IEncryptor` interface with `encrypt()`/`decrypt()` hooks
  - `TcpServer`: encrypts outgoing, decrypts incoming
  - `TcpClient`: bidirectional encryption support
  - `IConnectionHandler::encryptor_for()` virtual method
- `version_test.cpp`: 27 tests for version constants, negotiation logic, payload encoding

**Changed**
- `net.cpp`: TcpClient now supports receive loop with encryption
- `net.cpp`: TcpClient::send() applies encryption via `encryptor_for()`

## [0.6.0] - 2026-07-09

### Phase 3: Crypto Compatibility ✅

**Added**
- `HselStream`: Modern C++ replacement for HSEL_STREAM
- `HselEngine`: Stateful encryption engine
- `crypto_test.cpp`: 23 tests for HSEL encryption/decryption

### Phase 2: Database Layer ✅

**Added**
- `IDbAdapter` interface: Database abstraction layer
- `SqliteAdapter`: SQLite implementation (replaces MSSQL dependency)
- `db_test.cpp`: 11 tests for database operations

## [0.5.0] - 2026-07-08

### Phase 1: Resource Compatibility Layer ✅

**Added**
- `MhFileEx`: BIN file reader (XOR encryption, CRC verification)
- `PackFile`: PAK file parser (4DyuchiFileStorage format)
- `BsadAreaParser`: BSAD skill area file parser
- `TgaLoader`: TGA image decoder (uncompressed/RLE, RGBA32)
- `ResourceExplorer`: CLI tool for inspecting game resources

**Changed**
- `test-extract/`: Added sample resources for testing

## [0.4.0] - 2026-07-07

### Phase 0: Project Setup ✅

**Added**
- CMake build system (`modern/CMakeLists.txt`)
- GoogleTest integration (FetchContent)
- Unit test framework
- `mxh` namespace structure
- Logging compatibility (`MLOG` macro)
- Protocol constants (`Protocol.h` modernization)

**Documentation**
- `MODERNIZATION_PLAN.md`: 12-phase roadmap
- `docs/KNOWN_BUGS.md`: Known issues tracker
- `docs/RESOURCE_FORMATS.md`: Binary format documentation

## [0.3.0] - 2026-07-06

### Initial Project Structure

**Added**
- `modern/` directory for new code
- `include/mxh/` header organization
- `src/` implementation structure
- `tests/unit/` test organization

---

## Test Coverage Summary

| Phase | Test Suite | Tests | Status |
|-------|-----------|-------|--------|
| Phase 0 | Protocol constants | 16 | ✅ |
| Phase 1 | Resource formats | 23 | ✅ |
| Phase 2 | Database adapter | 11 | ✅ |
| Phase 3 | HSEL encryption | 23 | ✅ |
| Phase 4 | Network layer | 30 | ✅ |
| Phase 5 | Rendering engine | 141 | ✅ |
| Phase 6 | UI system | 254 | ✅ |
| Phase 7 | Build system | - | ✅ |
| Phase 8 | Performance utils | 17 | ✅ |
| Phase 9 | Cross-platform socket | 20 | ✅ |
| Phase 10.4.9 | util / version / monitor tests | 57 | ✅ |
| **Total** | | **592** | **506/506 tests passing** |

Last verified: 2026-07-15 (commit 99c9b24, ctest 33.35 sec wall).

---

## Upcoming

- P10.5: archive modern/scratch/ agent leftovers (per AGENTS.md trap #10) — done
- P10.6: sync test count (this commit)
- C-32: real docker compose up mssql + MoxianLoginServer --backend mssql_odbc smoke
