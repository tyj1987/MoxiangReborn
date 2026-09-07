# Playable status

This file is the current player-facing truth. Historical session claims are intentionally not retained here.

## Current state

Status: `IN_PROGRESS — vertical slice integration`

The modern client has protocol and partial rendering foundations, but it is not yet a mature playable release. Evidence-backed facts:

- **2026-09-07 visual / UI / map polish session (scratch/2026-09-07-visual-polish/)**:
  - Resource tooling landed: `unpack_pak.py` upgraded to named/manifest/HFL-sniff mode (8/8 ctest) and `polish_assets.py` (16/16 ctest) generating 234 differentiated files for the 5 placeholder classes (65 `maploadingimage*.dds` 1024x768, 32 `LoadingTip*.dds` 256x128, 3 `now_loading*.TIF` 256x64, 65 `mini_*.dds` + 65 `mini_*_ful.dds` 256x256, and 4 UI backgrounds 768x1024 BGRA8 — `login.dds` retains the sky-band layout that `mxh_texture_loader_test::LoginDdsSkyBandIsOnTopAfterTitleFlip` depends on).
  - **polish_assets minimap fold mode** (2026-09-07 follow-up): `--generate-minimap` now scans the existing 80 real minimap DDS in `Image/MiniMap/` and only synthesizes the 0 missing entries by default. New `--force` rewrites all 65 maps, `--audit` reports existing vs missing counts without writing. 80/65 real minimap DDS stay byte-identical after the run; no more clobbering shipped assets.
  - **HFL placeholder generation** (2026-09-07 follow-up): `gen_hfl_placeholders.py` synthesizes a 16x16 procedurally-noised height grid for every map id whose `Map<N>.bmhm` exists but `<N>.hfl` is missing, using `10.hfl` as a header template (mutates only `height_count_x/z` + `width/height`; preserves the 37-entry texture table verbatim). **78 placeholder HFLs written** (`0/1/2/3/4/5/6/7/8/9/11/12/13/14/15/17/18/19/20/22/23/24/25/26/27/28/32/33/34/35/37/39/40/42/44/54/55/58/59/60/65/66/67/68/69/70/72/73/75/94/95/96/97/99/100/102/103/104/105/106/108/109/110/114/115/116/117/118/198/199/200/201/202/203/204/205/206/207.hfl`), bringing `Resource/Map/` to **81 HFL files** (3 real: 10/21/101; 78 placeholder). Modern `parse_hfl` round-trips the placeholder bytes: `HflHeightField.PlaceholderFilesParse` (3/3 ctest) green.
  - **CaptureScreen TGA round-trip verified** (2026-09-07 follow-up): `CoD3DDeviceDX11::CaptureScreen` (modern/src/render/dx11/renderer.cpp:908) writes the back buffer as a TGA via `saveTGA`. Added `TgaSaveRoundTrip.EncodeDecodeMatchesInput` and `TgaSaveRoundTrip.EmptyTextureReturnsEmpty` ctests (2/2 PASS) to lock down the encode/decode property that any future CaptureScreen rewrite must preserve. No live D3D11 device is required.
  - HFL/STM sync: 3 HFL (`10/21/101.hfl`) + 1 STM (`10.stm`) copied from `out/runs/`, `tmp-extract/`, and `scratch/2026-08-26-map21/` to `Resource/Map/`. `HflHeightField.*` (5/5 + 1 placeholder = 6/6) and `StmStaticModel.*` (7/7) ctests stay green; map10/21/101 are now renderable in principle.
  - 7 `.pak` re-investigated: each one holds **1 valid entry** (the rest is metadata/index), so the `Map.pak`/`Character.pak` etc. do not contain the model/HFL/STM assets the plan assumed. Real assets are already on disk under `modern/data/PlayDH/`.
  - 12,432 ctests pass 100% (excluding 6 SQL Server / in-game smoke Skips); no new failures introduced. The +3 ctests come from `mxh_gen_hfl_placeholders_tests` (Python unittest harness, 17 sub-cases) and `HflHeightField.PlaceholderFilesParse` and the 2 new `TgaSaveRoundTrip` ctests.
  - **UI P0 1:1 port status: 5/5 dialog hpp complete (1:1 API), 5/5 cpp implementations landed** (`cquickdialog.cpp` 1→65 行 1:1 port; `minimapdlg.cpp` and 3 hpp-inline dialogs already at git HEAD). The "visual one-paste-blob" symptom remains; this session proves the resource + capture path is clean, but a real human-visible acceptance run is not in scope.
  - **DistributeServer 4 LOCALE target — STILL BLOCKED by C-35 (shared header enum)**: building the 4 `DistributeServer_Debug_KOR/JP/HK/TL` targets hits mfc71.lib + 4 anonymous-enum redefinitions in `[Server]Distribute/`. Constraint: cannot modify the legacy `[Server]Distribute/` source per AGENTS.md §0 ("不动老源码"). The CHINA locale target builds cleanly. The 4-locale block is a known dead-end in the modern/legacy boundary; no follow-up is committed until a shared-header refactor is explicitly authorized.

- Login, character-list/create protocol paths and several UI parser tests exist.
- Map10 has real monster data and its current opaque AIGroup resource now decodes into 114 groups / 228 spawns; it remains the first combat acceptance map, but no human combat E5 has been recorded yet.
- The headless client E2E can now explicitly target Map10: a fresh SQLite run completes LoginAck, character creation/selection, GameInAck(map=10), effect/skill catalog loading, and MonsterAdd events with the canonical PlayDH root. This is protocol/resource integration evidence, not a visible or human-playable acceptance.
- The client now has a staged GameLoading/MapChange pipeline, resource-backed character/entity rendering, post-LoginAck 800×600 → saved-resolution transition, and automated Map10 inventory/skill HUD interaction evidence. These are integration gates, not proof of a finished commercial client.
- Remaining gaps include human credential/login recovery acceptance, complete live UI service binding, legacy visual/audio comparison, full combat/loot/map-change human flow, signed remote update transport, and unresolved canonical assets or server tables across Map0, Map4, Map7, Map9, Map17, Map20, Map39, Map67, Map70, Map101, Map102, Map105, Map108, Map110, Map201, Map202, Map204, Map205 and other blocked maps recorded in the verification matrix. Map1 is no longer listed as blocked.
- Automated/headless E2E and unit tests do not prove a visible, human-playable game.

## Evidence policy

Every PASS must link to a commit, resource-profile manifest, command and evidence ID in `docs/VERIFICATION_MATRIX.md`. The following do not qualify as final release evidence by themselves:

- CTest totals.
- Parser counts.
- Headless state transitions.
- Auto-login or auto-create screenshots.
- A modern frame compared with another modern frame.

## Next gates

1. G0/G1: protected legacy reference, explicit `playdh-current` profile, no scratch runtime dependency.
2. G2: clean Git tree, clean build, current documents only, no secrets or self-junctions.
3. G4: launcher/login human acceptance and 800×600 → saved 1024×768 client transition.
4. G5: human character preview/create and loading/map-change acceptance.
5. G6–G9: Map10 no-placeholder vertical slice with UI, combat, effects and audio.
6. G10–G11: all active resources/maps/UI plus clean-machine and soak evidence.
