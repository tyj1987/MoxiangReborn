# Playable status

This file is the current player-facing truth. Historical session claims are intentionally not retained here.

## Current state

Status: `IN_PROGRESS — vertical slice integration`

The modern client has protocol and partial rendering foundations, but it is not yet a mature playable release. Evidence-backed facts:

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
