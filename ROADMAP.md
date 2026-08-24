# Moxian modern roadmap

## Current truth

The project is in the vertical-slice integration stage, not final polish. Protocol and server foundations, resource parsers, partial DX11 rendering and several UI trees exist, but the complete player-visible chain is not yet release-ready.

Authoritative status is maintained in:

`README.md → ROADMAP.md → docs/PLAYABLE_STATUS.md / docs/KNOWN_BUGS.md / docs/VERIFICATION_MATRIX.md`

## Execution order

1. Protect recovered source and resource provenance; remove hidden scratch dependencies.
2. Normalize Git, build outputs, secrets, scripts and current documentation.
3. Introduce explicit resource profiles and client settings/display infrastructure.
4. Complete launcher → login → LoginAck → display transition.
5. Complete character select/create previews and real GameLoading/MapChange.
6. Complete Map10 with real terrain, entities, UI, interaction, combat, effects and audio.
7. Expand all active UI, maps, entities, skills, audio and player-visible systems.
8. Run legacy comparison, clean-machine deployment, fault injection and long soak.

## Non-negotiable constraints

- Do not modify `[CC]Header/Protocol.h` or `CommonStruct.h`.
- Do not change resource bytes, gameplay formulas or HSEL/HackShield/nProtect signatures.
- `playdh-current` is the only normal runtime profile.
- The recovered 2008 SWorking material is read-only reference data.
- Release evidence must include real human input; `--auto-*` is never a release gate.
- A parser or unit-test pass is not a visual or playable pass.

## Completion gate

The roadmap is complete only when a clean-machine user can update, log in, transition from 800×600 to the saved 1024×768 default, select/create a fully rendered character, load Map10, interact with UI/NPCs, fight, use skills, see/hear effects, pick up and equip items, change maps, recover from faults and relog with persisted state. Full-resource and full-map coverage must be recorded in `docs/VERIFICATION_MATRIX.md`.
