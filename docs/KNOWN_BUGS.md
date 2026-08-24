# Active known bugs and blockers

Only unresolved issues belong here. Resolved history is available through Git.

## Blocking the playable release

- Launcher and patch/update flow are not yet a single tested product path.
- Login failure, timeout, retry and return-to-login UI recovery are incomplete.
- Login success display transition is now transactional in the client; the remaining release gap is genuine human verification across DPI, multi-monitor, Alt-Tab and swap-chain failure cases.
- Character select/create lacks complete 3D appearance, equipment and animation presentation.
- `GameLoading` and `MapChange` still require real asynchronous loading states.
- Runtime map rendering does not yet apply all BMHM/TTB/HFL/STM/sky/environment semantics.
- Entity rendering still has placeholder fallback paths and incomplete action/equipment coverage.
- Only a subset of InterfaceScript roots is live-bound to client services.
- Movement, targeting, collision, NPC routing, loot and map transition are simplified.
- BEFF/BEFL/Effect.pak triggers and WAV/3D SFX are not fully wired.
- Full-map, full-resource and genuine legacy/modern visual evidence is incomplete.
- `playdh-current` `Resource/Server/Monster_10.bin` is a distinct 22,766-byte variant that the current AIGroup decoder rejects; the modern three-server Map10 startup therefore fails closed until this variant is decoded 1:1. The 2008 reference variant remains read-only and must not be silently substituted.

## Governance blockers

- The canonical PlayDH tree contains derived runtime files that require classification before removal.
- The read-only legacy reference and both resource-profile manifests must remain hash-verified.
- A clean-machine build and deployment have not yet been re-established after workspace cleanup.

## Rules

Do not close an item because a parser or isolated unit test passes. Close it only when the required runtime behavior, human path and evidence row pass.
