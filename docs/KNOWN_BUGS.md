# Active known bugs and blockers

Only unresolved issues belong here. Resolved history is available through Git.

## Blocking the playable release

- Launcher and patch/update flow are not yet a single tested product path.
- Login failure/timeout now return to the editable login dialog with the account
  retained and password cleared; remaining release gap is genuine human
  verification of every legacy error-code/message variant and retry timing.
- Login success display transition is now transactional in the client; the remaining release gap is genuine human verification across DPI, multi-monitor, Alt-Tab and swap-chain failure cases.
- Character select/create lacks complete 3D appearance, equipment and animation presentation.
- `GameLoading` and `MapChange` now use the staged runtime load pipeline and have a Map10→Map12 E4 E2E gate; remaining release gap is GUI/human visual verification and failure-injection coverage across every map/profile.
- Runtime map rendering does not yet apply all BMHM/TTB/HFL/STM/sky/environment semantics.
- Entity rendering still has placeholder fallback paths and incomplete action/equipment coverage.
- Only a subset of InterfaceScript roots is live-bound to client services.
- Movement, targeting, collision, NPC routing, loot and map transition are simplified.
- BEFF/BEFL/Effect.pak triggers and WAV/3D SFX are not fully wired.
- Full-map, full-resource and genuine legacy/modern visual evidence is incomplete.
- `playdh-current` `Resource/Server` uses the distinct `server-size-prefixed-opaque-v1` profile encoding. Map10 (22,766 bytes) now decodes through the recovered 20-byte container header and eight-byte XOR body transform, and the three-server Map10 startup is operational. Full all-map coverage is still open for small/nonstandard opaque entries; the 2008 `mhfileex-classic-rxmso` profile remains read-only and must not be silently substituted.
- Map101 representative-map loading is currently blocked by 40 unresolved HFL terrain textures (the Tatan background/area texture family). The staged loader rejects the map before STM/entity activation; no debug texture fallback is allowed in release.
- The `playdh-current/Resource/Server/MonsterDropItemList*.bin` copies are byte-readable but decode to `$Group` AIGroup data; MapServer deliberately routes `playdh-current` to the verified top-level `Resource/MonsterDropItemList.bin` instead. The server-directory copies remain non-canonical and must not be used as a fallback.

## Active uncommitted fixes (waiting for user commit decision)

- `modern/src/schema_migration.cpp` line 213 — T-SQL reserved word `option` quoted as `[option]` in `CREATE TABLE modern_party`. Without the fix, MSSQL bootstrap of `modern_party` fails; SQLite is unaffected. 8/31 上一 session 留下。
- `modern/src/server/map_handler.cpp` line 549 — same `[option]` quoting in `INSERT INTO modern_party(party_id,[option])`. Without the fix, MSSQL `persist_party` throws; SQLite is unaffected. 8/31 上一 session 留下。

## Deferred items (per 2026-08-31 plan decision)

- 5 个 ui 头文件 out-of-sync (`ccheckbox.hpp` 等)：脚本 `-Fix` 默认 `src/ui → include/mxh/ui`，但实测 3 个文件 (ccheckbox.hpp 1.5天 / ccombobox.hpp 1分 / ctextarea.hpp 3分) include 比 src 新，盲目 -Fix 会回滚。**本 plan 留 G10**，等用户拍板方向后单独 commit。
- 3 个 include-only 头文件 (`cAni.hpp` / `cItemShopInven.hpp` / `resolution_mode.hpp`)：原计划标"决策待定"，但实测这 3 个头文件**被 cDialogLoader.cpp / cWindowManager.cpp 主动 include**，属于"include-only 头是设计如此"（R-36/R-37 注释允许这种情况）。**G-4 不需要操作**。

## Governance blockers

- The canonical PlayDH tree contains derived runtime files that require classification before removal.
- The read-only legacy reference and both resource-profile manifests must remain hash-verified.
- A clean-machine build and deployment have not yet been re-established after workspace cleanup.

## Rules

Do not close an item because a parser or isolated unit test passes. Close it only when the required runtime behavior, human path and evidence row pass.
