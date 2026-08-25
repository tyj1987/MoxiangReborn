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
- Map1 representative-map smoke is currently blocked by two missing canonical NPC model entries: `NpcChxList.bin` resolves kinds 73 and 59 to `N073.chx` and `N059.chx`, but neither entry exists in the current PlayDH filesystem or the seven PAKs. The client fails closed with `failedModelCount=2`/`placeholderCount=2`; no substitute geometry may be added.
- The `playdh-current/Resource/Server/MonsterDropItemList*.bin` copies are byte-readable but decode to `$Group` AIGroup data; MapServer deliberately routes `playdh-current` to the verified top-level `Resource/MonsterDropItemList.bin` instead. The server-directory copies remain non-canonical and must not be used as a fallback.

## Governance blockers

- The canonical PlayDH tree contains derived runtime files that require classification before removal.
- The read-only legacy reference and both resource-profile manifests must remain hash-verified.
- A clean-machine build and deployment have not yet been re-established after workspace cleanup.

## Rules

Do not close an item because a parser or isolated unit test passes. Close it only when the required runtime behavior, human path and evidence row pass.
