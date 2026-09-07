# Playable status

This file is the current player-facing truth. Historical session claims are intentionally not retained here.

## Current state

Status: `IN_PROGRESS — vertical slice integration`

The modern client has protocol and partial rendering foundations, but it is not yet a mature playable release. Evidence-backed facts:

- **2026-09-07 visual / UI / map polish session (scratch/2026-09-07-visual-polish/)**:
  - Resource tooling landed: `unpack_pak.py` upgraded to named/manifest/HFL-sniff mode (8/8 ctest) and `polish_assets.py` (6/6 ctest) generating 234 differentiated files for the 5 placeholder classes (65 `maploadingimage*.dds` 1024x768, 32 `LoadingTip*.dds` 256x128, 3 `now_loading*.TIF` 256x64, 65 `mini_*.dds` + 65 `mini_*_ful.dds` 256x256, and 4 UI backgrounds 768x1024 BGRA8 — `login.dds` retains the sky-band layout that `mxh_texture_loader_test::LoginDdsSkyBandIsOnTopAfterTitleFlip` depends on).
  - HFL/STM sync: 3 HFL (`10/21/101.hfl`) + 1 STM (`10.stm`) copied from `out/runs/`, `tmp-extract/`, and `scratch/2026-08-26-map21/` to `Resource/Map/`. `HflHeightField.*` (5/5) and `StmStaticModel.*` (7/7) ctests stay green; map10/21/101 are now renderable in principle.
  - 7 `.pak` re-investigated: each one holds **1 valid entry** (the rest is metadata/index), so the `Map.pak`/`Character.pak` etc. do not contain the model/HFL/STM assets the plan assumed. Real assets are already on disk under `modern/data/PlayDH/`.
  - 12,429 ctests pass 100% (excluding 6 SQL Server / in-game smoke Skips); no new failures introduced.
  - **UI P0 1:1 port status: 5/5 dialog hpp complete (1:1 API), 0/5 cpp implementations landed** (cpp stubs preserved at git HEAD to avoid breaking 12,021 ctests that depend on the empty-body 1:1 surface). The "visual one-paste-blob" symptom remains; this session proves the resource side is clean, but a real human-visible acceptance run is not in scope.

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
