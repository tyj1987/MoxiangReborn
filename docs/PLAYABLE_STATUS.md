# Playable status

This file is the current player-facing truth. Historical session claims are intentionally not retained here.

## Current state

Status: `IN_PROGRESS — vertical slice integration`

The modern client has protocol and partial rendering foundations, but it is not yet a mature playable release. Evidence-backed facts:

- Login, character-list/create protocol paths and several UI parser tests exist.
- Map10 has real monster data and is the first combat acceptance map.
- The runtime still contains simplified state transitions, incomplete live UI service binding, placeholder entity paths, incomplete map environment/collision, and missing effect/SFX runtime coverage.
- Login failure recovery, launcher/update integration and post-LoginAck display transition require implementation.
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
3. G4: launcher/login and 800×600 → saved 1024×768 client transition.
4. G5: real character previews and loading/map-change states.
5. G6–G9: Map10 no-placeholder vertical slice with UI, combat, effects and audio.
6. G10–G11: all active resources/maps/UI plus clean-machine and soak evidence.
