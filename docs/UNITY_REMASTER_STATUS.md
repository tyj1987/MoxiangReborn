# Unity remaster — implementation status

## 2026-10-01 evidence update — source snapshot and ROG acceptance boundaries

This versioned update applies to `codex/fix-pickup-loop-20261001` at
`eeb92238012e8849f6173686fcd2b99a036f48f8`, derived from the selected ROG
source snapshot `99b3a72f6fe5093c07495a311d007f6e5a51ea75`. The branch and
baseline under Approved scope below describe the historical remaster approval,
not the current audit checkout. Historical entries remain dated evidence;
their passing counts do not automatically apply to this snapshot.

The snapshot selected source changes and excluded assets from that source
sync. Consequently the checkout's asset tree can differ from the ROG workspace
and from paths described in older reports. An omitted asset, an unhydrated LFS
pointer, and an invalid/missing canonical source are different conditions;
none alone proves missing implementation. No asset completeness or full-game
completion percentage is inferred from this source snapshot.

### Windows code regression at eeb9223

ROG reported the following results for the full SHA above; these are external
Windows executions reported by the coordinating thread, not cloud Linux runs:

| Suite | Reported result | What remains outside this evidence |
|---|---|---|
| Native / equipment / schema | 114/114 passed | Unity rendering and human gameplay |
| Two previously failing handler cases | 2/2 passed | Full Player flow |
| EquipmentPersistence | 3/3 passed | Human equip/combat/pickup/relogin sequence |
| Complete handler suite | 194/194 passed; 3 additionally disabled | Disabled coverage, deployment and live Unity acceptance |

The production and fixture changes are documented in
[Equipment handler regression](EQUIPMENT_HANDLER_REGRESSION_20261001.md) and
[Equipment combat hydration](EQUIPMENT_COMBAT_HYDRATION_20261001.md).
Their earlier "Windows rerun pending" statements record the state before this
ROG result. This update resolves that code-regression rerun only; it does not
resolve the visual, gameplay or release gates below.

### Visual evidence remains incomplete

- **Map10 population:** early 228-monster/6-kind results establish component,
  instance and event counts. The reviewed early screenshot does not establish
  a discernible player, trees or monsters. Counts and first-hit events are not
  readable-scene or human combat acceptance. See the existing
  [pickup probe scope](UNITY_PICKUP_LOOP_PROBE_20261001.md) and its distinction
  between first hit and kill/drop/pickup/persistence.
- **Cold import:** at `132950b02fa978318845d2f0e62c731cfdabe6bb`, ROG reported
  DefaultAsset/0 chunks on first import and 3/5 new EditMode cases passing.
  One recovery produced 64 chunks/renderers and 13 DDS, which does not pass
  the cold-import gate. Shader dependency/load changes in
  `b349232215a30e4ee624ca0efbde903f13dc147f` and the assertion/geometry-propagation
  follow-up `c10b49be6861d0d8e49901d55e50f2a07e3a1380` have **not received a
  reported ROG rerun**. The two failed assertions and the separate null-shader
  log must not be conflated; details and the current six-case rerun are in
  [Terrain import fix](UNITY_TERRAIN_IMPORT_FIX_20261001.md).
- **NPC:** ROG reports that a real FBX imported successfully. Play-mode
  presentation, live identity/interaction, animation and human inspection
  remain untested. This does not replace the separate NPC/B16 executor's
  acceptance or justify generating substitute assets.
- **Map17:** the ROG source audit found two real 513×513 HFL candidates;
  the loose 16×16 file remains a generated placeholder. The standard package's
  42-DDS HFL closure lacks nine DDS and its STM references missing `test4.tif`;
  another version has 440 DDS references with seven missing. These versions
  must not be mixed. Standard-package `17.map` references to its HFL/STM were
  verified. L001 CHX/model/textures/12 actions have hashes and a successful
  exporter report, but neither that result nor source availability is Unity
  scene acceptance. Visual integration remains blocked pending the bounded
  same-source search; no Map17 scene expansion is claimed here.

ROG raw logs, screenshots and asset receipts are external evidence, not files
published by this documentation commit. Exact ROG log locations/run identifiers
for this update have not yet been supplied to this checkout; they must be
attached by the executing/coordinating thread before independent reproduction.
Paths such as historical `modern/out/unity-remaster/...` entries below identify
the executor's output layout, not artifacts guaranteed to exist in every clone.
Repository links above provide reviewable implementation reports; they are not
substitutes for the external raw logs. No private configuration is included.

### Gate disposition (2026-10-01)

A remains **Partial** (source/dependency closure); B remains **Partial**
(the reported code suites pass, but not every runtime/fault case); C remains
**Not accepted** (cold import, discernible visual sample and human review);
D remains **Not accepted** (complete real playable chain and two-player/DB
acceptance); E and F remain **Not accepted** (full-game and release conditions).
No deployment, PR merge, visual approval or release promotion accompanies this
documentation update. The historical gate table below is retained unchanged.

## Approved scope

Branch: `codex/unity-client-remaster`, source baseline
`89a11b3653549bab7471f53f8f0aafb72e98ce2a`.
Windows x64 Unity 6000.6.0f1, URP 17.6.0, D3D11. Modern high-detail wuxia
presentation, reuse and selective remake of existing assets. Preserve original
gameplay, values, map topology, input semantics and protocol/security signatures.
The modern Login/Agent/Map servers remain authoritative. No Unity database access.

Original resources remain immutable. `unity-remaster-v1` is an explicitly
authorized derived visual profile linked to `playdh-current` gameplay. This
exception permits visual remakes, not changed collision, skill timing or balance.

## Gates (2026-09-11)

| Gate | Status | Remaining acceptance |
|---|---|---|
| A: baseline | Partial | full gameplay/UI dependency inventory and source selections |
| B: Unity/native core | Partial | domain reload, fault coverage and complete command/state extraction; initial real flow verified |
| C: visual sample | Not accepted | actual Map10/character/material/animation sample and human review |
| D: Map10 playable | Not accepted | two-player input, gameplay, map change, SQLite/MSSQL |
| E: full game | Not accepted | all original target systems, maps and assets |
| F: release candidate | Not accepted | clean machine, signed updates/rollback, performance, 24-hour soak |

The inherited gameplay register is `docs/UNITY_GAMEPLAY_COVERAGE.md`. Trade,
MurimNet client integration and paid-shop fulfillment are explicitly not complete.

Existing `PLAYABLE_STATUS.md` and `VERIFICATION_MATRIX.md` remain evidence for the
DX11 client; their passes do not automatically transfer to Unity. Original E5
continues to mean legacy comparison plus human review. Remaster art acceptance
is separate and must not be labelled pixel-identical original rendering.

## Resource baseline

Map10 source-material restoration (2026-09-11): the 13 used HFL palette slots
resolve explicitly from authoring `.tga` names to unique same-name `.dds` entries
in Map.pak, consistent with the existing terrain renderer's mapping. The reviewed
selection is `unity/baselines/map10-terrain-palette.json`. All source hashes match.
Each source has a partial five-level DDS mip chain; Unity's default importer
rejects it. `MxhDdsImporter` keeps the original DXT blocks and mip count without
recompression. `MxhTerrainImporter` generates 64 meshes/materials/Prefab subassets
from `.mxhterrain`, preserving tile rotation, global normals and existing winding.
These generated meshes are not duplicated in version control. No collision or
high-detail remaster quality acceptance is inferred from source-material display.

Verification: Unity 13 passed / 1 real-server test ignored without its fixture;
full C++ 12,454 registered / zero failures / 6 skips, 92.10 seconds; source-byte
check 5,372 passed. Independent Player run `14edba8c09164acb8a110e865df70644`
reached real three-server GameIn and visibly rendered the textured map. Evidence:
`modern/out/unity-remaster/terrain-importer-tests.xml`, `terrain-ctest.log`, and
`three-server/14edba8c09164acb8a110e865df70644/player.png`.

The new audit reads all seven PAK containers and loose files without extraction
into the source tree. It records 19,682 logical assets, 19,741 physical sources,
14,376 PAK entries and 5,365 loose files. All PAK entry counts match their headers.
53 assets have differing source bytes; 6 have identical duplicates. All 59 need
an explicit source selection. There is no loose-file-first policy.

All 81 loose HFLs match the generator's 16x16 grid, map-derived extents and exact
generated height sequence. 55 packed HFLs remain unverified provenance. This
supersedes the historical claim that 10/21/101 loose HFLs are original. A further
18 map descriptors have no matching TTB in the audit's logical inventory.
These findings are provenance/coverage evidence, not proof of render semantics.

Inventory digest:
`c3f54ffad61641e7de0475178914ab5fabd76975487af6fb8d14851b79544e3a`.
The full reproducible baseline is `unity/baselines/playdh-current.json.gz`.
The manifest checksum protects metadata against accidental changes; it is not a
digital signature. Production signing remains a separate release gate.

### Current remaster-v1 audit (2026-09-12)

The reproducible audit was rerun from the current branch HEAD into
`modern/out/unity-remaster/unity-remaster-v1-manifest.json`. It found 19,682
logical assets and 19,741 physical sources, with 53 conflicting assets, 6
identical duplicates, and 59 unresolved source selections. Release validation
reported 108 blockers, so the manifest is development evidence only; no
unselected or generated-placeholder source was copied into the Unity project.

Commands (repository root):

```powershell
python modern/tools/unity_resource_audit.py audit modern/data/PlayDH --output modern/out/unity-remaster/baseline/resource-manifest.json --profile-id unity-remaster-v1 --source-commit 89a11b3653549bab7471f53f8f0aafb72e98ce2a
python modern/tools/unity_resource_audit.py verify-source unity/baselines/playdh-current.json.gz modern/data/PlayDH
python modern/tools/unity_resource_audit.py validate unity/baselines/playdh-current.json.gz --mode release
```

The source-byte verification checks 5,372 files (all loose files and complete PAK
containers). The initial release provenance check intentionally fails with 107
blockers. Passing that check alone would not establish full dependency closure,
visual acceptance or game release readiness.

## Latest runtime evidence (2026-09-12)

The current branch passed the five-stage `MoxianClientE2E` against the local
MSSQL ODBC backend with HSEL enabled: LoginAck, character list, character
creation/relist, Map10 GameInAck, and initial entity delivery all succeeded.
The Map10 session received 228 monster additions and ended with a valid player
snapshot (`map=10`, `life=100/100`) before clean GameOut/disconnect. This is
protocol and persistence evidence; it does not replace visible human combat or
resource-quality acceptance.

## Verification and environment

- Bounded timed-movement activation (2026-09-11): native ABI bumped to
  `0x00010003` with new `mxh_unity_submit_extended_command` entry and a 140-byte
  `mxh_unity_extended_command` carrier. `MoxianMapServer
  --experimental-timed-movement` flips the gate; without the flag, the legacy
  4-byte dispatch is preserved verbatim. 14 new server-side tests + 5 new Unity
  EditMode tests added; no existing test removed. See
  `docs/UNITY_TIMED_MOVEMENT.md` ("Step 1 activated") and
  `docs/UNITY_MOVEMENT_STATUS.md` ("Bounded timed-movement activation").
  OFF reference run `759f279828f049ceb0d85ec5b96bb9ab` remains authoritative
  for legacy 4-byte; real Player run under
  `MXH_TIMED_MOVEMENT=1 --experimental-timed-movement` is the next gate.
- Final x86 build passed; full CTest registered 12,453 tests, zero failures,
  6 explicit skips, 74.94 seconds. The skips are Map10 release/human smoke,
  deployed resource digest, and four MSSQL tests. Evidence:
  `modern/out/unity-remaster/final-ctest.log`. They are not acceptance passes.
- Before code extraction: `cmake --build modern/build --config Debug` passed.
- Baseline CTest: 12,446 registered tests, zero failures, 6 explicit environment
  skips, 139.17 seconds. Log: `modern/out/unity-baseline-ctest.log`.
- Unity project created from installed `com.unity.template.urp-blank`; package
  versions are recorded by Unity in `Packages/packages-lock.json`.
- Unity CLI 1.0.0-beta.8 and local Personal license verified active. Sandbox
  access to Hub state/IPC required escalation; no account or license was changed.
- Pipeline 0.6.0-exp.1 is an Editor automation dependency, not a gameplay service.
- Development machine: i9-14900K, Intel Arc B580, driver 32.0.101.8992. WMI VRAM
  is a 32-bit field and is not accepted as the card's memory capacity.
- The 16 GB RAM / 4 GB VRAM minimum-target machine is not verified by tests on
  this development machine. Performance, full-map and soak gates remain open.

### Initial implementation evidence

- Native x64 core extracted with fixed-size C ABI and shared ClientWire helpers;
  legacy x86 client builds after extraction. Initial targeted legacy regression:
  93 tests, no failures, one pre-existing Map10 release smoke skip.
- Unity EditMode final run `694e8e79d40d4a95aa60f29cca68757c`: 10 tests passed,
  including native DLL create/snapshot/destroy 100 times and 100 real three-server
  login/list/select/GameIn/disconnect cycles. All servers stayed alive. This is
  connection lifecycle evidence, not a measured memory-growth or persistence test.
- Native exporter contract passed: deterministic bytes, source/sidecar SHA-256,
  no overwriting existing output, truncated/non-finite input rejection.
- Map10 inspection derives from explicitly selected Map.pak entry 265, source
  SHA-256 `a774fc78a439afe0b435b88a562ede471af013d731b7f10c1c1f2a402e2aa454`.
  Existing C++ HFL parser produced a 513×513 height grid, 256×256 tiles and 37
  palette names. Unity mesh has 263,169 vertices. Source provenance, alpha,
  collision, materials and visual quality are not accepted.
- First standalone Player run `b043197cbbd34161a27309e32d78ca21` reached frame
  120, displayed the real terrain and connection panel, passed 100 native
  lifecycle cycles and exited 0. It did not connect to servers or prove gameplay.
  Evidence: `modern/out/unity-remaster/player-smoke/report.json` and `player.png`.
- Actual Player reported Arc B580 VRAM 12,118 MB; this is development hardware,
  not the proposed 4 GB target. Initial Player exposed missing AMD/NVIDIA module
  native libraries; built-in module dependencies were corrected and rerun.
- Built-in `com.unity.modules.amd/nvidia@1.0.0` resolved through Package Manager;
  the subsequent Player no longer reports the missing native module errors.
  Unused URP depth-of-field/Panini shader stripping warnings remain to inspect.
- Real server Player run `bdd9a56c868e48e0a51381bab7d23a62` passed Login →
  character list → select → GameIn, player 111 / Map10. Three existing modern
  executables used a newly migrated isolated SQLite fixture. Client-facing
  links used HSEL; the existing Agent→Map legacy link was loopback-only plaintext.
  No database credentials or passwords are embedded in the Unity project.
  Evidence: `modern/out/unity-remaster/three-server/<run-id>/`.
  This does not establish human playability, new-character creation, combat,
  persistence equivalence or MSSQL acceptance.
- Final Player run `56423dc796c6440aaf4087c5589f1298` passed the same real
  three-server flow using the final 132/992-byte ABI and native library SHA-256
  `97E8D2BBB46669D7381633BACB7FF2B295282D9C5A2F6CB687C088E2B574DA03`.
  The actual rendered frame and report are `player.png` and `report.json` in
  that run directory. This is still the geometry inspection scene.
- Initial headless Pipeline captures were blank despite valid geometry. The
  standalone Player capture is the first accepted visibility evidence; it remains
  an inspection scene, not the high-detail art sample.

### Review and release boundaries

The independent review identified terminal event loss on queue overflow, stale
event capacity across reconnect, permissive character-list parsing, GameIn
identity/map association, destroy races, cleanup and Agent timeout propagation.
These core fixes now pass 23 standalone x64 tests and the final Editor tests.
The GameIn decoder preserves 32-bit life and 64-bit experience; corrected legacy
experience/money offsets are +22/+36 within HERO_TOTALINFO. C ABI game/snapshot
sizes are 132/992 bytes, with matching managed assertions and high-value fixtures.

Real-server validation exposed two inherited defects: MapServer populated the
account field with character ID, and TcpServer could use a handler-owned cipher
after disconnect released it. Map now propagates the authenticated Agent account
field. TcpServer synchronizes cipher use/invalidation before disconnect callbacks;
callbacks run outside the cipher lock. Tests cover callback reentry and a blocked
encryption call concurrent with cipher destruction. The failed pre-fix Editor
run `600839cf9778444abbcfd02dfb23eae3` recorded Agent exit `0xC0000005`; the fixed
100-cycle run above passed. No original protocol header was changed.
HSEL compatibility does not establish authenticated transport or MITM resistance.
The existing server progression state (`server/player_state.hpp`) still stores
level/total experience in 32-bit fields, and MapHandler's database loader narrows
experience. The new bridge preserves the full 64-bit wire value; this does not
resolve server-wide high-experience persistence/progression parity. That inherited
gap requires a separate end-to-end gameplay change before full-game acceptance.
The inherited security stubs and credential transport policy remain release
blockers. `RemasterBuildGuard` refuses non-development builds while these gates,
resource provenance and full gameplay acceptance are incomplete.

### Character creation validation (2026-09-11)

The native command ABI is now `0x00010001`, with a 128-byte command. Event and
snapshot sizes remain 320/992 bytes. Creation validates UTF-8, legacy encoded
name length without truncation/best-fit conversion, semantic appearance options,
slot capacity and session/map generations. It confirms creation only after the
server returns a consistent refreshed list; rejection preserves the old list.
Original protocol headers and gameplay data were not modified.

The independent native implementation passed 29 x64 tests. The main x86 build
and full CTest completed with zero failures (12,454 registered, six skipped,
89.05 seconds). Skips include MSSQL integration and release/resource checks;
they are not acceptance evidence. Log: `modern/out/unity-remaster/create-ctest.log`.

Real isolated SQLite three-server runs started with no character:
- Editor `414e13942d634476b3e50e076fa549ea`: creation, GameIn and 100 reconnects.
- Player `ce8c15e258af4b3cb02d9ecb876a093f`: actual creation button callback,
  refreshed slot and GameIn; rendered frame inspected in `player.png`.

Both persisted character 100000 / UnityNew / start area 17 and equipment
slots 1/2/3 with item IDs 11000/23000/27000. Evidence is under
`modern/out/unity-remaster/three-server/<run-id>/`. Staged native DLL SHA-256:
`84D1D3343E7BF2BE66E8E84270BD906963C1F5D9B43042F19E35EB0AA7053C9A`.
These are automated development checks, not human acceptance. Appearance
preview, CJK input/font coverage, movement, combat and full Map10 gameplay remain
unaccepted. The rendered terrain is source material inspection, not the modern
high-detail quality sample. MSSQL/PVE and full-game delivery remain open.

### Movement bridge and network recovery (2026-09-11)

API 0x00010002 now bridges Move/Stop, predicted local position, own server
Correction and remote movement events, using shared DX11/native wire helpers.
Real two-session testing uncovered and fixed duplicate Map-to-Agent fan-out,
misrouted owner corrections, shared HSEL directional key schedules and encrypted
send ordering. These fixes preserve public crypto signatures and protocol headers.
Client/core/server binaries must be updated or rolled back together.

The x64 core/crypto suite passed 40 tests. Full x86 CTest had 12,456 registered,
zero failed and six skipped. Editor run `e76269ba960c4c1682af3ef2ed18aba4` and
standalone Player run `759f279828f049ceb0d85ec5b96bb9ab` passed actual three-server
move/stop/correction with two native sessions. Editor also completed 100 reconnects.
This does not prove two-human gameplay, full movement rules or persistence.

Read-only source review also located real Map10 fixed-attribute collision data
at `Resource/Map/10.ttb` (1024x1024 WORD attributes, 50-unit cells); six source
copies match. It remains to integrate strict loading, original time/state rules
and segment collision. See `docs/UNITY_MOVEMENT_STATUS.md` for exact contracts,
source evidence, known gaps and run paths. Full Map10/game acceptance remains open.

## Delivery rules

Latest collision increment: MapServer now requires strict original fixed-tile
data and validates movement against its segment scan (plus OneTarget endpoint).
Malformed, unknown-owner and client authority-only movement packets are rejected.
Source Map10 SHA-256 is unchanged. Player SQLite/HSEL movement regression run
`1b0dafffeb7647678a19c69c675348b1` passed. See `UNITY_MOVEMENT_STATUS.md` for
tests and remaining timed movement, state, persistence and wall-probe gaps.
This supersedes the earlier statement that all segment collision is pending.

Further verification: 124,852 independent reference-path comparisons pass.
Audited Map10 blocked-cell movement now passes in real Editor run
`f5d80dbe7bbb4b61bb651797623e6f7e` and Player run
`d7193e33e2ce402daee4fda301e714fc`. The target is below the jump threshold,
the owner is corrected, and the observer receives no rejected movement.
Versioned Player reports prevent older movement-only binaries from passing.

Keep the original x86 build and DX11 executable for differential testing. Native
Unity builds use their own x64 output directory. Do not automatically fix duplicate
UI headers by copying one side: newer declarations exist on both sides.
Record each gate with source revision, resource digest, exact command and run ID.
Default deployment order remains local acceptance, then PVE, with SQLite and MSSQL
both verified. No release tag, production rollout or completion claim is implied
by this status document.

### 2026-09-13 — authoritative merchant sale and SQLite persistence

C ABI v1.10 adds `SellSyn` with an exact eight-byte little-endian payload
(`position`, `item`, `quantity`, `dealer`) and strict Ack/Nack correlation. The
native core derives the item ID and available stack from its server snapshot;
MapServer remains authoritative for dealer range, catalog sell price, inventory,
money overflow and persistence. Unity's trade panel now exposes a fourth sale
mode and requires a live shop/NPC context before submitting a carried slot.

The x64 native suite passed 94/94 and Unity EditMode passed all runnable tests
after staging DLL SHA-256 `A2F938EE28D410C090986C14A946082D9C977F0FD59A478BDFBAE383FFAD89A5`.
Real isolated SQLite/HSEL run `3ad439c42472495c9fff211282714c4c`
sold item 53343 to canonical Map12 NPC92, changed money 1000→1250, removed the
item, then reconnected and independently verified the persisted database state.
The structured result reports `sell.persistencePassed=true`. MSSQL and human
visual/input acceptance remain open.

### 2026-09-13 — confirmed item discard and SQLite persistence

C ABI v1.11 adds an exact two-byte `DiscardSyn` and strictly correlates the
echoed position in `DiscardAck`/`DiscardNack`. Only nonempty carried slots are
accepted locally; MapServer owns deletion and persistence, and now publishes a
complete authoritative inventory refresh after a successful discard. Unity's
fifth trade-panel mode requires selecting the same slot twice before sending
this destructive command.

The native x64 suite passed 94/94 and Unity EditMode passed with zero failures.
Staged native DLL SHA-256 is
`617F611FABF233428F26DFEC35FEEF05CC3BE9098F0F03499516DDBD2C787F73`.
Real isolated SQLite/HSEL run `5d4ef9d9655c494db93677b984e908c7`
discarded Map10 item 53343, received the authoritative empty-slot refresh,
reconnected and independently verified an empty persisted item table;
`discard.persistencePassed=true`. MSSQL and human UI acceptance remain open.

### 2026-09-13 — independent legacy Map.pak comparison

A whole-drive read-only search found one additional distinct Map.pak candidate:
`D:/MX/client-legacy/Map.pak` (584,312,358 bytes, timestamp 2008-10-02,
SHA-256 `b3b4c8e504877afaa1636f9a9e7ec88f187246a525b0004ea0f5a9268a0effb0`).
A second path under the historical smoke-test tree is byte-identical to this
same container. Strict traversal parsed all 4,192 entries to EOF with no bad
names or truncation and extracted all 59 HFL/TTB entries into the dated scratch
area for comparison.

Every one of the 59 entries has a same-name, byte-identical source in the frozen
baseline: 59/59 name matches and 59/59 SHA-256 matches. It therefore supplies no
new HFL or TTB and cannot honestly reduce the 45 release blockers. A trial
three-root audit was deliberately not promoted because un-attested duplicate
sources add provenance failures without adding content. Evidence is in
`modern/scratch/2026-09-13-unity-skin-expiry/recovered-client-legacy-map/` and
`legacy-map-pack-comparison.json`. The current composite baseline and its 45
explicit blockers remain authoritative.

## 2026-09-12 Unity compile and EditMode recovery

The Map10 scene-builder collider typing error was fixed in `25c92cb1`. After
terminating the stale pre-fix Editor process, Unity 6000.6.0f1 EditMode tests
completed successfully: 27 total, 25 passed, 0 failed, 2 skipped. Report:
`unity_editmode_results_latest.xml`. The skipped cases remain environment or
runtime-dependent and do not constitute Player combat acceptance.

Combat input EditMode subset subsequently passed 6/6 (target selection, skill
hotkeys and Map10 pointer input), with report `unity_combat_editmode.xml`.
This verifies Unity-side request gating and coordinate/identity handling only;
it does not replace server-backed hit, damage, or two-player acceptance.

The remaining two tests in the full EditMode report are intentionally skipped:
both require the isolated real-server fixture (`unity_three_server_smoke.py
--editor-test --movement`) and one requires the two-account movement setup.
There is currently no built Unity Player executable under `unity/`, so these
gates remain pending rather than being marked passed.

`unity license status` reports an active signed-in Unity Personal ULF license,
but two consecutive StandaloneWindows64 batch builds still exit with
`LicensingClient has failed validation; ignoring`. EditMode remains runnable;
Player output is therefore still withheld pending a successful licensing
validation during the build process.

The reviewed development build method (`Moxiang.Editor.RemasterSetup.BuildDevelopment`)
successfully produced `modern/out/unity-remaster/player/MoxiangClient.exe` with
the native x64 core. Real three-server Player smoke then passed with run ID
`e08a27b6eeba4b758280a4c53d1e1903` (SQLite, client HSEL, internal legacy
plaintext loopback). This is an internal Development build; human acceptance,
combat presentation, two-session movement and release-gate criteria remain open.

## 2026-09-12 Map10 combat input increment

The Unity remaster now exposes a server-authoritative skill command through the
fixed-width native ABI (`MXH_UNITY_COMMAND_SKILL=8`). The command validates
skill and target IDs, finite target coordinates, session generation and map
generation before constructing the existing `Skill StartSyn` payload. Commits
`f502941e`, `4cd1986b`, `0ac91c06`, `a309cda6`, `a4e6b898`, `6ac77fbe` and
`05200461` add the Unity bridge, stable target identity, scene wiring and target
invalidation rules without changing locked protocol headers or server formulas.

The x64 Unity core regression suite remains 69/69 passed. Unity Editor/Player
combat evidence is still pending because the local Unity licensing IPC client
is unavailable; entity snapshot synchronization, visible hit presentation and
two-human combat acceptance remain open.

Development Player movement smoke subsequently passed with run ID
`5017b7094e40469288ad4f3d0a4fad4f`: two native sessions, the real three-server
SQLite/HSEL path, Map10 collision fixture and server correction all passed.
This closes the automated two-session movement gate; human acceptance and
visible combat presentation remain open.

The Development Player was rebuilt through Unity CLI with
`Moxiang.Editor.RemasterSetup.BuildDevelopment`; the build returned success and
produced a success provenance manifest at
`modern/out/unity-remaster/player/MoxiangClient.provenance.json` (source
revision `d045b130`). A fresh three-server Player smoke passed afterward with
run ID `39b2adecb1fa4a2c9051a131d8b75dc2`. The CLI still reports a non-fatal
licensing validation warning; the project build method remains Development-only.

The same isolated fixture also passed through the Unity Editor surface with
run ID `0a0dea4b56fa4b71b42decb427e0e06d` (`--editor-test --movement`), including
two native sessions, Map10 collision and server correction. Editor and Player
movement paths now both have fresh real-server evidence.

The latest provenance-backed Player was rechecked with the movement fixture:
run ID `be7875040e2946bba72d269c7feed48e` passed two-session Map10 movement,
collision and server correction over SQLite/HSEL.

Development Player empty-account creation smoke also passed with run ID
`0a3727158a2b4e788120f6764b9eb9cb`: the real three-server SQLite/HSEL path
created character `UnityNew` (level 17) and returned the expected initial
equipment records. Movement and creation remain separate isolated fixtures.

The existing client entity parser baseline was revalidated independently:
9/9 tests passed for MonsterAdd/NpcAdd decoding and short-payload rejection,
NPC projection/unprojection, and nearest-monster cursor targeting. Native Core
entity-event extraction will use these existing tested layouts rather than a
new inferred wire format.

After entity add/remove event integration, the Unity combat/entity EditMode
subset passed 6/6 again. Report: `unity_entity_editmode_latest.xml`.

Unity now contains `ServerEntityRegistry`, which consumes the bounded
MonsterAdd/NpcAdd events and instantiates only explicitly assigned audited
prefabs, attaching the stable target ID and MapCoordinates projection. Missing
prefabs are ignored safely rather than replaced with fake gameplay entities.
Entity registry scripts compile and the combat EditMode subset remains 6/6.

After the Input System preload rebuild at source revision `db85bd6b`, the
latest Player movement smoke passed with run ID
`1e3b432b5fe148899b82a0086a95d79a`: two native sessions, Map10 collision and
server correction over SQLite/HSEL.

An x86 Debug full CTest rerun completed with exit code 0: 12,548 tests passed
in 75.06 seconds. Eight tests were intentionally skipped for MSSQL, deploy
manifest, or release-only fixtures. Reproducible wrapper:
`modern/scratch/2026-09-12-unity-remaster/run-ctest-summary.ps1`.

The empty-account creation fixture also passed through the Unity Editor surface
with run ID `c9160b99a73c46a7b497773bb8edc3ba`, returning the same character and
initial equipment records. Editor and Player now both have real-server creation
evidence.

After adding the skill ABI rejection test, the x64 Unity core suite passes
70/70. The new coverage asserts that skill commands are rejected outside
InGame and when the bounded payload length is malformed.

Full Unity EditMode was rerun after the ABI test addition: 27 total, 25 passed,
0 failed and 2 intentionally skipped for the isolated real-server fixture.
Report: `unity_editmode_full_latest.xml`.

The current branch also revalidated the MSSQL ODBC five-stage E2E after the
Unity combat ABI changes: Login, character list, relist and Map10 GameIn all
passed, with 228 initial monsters observed. The run reused an existing test
character because the account already contained one; no database schema or
server formula changes were made.

On 2026-09-12, the reproducible full modern Debug CTest wrapper completed with
12,548/12,548 tests passed and exit code 0 in 75.06 seconds. Eight tests remain
intentionally skipped for release-only, deploy-manifest, or MSSQL fixtures;
this does not close the independent MSSQL or human gameplay gates. The latest
Unity social/entity coverage includes chat, quest, ground-drop, pickup and
entity-health paths; those targeted EditMode and x64 core suites also passed.

A fresh Development Player three-server movement smoke on 2026-09-12 passed
with run ID `48e4b38117d34ab9ac16859dd0faab32`. It exercised the real modern
SQLite/HSEL path, two native sessions, Map10 collision data and server
correction. This is regression evidence only; human acceptance remains open.

The 2026-09-12 resource gate was rerun against canonical `modern/data/PlayDH`.
The parser coverage audit passed 600/600 files (598 parsed without sidecar
material and 2 present source files), while the profile manifest still reports
19,682 logical assets, 53 conflicts, 59 unresolved selections and 108 release
blockers. Release remains closed until each selection is resolved and the
visual sample is accepted.

The current Player was rechecked against the real three-server SQLite/HSEL
fixture on 2026-09-12. Run ID `efb0cf183bc64469851ca980058545ce` passed login,
two native sessions, Map10 collision and server correction. The fixture does
not provide human acceptance evidence.
The manifest was also regenerated with the actual branch commit hash instead
of the literal `HEAD`; this removed the synthetic source-commit blocker. The
current release blocker count is 107, all from unresolved provenance,
source-selection or missing map-data findings.

MapChange baseline verification was rerun on 2026-09-12: the modern route
catalog, loading coordinator, cancellation and late-failure handling, entity
cleanup, NPC route semantics and map-change UI callbacks passed **40/40**.
This proves the preserved core baseline only; Unity NPC interaction and the
Unity-side route resolver remain intentionally unexposed until their session
level resource lifetime is implemented.

### 2026-09-12 — skill input compile recovery and coordinate correction

The live Editor at the canonical Unity project was retaining old assemblies
because `Moxiang.EditorTests` lacked a direct `Unity.TextMeshPro` reference.
Its actual `Logs/Editor.log` reported CS0246 in the new chat-focus test.
The test assembly now references TextMeshPro and uGUI. Pipeline `recompile`
completed with `failed=false` and no errors, and live reflection confirmed
`SkillHotkeyController.TextInputOwnsKeyboard` exists in the loaded assembly.
Earlier 2-test skill runs did not validate the new code and are superseded.

Skill hotkeys now respect active text input, reject a missing current target,
and clear stale target request data. Target selection converts scene positions
back to original game coordinates using the owning registry's map dimensions;
inactive targets and mismatched map generations are rejected.

Fresh live Editor validation via `unity command run_tests --mode editor
--project-path C:/moxiang/unity/MoxiangClient --json`: 35 total, 33 passed,
0 failed, 2 skipped. The targeted skill suite passed 3/3 including the new
chat-focus test; target selection passed 2/2 including game-coordinate and
inactive-target assertions. These are EditMode checks, not physical keyboard,
Player, or real-server skill acceptance. No Player rebuild is claimed here.
NPC interaction and map-change session implementation remain development work;
unresolved resource provenance blocks affected release assets, not that work.

### 2026-09-12 — native NPC session directory

`NativeClientCore` now retains NPC role and game coordinates from parsed
NpcAdd messages in a bounded directory (4096 unique objects). Duplicate adds
replace the existing record, ObjectRemove erases it, and connect, disconnect,
shutdown and successful GameIn clear the directory. Capacity exhaustion enters
the existing protocol-failure path instead of silently dropping an NPC record.
This is the state prerequisite for NPC interaction; no new interaction command,
shop-response UI or map-change completion is claimed by this change.

`scripts/build-unity-core.cmd` rebuilt the x64 RelWithDebInfo core and passed
77/77 CTests in 5.81 seconds. Two new directory tests cover replacement at
capacity, overflow without corruption, zero identity, removal and reset with
identity reuse. They do not yet prove NPC lifecycle over the real server.
The first direct build lacked the MSVC include environment; the existing
wrapper initialized x64 vcvars and completed successfully. The rebuilt DLL
has not been staged into the running Editor or rebuilt Player in this step.

### 2026-09-12 — NPC speech command and socket regression

ABI 0x00010004 adds NPC_INTERACT (12), with matching NativeClient and
ConnectionPanel entry points. The core validates session/map generations,
InGame state, empty payload, known object and the existing 500-game-unit NPC
range before sending the existing Npc/SpeechSyn packet. MapChange roles return
NOT_READY pending route-catalog integration. OK means sent, not server approval.

The x64 RelWithDebInfo build and all 79 CTests passed (5.22 seconds). A new
HSEL socket fixture verifies the packet category, protocol, player ID and exact
NPC payload, plus stale-map, malformed, unknown, distant, warp and removed NPC
rejection. Its initial failure was the test state-wait helper consuming NpcAdd
events; the helper now optionally preserves events for this fixture. This is
a protocol fixture, not the production three-server NPC acceptance.

The matching DLL was staged successfully with SHA-256
FFE8A9F7A9CB9F9734BDA2F369C84439C7F05CDD9D83058E6F20E6F627AD172E.
The previous live Pipeline instance is no longer discoverable; two Unity
processes exist but their project/initialization status has not been established.
C# recompile could not run through Pipeline, so fresh managed compilation and
Editor DLL loading remain unverified. No Player rebuild, NPC response UI or
map-change completion is claimed.

### 2026-09-12 — NPC response events and Editor recovery

Npc/SpeechAck and SpeechNack are now decoded into NPC_RESPONSE (21), carrying
the NPC ID and OK/REJECTED result. The parser requires the local player object
ID and exactly four payload bytes with nonzero NPC identity. No request ID is
invented for this legacy reply. The HSEL NPC socket fixture now also verifies
the successful response reaches the event queue before ObjectRemove.
All 79 native CTests passed in 5.68 seconds; Nack and malformed-response
network fixtures remain to be added.

The previous Editor log confirmed shutdown. Process inspection showed the
remaining Editor belonged to another project, so the canonical Moxiang project
was reopened through Unity CLI. Pipeline reports ready, PID 47236, version
6000.6.0f1. The newly staged DLL SHA-256 is
1A8BD12F8034BF4BB0C150F88BCEFA01A79B05E081440C2BCCA9E81BDD884500.
Live eval successfully constructed/disposed NativeClient (including its native
ABI check), reporting API 65540 / 0x00010004 and event 21. Full EditMode:
35 total, 33 passed, 0 failed, 2 skipped (1.01 seconds). This supersedes the
earlier managed-compilation/Editor-load uncertainty. Player, NPC presentation,
shop inventory and map-change acceptance remain incomplete.

### 2026-09-12 — NPC reply fault injection

The NPC HSEL socket fixture is now parameterized for Ack, Nack, truncated
payload, wrong player identity and zero NPC identity. Ack/Nack retain their
distinct OK/REJECTED result and map generation with no fabricated request ID.
Each malformed response must enter FAILED/PROTOCOL_ERROR and must not emit an
NPC_RESPONSE event. The existing invalid-target and exact outgoing packet
checks run in every case. All five variants passed; the full x64 suite passed
83/83 in 6.87 seconds via `scripts/build-unity-core.cmd`. These tests close the
previously noted reply-parser coverage gap; they do not establish NPC UI or
production-server gameplay acceptance.

### 2026-09-12 — NPC click routing

ServerEntityRegistry now marks identities produced by NpcAdd as NPCs. Valid
NPC selection submits the existing interaction command through ConnectionPanel;
the development status area distinguishes awaiting response, accepted and
rejected. Map10 terrain input now checks for a selectable entity in front of
the ground hit and suppresses the competing movement command for that click.
Right-click stop behavior is retained.

Live Editor compilation completed without errors. Full EditMode: 36 total,
34 passed, 0 failed, 2 skipped (4.18 seconds). The added physics test covers
a child collider belonging to an NPC identity, a ground hit before the NPC,
and a zero-ID object. Audited NPC prefab assignment, physical input, readable
runtime feedback and shop/dialog rendering still require actual scene acceptance;
this code/test result does not claim those gates.

### 2026-09-12 — shop catalog wire bridge

The core validates modern ShopList exact length (6 + count * 6) and local
player identity, then queues SHOP_CATALOG (22) chunks of at most 40 records.
Each event carries NPC ID, total count and entry offset with literal LE
item-ID/uint32-price bytes. Queue capacity is checked for the entire batch
before enqueue; failure cannot expose a partial batch. Empty catalogs produce
one empty chunk, including the server's unresolved empty NPC-0 response.

Parser tests reject truncated/trailing records and oversized declared counts.
The HSEL NPC fixture now receives 81 offers across three chunks and checks
every item ID and unsigned price, total and offset. x64 build and 84/84 CTests
passed in 6.68 seconds. Managed catalog assembly, presentation, buy/sell and
live server acceptance remain incomplete. This newer DLL has not been staged
into the already running Editor; its earlier staged hash is not evidence for
the shop bridge.

### 2026-09-12 — managed shop catalog assembly

ConnectionPanel now owns ShopCatalog and feeds it current snapshot generations
and filtered native events. The assembler checks exact chunk sizes, ordering,
NPC identity, total and generation; only a complete read-only offer list becomes
Ready. New interaction, session/map change and release clear old offers. Empty
catalogs replace prior offers. Item IDs and uint32 prices remain server values.

Editor recompile completed without errors. Full EditMode passed 38 of 40,
0 failed and 2 skipped in 4.28 seconds. Four new tests cover 81-item completion,
unsigned prices, missing/out-of-order/truncated chunks, map invalidation and
empty replacement. This is managed assembly validation; staged native shop
events, visible offers and buying/selling are not yet accepted end to end.

### 2026-09-12 — server money updates

Purchase-path inspection confirmed that MapServer sends Item/Money before
BuyAck and TotalInfoLocal. The core previously ignored Money after GameIn.
It now validates local player identity and the exact four-byte payload, decodes
the unsigned value into the existing game snapshot and advances its revision.
There is no predicted subtraction. The HSEL catalog fixture verifies that
0xfffffffe survives into snapshot.game.money. The x64 build and all 84 CTests
passed in 6.50 seconds. This is a purchase prerequisite, not a completed buy
command or inventory update; the latest DLL is still unstaged in Editor.

### 2026-09-12 — buy command and acknowledgements

ABI 0x00010005 adds BUY (13) and BUY_RESPONSE (23). NativeClient and
ConnectionPanel.BuyOffer submit nonzero uint16 item/quantity using the existing
four-byte BuySyn. The server continues resolving catalog, price and inventory
legality. One pending purchase is allowed; echoed item/quantity must match
before Ack/Nack emits the local request ID. Unsolicited replies are ignored,
malformed/mismatched replies fail, and timeout reports unknown outcome and
requires reconnect rather than automatically repeating a purchase. No local
money/inventory prediction is introduced. Managed successful purchase closes
the catalog, matching the current modern client behavior.

HSEL Ack/Nack fixtures verify exact BuySyn bytes, zero quantity rejection and
duplicate-submit suppression; all 84 native CTests passed (6.50 seconds).
Timeout and mismatched-buy-response fixtures remain outstanding. After saving
the project, the Editor was restarted to release its native DLL. An initial
copy attempt while it was still exiting failed; the subsequent successful
stage hash is F597F18069C5781A3A49FE51705DC6572EF7BEDF6C11824BE58FA2BEFD852C82.
Pipeline ready PID 26248; live NativeClient construction/disposal reported
65541 / 0x00010005, proving matching managed/native load. Full EditMode:
40 total, 38 passed, 0 failed, 2 skipped (2.36 seconds).
Visible shop controls, inventory refresh, real-server purchasing and Player
acceptance remain incomplete.

### 2026-09-12 — purchase uncertainty tests

Added HSEL socket cases for a mismatched echoed purchase quantity and a silent
server after BuySyn. Mismatch enters PROTOCOL_ERROR; silence reaches the configured
one-second acknowledgement timeout and enters NETWORK_ERROR. Both cases assert
no BUY_RESPONSE, exactly one outgoing purchase, no further submission while
failed and unchanged authoritative money. The successful/rejected fixture now
also fetches a fresh post-reply snapshot rather than checking a stale copy.
All 86 x64 CTests passed in 8.13 seconds. These results close the previously
listed timeout/mismatched-buy-response fixture gap; they do not prove server
persistence or visible inventory updates.

### 2026-09-12 — authoritative inventory state bridge

GameIn and Item/TotalInfoLocal now share INVENTORY (24): 124 legacy ItemBase
records, 22 bytes each, sent in 12 bounded chunks. Incoming refreshes require
the local player ID and exactly 2728 bytes. Batch capacity is checked before
enqueue. Core game.items is updated only from server data.
ConnectionPanel feeds InventoryState, which publishes all seven ItemBase
fields only after ordered completion and clears on session/map/player changes.
Slot order includes inventory, worn, shop, pet and titan regions; nothing is
reduced to a client-generated item/count pair.

The HSEL buy fixture verifies every refreshed byte across all 124 slots.
Native build/CTest: 86/86 passed (8.50 seconds). Live Editor compilation passed;
EditMode: 42 total, 40 passed, 0 failed, 2 skipped (4.87 seconds), including
all-field decoding and interrupted/map-changed inventory assembly. Latest
native inventory DLL remains unstaged; these are separate native/managed
checks, not an integrated Editor or visible inventory acceptance.

### 2026-09-12 — development trade controls

Added TradePanel and additive ConnectionSceneBuilder.AddTradeControls, then
applied it to the existing ConnectionValidation scene through live Editor API.
Eight paginated offer buttons call BuyOffer with a validated uint16 quantity;
inventory mode reads the 124-slot state. Offer-reference checks prevent a
button rendered for an older catalog from submitting against its replacement.
Names/icons remain explicit development IDs, not finished item presentation.

Actual ScreenCapture revealed overlap with the existing creation panel. The
trade CanvasGroup now hides and stops intercepting clicks outside InGame;
its background is opaque when shown. The existing scene was updated and saved.
Fixed-idle screenshot `modern/out/unity-remaster/trade-panel-idle-fixed.png`
was visually inspected and confirmed creation UI is unobscured. The earlier
camera-only screenshot omitted overlay UI and is not layout acceptance.
Full EditMode: 42 total, 40 passed, 0 failed, 2 skipped (4.40 seconds).
InGame trade rendering, real item names/icons and actual buy-button operation
remain unverified; no full gameplay or visual-quality acceptance is claimed.

### 2026-09-12 — integrated real-server initial inventory

RealThreeServerGameInAndReconnect now waits for InventoryState.Ready and checks
124 slots on every connection. With --create-character it verifies initial
weapon/clothing/boots at slots 81/82/83 against 11000/23000/27000 on all 100
reconnections. Latest inventory-capable DLL was staged after confirmed Editor
exit: SHA-256 216C5A041FF166D161A4C51368139D5BA4FC523D7896CCE2CE3EF1E31CB52ADE.

Command: `python modern/tools/unity_three_server_smoke.py --player
modern/out/unity-remaster/player/MoxiangClient.exe --editor-test --create-character`.
Run `0ac145b91a86431a9c26a1dd57111738` passed over real isolated Login/Agent/Map,
SQLite and HSEL. The Editor test took 40.45 seconds; the filtered Moxiang.Tests
run had 33 total, 32 passed, 0 failed, 1 skipped. SQLite inspection independently
confirmed UnityNew and equipment rows (1,11000), (2,23000), (3,27000).
Evidence: `modern/out/unity-remaster/three-server/0ac145b91a86431a9c26a1dd57111738/`.
The --player argument identifies an existing artifact required by the wrapper;
this invocation ran Editor tests, not that Player. This closes the initial
inventory native-to-managed integration gap, not human trade/purchase acceptance.

### 2026-09-12 — canonical merchant location probe

Added `mxh_unity_shop_catalog_probe`, reusing the existing Dealitem parser with
parse-error and CRC validation. It exports numeric NPC identity, map, role,
coordinates and item IDs for a map or `all`, without changing source bytes.
The initial compile required the complete npc_shop.hpp type before instantiating
optional<NpcShopCatalog>; after that fix the x64 build and 86/86 tests passed
(latest 8.29 seconds).

Canonical Dealitem.bin has no Map10 merchant records. The two Map10 NPCs seen
in the real-server run therefore came from the separately loaded quest catalog;
the earlier log observation must not be interpreted as two shops. The complete
probe confirms merchants on other maps, including 17 on Map1 and 15 on Map12.
Use an actual merchant map for the independent trade fixture and implement
the catalog-based route for the player flow; do not fabricate a Map10 merchant
or redefine quest NPCs as dealers to satisfy the slice test.

### 2026-09-12 — real merchant purchase and logout persistence fixes

Added isolated `unity_three_server_smoke.py --editor-test --trade` and
RealMerchantPurchasePersistsAcrossReconnect. Uses canonical Map12 NPC92,
within 500 units of the existing default spawn, and item53343; only the
fresh SQLite fixture receives starting money. No resource or spawn edits.

First real run cdbd17d2c07040129d95022b681ae4e7 exposed two server bugs:
BuySyn chose the first global catalog selling an item even when that NPC
was on another map; repeated GameOutSyn persisted a synthetic zero balance
after the live runtime had already been removed. Buying now resolves an
in-range live NPC among catalogs containing the item; duplicate logout
does not persist absent runtime state. Prices and the 500-unit range stay
unchanged. Separate regressions cover the duplicate catalog, out-of-range
rejection and repeated logout (2/2 passed).

Rebuilt all modern Debug targets using scripts/build-modern.bat Debug.
Real run a4357280919e4e02a3904c8209bd7190 passed: server quote1000,
money100000000 -> 99999000, one item53343, matching managed state after
reconnect and independent SQLite inspection after disconnect. Source
Dealitem SHA256: 5d5e8023d0071da3e1da0161c75cc98ffcd9c0540b782422f3fb426a8ba04a2a.
Evidence is in modern/out/unity-remaster/three-server/<runId>/.
Full modern CTest completed in134.91 seconds: 12550 registered,
12542 passed, 8 skipped, zero failures. The skipped MSSQL tests are not
MSSQL acceptance. Separate fixes committed as1ff37b2a (duplicate logout)
and a599a59b (nearby dealer). This does not prove MSSQL,
Player, human shop UI acceptance, or the Map10-to-merchant route.

### 2026-09-12 — source-map forwarding isolation prerequisite

Read-only Sol sub-agent reviewed Agent routing and the Unity transfer boundary;
the main agent implemented changes. Production MapClientHandler now carries
the configured map number independently of each TcpClient's local connection
ID. Agent takes a consistent character/connection/map recipient snapshot under
both state locks, then invokes reply callbacks outside those locks. World
broadcasts and world replies require the recipient's source map; Party/Guild
directed cross-map routing remains available. NPC SpeechAck/Nack now returns
to the requesting player instead of the old broadcast-excluding-owner path.

Debug build passed; AgentHandlerTest 31/31 passed, including equal socket IDs
on distinct maps, old-map money exclusion after route change, NPC response
ownership and cross-map Party delivery. Real Editor/three-server SQLite trade
run c061467b1a29432b9cd67e1871509215 passed with an additional required NPC
SpeechAck assertion, quote1000 and persisted balance99999000/item53343.

This is a prerequisite, not completed map transfer. Agent still commits its
target route before target GameInAck; target-ack ordering, failure rollback,
pending-transfer handling and Unity catalog/state integration remain to fix.
The earlier full CTest result applies before these forwarding changes; a new
full run is tracked separately.

### 2026-09-12 — reliable source-exit result and fixture isolation

The first forwarding full run had one failure in FindSkillReadsFromLoadedSkillListBin:
parallel tests shared mxh_map_handler_load_test.bin and overwrote each other.
The helper now uses process ID plus an atomic per-process counter. After
isolation, the full12552 run passed with8 skips (134.22 seconds); the relevant
three cases also passed10 repeated runs. The log is
modern/out/unity-remaster/map-source-isolated-fixtures-ctest.log.

Map GameOutAck now echoes the character in the existing header. Unknown or
non-owned runtime requests receive GameOutNack; duplicate requests no longer
produce a new successful flush acknowledgement. Item/money/quest persistence
helpers return their actual success, and GameOut keeps the live runtime on
failure instead of deleting it and acknowledging success. Tests inject begin,
commit, inventory-delete and money-write failures, then verify successful retry.
Quest write failures are propagated but fault-injection coverage remains open.
The writes are not yet one atomic snapshot transaction.

Current full Debug build passed after bounded linking recovered an MSVC PDB
RPC error from the unrestricted parallel build. Agent/Map102 tests passed.
Real Editor/three-server trade run da5a0b3c77284855bd292613db3a9ec8 passed with
the source-exit checks enabled and the same persisted money/item results.
The12552 result predates the persistence-result changes; do not reuse it as
their full-suite result. Pending-transfer coordination and failed target
load/rollback remain incomplete. Read-only Sol review also identified that
target GameIn currently substitutes default state on DB query failures;
that must be corrected before transfer can be accepted.

### 2026-09-12 — strict production GameIn loading

Source-exit full CTest completed:12553 registered,12545 passed,8 skipped,
zero failures in127.93 seconds (gameout-failure-regression-ctest.log).
The source-map forwarding fix is now separately committed; other work remains
in progress in the worktree.

MapServer now explicitly disables character defaults alongside its existing
development fallbacks in normal startup. Strict entry requires a stored
character owned by the authenticated user and successful state, quest/subquest,
inventory and initial-equipment queries. Any failed required query emits
GameInNack before publishing a runtime. Empty inventory/quest results remain
valid. Direct handler development fixtures retain their existing default mode;
production uses strict loading unless --allow-dev-fallbacks is explicitly set.

StrictGameInRejectsMissingOwnedCharacterAndReadFailures covers9 real in-memory
SQLite scenarios (valid empty state, missing character, wrong owner and6 missing
tables). It and the two source-exit regressions passed; full Debug build passed
with bounded linking. Real strict Editor/three-server trade/reconnect run
741474a3b0dc49f2952fd209f631638e passed, retaining money99999000 and item53343.
The preceding full-suite result predates strict loading; latest full regression
is separate. Pending-transfer orchestration, atomic whole-character persistence,
malformed persisted-row validation, and Unity transfer presentation remain open.

### 2026-09-12 — Agent transfer acknowledgement coordination

Strict-entry full regression completed: 12554 registered, 12546 passed,
8 skipped, zero failures in 126.14 seconds (strict-entry-regression-ctest.log).

Agent now publishes a pending transfer before sending source GameOutSyn, waits
for the identified source GameOutAck before target admission, and validates the
target GameInAck character/user/map payload before switching the route. It
emits ChangeMapAck before the accepted GameInAck, including synchronous sender
callbacks. Source rejection resumes the source route; explicit target rejection
requests source admission and waits for its confirmation. Gameplay commands and
world delivery are gated during the pending handshake. Ambiguous send failures,
malformed admission and 15-second deadlines request transport disconnect rather
than pretending the old runtime remains active. Production main drains error
replies and performs the requested disconnect; cleanup targets the possible owner.

A read-only Sol review identified terminal duplicate forwarding, synchronous
disconnect replies, old-session cleanup of a rebound character, and unbounded
quarantine. Root implemented terminal suppression, route detachment before send,
owner-conditional cleanup and one-shot disconnect requests. Tests exercise these
paths, target synchronous admission, source/target rejection and late replies.
No original resource, formula or legacy protocol header was changed.

This is server-side coordination, not accepted end-to-end Unity map transfer.
Unity transfer command/catalog/state presentation, restored position fidelity,
atomic whole-character persistence and real two-map Player acceptance remain
open. Multi-thread callback interleavings and transport reconnection generations
need further coverage before this can be considered production-ready. Current
changes are uncommitted; final build and full regression are recorded separately.

Final bounded Debug build passed. Agent regressions passed 34/34. Real Editor
trade run aff5b4cb576e4757a897299edd6a1902 passed against all three production
executables with HSEL and SQLite; XML records 32 passed, 2 skipped, 0 failed.
Independent DB inspection again found money99999000 and exactly item53343.
This is a trade/reconnect regression, not a map-transfer or human-UI test.

Full post-coordination CTest completed in151.96 seconds: 12557 registered,
12549 passed, 8 skipped, zero failures. Evidence:
modern/out/unity-remaster/transfer-coordination-ctest.log. MSSQL skips remain
unverified; this result does not replace the outstanding two-map runtime tests.

### 2026-09-12 — native map-route resource boundary

Native NPC directory now retains bounded original name bytes for the same
current-map/name lookup used by the DX11 client. Lookup rejects missing objects,
zero/self destinations and conflicting destinations for the same name. A new
explicit UTF-8-path C ABI loader reads MapChange.bin only while idle/failed,
caps input size and catalog entries, and clears a prior catalog on failed load.
The managed wrapper exposes LoadMapRoutes; ABI version is now0x00010006 with
unchanged packed struct sizes. This loads configuration only: NPC MapChange
still returns NOT_READY until transfer submission/state handling is integrated.

The existing parser reads180 entries from canonical
modern/data/PlayDH/Resource/MapChange.bin, SHA256
66ACA3CCCA86469E4F8EC1F0DADA451FD2422D5C4B0C2698408E56C0101B7048.
The new read-only mxh_unity_map_route_probe reports Map10 exits to2,70,82,79;
there is no direct Map10-to12 route in this catalog. Do not create a convenient
Map10-to-merchant shortcut. Resource bytes were not changed.

x64 build and88 native tests passed, including C ABI loading of the canonical
file, invalid bounds and ambiguous legacy-name matching. Staged DLL SHA256:
BF3FA06D6E5B55118C203F90FDE8F5EC67B09AD929E7BE9247FE49BE7607FE1C.
Real Editor/three-server SQLite run e58c14c62efa433bb464c1ce8ed6512a passed,
including managed catalog loading and the existing purchase/reconnect checks.
Player route packaging, actual transfer commands, failure/restore presentation
and two-map runtime acceptance remain open. No release claim is made.

### 2026-09-12 — native NPC transfer submission and admission states

NPC MapChange interaction now resolves the loaded original name/map catalog
and sends ChangeMapSyn. Native state12 (AwaitMapChange) gates commands until
the Agent response. Source rejection reports a nonterminal map-change result;
target/source admission ACK selects the map before normal GameIn validation.
Accepted re-entry increments map generation and resets timed-movement epochs.
Wrong destinations fail the session. Source messages queued before the request
retain the old generation while waiting for the route decision. Pending buys
prevent starting a transfer. The C ABI is now0x00010007, with event25 carrying
the requested/actual map and success/rejection; packed sizes are unchanged.

Agent recovery now emits ChangeMapAck(source map) followed by source GameInAck.
It no longer sends ChangeMapNack followed by an unexpected GameInAck. Source
exit rejection still emits ChangeMapNack only. The native client distinguishes
these outcomes using its requested destination. The existing Agent regression
was updated to assert this recovery ordering.

ConnectionPanel loads the explicitly packaged catalog before connecting and
shows map-change results. scripts/stage-unity-map-routes.ps1 verifies the frozen
canonical SHA256 before/after byte-copying it into StreamingAssets/Gameplay;
the packaged file has a provenance sidecar. Neither the catalog nor source
resources were edited. Original map10 destinations remain2,70,82,79.

x64 core89 tests passed after the final changes. The new TCP/HSEL fixture
exercises success, source rejection, source recovery and wrong destination,
checking request identity, admission map, stale-command rejection and generation.
These are protocol fixtures, not two real MapServers. Real dual-map persistence,
arrival/restore coordinates, destination visuals and independent Player remain
required. Current Debug build passed; full regression and latest Editor run are
tracked separately. This work does not establish complete map-transfer acceptance.

Final evidence: native-transfer-regression-ctest.log has12557 registered,
12549 passed,8 skipped,zero failed in151.45 seconds. The final x64 build has
89/89 passing tests. Staged DLL SHA256 is
B9AA1DF19C8738AD2E5DBF67C579225F98EBB9D88A66A7D49830FA41442626DA.
Editor run2708d22c1fbf44a492a0f4c6234b5fa5 passed with33 tests passed,2 skipped,
0 failed, including packaged catalog loading and real three-server trade/reconnect
persistence (money99999000,item53343). This is still not independent Player,
MSSQL or real dual-MapServer transfer evidence.

### 2026-09-12 — real Map10 to Map2 transfer and SQLite reconnect

Real Editor run `a9f1b294345247d29e15e75386cbfe9a` passed the canonical
Map10 to Map2 transfer using Login, Agent and two independent MapServer
executables. `RealMap10ToMap2TransferAndReconnect` checks destination admission,
then disconnects and reconnects. Final SQLite queries confirm position
`(map=2,x=7211,z=43329)`, character selection map `2`, and unchanged money
`4242`. Editor XML reports 33 passed, 3 skipped, zero failed. Evidence lives in
`modern/out/unity-remaster/three-server/a9f1b294345247d29e15e75386cbfe9a/`.
Client transport is HSEL; internal map links are plaintext loopback. The player
is a seeded fixture and this run is Editor integration, not human acceptance.

The previous dual-map run failed because Map10 published only two non-portal
NPCs. `StaticNpc.bin` supplies the missing original spawn identity 1017 at
`(46973,4198)`. Its localized name differs from `MapChange.bin`; the compatibility
projection associates only unique exact map/coordinate matches, preserves
StaticNpc identity and name bytes, and derives portal behavior from the route.
It does not reinterpret StaticNpc job 31 or NpcList job kind 8 as legacy role
27. Disabled map-zero rows are excluded. The verified runtime now publishes
four Map10 NPCs. No canonical resource bytes were changed.

GameOut materializes movement and persists position before acknowledging exit;
character_info.map_num follows that saved map for character selection. Target
admission uses original route arrival coordinates and rejects missing or
ambiguous routes in strict mode. Same-map admission restores saved coordinates.
The latest x64 native suite passed 90/90 tests. The staged core SHA256 is
`2568A83D499F89BE19743407A31E3FF4CD6B8782E1B606D2253A74D178AF7939`.

Remaining transfer gates include target-failure recovery on real servers,
atomic whole-character persistence and durable target admission on crash,
MSSQL, independent Player, destination visual loading and two-player operation.
This result does not close stages C/D or full-game acceptance.

After these changes the bounded x86 Debug build completed successfully. Full
CTest registered 12559 tests: 12551 passed, 8 skipped, zero failed in 151.78
seconds, recorded in `modern/out/unity-remaster/static-portal-regression-ctest.log`.
The skips include six MSSQL fixtures plus release-human and deployment-resource
checks, so they do not establish those acceptance gates. Source/docs diff checks
pass; the Editor-generated ConnectionValidation scene still contains trailing
whitespace in serialized empty fields reported by the whole-worktree diff check.

### 2026-09-12 — transactional exit writes on the current MapServer path

GameOut now owns one transaction covering inventory/equipment rows, money,
existing quest writes, position and character selection map. Inventory writing
is separated from its standalone transaction wrapper, avoiding a nested commit.
The runtime is removed and GameOutAck sent only after commit succeeds. MapHandler
serializes message, disconnect, AI tick and shutdown callbacks before state
locks and database work so other production callbacks on this handler cannot
interleave SQL into the same single connection's exit transaction.

`LateExitFailureRollsBackItemsMoneyAndPositionBeforeRetry` uses a real SQLite
trigger to abort the final character map UPDATE. It verifies rollback of the
earlier inventory DELETE, money update and position UPSERT, retained runtime
and GameOutNack; dropping the trigger permits retry and GameOutAck with the new
values. Agent/Map focused tests passed 109/109. The first bounded build hit
LNK1318 RPC; the serial x86 Debug rebuild completed successfully.

Real Editor dual-map run `b94faf1935c347b09d994b227303eb1b` also passed normal
Map10-to-Map2 transfer and reconnect, with SQLite position `(2,7211,43329)` and
money `4242`. This does not inject a failure into the running servers.

Read-only subagent review identified limits still requiring work: pooled
adapters need an explicit connection lease spanning the transaction (the current
MapServer executable uses a single adapter); MSSQL can report an error after a
successful commit when restoring autocommit; rollback failure and exception
handling need a terminal recovery policy. Existing quest writing is not full
snapshot replacement, and experience/vitals are outside this transaction.
Therefore this is not a claim of complete character durability or full failure
recovery. Independent Player, MSSQL and human gameplay gates remain open.

Full x86 CTest after this change: 12560 registered, 12552 passed, 8 skipped,
zero failures in 148.93 seconds, recorded in
`modern/out/unity-remaster/exit-transaction-regression-ctest.log`. The real Editor
run above reports 33 passed, 3 skipped, zero failed. Source/docs diff checks pass.
The callback lock review found no new production network lock cycle; custom
synchronous callbacks and external callers remain outside that conclusion.
Shutdown still needs to reject queued messages after draining disconnects DB.

### 2026-09-12 — reject queued work after shutdown starts draining

MapHandler now checks draining after acquiring its dispatch lock in on_message
and tick_monster_ai, preventing work already queued on another thread from
running after the shutdown save phase closes the database. Disconnect callbacks
still release HSEL session state and remove player runtime/routing, but skip
movement materialization and inventory persistence while draining.

`DrainingRejectsQueuedMessagesAndTickWithoutDatabaseWork` admits a player,
starts shutdown, then delivers another GameIn, GameOut, AI tick and disconnect.
It verifies no new DB query, write or transaction, no gameplay reply/admission,
rejected new connections, and successful runtime cleanup. The x86 Debug serial
build succeeded. All 110 Agent/Map tests passed; evidence is
`modern/out/unity-remaster/draining-agent-map-tests.xml` and its matching log.
Source/docs diff checks pass. The previous full CTest result predates this guard;
this turn ran the affected Agent/Map suites rather than claiming a fresh full run.

This closes the post-drain DB-access window, not the entire shutdown durability
gate: shutdown's save phase still persists positions only. Full character save,
failure reporting, crash recovery and real server stop/restart acceptance remain
required alongside the already recorded transaction limitations.

### 2026-09-12 — shutdown reuses transactional player saves and reports failure

MapHandler shutdown now freezes callbacks and executes the existing exit
transaction for every connected player before disconnecting its database,
instead of writing positions alone. It caches and returns whether every save
succeeded. MapServer main stops networking and exits with status 1 if any save
failed, rather than unconditionally returning 0. Repeated shutdown calls do not
write again or turn an earlier failure into success.

`ShutdownSavesExitStateAndReportsLateFailureAfterDatabaseReopen` uses a fresh
file-backed SQLite database for success and injected final-UPDATE failure.
After shutdown closes the adapter, the test reopens the file and checks money,
inventory and position. Success keeps the new values; failure keeps the old
values and reports false, including on a repeated shutdown call. All 111
Agent/Map tests passed, recorded in
`modern/out/unity-remaster/shutdown-agent-map-tests.xml` and its matching log.

This reuses the exit write coverage; it does not add experience/vitals or fix
quest snapshot replacement. Failure is now visible, but a durable retry/recovery
mechanism is still required to preserve unsaved runtime changes across process
exit. Real process stop/restart and MSSQL fault behavior remain unverified.

### 2026-09-12 — contain exit exceptions and stop on uncertain commit

Exit persistence catches adapter exceptions and attempts rollback without a
throwing RAII destructor. A write failure or exception may be retried only if
rollback succeeds. Failed/throwing rollback and any failed COMMIT enter draining,
close the adapter, and retain a failed shutdown result. The MapServer main loop
now observes draining and exits through its nonzero failure path. No further
gameplay is accepted against the uncertain runtime.

This conservative COMMIT policy follows the current MSSQL adapter: SQLEndTran
can commit successfully before SQLSetConnectAttr restoring autocommit fails.
A subsequent rollback cannot establish that the original commit was undone.
This source analysis is not live MSSQL fault-injection evidence.

All 112 Agent/Map tests passed, including write exceptions, rollback returned
errors, rollback exceptions, commit uncertainty, successful retry after a known
rollback, and rejected subsequent work after terminal failure. Evidence:
`modern/out/unity-remaster/exit-fault-agent-map-tests.xml` and matching log.
These mock fault cases do not prove real ODBC crash recovery. A durable recovery
record and complete character-state reconciliation are still required; stopping
on uncertainty prevents continued mutation but does not preserve unsaved data.

### 2026-09-12 — disconnect uses the exit transaction

Unexpected map-link disconnect now saves each owned player through the same
exit transaction, including money and position, rather than inventory alone.
Failure drains the handler and returns a failed shutdown status, preventing
new admissions against a silently stale save. Old disconnect callbacks remain
scoped to their connection identity and cannot remove a newer player session.

Real Editor dual-map run `25a12c6fb6664080a406cfad79f930dc` passed normal
Map10-to-Map2 transfer/reconnect with saved position `(2,7211,43329)` and money
`4242`. This is not an injected unexpected map-link loss. Targeted testing
identified an old inventory fixture that hand-created only partial tables;
it is being changed to initialize the real schema before inserting its rows.
Final focused/full regression results must be recorded after that correction.

Failure recovery remains incomplete: terminating on a failed disconnect save
does not retain unsaved runtime across restart. Durable recovery and multi-player
fault injection are still required before claiming zero persistence loss.

Final focused result: all 114 Agent/Map tests passed in
`modern/out/unity-remaster/disconnect-agent-map-tests.xml`. The inventory
fixture now uses the actual schema and supplies its required userid, while
keeping all original inventory/equipment assertions and adding a non-draining
assertion after successful disconnect. Final serial Debug build succeeded.
The earlier full run in `lifecycle-persistence-regression-ctest.log` finished
with only that pre-correction fixture failing and 8 skips; a fresh full run
after the fixture correction has not been performed. Source/docs diff checks
remain clean.

### 2026-09-12 — final lifecycle regression and Map2 heightfield import

Fresh full native CTest passed after the fixture correction: 12565 registered,
12557 passed, 8 skipped, zero failed in 153.38 seconds. Evidence is
`modern/out/unity-remaster/lifecycle-final-regression-ctest.log`; skipped MSSQL,
release and deployment checks remain unaccepted.

Client inspection confirmed that ConnectionValidation still uses fixed Map10
geometry and lacks runtime map visual switching. Server admission into Map2
therefore does not establish destination visual loading. Map2's loose HFL is
a detected generated placeholder; the explicitly selected Map.pak entry 583
has SHA256 `d49dd4b864f09e3880124d31633d1b0860175805a3273284988c2b421d9e31e2`.
The extractor checked the source/container baseline before copying bytes to a
development output. Native HFL export produced 263169 heights in
`unity/MoxiangClient/Assets/Moxiang/Derived/Map2/`, with explicit selection
metadata and releaseReady false. Source historical classification remains
unknown, not verified-original; no original resource bytes were edited.

Real Editor run `8062b72fa2d44ea8b87619e0df419635` passed after adding the Map2
import test. The test checks source identity, full grid/tile counts, imported
mesh vertices and the canonical arrival coordinate within scene X/Z bounds.
Normal real dual-map transfer/reconnect also passed. This does not validate
arrival height, material palette, scene props, visual switching or human play.
Those are the next client-content steps; the full remaster gates stay open.

### 2026-09-12 — Map2 palette recovery with one unresolved tile

The 20 used Map2 palette slots resolve to 19 unique Map.pak DDS sources plus
slot 13 named `1`, which has no matching map DDS. The explicit development
selection in `Derived/Map2/palette-selection.json` records complete=false and
the unresolved slot. The existing extractor verified source hashes and copied
the 19 selected textures into `Derived/Map2/Palette`; no default source priority
or arbitrary replacement texture was used, and no complete terrain descriptor
was created from this partial palette.

Editor run `601e75b7aa1943c082a2aaeff356dd4e` passed 35 tests, skipped 3, failed
zero. The new test verifies all 19 imports are 64x64 with five mip levels and
that constructing the tile requiring slot 13 fails with the incomplete palette.
Real dual-map transfer/reconnect remains passing. This tests import and refusal
to hide missing data, not final visual quality or runtime scene replacement.

Read-only source review so far locates the anomalous tile at index 37295,
tile coordinates x175/z145, raw index 0xC00D (rotation 3, slot 13). Legacy
GetTileIndex uses the same 0x3fff mask; the tile is internal, not a table edge.
The meaning/origin of texture name `1` still needs source investigation.

Completed bounded source review confirms slot 13 is literally ASCII `1` in
the packed HFL, with matching serialized slot index and no table misalignment.
Legacy HFieldManager.cpp loads the texture name directly and only logs failure;
no special meaning for `1` was found in that load path. The tile lies at a
slot-0/slot-11 boundary, but this does not justify choosing either neighbor.
Resolution requires another original source or an explicitly classified remaster
art override with visual review; it must not be labeled original restoration.

### 2026-09-12 — runtime map presentation binding and missing-map handling

MapVisualController now selects the configured visual prefab by admitted map ID
and rebuilds it when session/map generation changes. It derives movement and
entity coordinate extents from the imported heightfield and replaces the click
collider with that field's mesh. Old visuals are disabled immediately before
deferred destruction. Missing/duplicate configurations clear the old world and
disable world commands instead of allowing Map10 presentation to represent Map2.
ConnectionPanel updates presentation before dispatching current-generation events;
entity presentation is cleared on map teardown and ignored while unavailable.

Unity-executed MapVisualSetup updated ConnectionValidation. Inspection found the
saved scene lacked Map10InputController and ServerEntityRegistry despite those
scripts existing; setup adds and binds them to the connection and main camera.
The previous fixed visual and inspection renderer are disabled. Current catalog
contains only the source-textured Map10 prefab: Map2 is explicitly unavailable
until its visual dependency is resolved. This is a functional guard and reusable
switching path, not completed destination rendering or high-detail acceptance.

Editor run `9345714bd07e4f0e9beb9bb9d254395f` passed after the change. The new
controller test exercises stable-generation reuse, changed-generation replacement,
missing Map2 clearing collider/world, restored Map10, and disconnect teardown.
Real native dual-map transfer/reconnect remains passing, independently of the
presentation guard. Setup evidence: `modern/out/unity-remaster/map-visual-setup-v2.log`.
Independent Player and human scene input still require fresh verification.

### Map2 development presentation and fresh Player findings

Map2 now has a development terrain prefab with 64 chunks and 20 effective
materials. Its original palette remains incomplete: slot 13 (one tile, index
37295, authoring name `1`) has no recovered texture. The separate
`visual-override.json` explicitly borrows slot 11 for development inspection;
it is classified `development-placeholder`, releaseReady=false, and is not an
accepted remaster replacement or original recovery. Source bytes are unchanged.
The Editor-rendered `modern/out/unity-remaster/map2-development-preview.png`
is an asset preview, not gameplay or high-detail visual acceptance.

The scene catalog now includes Map10 and this development Map2. A real defect
was found in retiring the old fixed terrain: Unity named the imported instance
`Map10`, so lookup by `Map10TexturedTerrain` failed. Setup now matches the prefab
source reference, records its inactive override, and saves through Unity.
The regression opens the saved scene and verifies fixed terrain is inactive.
Actual imported Map10/Map2 prefab replacement and click-mesh replacement are
also tested. Importer name assertions follow Unity's actual main-asset naming.

Editor run `aa44b15265d847318441dabd88fdd218`: 42 tests, 39 passed,
3 skipped, 0 failed. Real modern three-server transfer/reconnect passed with
SQLite map 2, position 7211/43329 and money 4242. This is not human acceptance.

Windows x64 development Player rebuilt successfully; evidence:
`modern/out/unity-remaster/map2-player-build.log`. Fresh Player movement run
`ec9ea1bde74741098eb24730ce53e920` FAILED after reaching GameIn. It reported
`public event queue overflow`; movement/collision did not pass. Its log also
exposed UnityEngine.Input calls while only Input System was enabled.
`RemasterSetup.ConfigureInputCompatibility` now explicitly enables Both through
the Editor (ProjectSettings activeInputHandler=2), retaining the existing world
controls and Input System UI. This setting was applied successfully, but a fresh
Player rebuild/retest after that setting and queue-overflow diagnosis remain
required. No independent Player movement success is claimed for these changes.

### Bounded native event pumping: independent Player movement restored

Input-compatible Player run `42171248a8254d79b47a371190afc2df` reproduced
public queue overflow without the old Input API exception. The core previously
drained up to 4096 queued network messages into 256 public event slots in one
tick; MonsterAdd expands to three public events. A terrain-loading stall can
therefore turn a normal entity burst into failure.

NetworkEventQueue now supports conditional FIFO pop. Native tick reserves room
for message expansion and leaves a head that cannot currently fit in the bounded
network queue. Each queue is limited to 256 processed messages per tick. Inventory
and GameIn expansion are budgeted conservatively; shop catalog chunk count is
budgeted dynamically. With an empty public queue, an atomic message is processed
and the existing checks still reject expansion exceeding total capacity. No
unbounded staging queue or silent event dropping was introduced. Terminal event
replacement and true network overflow detection remain. The capacity 1/3 tests
now verify observable terminal timeout under backpressure rather than expecting
immediate public overflow from an unconsumed login queue.

x64 core: 91/91 tests passed, including a paused-consumer 100-monster burst
checking all 300 expanded events, IDs, ordering, InGame state and zero drops.
Staged DLL SHA-256:
`FAA57DC881B4A7901EA3A0B3A9E36BA783695EAF4CE6DF816412E2E44B6EC251`.
Windows Player rebuilt (`bounded-events-player-build.log`). Real three-server
Player run `bb6175e636a94e548917a857dda89dd0` PASSED GameIn, two native session
move/stop broadcasts, rejected-jump correction and audited collision rejection.
This is one Player containing two native sessions, not two human players.

Its `player.png` was inspected: terrain and development panels render, but no
character entities are visible and a bottom Chinese label has missing glyphs.
These remain real presentation defects; gameplayAccepted stays false. The full
game, formal art replacement, human input and performance acceptance remain open.

Editor transfer retry `6f0638fd15244af3a2b63ed7f60109f1` did not execute tests:
CLI refused a second batch Editor because an interactive project Editor was
already open (PID 31564, Pipeline ready). This is not a protocol regression
verdict. The open Editor was preserved. x86 Debug rebuild passed; full modern
CTest completed in `bounded-events-modern-ctest.log`: 12,565 registered,
12,557 passed, 8 skipped, 0 failed, 140.15 seconds. Skips remain the six MSSQL
fixtures, release human acceptance check, and deployment resource manifest;
they do not establish those acceptance gates.
One read-only subagent reviewed queue expansion/bounds; root implemented and
ran verification as sole writer.

### Bundled CJK font fallback and rendered glyph verification

Added an audited Noto Sans CJK SC Regular source font from the official
`notofonts/noto-cjk` repository, pinned to revision
`f8d157532fbfaeda587e826d4cd5b21a49186f7c`. Font SHA-256:
`2c76254f6fc379fddfce0a7e84fb5385bb135d3e399294f6eeb6680d0365b74b`.
`Assets/Moxiang/Fonts/source-manifest.json` records source URLs and byte hashes;
the upstream SIL OFL notice ships in StreamingAssets/ThirdPartyNotices.
Official source: https://github.com/notofonts/noto-cjk/tree/f8d157532fbfaeda587e826d4cd5b21a49186f7c/Sans

CjkFontSetup creates a 1024-square dynamic fallback with bundled source data,
40-point sampling and padding 4, and enables clear-dynamic-data-on-build.
Existing primary font assets retain their Latin glyphs and reference this
fallback. Setup checks all saved scene label text through TMP before saving.
The live connected Editor ran CjkFontTests: 1 passed, 0 failed. It clears the
dynamic atlas and verifies Chinese UI vocabulary can repopulate through the
primary fallback chain, with the bundled source and build cleanup enabled.

Live Play Mode screen capture was inspected at
`unity/MoxiangClient/Assets/Screenshots/cjk-scene-check.png`: both bottom Chinese
lines render fully, replacing the previously visible missing-glyph boxes.
The Editor was returned to Edit Mode after capture. This is an unconnected
development UI scene, not real gameplay/human acceptance. A fresh independent
Player build containing the font is still required. No native changes this turn.

Entity inspection confirms the registry currently exposes only monster/NPC/drop
prefabs and has no player-character presentation entry. Character source meshes,
skinning, appearance mapping and animation remain to be integrated; none of the
font work is evidence of completed character visuals.

### Independent Player CJK verification and model-local CHX motions

The live Editor completed a fresh Player build at 2026-09-12 13:48:07 UTC.
The synchronous Pipeline eval request timed out at five seconds, but the build
continued. Latest BuildReport was inspected: Succeeded; its one reported error
was that Pipeline request timeout, not a compiler/build failure. No duplicate
build was started. Real Player run `dab69bce465f4a329dfeb6b16250dc39` passed
GameIn, movement/stop, correction and collision checks. Its `player.png` was
inspected and the Chinese notice now renders correctly in the independent
Player. Character meshes and final UI remain absent/unaccepted.

Explicitly extracted Character.pak entry `man.chx` for development, source ID
`mxh:e8c41dbc55212b6a967d7f203a55415f05b75df0b3f1084ede5e8b7652456e73`,
SHA-256 `d7968a72d017e3b47e169cf90fa6b27e1b3192d1b7590fc72b660f31298f4155`.
Provenance remains unknown and releaseReady=false. The file contains five
model-local motion sections, each with 466 entries. The old modern parser
flattened all 2330 paths and discarded their association with the model part.
Original executive.cpp LoadModelData (1597-1609, 1633-1636) confirms each model
loads its own motions and stops at the next model-name token.

ChxModel now exposes ordered parts with their own motion paths while retaining
existing mod_files/motions flattened views for current consumers. Format comments
were corrected. Synthetic differing-list and real Character.pak assertions passed
with all 20 CHX tests (`chx-parts-tests.xml`); x86 Debug rebuild passed.
Full regression passed in `chx-parts-modern-ctest.log`: 12,558 passed, 8 skipped,
0 failed (12,566 registered, 165.97 seconds). Existing MSSQL/release/deployment
skips remain unverified acceptance gates. This establishes data
needed for Unity model import, not implemented skinning/animation presentation.

The source dependency audit is saved as
`modern/out/unity-remaster/character-source/man-dependencies.json`. Five MOD
parts and their repeated motion references resolve to 234 distinct names;
233 have a unique Character source candidate, while `m311.anm` has no entry in
the current whole-resource manifest. This is a scoped missing dependency,
not proof that no historical copy exists. Its original action semantics and
remaster replacement remain unresolved; it must not silently alias another
clip or be marked release-ready.

### Native MOD intermediate exporter and five real character parts

Added `modern/tools/MoxianUnityAssetExport/model.cpp`, built as
`mxh_unity_model_export`. It reuses parse_mod and emits versioned `.mxhmodel`
JSON preserving mesh positions/normals/UVs, material-indexed triangles, bone
hierarchy/transforms and per-vertex physique influences including local offsets.
Output is explicitly legacy-unconverted and releaseReady=false. Source SHA-256
must match, existing output is refused, malformed/non-finite data is rejected,
and original pack bytes remain read-only. This tool does not create Unity
bind poses, transform coordinate handedness, resolve textures, or import clips.

The real hash-pinned body contract verifies deterministic output, 355 positions,
normals, UVs and physique records; 96 bones; 365 influences; known texture name;
parent/bone references and normalized source weights. Wrong digest, truncated
input and overwrite attempts fail. Both native exporter CTests passed (model
and existing HFL). Build command is recorded in
`modern/scratch/2026-09-12-unity-remaster/build-model-export.cmd`.

Explicitly extracted and exported all five man.chx MOD parts under
`modern/out/unity-remaster/character-source`; `man-export-summary.json` records
derived hashes and texture names. Vertex counts: body 355, face 118, hair 212,
hands 94, shoes 68 (847 total). Each contains one mesh. Body/hair/hands/shoes
contain 96 bones each; face contains none, so assembly/attachment semantics need
verification rather than inventing a separate face skeleton. Material inputs
are m_nude.tga, m_face01.tif, m_hair01.tga, m_hands.tga, m_foots.tga. Their source
resolution and Unity import, skinning, animation and visible gameplay validation
remain outstanding. No claim of a rendered or complete character is made.

### Unity model import and source-pose visual inspection

Added ModelDescriptor/ImportedModel and `.mxhmodel` ScriptedImporter. Five
parts and five explicitly selected DDS inputs are staged under Derived/Man;
texture-sources.json records source identities/hashes. Imported meshes retain
the full source descriptor as a subasset. The initial inspection pose evaluates
weighted per-influence position/normal offsets using original world bone
matrices and the 0.001 scene scale. It does not substitute Unity single-bind-pose
skinning: measured body multi-influence bind positions differ by up to 1.0404
legacy units. Runtime animation deformation is still to be implemented.

The first rendered assembly exposed a floating face. Original geom_obj.cpp
ReadFile 992-995 reads vertices into m_pv3World; CommitDevice 437-439 applies
the inverse object matrix before local rendering. Thus unskinned exported
source-pose vertices are already world-space. Removing the duplicate object
transform fixed the face position. A regression checks non-identity object
matrices do not translate these source-pose vertices a second time. A separate
synthetic skin test checks unequal offsets, weighted translation and missing
bone rejection; five real-part cases check geometry/texture/data retention.

`ModelPreview.Capture` renders a temporary additive inspection scene and restores
the prior scene. `modern/out/unity-remaster/man-source-pose.png` was inspected:
body, face, hair, hands and shoes now form a recognizable textured character.
Ankle/part seams and material response remain visually rough. This is the
original low-detail base appearance in a source-pose asset preview, not the
modern high-detail quality sample, animated character or actual gameplay.
No runtime player binding, equipment switching or animation acceptance is claimed.
Live Editor model tests finished with 7 passed, 0 failed, 0 skipped after the
face fix; initial zero-test discovery during compilation was not counted.

### Native ANM motion export

Added `mxh_unity_motion_export`, using the existing native ANM parser. Export
retains header bytes/timing fields, object names/IDs/type/flags, key ticks and
frames, position/quaternion tracks, scale vectors plus scale axis/angle, and
the validated raw animated-mesh payload where present. The original
motion_obj.h SCALE_KEY stores an axis and angle, not a quaternion; the output
names reflect that distinction. Mesh animation bytes are retained but not yet
interpreted by Unity. No clip is silently represented as fully supported.

Explicitly selected Character.pak m001.anm source
`mxh:eede1044850da7909dfa56d9b14a70518da45bbbb7d1bef426d58f1c484ac115`,
SHA-256 `6e2b380afbe3fc3aece317427247dcc490db5d51a9312898acc46f1b13469bec`.
It contains 116 tracks and 5760 keys (1020 position, 4740 rotation), frames
0-59, ticksPerFrame=160, frameSpeed=30, no scale or mesh-animation keys.
The versioned intermediate is staged in Derived/Man/Motions/m001.mxhmotion,
releaseReady=false; provenance remains unknown. A prior scratch export without
the extra preserved fields is superseded by character-source/motion-v1/m001.mxhmotion.

Independent synthetic binary ANM contract tests verify timing, vector/quaternion
values, scale axis/angle, exact retained animated-mesh bytes, determinism,
hash mismatch, overwrite refusal, truncation and non-finite rejection. All three
standalone exporter CTests passed (ANM, MOD, HFL). Unity clip importing, hierarchy
sampling, source-vs-Unity sampled matrix comparison, looping/action timing and
actual animated rendering remain next work; no animation playback is claimed.

### Unity explicit-frame motion sampling

Added ImportedMotion and a .mxhmotion ScriptedImporter preserving source metadata,
plus LegacyMotionSampler for explicit source frames. The sampler maps tracks by
exact name, composes parent/world matrices independent of bone array ordering,
interpolates position and shortest-arc quaternion rotation, and clamps key
endpoints. Its matrix conversion follows the current native entity_scene.cpp
row-vector quaternion convention; independent original-engine numerical parity
is still required before accepting action playback. It does not drive root motion
or infer wall-clock duration from unverified timing fields.

Unsupported oriented scale and animated-mesh tracks remain retained in imported
data and throw explicitly if evaluated. Missing parents, cycles, duplicate tracks,
non-finite inputs and malformed key ordering are rejected.

Live Unity 6000.6.0f1 EditMode: ImportedMotionTests 4 passed, zero failed/skipped.
Evidence: modern/out/unity-remaster/motion-sampler-editor-tests.json. Tests cover
parent composition/rotation direction/end clamping, quaternion sign equivalence,
invalid hierarchies/unsupported tracks, and real m001 import with 116 tracks and
355 body vertices deformed at source frames 0, 30 and 59 using the 96-bone model.
This is sampled geometry evidence, not full animated-character visual acceptance.
Rigid face attachment, original-engine matrix parity, complete rendered playback,
action timing and actual Map10 player binding remain implementation work.

### Original x86 quaternion oracle and face attachment audit

Executed original SWorking/SS3DGFunc.dll exports through the independent x86
legacy_motion_oracle.cpp tool. SHA-256 is
F87933EEEFFDF820D35A1D7E37745E901DB1D78E7B0D9FEE3D80B5916CD796DA;
SS3DGeometryForMuk.dll imports this binary. The first oracle test failed against
Unity Slerp (matrix element 0.835807323 vs original 0.835346639). Disassembly
identified polynomial Sin/ACos, a 0.05 near-parallel threshold and no normalization.
LegacyMotionSampler now reproduces that computation without mutating source keys.
All 36 oracle pairs match matrices within 1e-5. ImportedMotionTests: 5 passed,
zero failed/skipped. Evidence: modern/out/unity-remaster/motion-oracle-editor-tests.json;
the checked-in fixture and provenance are under Assets/Moxiang/Tests/Editor.
This replaces the previous Unity Slerp implementation; it does not yet prove
complete original animation timing, oriented scale or rendered character parity.

A read-only subagent traced the real face attachment: original
[Client]MH/AppearanceManager.cpp:260 removes the CHX face, and :796-802 creates
the FaceList-selected object and AttachDress to Bip01 Head. EngineObject.cpp:781-788,
gxobject.cpp:392/441-445 and model.cpp:1405-1416 propagate the parent matrix.
geom_obj.cpp:439 first converts source world vertices through the inverse source
mesh matrix. Thus the face uses headWorld * inverse(faceSourceWorld) * sourceVertex,
not an invented face ANM track. FaceList_M.bin selection and this binding remain
to be implemented; the current default CHX face cannot establish player appearance.

### Rigid face attachment and sampled full-body previews

Added LegacyModelGeometry.AttachedPositions with source inverse followed by the
attachment node matrix, explicit singular/non-finite rejection and a regression
test for translated source coordinates plus a rotated/moved head. ImportedModelTests
now pass 8/8 in live Editor (the initial 7-test result during compilation was the
old assembly and was not accepted as new-code verification).

ModelPreview.CaptureFrame now evaluates all four skinned male parts with m001 and
attaches the rigid face to the sampled Bip01 Head. Temporary mesh copies and the
additive scene are destroyed; previous scene/render target are restored. Actual
Unity captures man-motion-frame-0.png and man-motion-frame-30.png under
modern/out/unity-remaster were visually inspected. Face/hair/body remain aligned,
with visible wrist/ankle seams and overly glossy source materials. These are fixed
frame restoration previews, not continuous gameplay or high-detail art acceptance.

The existing native CharacterAppearanceCatalog parser was compiled into a read-only
audit tool and run on FaceList_M.bin (SHA-256
7234F94EBFFFCE101CD123FD9FAEC1C7EE0C1156C6893E77EA8DF3EEB5300CDB).
It maps zero-based face types 0..4 to M_face01.MOD .. M_face05.MOD. Preview selection
of type 0 is now evidenced by unity/baselines/man-face-selection.json, not inferred
solely from CHX. Evidence: modern/out/unity-remaster/face-list-male.tsv. Runtime
selection from server appearance, other face imports and full Player binding are
still required. No original resource bytes changed.

### Runtime deformable model instances

Added AnimatedModelPart and importer v3 bindings to the source-model subasset.
Each instantiated part clones and owns its deformable meshes; source mesh assets
remain immutable. Explicit-frame sampling uses the same bone and rigid-head paths
as the preview. ModelPreview now calls this runtime component instead of maintaining
a separate deformation implementation. A live frame-30 capture completed with it.

The first lifetime test exposed that edit-time OnDestroy was not invoked on a
plain MonoBehaviour; ExecuteAlways fixes that path. Disable also restores imported
mesh references and releases owned copies, permitting reinitialization after enable
and providing the cleanup path used during script reload. Actual domain-reload and
Player lifecycle tests are still required.

AnimatedModelPartTests: 1 passed, zero failed/skipped, covering two simultaneous
instances, source immutability, independent poses, disable/re-enable, component
removal and whole-object destruction. Evidence:
modern/out/unity-remaster/animated-part-lifetime-tests.json. This is the runtime
mesh ownership layer; server-driven local/remote player presentation, clothing
selection and the original integer animation clock remain to be connected.

### Integer animation clock and composite runtime sampling

A read-only timing audit verified original executive.cpp:203-222,3612-3667 and
EngineObject.cpp:210-271,420-452. Added LegacyFrameClock using unsigned millisecond
tick differences and integer 1000/FPS, plus LegacyAnimationState preserving the
inclusive last frame, discarded loop overshoot, same-motion no-reset, custom restart,
pause, and non-loop return to the base motion. LegacyAnimationDriver broadcasts a
shared increment to scene characters. Its initial 30 FPS profile matches the current
GameDesc.ini (33 ms); ANM frameSpeed is not used as the runtime clock. Deployment
config override precedence and old executable callback timing remain unverified.

CharacterAnimation composes the runtime skinned parts and rigid face using the
same integer frame and head attachment matrix. It accepts explicit motion-number
bindings and releases driver subscriptions on disable. No server gameplay timers,
damage/cooldown rules or authority positions changed. Weapon/mode-specific action
selection is not yet connected; a single generic run animation is not sufficient.

Live Editor tests: LegacyAnimationClockTests 4/4 passed, CharacterAnimationTests
1/1 passed. These cover clock boundaries/wrap, loop/non-loop transitions and an
actual five-part male model sampled at frame 30 then looped at frame 60. Continuous
Play Mode scheduling, Player lifecycle and server-driven animation remain open.
Original GetAnimationTime uses float 1000/FPS and a different duration calculation;
that separate gameplay timing path must not be replaced with this visual clock.

### Continuous Editor Play Mode animation verification

Added CharacterAnimationPlaybackTests with EnterPlayMode/ExitPlayMode lifecycle.
The live test instantiated all five imported male parts and a scene driver, then
observed real Update callbacks and changing mesh vertices. While the actor was
inactive, the driver continued but the actor frame stayed unchanged and its owned
mesh was released. Reactivation resumed playback with a new mesh; destruction
released that mesh. The test passed 1/1 with no unexpected logs. This is actual
Editor Play Mode execution inside an EditMode test harness, not an isolated Player
test or server-connected character acceptance. Afterwards Editor reported
playing=false and zero remaining LegacyAnimationDriver scene objects.

Next server-binding boundary verified in current code: CoreSnapshot.characters
carries gender/face/hair plus character-list equipment, while ConnectionPanel.Inventory
publishes current 124-slot ItemTotalInfo including worn slots 80..89. Runtime
appearance must use current equipment after inventory admission rather than treating
the stale character-list equipment as an ongoing authority. Local/remote player
presentation and equipment-dependent motion selection remain incomplete.

### Admitted local appearance state

ConnectionPanel now updates LocalAppearanceState after draining admitted inventory
events and clears it on release. Face/hair/gender come from the uniquely selected
valid character; all ten worn item IDs come from the current completed ItemTotalInfo
slots 80..89. InventoryState.Matches ties those items to the same player, session
and map generation. Missing/partial inventory, stale generations and ambiguous or
missing selected characters clear appearance rather than retaining a previous actor.
Published equipment is an immutable copy; unchanged snapshots preserve object identity
so a visual consumer can avoid rebuilding meshes every Update.

LocalAppearanceStateTests passed 2/2 in live Editor: current equipment overrides
stale character-list equipment, partial replacement is not published, retained
snapshots cannot mutate, and map/session/identity mismatches are rejected. This
completes the admitted appearance data boundary, not resource resolution or player
rendering. Catalog-driven model assembly and actual local/remote actor binding remain.

### Native appearance catalog export

Added mxh_unity_appearance_export, reusing CharacterAppearanceCatalog and the current
ItemList parser. Explicit loose inputs are Resource/Client/{ModList,FaceList,HairList}_{M,W}.bin
and Resource/ItemList.bin; the output records each path, byte count and SHA-256.
It preserves list order, base CHX names, all item IDs/Part3DType/Part3DModelNum/WeaponType
without changing any gameplay fields. Output appearance-v1.mxhappearance under
modern/out/unity-remaster contains both genders and 9,887 distinct items, remains
releaseReady=false, and has not yet been bound into Unity runtime assembly.

The new contract test independently hashes all seven inputs, checks the male face
index sequence and all item ID uniqueness, deterministic output, overwrite rejection
and missing-input failure without an output file. Standalone x64 exporter build
succeeded; all four exporter CTests passed. Existing x86 client/core code was not
changed in this step. Full appearance resolution must also port active skin/avatar,
headgear and weapon handling: AppearanceManager.cpp:810-979 is commented-out history;
the active worn-item implementation starts at :980 and must be the reference.

### Unity appearance catalog import and indexed lookup

Staged the derived appearance-v1.mxhappearance under Assets/Moxiang/Derived and
added a ScriptedImporter plus ImportedAppearance/AppearanceIndex. Unity now retains
all 9,887 records and resolves gender-specific face/hair and item model names using
the original zero-based table indices. Missing items, invalid gender/face/hair indices,
nonvisual 0xffff part types and model indices outside the selected gender's table
return failure without a substitute model. Duplicate item IDs and invalid resource
names reject the catalog. AppearanceIndexTests passed 2/2 in live Editor, covering
the real catalog and malformed/out-of-range lookup cases.

This is an indexed source lookup, not final appearance composition. Active original
AppearanceManager.cpp:983-1134 confirms additional avatar/skin priority, gender-specific
head attachment and material-stage rules. Those must be represented explicitly;
successful lookup alone does not imply an equipped item should replace a body mesh.

### Source part operation classification

Added AppearancePartRule for the operation after appearance-priority admission:
female part type 0 becomes a Bip01 Head attachment; type 7 first requests the
gender-specific NULLHAIR model and then attaches to the head; ordinary types 0..4
replace their corresponding part. Type 5 is explicitly reserved for separate weapon
handling, never appended as a body mesh. Nonvisual 0xffff yields no operation;
unresolved indices and unsupported part types throw rather than substituting content.
Source: active AppearanceManager.cpp:1000-1017,1048-1050,1081-1126.

AppearancePartRuleTests passed 4/4 in live Editor after correcting unsigned NUnit
test arguments. A transient request failure was checked with test_status=no_tests
before retrying. These tests do not close avatar/skin/worn priority admission or
weapon placement; those remain required before complete character assembly.

### Worn-item appearance priority pass

Added WornAppearancePolicy over explicit effective Avatar[23], optional resolved
avatar dress row, named skin overrides and full-moon event state. It preserves
the active source branch order: ordinary slot admission (including the original
slot+14 indexing), headgear suppression, type-7 hair replacement before suppression,
the dedicated glove override branch, and dress-driven hand/foot hiding before a
shoe replacement may be suppressed. Weapons remain outside this worn-part pass.
Source: AppearanceManager.cpp:985-1126 and CommonGameDefine.h:1014-1029,2710-2737.

WornAppearancePolicyTests passed 4/4 in live Editor. Tests cover ordinary flags and
dress skins, hidden hair despite masked headgear, persistent hand/foot hide effects,
glove overrides bypassing the generic dress branch, and missing-context rejection.
Missing item/model records fail explicitly rather than silently resolving another
model; unsupported glove override part types remain errors. No default all-zero
avatar array is manufactured. Server shop/avatar/skin fields and lookup of the
avatar dress row are not yet bridged into this pass, and actor assembly is still open.

### Shop appearance wire layout boundary

Verified HERO_TOTAL_SHOP_OPTION_OFFSET=225 and MUGONG_OFFSET=345: the current
wire block is 120 bytes. Reference CommonGameDefine defines 23 avatar and 5 skin
slots; CommonStruct.h:1218-1284 places skin at +106 and street-stall decoration at
+116. The modern internal ShopItemOption instead has 24/6 slots and size 124.
It must not be memcpy'd into this wire region or silently truncated.

Added ShopAppearanceWire parser preserving all 120 raw bytes and decoding exactly
23 avatar values, 5 skin values and the decoration field. The parser rejects both
truncation and the 124-byte internal layout. Native x64 build and all 93 core tests
passed (including two new byte-boundary tests), 9.80 seconds. No C ABI size or wire
header changed, and the parser is not yet connected to outgoing public events.

Current Map GameIn construction leaves this shop region zero-filled rather than
serializing the runtime shop state. Existing internal type documentation claiming
exact original layout is therefore not sufficient evidence. State persistence,
source mapping, server serialization and Unity event admission must be implemented
before the worn-policy context can be considered authoritative.

### Admitted character appearance snapshot

The selected character snapshot now obtains gender, face and hair from the admitted
GameIn state while InGame, with both selected-character and player identity checks.
Character selection continues to expose character-list data. This changes neither
the C ABI layout nor original protocol headers. The real socket round-trip fixtures
send different list (1/2/3) and GameIn (0/4/1) appearance values and check both states.
The x64 core build and all 93 tests passed (9.85 seconds).

The initial DLL staging attempt failed because the live Editor held the old plugin.
After verifying Play Mode and compilation were inactive and the only open scene
ConnectionValidation was saved, the Editor exited cleanly through its API. Staging
then succeeded: SHA-256
`8502BB28659D1B16472E168DCB7E0F31463E553F2DEE6E8F1C73E75EB0263FCA`.
The reopened Editor (PID 23444) reported ready and NativeClientTests passed (1/1),
exercising repeated plugin create/snapshot/dispose operations.

The development Player was rebuilt successfully at 2026-09-12T15:22:48Z. Its plugin
hash matches the staged core above. BuildReport contained one error message from
the Pipeline request's 5000 ms observation timeout; inspecting all error messages
confirmed this was the timeout, and the actual build result was Succeeded.
The build was not repeated on timeout.

Fresh real-three-server SQLite/HSEL Player run
`334df14944a847bd9f254be61447d4c4` passed GameIn and the two-native-session movement
and collision fixture checks. All three servers remained alive until cleanup.
Evidence is under `modern/out/unity-remaster/three-server/334df14944a847bd9f254be61447d4c4`.
These are automated sessions within one Player, not two human players or MSSQL
acceptance. The assembled animated character still needs integration into gameplay;
this does not pass the visual quality sample gate.

### Shop appearance event bridge and source loading audit

Core event 26 transports the exact 120-byte GameIn SHOPITEMOPTION block, tagged
with player, session/map generation and wire schema 1. The public queue reserves
one additional slot for GameIn expansion. Existing ABI structure sizes and wire
headers are unchanged. Socket fixtures compare every byte (including embedded
zero/high-bit values) in plaintext, HSEL and CP936 paths. All 93 native tests pass
(9.95 seconds); the first test attempt drained the event in the wait helper, and
was corrected to retain events for explicit byte assertions.

ConnectionPanel now admits this event through ShopAppearanceState: exact length,
schema, identity, generation and sequence checks; immutable copied raw/decoded
data; malformed data and transitions clear state. Its two Editor tests pass.
Received intentionally does not certify server persistence loading and is not used
to enable final character assembly. The development Player probe now requires this
block to reach managed state before reporting its network smoke success.

Read-only subagent shop_state_audit verified these original-source paths under
reference/legacy-source/4dddd9a6: [Server]Map/Player.cpp:117-119 initializes Avatar
indices 0-11 to zero, 12-22 to one, and Skin to zero. Player.cpp:3452-3479 waits for
SHOPITEM_USEDINFO; MapDBMsgParser.cpp:6659-6688 distinguishes successful zero rows
from used-item rows, processing UsedShopItem then PutOnAvatarItem. Independent Skin
loading is at MapDBMsgParser.cpp:8573-8590. GameIn copies the calculated state at
Player.cpp:3533-3534. Ordinary inventory is not a substitute for either data source.

Correction to prior assumptions: modern PlayerState does not currently own a
ShopItemOption. MapHandler loads ordinary containers only, does not restore used
shop timers/equip state or Skin, and also zero-fills other-player appearance.
Missing persistence must not be treated as a successful empty query. Next server
work requires explicit unloaded/loaded/failed outcomes, authoritative shop/Skin
data sources, original expiry/equipment calculations and matching GameIn/observer
serialization. The event bridge alone does not complete this server work.

The new core was staged after a clean saved-scene Editor restart. Core and Player
plugin SHA-256 both equal
`7E8C812FBFADA357424BA9BF622C788094CA0E9C13A3284DC9D915F1250C11A1`.
Player build succeeded at 2026-09-12T15:30:39Z with zero BuildReport errors; scheduling
the build via Editor delayCall avoided the synchronous Pipeline timeout.
Real-three-server SQLite/HSEL Player run `f8ea9df7d9524b48806f77c4cabab9e9` passed
with `shopAppearanceReceived=true`, plus movement/collision checks. This verifies
the current server bytes reach managed state, not their missing persistence source
or the final visual appearance. No human/MSSQL/quality gate is claimed.

### Legacy shop and Skin database source reader

Read-only inspection of local restored `mhgame` confirmed the actual definitions
of MP_SHOPITEM_UseInfo and MP_CHARACTER_SkinInfo. The first reads
TB_SHOPITEMUSEINFO (ITEM_IDX, ITEM_DBIDX, ITEM_PARAM, BEGIN_TIME, REMAIN_TIME), left
joins TB_ITEM for item_position with a zero fallback. The second reads five fields
from TB_SKININFO and explicitly returns zeros when the character has no row.
These definitions confirm that missing tables are not empty-character evidence.
Both tables currently contain zero rows; `mhgame` has no modern `character_info`
table, so modern and legacy character IDs must not be silently equated.

Added `modern/include/mxh/db/legacy_shop_appearance.hpp`: parameter-bound, read-only
source loading with explicit invalid-character/query-failed/invalid-data/loaded
outcomes. Data is published only after both reads validate. Preserves signed SQL
INT bit patterns in DWORD fields, validates WORD ranges/types, rejects ambiguous
Skin rows and partial query failures. It makes no schema changes or expiry/avatar
calculations; caller must establish legacy identity and transaction context.

`mxh_db_tests` builds successfully. Four SQLite tests cover missing vs empty source,
character isolation, missing joined position, signed DWORD preservation, partial
failure, malformed/range-invalid values and duplicate Skin rows. One opt-in MSSQL
test verifies the restored source against both stored procedures without writes.
Both registered CTest groups passed (0.65 seconds) via
`modern/scratch/2026-09-12-unity-remaster/test-shop-source-mssql.cmd`.
Initial TCP localhost:1433 connection timed out; explicit local shared-memory
`lpc:localhost`, port 0, Windows authentication succeeded. No service/network setting
changed. Live MSSQL evidence covers empty source rows only; non-empty rows are
currently tested in SQLite fixtures. This is not full MSSQL gameplay acceptance.

MapHandler does not yet consume this loader: verified legacy-to-modern ownership
mapping and modern persistence coverage are still missing. Retaining the reader's
explicit boundary avoids applying another character's legacy cosmetics by numeric
ID coincidence. Next work remains restoration/calculation and runtime admission,
not substituting default appearance for unavailable sources.

### Corrected existing avatar layout and PutOn restoration semantics

Before wiring restored rows into the server, source comparison found existing
calculation helpers incompatible with the verified avatar data. Corrected the
modern Avatar count/sentinel from 24 to 23 and Skin count from 6 to 5. SHOPITEMOPTION
is now 120 bytes; compile-time assertions pin Gengol at 46, Skin at 106 and street
decoration at 116. Original headers remain unchanged; prior 124-byte notes above
describe the superseded implementation. This alone does not add GameIn serialization.

PutOnAvatarItem now derives the sword boundary from enum value 17, accepts absolute
inventory positions via the inventory provider (rather than comparing them against
the avatar count), and follows source ShopItemManager.cpp:1847-1904 in order.
Cosmetic removals restore the old flags before the new mask clears indices 12-16;
missing old cosmetic records preserve the slot, and a zero own-slot mask executes
the source's immediate removal path. The unusual source effect that updates the
new ItemIdx during removal was deliberately preserved, not silently fixed.

Existing tests that encoded wrong counts, shifted fist indices and reversed mask
semantics were corrected using explicit valid masks. Added source-boundary cases
for absolute positions 240/390, exclusive avatar bound 23, sword slot 17, missing
old records, own-mask removal and ordered DB/in-memory effects. Read-only agent
avatar_layout_audit independently reviewed the revised PutOn against original
1792-1921 and found no material remaining mismatch in this path.

Known separate discrepancy: TakeOff's existing continue skips the subsequent own
mask check (source 1991); abnormal own-mask-zero data can therefore return a
different outcome. Keep it open for a distinct behavior fix. The complete modern
build including CHINA/KOR/JAPAN/HK/TL binaries succeeded; full regression is running.

The first full regression registered 12,577 tests and found one failure after
456.68 seconds: the old AllTwentyFourSlotsCanContribute fixture still wrote 24
entries into the now-correct 23-element array, triggering a stack-cookie failure.
The fixture now iterates EAvatarCount and asserts the original 23-slot sum; related
test names were corrected. Production logic was not changed to accommodate the
invalid fixture. Rebuild succeeded; the full regression is being rerun with four
workers. The initial failed run is retained in avatar-layout-modern-ctest.log.

The corrected full run passed: 12,577 registered, 12,568 passed, 9 skipped,
zero failures, 121.64 seconds. Evidence:
`modern/out/unity-remaster/avatar-layout-modern-ctest-final.log`.
Skipped checks include optional MSSQL connections/flows and release/deploy gates;
they do not imply gameplay acceptance. The eight-file avatar layout/PutOn fix was
committed independently as `15fb5d0f` (`bug: align avatar layout and equip transitions
with original source`). Earlier unrelated remaster work remains in the worktree.
TakeOff anomaly, expiry restoration, persistent identity mapping and MapHandler
admission remain unfinished; no full character rendering or quality gate is claimed.

### 2026-09-13 — TakeOff own-mask fidelity and expiry audit

Removed the premature continue in TakeOff after clearing its own slot. The source
ShopItemManager.cpp:1974-2005 still checks the mask in the same iteration, including
the unusual item-info lookup at index zero when the own cosmetic mask is zero.
Two tests pin both missing-zero-info failure (after the initial ordered effects,
before broadcast/recalculation) and present-zero-info behavior. The shop test target
build succeeded; all 58 related AvatarEquip/PutOn/TakeOff tests passed (1.97 seconds).
This change is not yet separately committed and has not had another full regression;
the 12,577-test full result above applies to the previous commit.

Further source audit: current ShopItemManager::used_shop_item only inserts a row.
Original UsedShopItem at 3493-3618 also restores special counters, checks ItemInfo
SellPrice for realtime expiry, discards/deletes expired items, and applies options
with locale-specific exceptions. Existing collect_realtime_expired instead filters
on stored Param==1 and skips zero end times. These cannot substitute for the original
configuration-driven path, especially equipped avatars whose Param is 10. Integrate
the actual item metadata and preserve expiry side-effect/failure ordering before
publishing restored appearance. PackedTime's field extraction was checked against
CommonStruct.h:4088-4119; no time-layout change was justified by that check.

### 2026-09-13 — Catalog-driven realtime expiry

The realtime and avatar expiry collectors now require the actual ItemManager
catalog and test ItemInfo.SellPrice, instead of mutable ShopItem.Param. Equipped
avatars with Param=10 therefore remain subject to their configured realtime
expiry. Missing item definitions are skipped, catalog playtime rows are excluded,
and the source's strict current-time > end-time comparison includes end-time zero
without an invented immortality exception. The consuming helper uses the same
catalog predicate. Four new boundary tests and the existing expiry tests passed
(19/19). The test catalog uses an explicit uint16_t item-id conversion.

The complete modern build succeeded, including CHINA/KOR/JAPAN/HK/TL targets.
Full CTest regression finished: 12,583 registered, 12,574 passed, 9 skipped,
zero failures, 123.35 seconds. Evidence is recorded at
`modern/out/unity-remaster/shop-expiry-modern-ctest.log`. The skipped cases are
optional MSSQL checks and release/deploy gates; these remain unverified by this run.
This regression also covers the preceding TakeOff own-mask correction.
Scoped diff checks pass;
the wider worktree still contains pre-existing Unity-serialized trailing spaces.
This is a scanner correction, not completed MapHandler restoration or expiry
side-effect integration. Persistent identity mapping and actual restored shop
appearance admission remain open. No new Player or visual acceptance is claimed.

Committed separately after that build/regression: `056aa3e` (catalog-driven
realtime expiry, three files) and `b46d20ae` (source own-slot unequip mask handling,
two files). Other in-progress remaster changes remain in the worktree. The root
agent performed these changes and verification; no new delegation was used.

### 2026-09-13 — Used-shop restoration orchestration

Added `modern/include/mxh/server/restore_used_shop_item.hpp`, implementing the
original UsedShopItem ordering at ShopItemManager.cpp:3493-3619 against an actual
ShopItemManager, ItemManager catalog and ShopItemOption. Required abstract effect
bindings expose the database update/delete, discard, expiry log, option calculation
and duplicate-counter operations; there are no default success implementations.
The caller must supply the player identity and persistence/error handling.

Restores the two special missing-definition counters, preserves the WORD casts,
converts StatePoint_30's icon before its database update while retaining the original
row parameters, rejects expired rows before admission, and stops after a failed
avatar discard. Locale selection explicitly covers China/Korea/Japan/HongKong/
Thailand and the eight expansion ids. Incantation CheRyuk selects Param; ended
personal-plustime charms skip option/duplicate effects after insertion. The modern
table's insertion failure is explicitly reported instead of pretending admission.

The shop test target builds successfully. Nine new restoration tests plus 23
existing insertion/avatar-expiry tests passed (32/32, 1.39 seconds), recorded in
`modern/out/unity-remaster/shop-restore-targeted-ctest.log`. The first run exposed
a duplicate catalog registration in the locale test fixture; clearing that fixture
between cases fixed it, without changing production behavior. No full regression
or separate commit has been made for this addition yet.

This entry point is not yet bound to MapHandler or the live databases. Restored
identity mapping, effects, admission and real re-login/appearance acceptance remain
unfinished; the helper cannot establish those results on its own. The previous
insertion-only API is now documented to direct restoration callers to this API.

### 2026-09-13 — Restored items use the actual option calculator

Added ItemInfo-to-CalcShopItemOptionInfo conversion and a
ShopRestoreCalculatedEffects binding that invokes apply_calc_shop_item_option on
the same manager/options. It re-queries the converted icon, preserving the original
StatePoint_30 -> StatePoint lookup behavior rather than reusing the old definition.
Database, discard, logging and duplicate-table bindings remain mandatory abstract
operations. Player identity and MapHandler admission are not yet wired.

The source casts AttrRegist.GetElement_Val(ATTR_FIRE) to DWORD at line 1630;
CommonGameStruct.h maps fire to the first stored element. The calculation input
now retains DWORD width instead of uint16_t. Non-finite, negative or out-of-range
float metadata is rejected before integer conversion; this is explicit malformed
input handling, not a claim about the original undefined conversion behavior.

Tests execute actual ProtectCount/ProtectItemIdx and ItemMixSuccess restoration,
converted-icon re-query, and fire-value width/fraction behavior. An initial test
used the sundries category for the source's charm-only decoration branch; the
fixture was corrected to charm. Shop-target build and 111 related CTests pass
(3.30 seconds), including option and plus-time regressions. Evidence:
`modern/out/unity-remaster/shop-restore-calculation-ctest.log`. No full regression,
commit, live database restoration or new Unity Player acceptance in this step.

Further integration audit found that current MapHandler still writes a zeroed
SHOPITEMOPTION block. The duplicate-parameter helper also needs source comparison:
original AddDupParam returns on a missing nonzero table key, whereas the current
lookup API exposes only a zero bitset and cannot distinguish a missing entry.
Resolve that lookup distinction and category masks before binding restored dup
state; do not silently substitute counter increments for source bitsets.

### 2026-09-13 — Restore duplicate restrictions into real manager state

DupParamLookup now requires a presence-aware lookup. AddDupParam stops at the
first missing nonzero category key, preserves earlier effects, and continues past
a present zero Param. Each of the five categories masks only the source-listed
flags (ShopItemManager.cpp:2286-2390), preserving unrelated existing counter bits.
Three tests cover every missing-category position, present-zero continuation and
all-bit input filtering.

ShopRestoreCalculatedEffects now binds the actual duplicate helper to
ShopItemManager's five counters and ShopItemOption.bStreetStall using the supplied
lookup. Nonzero fractional LifeRecoverRate retains the source's lookup at key zero
after DWORD truncation. One restoration test verifies real incantation/sundries/
pet counters and the street-stall flag. Invalid float indices are rejected before
integer conversion. Required database/discard/logging effects remain unbound.

Shop test target built; 51 related restoration/duplicate tests passed (1.85s),
evidence `modern/out/unity-remaster/shop-restore-duplicates-ctest.log`. Full modern
regression and commit remain pending for this accumulated restoration work.
DeleteDupParam and IsDupAble have not yet received complete source-fidelity fixes:
the former has source unconditional XOR cases for later charm flags and early
returns on missing definitions, while the existing helper differs. Passing existing
tests for them is regression evidence only. Real login/database/Player acceptance
is still open. This turn was implemented and verified by the root agent.

### 2026-09-13 — Duplicate deletion and eligibility fidelity

Compared DeleteDupParam and IsDupAble against source lines 2394-2634. Deletion
now preserves the source's unconditional XOR for Ghost/Woigong/Naegong/Hunter/
ExpDay, clears only MemoryMove/ProtectAll in the incantation category, and stops
at missing table entries after preserving earlier category effects. Other category
updates affect only their listed masks. Eligibility rejects missing entries and
ignores unlisted bits; present zero-bit entries remain valid. Both paths honor a
nonzero fractional pet index that truncates to key zero.

Five new boundary tests cover these cases. The old test that treated a missing
entry as a present zero was corrected to actually register the zero entry.
The shop target builds and 54 relevant tests pass (1.93s), evidence:
`modern/out/unity-remaster/shop-duplicate-delete-check-ctest.log`.
Complete modern build also succeeded across all five locale binaries. Full CTest
finished: 12,605 registered, 12,596 passed, 9 skipped, zero failures, 123.34 seconds.
Evidence: `modern/out/unity-remaster/shop-restoration-full-ctest.log`.
Skipped MSSQL/release/deploy cases remain unverified by this run. This covers
accumulated restoration changes, not live login or visual acceptance.

Committed separately after validation: `b769f894` (three-file source duplicate
transition fix) and `2ab14f91` (six-file used-shop restoration and calculator binding).
Other in-progress Unity and server work remains uncommitted. Next integration
requires actual persistence/identity/effect bindings and admission wiring; these
commits do not make the zeroed MapHandler shop block authoritative.

### 2026-09-13 — Real duplicate-resource startup binding

Located the original `modern/data/PlayDH/Resource/ItemdupOption.bin` (7,079 bytes,
SHA-256 `3936cd18714165febe6126ee649a9310ebcb80a63123888b71f0927a3d7809e3`).
Original LoadShopItemDupList at GameResourceManager.cpp:2389-2443 reads four tokens
per entry. Added a presence-aware catalog using the existing MHFile text decoder,
with payload bounds, both checksum bytes, numeric/category/duplicate-key checks.
Raw comment tokens are consumed without changing their encoding. Successful load
publishes the complete catalog; invalid files cannot partially replace it.

MapServer now requires this resource from its selected resource root and stores
the parsed catalog in MapHandler. This replaces the missing startup resource
binding; player restore admission remains unfinished. The real file supplies 28
entries, and tests verify actual category masks and corrupted/truncated rejection.
Shop test target and CHINA MapServer built; 14 relevant CTests passed (0.95s).
Evidence: `modern/out/unity-remaster/shop-dup-catalog-ctest.log`.

Existing Unity Player passed isolated real Login/Agent/Map SQLite/HSEL smoke,
including two native movement sessions and collision correction:
run `16d69e043987444891171603bfaf69ef`, under
`modern/out/unity-remaster/three-server/16d69e043987444891171603bfaf69ef`.
This proves the new required resource does not prevent that real startup/GameIn
path. It is not human acceptance, MSSQL restoration, or new character visuals.
No full regression or separate commit for this startup change yet.

### 2026-09-13 — Modern character-owned shop persistence

Source inspection found no legacy identity mapping in the current login path:
GameIn verifies character_info.chrid/userid against the authenticated modern account.
The existing character_data BLOB/VARBINARY(MAX) column has no other runtime readers
or writers in modern code. Added `modern/include/mxh/db/modern_shop_state.hpp` to
store a versioned modern shop record there, without changing database schema or
implicitly mapping legacy numeric ids. Legacy import remains a separate explicit
identity-mapping operation.

Format v1 is little-endian MXSH/version/player/count, five skin WORDs and bounded
20-byte used-item records. It preserves DWORD bit patterns, rejects duplicate icons,
invalid versions/lengths/owners and unknown pre-existing blobs. Only SQL NULL on a
successfully queried owned character represents an empty modern record. Saves
revalidate ownership and compare the exact original BLOB/NULL in the UPDATE so a
stale snapshot cannot replace a newer state. Unknown blobs are never overwritten.

Database target built after qualifying db::bind to avoid std::bind ADL ambiguity.
Three real SQLite tests passed (0.60s): state round-trip and owner checks, stale
save/cross-character/unknown-blob rejection, duplicate icons and bad version.
Evidence `modern/out/unity-remaster/modern-shop-state-ctest.log`. No MSSQL write test,
full regression or live GameIn binding for this addition yet. The data representation
reuses the existing row value types; this does not imply that legacy source rows
have been imported or assigned to a modern character.

### 2026-09-13 — Restoration persistence effects and rollback evidence

Added ShopRestorePersistenceEffects, binding the source-ordered restore operations
to a private copy of the loaded modern shop record. StatePoint conversion updates
the persisted icon/parameters while the runtime manager retains the original
parameters for the current restore. Expiry deletion requires a unique matching
database identity; ambiguous/missing identities throw instead of editing an
arbitrary row. Failed discard never reaches deletion or expiry logging.

The binding saves through the ownership/CAS-protected database API and does not
commit or publish the player. The admission caller must keep inventory changes
in its unpublished candidate, persist them in the same transaction, and publish
only after successful commit. Discard and logging remain mandatory concrete hooks;
MapHandler is not wired to this binding yet.

Shop test target compiled without the newly introduced shadowing warning after
renaming the fixture variable. Seventeen related CTests passed (0.97s), including
actual SQLite save/rollback/reload of converted state and failed/successful discard
draft behavior. Evidence: `modern/out/unity-remaster/shop-persistence-binding-ctest.log`.
No full regression, separate commit, MSSQL write or live login acceptance for this
step. Existing live smoke remains the earlier resource-startup test only.

### 2026-09-13 — GameIn actor identity prerequisite

Admission audit found that load_char_data verified authenticated ownership but
handle_gamein never assigned PlayerSpawnInfo.user_id, leaving the live actor's
account id zero. It also populated appearance in PlayerInfo/wire data without
copying gender/face/hair into PlayerState. GameIn now propagates these verified
values to the actor; runtime snapshots expose them for verification. No protocol
structure or database schema changed.

Added a real SQLite handler test proving wrong-owner rejection and correct account/
appearance retention. The target-map test now asserts the account id. Handler and
CHINA MapServer targets built; four relevant CTests passed (0.64s), evidence
`modern/out/unity-remaster/gamein-actor-identity-ctest.log`.
Existing Unity Player passed real three-server SQLite/HSEL movement/collision smoke
run `36861b846d2d492ea2870ebb2e726317`. This validates continued network operation;
it does not prove shop persistence admission, new character rendering or human
acceptance. No full regression or separate commit for the identity fix yet.

### 2026-09-13 — Player shop-state carrier and resource caps

PlayerState now owns ShopItemOption with the source Player.cpp:117-119 defaults:
Avatar slots 12-22 are one, slots 0-11 and other fields are zero. These are worn
visual-enable flags, not synthetic equipment IDs or evidence of database restore.
Added apply_shop_options to retain the option record and replace the three shop
resource-cap contributions in CalcEquipBonuses before recalculating maxima. It
preserves equipment/avatar contributions and does not heal current vitals.

Player/state targets built; ten relevant CTests passed (0.69s). Two new tests
verify exact defaults and idempotent cap application/removal with unchanged current
HP and unrelated bonuses. Evidence:
`modern/out/unity-remaster/player-shop-state-ctest.log`.
This is the resource-cap binding only. Base attribute contributions, combat,
progression, expanded slots, full GameIn restoration/publication and visible
character assembly remain to be connected and verified. No full regression,
separate commit or new Player smoke was run for this state-layout change.

### 2026-09-13 — Player shop-state full regression

Reverified the canonical Unity 6000.6.0f1 Editor through `unity status --json`:
the project is connected and ready on port 7800. No Editor-connection blocker
was present during this check.

Rebuilt all modern Debug targets (299 steps), including CHINA/KOR/JAPAN/HK/TL
server variants, after the PlayerState layout change. Build exited zero.
Full CTest: 12,615 registered, 12,606 passed, nine skipped, zero failed;
124.94 seconds. Log: `modern/out/unity-remaster/player-shop-full-ctest.log`.
The skipped tests cover release human smoke, deployment manifest and opt-in
MSSQL checks; they are not accepted by this run.

The existing standalone Unity Player then passed the real modern three-server
SQLite/HSEL smoke, including two native-session movement and the Map10 blocked
cell check. Run ID: `4c48d78c74cd4315bb1425db70e72dbf`, evidence directory:
`modern/out/unity-remaster/three-server/4c48d78c74cd4315bb1425db70e72dbf`.
This is automated runtime regression, not human or visual-quality acceptance.
GameIn still needs full shop restoration and publication wiring; the new state
carrier alone does not complete that path.

### 2026-09-13 — Actual shop-option transmission to Unity

GameInAck's 120-byte block at offset 225 and CharacterAdd's block at offset 161
now encode the admitted actor's ShopItemOption, via the PlayerInfo snapshot,
instead of leaving zero-filled buffers. A shared explicit little-endian encoder
preserves all fields, including signed bytes and DWORD values. This transmits
the existing source defaults; it does not invent restored shop rows.

Seven scoped CTests passed (1.82s): packed-reference wire comparison, existing
layout checks, authenticated actor/target-map entry, and every shop byte in the
two self ACKs and four observer copies produced by the current two-link routing.
The first test run exposed an incorrect expected message count of four; the
actual routing sends six, and the count was corrected after inspecting routing.
Evidence: `modern/out/unity-remaster/shop-wire-state-ctest.log`.

The development Player probe now records the managed Avatar and Skin arrays.
The isolated three-server smoke requires exact no-shop fixture defaults, rather
than accepting mere event receipt. Unity's new BuildReport succeeded with zero
errors (started 2026-09-12T17:13:53.2778445Z). A CLI main-thread observation timed
out during the build; the later actual BuildReport and runtime test establish
completion. Three-server SQLite/HSEL run `3d8b83c630d24d63ab87ac090e909ae4`
passed managed shop values, movement and collision. Its `report.json` contains
Avatar[0..11]=0, Avatar[12..22]=1 and five zero Skin slots; Player run ID is
`1815135468e94b7cbdebe37fa13bf6b2`. This is not visual or human acceptance.
No full modern regression or separate commit was performed for this wire change.

Read-only admission audit by a Sol sub-agent confirmed the next binding work:
restore before connected_players_/player_runtimes_ publication, persist candidate
inventory and the optimistic shop blob in one transaction, and publish only on
success. Existing write_player_items reads published actors and needs a candidate
overload. Source shop inventory positions are 390..429 but the player initializes
only 20 slots, 390..409 (CommonGameDefine.h:1590-1594; Player.cpp:98-99 in the
read-only 4dddd9a6 reference). Modern ItemTotalInfo already has ShopInventory[20],
but actor load/save only implements normal inventory and equipment. The next
implementation must add actual shop container load/save and exact position/icon/
database-ID checks for expiry disposal. Expanded-slot hooks, source event-rate
gates and a real expiry audit sink also remain unbound; empty callbacks or the
default always-true event-rate hook must not be used to claim complete restore.

### 2026-09-13 — Shop inventory runtime, persistence and exact removal

PlayerState now owns the source 20-slot ShopInventorySlots. Existing
modern_player_item container 0/1 semantics remain inventory/equipment; container
2 now stores shop inventory, with zero-based DB slots mapped to absolute source
positions 390..409. No schema change was made. GameIn copies these cells into
the existing ItemTotalInfo.ShopInventory wire array. Quest reward snapshots
retain both equipment and shop cells instead of rebuilding inventory alone.

Item saving now snapshots and writes all three containers in the existing caller
transaction. An overload accepts unpublished inventory/equipment/shop candidates
for the forthcoming admission transaction. Malformed result shapes/types and
unsupported containers/slots reject strict entry rather than silently omitting
rows which the next DELETE-and-reinsert save would destroy. Shop fields are
range-checked before narrowing. The loader's next-ID update saturates rather
than looping on DWORD_MAX; global item-allocation exhaustion remains a separate
unverified boundary and is not claimed resolved here.

Added exact shop removal on Player: active lifecycle, open absolute position,
nonzero database ID/icon, and matching stored position/ID/icon are mandatory.
The returned item retains the original fields for future expiry audit; wrong
identity, unavailable slots, repeat removal and logout state do not remove data.
This method is not yet invoked by GameIn shop-restoration effects.

Validation: handler/CHINA Map targets built, all 83 MapHandler CTests passed
(2.96s), including real SQLite disconnect/fresh-handler relogin with shop slots
0 and 19, DWORD parameter/durability preservation, invalid-slot rejection with
unchanged DB rows, and injected shop INSERT failure after the initial DELETE.
The failed exit sends GameOutNack, retains the actor and rolls back all four
ordinary/equipment/shop rows; removing the injected trigger allows normal exit.
Log: `modern/out/unity-remaster/shop-inventory-handler-ctest.log`.
Player/state targets then built and eight scoped Player tests passed (0.66s),
including exact expiry-removal guards; log:
`modern/out/unity-remaster/shop-inventory-player-ctest.log`.

Standalone Player three-server SQLite/HSEL movement, collision and default shop
option smoke passed: `13e21aa71bf54fceace44a0431a549fd`. This run preceded the
new unused exact-removal method and contained no shop-inventory fixture; actual
shop row persistence is covered by the handler tests, not this Player run.
No full regression, MSSQL shop round-trip, visual acceptance or commit for these
changes yet. Full used-item admission restore, expansion/rate hooks and expiry
audit remain to be connected.

### 2026-09-13 — Source locale gates for shop expansion

Corrected CalcShopItemOption's expansion dispatch against source
ShopItemManager.cpp:1339-1399. China/Korea do not invoke these callbacks;
Japan/Thailand recognize the four base indices, and Hong Kong additionally
recognizes their four variants. ShopLocale is explicit in the calculator
environment and shared with ShopRestoreLocale. The default is China, rather
than silently combining all regional branches. Existing all-index test fixtures
now explicitly select Hong Kong.

The new matrix checks all five locales, eight indices and add/remove paths
(80 combinations), including source behavior that these callbacks do not depend
on the add flag and do not mutate ShopItemOption. Shop target built and all 83
calculator/runtime/restoration CTests passed (2.66s); evidence:
`modern/out/unity-remaster/shop-locale-ctest.log`. No new Player run/full regression
or commit was performed for this calculator change. Player expansion state and
slot enforcement still need actual binding.

A Sol read-only audit located source event-rate semantics: MeleeAttackMin selects
one of 12 current/baseline float entries; the three plustime additions require
exact current[id]==baseline[id]. ServerSystem.cpp:2103-2189 initializes both to
1, loads eleven named rates from DropRate.bin into baseline, then copies baseline
to current. Existing modern live_events exp/drop multipliers cannot substitute.
Existing extra_slot_count.hpp and InventoryItemSlot::set_extra_slot_count are
available slot arithmetic implementations but are not bound to PlayerState.

Live inspection found the canonical DropRate.bin is a 212-byte size-prefixed
container, SHA-256 d6f6199ed4de4b2fd2c1b7da2c4bd327240d6842126fc86ae93724b5e7fe0570.
The old generic resource test only checks decoding status/size, not named rate
tokens. The exploratory cross-file Monster_10 XOR transform did not produce
valid DropRate text, so its use is not authorized as a rate loader. Correct
profile-aware content recovery and token validation remain the next resource
step; do not install a default-true environment as if this file were verified.

### 2026-09-13 — Recovered DropRate content and production startup snapshot

Recovered the audited canonical DropRate.bin body at byte 24 using its own
eight-byte repeating XOR transform. Crib discovery used source label strings;
the accepted result contains all eleven complete labels and float values with
only whitespace separators and no undecoded suffix. The body is:
`#EXP 6000.0 #ABIL 2000.0 #ITEM 10.0 #MONEY 1.0 #GETMONEY 50.0
#DAMAGERECIVE 1.0 #DAMAGERATE 1.0 #NAERYUKSPEND 1.0 #UNGISPEED 6.0
#PARTYEXP 1.2 #MUGONGEXPRATE 150.0`.
The source SHA-256 remains the d6f6199e... hash recorded above; source bytes were
not modified. This supersedes the previous incomplete content-recovery status.

Added shop_event_rates.hpp with a bounded playdh-current decoder, explicit
container-marker/length checks and strict unique/all-eleven token parsing.
Non-finite floats, partial numbers, unknown/duplicate/missing labels and malformed
containers are rejected. This decoder is specifically the audited DropRate
variant, not the AIGroup transform or a generic legacy resource decoder.

ShopRateEnvironment stores baseline/current arrays by value with the source
0..11 index ordering, selected locale and exact float-equality gate. Startup
current equals baseline. with_current returns a new validated snapshot, so a
player's existing admission snapshot cannot change midway through restoration.
Bounds checks reject invalid indices. MapServer now requires the real DropRate
file at startup and stores the environment; five build-locale branches select
the matching ShopLocale. Runtime activity controls and recalculation of already
online players are not yet wired, nor is this snapshot yet used by GameIn restore.

Shop tests and CHINA Map built. All 85 event-rate/calculator/restoration CTests
passed (2.91s), including every real decoded rate, changed-current plustime
suppression through the actual calculator, snapshot independence, malformed
files/text and out-of-bounds rate IDs. Evidence:
`modern/out/unity-remaster/shop-event-rates-ctest.log`.
Real three-server SQLite/HSEL standalone Player movement/collision/default shop
smoke passed, run `513672ce721045be8d8e7998bbae5f68`, proving startup accepts
the canonical resource. No full regression, regional runtime, MSSQL shop or
visual/human acceptance is claimed; changes remain uncommitted.

### 2026-09-13 — Transactional GameIn shop restoration

GameIn now restores the owned MXSH use rows into the unpublished PlayerRuntime,
using the real startup rate snapshot, duplicate catalog and source-ordered restore
helper. Inventory is reread inside the transaction. Expired shop-inventory items
are removed by exact position/database ID/icon; item rows and the updated MXSH
record commit together. Operational expiration logs are buffered until commit.
An expansion callback during restoration rejects the candidate instead of replaying
activation-time expansion. This supersedes the earlier statement that GameIn does
not use the rate environment.

Read-only shop_admission_audit found BEGIN failure bypassing cleanup and a later
equipment read that could reject entry after commit. BEGIN uncertainty now drains
and disconnects; equipment is read before restoration, with an explicit presence
mask preserving zero-icon overrides and existing precedence. A too-broad first
patch briefly changed exit BEGIN handling; regression caught it, and that unrelated
change was reverted before the successful run.

Handler/CHINA Map builds passed. All 85 MapHandler CTests passed (2.96s), including
real SQLite successful restoration/wire options/expiry, injected MXSH write rollback,
equipment-read failure preserving both tables, and uncertain BEGIN draining.
The initial new fixture used charm Life (inner damage) as a life-cap input; corrected
it to source Plus_MugongIdx without changing the calculator. Evidence:
`modern/out/unity-remaster/shop-admission-ctest.log`.
Standalone Player real-three-server SQLite/HSEL movement/collision smoke passed,
run `fbc172d7c114405d9a9b7abc332906d8`. That smoke uses the default shop state;
nonempty restored options are covered by the handler test, not Player visual proof.

This does not complete shop behavior: ongoing timers/logout, full stat/combat binding,
source CheckAvatarEquip/DiscardAvatarItem appearance composition and recalculation,
runtime event changes, MSSQL and actual visible paid-item acceptance remain open.
Current MXSH persists skin IDs and use rows, not a complete worn Avatar array;
preserved skin IDs alone do not establish valid equipment or expiry cleanup.
Full regression for this batch is pending below; no release or completion claim.

Full modern Debug build subsequently passed all 297 steps, including the five
regional server variants. Full CTest registered 12,624 tests: 12,615 passed,
nine skipped, zero failed in 122.54 seconds. The skips remain the release human
smoke, deployment manifest and seven opt-in MSSQL cases. Evidence:
`modern/out/unity-remaster/shop-admission-full-ctest.log`.
The read-only admission reviewer rechecked both transaction fixes and confirmed
no new blocking defect within that review scope. Changes remain uncommitted;
this regression does not close the appearance/gameplay/acceptance gaps above.

### 2026-09-13 — Discard avatar weapon-boundary correction

Before wiring expiry appearance effects, source comparison found that the older
discard_avatar_item helper still default-filled [12,18), overwriting sword slot
17. Source ShopItemManager.cpp:2203 excludes eAvatar_Weared_Gum; CommonGameDefine.h
defines it as 17. The helper now derives both bounds from AvatarSlot and fills
only 12..16. Corrected tests that repeated the old six-slot assumption; explicit
sword-hidden/fist-equipped assertions verify weapon identities are preserved.
The invalid-position test now checks the actual first invalid index 23.

Compilation succeeded but normal test linking twice failed with LNK1318 PDB errors.
A scratch CMake-build wrapper appended /DEBUG:NONE /INCREMENTAL:NO for this test
executable only and linked successfully; this is not a verified normal-debug link.
All 33 scoped discard/put-on/take-off CTests passed in 1.33s; evidence:
`modern/out/unity-remaster/discard-avatar-boundary-ctest.log`.
No source resources or original code changed. This fixes the reusable helper;
production avatar catalog restoration, expiry effects and visual verification
remain unfinished. No new full regression or commit is claimed for this change.

### 2026-09-13 — AvatarEquip source decoder and duplicate conflict

Added bounded AvatarEquip MHFile decoding with both checksum checks and the source
26-field rows (id/gender/position/23 masks). The canonical 26,751-byte file hashes
to 32413ec55572bd58cb687ec6a5089ca7934107e06aa7112a20a176e3cb757605.
Its 469 rows contain 462 unique IDs. Six duplicate rows are identical; ID 57680
conflicts between hat position 0 and dress position 6, with different masks.
Identical duplicates coalesce; conflicting rows report the ID and prevent catalog
publication. The loader is not installed as a production startup requirement yet.
Original GameResourceManager.cpp:2327-2384 inserts into CYHHashTable, but the local
source tree lacks that container implementation; its duplicate selection semantics
still need trusted source or runtime evidence before selecting either 57680 row.

The first real-resource test exposed this conflict instead of confirming an assumed
unique catalog. Updated coverage asserts that exact real conflict and separately
checks valid/identical rows, checksum corruption, truncation and field bounds.
The no-PDB test build passed; 13 scoped tests passed in 0.88s, evidence
`modern/out/unity-remaster/avatar-catalog-ctest.log`. No full regression, production
catalog admission or completed external appearance restoration is claimed.

### 2026-09-13 — Resolved AvatarEquip duplicate semantics and startup loading

Expanded the source search to the existing read-only reference tree and found
`reference/legacy-source/4dddd9a6/[Lib]YHLibrary/hashtable.h`. Add:115-135 prepends
to a bucket; GetData:138-151 returns its first matching key. Therefore the last
file row wins for ID lookup. The decoder now reproduces that lookup behavior and
retains all seven duplicate IDs in file order for audit. ID 57680 resolves to
dress position 6 with the later masks. This supersedes the unresolved conflict
above; original bytes are unchanged. This catalog supports keyed lookup, not the
legacy hash table's duplicate-preserving iteration API.

MapServer startup now requires AvatarEquip.bin and publishes its complete catalog
only after successful parsing. Normal Debug shop-test/CHINA Map linking succeeded
this time without the no-PDB override. Thirteen scoped tests passed (0.93s), log
`modern/out/unity-remaster/avatar-catalog-ctest.log`; real standalone Player
three-server SQLite/HSEL movement/collision smoke passed with startup loading,
run `1180891d151a4f67955fbff7f6819c2c`.
Catalog availability does not yet restore worn avatar parameters or apply expiry
appearance effects. Those runtime bindings remain the next step; no full regression
or visual acceptance is claimed for this batch.

### 2026-09-13 — Concrete avatar transition lookup environment

MapDBMsgParser.cpp:6679-6688 restores each use row and immediately attempts
PutOnAvatarItem with calc-stats false when its original parameter is 10. A later
all-rows pass would change which using rows are visible during earlier transitions.
Added AvatarEquipEnvironment to snapshot the real ItemManager, ShopItemManager
and absolute inventory/equipment/shop positions for one transition, referencing the
loaded avatar catalog. Duplicate occupied positions fail rather than choosing one.
Recreate it after each applied transition; it is not a cache for an entire login.

Normal Debug test target built. Fourteen scoped tests passed (0.95s), including an
actual-manager use-row-before/after check, slot 390 dress restoration, database-ID
mismatch rejection and preservation of the earlier use snapshot. Evidence:
`modern/out/unity-remaster/avatar-environment-ctest.log`.
The environment is not yet invoked by MapHandler: ordered parameter persistence,
avatar-option recalculation and final publication still require implementation.
No full regression or visual acceptance is claimed for this helper-only change.

### 2026-09-13 — Ordered worn-avatar admission binding

GameIn now attempts put_on_avatar_item immediately after each restored use row
whose original parameter is 10. It uses the just-restored manager and actual
inventory snapshot, player-inited false and calc-stats false, matching the source
admission call. Rejected transitions retain the use row and do not fabricate a
worn appearance. Successful transition effects update pending MXSH parameters and
in-memory use parameters in order, then assign Avatar and recompute the retained
PlayerState.avatar_options. All database effects remain within admission's existing
transaction, before player publication. Full combat/resource-cap consumption of
avatar_options remains unbound and is not claimed here.

Normal compilation hit C1090 PDB API failure. The scoped build succeeded using
/Z7 and /DEBUG:NONE /INCREMENTAL:NO via a scratch wrapper. After that build fully
completed, all 85 MapHandler tests passed in 2.78s. The SQLite test now includes
real AvatarEquip dress 63130 with a controlled ItemInfo fixture, verifies worn slot
6 and mask flags 15/16 in GameInAck, retained inventory, and rollback on injected
MXSH write failure. Evidence: `modern/out/unity-remaster/avatar-admission-ctest.log`.
Standalone Player default-shop three-server SQLite/HSEL movement/collision smoke
passed, run `de5a644472484e928093393d35b44cca`; it does not prove dressed rendering.
Replacement-effect combinations, expiry appearance effects, all stat consumers,
MSSQL, full regression and actual visual acceptance remain open.

### 2026-09-13 — Avatar resource-cap consumption

PlayerState::apply_avatar_options now replaces avatar_life/avatar_shield/
avatar_naeryuk contributions before recomputing maxima. GameIn's successful
worn-avatar transition calls it. Source CharacterCalcManager.cpp:172/220/264
adds precisely these three avatar fields. The stored full option record remains
available for the other consumers, which are still incomplete.

A state test verifies all three cap deltas, repeated application without stacking,
removal, unchanged current vitals, and preservation of shop/equipment bonuses.
The scoped embedded-debug/no-link-PDB build passed. All 86 avatar-cap/MapHandler
tests passed in 2.82s: `modern/out/unity-remaster/avatar-caps-ctest.log`.
Default-shop standalone Player three-server SQLite/HSEL movement/collision smoke
passed, run `f11676509b3b48fabb6c9f127a6d37e7`. This is not proof of live dressed
combat or HUD maxima; authoritative combat/serialized-stat consistency, other
avatar fields, expiration, MSSQL and visual acceptance still need work.

### 2026-09-13 — Admitted vitals shared by self and combat snapshots

GameInAck now serializes current/max HP, shield and MP from the restored actor's
PlayerVitals instead of CharData/fixed 50 MP. Admission copies that same HP/MP
snapshot to PlayerInfo.combat before publication, so CharacterAdd also reports the
actor's current/max HP. This resolves snapshot disagreement; it does not validate
the still-incomplete source base-attribute initialization (currently level terms
plus bootstrap bonuses, with the default actor yielding 105 HP versus old wire 100).

The SQLite worn-avatar test compares actual runtime HP/MP with wire offsets and
asserts the independent 155 HP cap increment (100 charm + 55 dress) without healing.
The observer test now compares with the admitted actor instead of fixed 100.
The embedded-debug/no-link-PDB target build and all 85 MapHandler tests passed
(2.80s), evidence `modern/out/unity-remaster/admitted-vitals-ctest.log`.
Standalone Player default-shop three-server SQLite/HSEL movement/collision smoke
passed, run `29dd24e4d575477fb9152abaa1dec1af`.
Shield observer serialization, remaining base/stat fields, live combat effects,
expiry, full regression and visual acceptance remain incomplete.

### 2026-09-13 — Observer shield snapshot

PlayerInfo now retains current/max shield from admission and refreshes it alongside
HP/MP during equipment moves and quest rewards. CharacterAdd writes both DWORDs
at offsets 43/47 instead of leaving zero-filled fields. The observer test compares
these values to the self GameInAck and verifies a nonzero fixture shield cap.
This does not add shield damage resolution to the simplified combat model.

Scoped embedded-debug/no-link-PDB build passed. All 85 MapHandler tests passed in
2.77s, log `modern/out/unity-remaster/observer-shield-ctest.log`. Standalone Player
three-server SQLite/HSEL movement/collision smoke passed, run
`998f09fcc6c749e5976574834fadaaf7`. Live shield combat, source base stats, expiry,
full regression, MSSQL and actual visual acceptance remain open.

### 2026-09-13 — Replacement persistence and relogin evidence

Expanded the real SQLite admission fixture with two source-catalog dress entries:
57680 is restored first, then 63130 replaces it. Controlled ItemInfo bonuses differ
(77 and 55), so the existing +155 cap assertion proves the replaced dress bonus
does not stack. The earlier row persists Param=SellPrice(1), the later Param=10;
both inventory items remain. Injected write failure preserves both original worn
parameters alongside the expired row/inventory, proving rollback of replacement
and expiration together. A successful GameOut followed by fresh runtime admission
on the same handler retains the final avatar, cap and parameters.

The embedded-debug test build passed; the expanded three-scenario SQLite test
passed (0.61s CTest elapsed), log `modern/out/unity-remaster/avatar-replacement-ctest.log`.
This turn changed test coverage only, not production behavior. It does not prove
cross-process/MSSQL persistence, online expiry or real rendered clothing.

### 2026-09-13 — Shop playtime exit persistence

Complete exit transactions now include persist_player_shop_exit. It snapshots
the live use rows, loads the owned MXSH record and applies the source
UpdateLogoutToDB:1195-1237 rule: SellPrice=2, unsigned elapsed capped at 30,000ms,
remaining time saturating at zero, and personal event-charm pause conditions.
It changes only pending database rows before commit; failed saves leave the live
manager untouched, so retry cannot double-decrement the same elapsed interval.

The SQLite fixture now uses a 60,000ms playtime charm, advances an injected clock
45 seconds, injects an exit MXSH failure and verifies the persisted remainder is
still 60,000. Retry and relogin yield 30,000, proving the source cap and rollback.
All 85 MapHandler tests passed (3.00s) after the embedded-debug/no-link-PDB build;
log `modern/out/unity-remaster/shop-logout-ctest.log`.
The 30-second rule is only the source final exit slice: periodic online CheckEndTime
and 10-minute flush are still unbound, so this does not yet account for an entire
long session. Runtime event changes, online expiry/removal, MSSQL and full
regression remain required. No new Player smoke or visual acceptance this turn.

### 2026-09-13 — Shop timer cycle fixes before live integration

Source CheckEndTime:905-915 increments both clocks even when the ten-minute flush
fires, forces a sweep on that flush, and resets the check clock at line 1138.
The old helper returned before incrementing the check clock, ignored forced flush,
and never reset the sweep window. Corrected all three. Also corrected the returned
count: collect_expired replaces the output vector, so subtracting its previous size
could underflow. A regression with a prefilled three-item buffer and a one-item
forced sweep now returns one and proves the next short tick does not repeat it.

Normal Debug shop-test build passed; 13 timer/expiry helper tests passed (0.92s),
log `modern/out/unity-remaster/shop-timer-cycle-ctest.log`.
These helpers are not a production CheckEndTime implementation: generic expiry
collection lacks catalog-based realtime/playtime discrimination and event pauses.
Live integration still needs those branches and transactional removal/options/
notifications. No online-duration completion, full regression or Player run claimed.

### 2026-09-13 — Source playtime candidate calculation

Added plan_shop_playtime_step matching CheckEndTime:1063-1117: catalog SellPrice
selects playtime, personal event charms pause by updating LastCheckTime, exhausted
personal charms skip, DWORD elapsed/subtraction retains the source signed checksum,
and the candidate reports expiry, ten-minute persistence and crossing-one-minute
notification independently. The input use record is never mutated. A caller must
complete removal, duplicate counters, DB writes and messages before publication;
the helper is not yet called by live MapHandler ticks.

Normal Debug test build passed. Fourteen playtime/timer helper tests passed (1.01s),
log `modern/out/unity-remaster/shop-playtime-step-ctest.log`, covering countdown,
pause, unsigned clock wrap, expiry, minute threshold, zero personal remainder and
realtime exclusion despite Param=10. Live integration and full acceptance remain
unfinished; no Player/full-regression run for this helper-only change.

### 2026-09-13 — Correct shop expiration notification protocol constants

Live-integration review found check_end_time_side_effect.hpp labelled a zero
placeholder as the original USEEND protocol. Original Protocol.h:968/971 enum
evaluation gives USEEND=106 and ONEMINUTE=108, cross-checked with CHASE_SYN=154.
Added these named modern ItemProtocol entries and bound the legacy alias to 106;
original protocol headers remain unchanged. The local source enumeration probe is
`modern/scratch/2026-09-12-unity-remaster/inspect-shop-protocol.py`.

Normal Debug shop-test build passed, as did 13 scoped side-effect tests (1.00s),
log `modern/out/unity-remaster/shop-expiry-protocol-ctest.log`. This fixes protocol
identity only: no new live expiry messages are sent yet, and Unity handling,
transactional timer application and full regression are still required.

### 2026-09-13 — Shop expiration notification reaches Unity

The x64 core now accepts the original four-byte `MSG_DWORD` payload for Item
protocol 106/108 only for the admitted player. It rejects a wrong player, malformed
length, zero identity or an icon outside the source WORD range. Successful packets
become distinct UseEnd and OneMinute C ABI events. The existing event publisher
stamps them with the current session generation, map generation and monotonic
sequence; ConnectionPanel and ShopItemNoticeState reject stale generations and
replays before displaying the Chinese player-facing status.

The native RelWithDebInfo build passed 94/94 tests, including a real HSEL socket
fixture that sends both original protocol bytes after GameIn. The scoped evidence
is `modern/out/unity-remaster/shop-notice-native-ctest.log`. The Unity Editor full
EditMode run reports 88 total, 84 passed, zero failed and four environment-gated
real-server skips; its report is
`modern/out/unity-remaster/shop-notice-unity-editmode.xml`. The Player rebuilt
successfully with native core SHA-256
`62A7FF5F17E0A0AD9F5AACEDDB4E98533A6DDD03C0C7A76553BD24B1A8594A1F`.
Standalone Player run `7514cb88d4c04de4a4902fde7a38ff18` then passed real
SQLite/Login/Agent/Map HSEL GameIn, two-native-session movement and Map10 collision.

This closes the client notification bridge only. The live MapHandler still does
not run the periodic transactional shop timer, so a real server does not yet emit
these notifications or remove expired effects during an online session. The smoke
therefore verifies regression safety, not a naturally elapsed shop item.

### 2026-09-13 — Live transactional shop timer in MapServer

MapServer now advances admitted shop items from its production main loop. The
handler preserves the source 30-second and ten-minute latches, distinguishes
playtime from packed-calendar expiry, pauses personal event items, persists the
ten-minute playtime checkpoint and emits the original one-minute/use-end protocol
identities. Expiry removes duplicate parameters and calculated shop options from a
candidate actor before any live publication.

Every mutation follows candidate, database transaction, commit, runtime publish
and notification order. The transaction writes inventory and the encoded shop
state together. It compares the current database blob with the exact state loaded
at GameIn and then uses the existing exact compare-and-swap write, so an external
update cannot be overwritten by an online timer. Begin, write, commit, baseline or
ownership uncertainty makes the handler drain and publishes no candidate state or
success notification. Cosmetic physical expiry resolves its real shop-inventory
slot by database identity, then sends DiscardAck, authoritative TotalInfoLocal and
ShopItemUseEnd after commit.

The x86 server targets built successfully. Four focused online timer tests and the
full MapHandler suite (89/89) passed, including rollback, post-admission database
drift, playtime checkpoint, stored-time warning and stale-position relocation. The
x64 Unity core remained green at 94/94. Standalone Player run
`0f58ee11215242e4b27efef3ded74da1` passed real SQLite/Login/Agent/Map HSEL GameIn,
two-native-session movement and Map10 collision with the production timer loop.

This does not complete shop parity. Pet-worn, Titan-shop and other original item
containers are not represented by the current PlayerState and therefore are not
searched during physical expiry. TotalInfoLocal refreshes Unity inventory but the
current wire bridge does not carry a complete post-expiry ShopItemOption/avatar
appearance snapshot, so some visual effects can remain until a later authoritative
refresh or reconnect. The Player smoke is automated (`humanAcceptance=false`);
fresh visible GUI/E5 acceptance remains required.

### 2026-09-13 — Authoritative post-expiry shop appearance refresh

Closed the post-expiry Unity appearance gap above with modern Server protocol 198;
using Category::Server avoids collision with the original Item value 198.
MapServer sends the explicitly encoded 120-byte legacy SHOPITEMOPTION block after
the committed TotalInfoLocal inventory refresh and before ShopItemUseEnd. The x64
core accepts it only in-game, for the admitted player and with the exact legacy
size, then republishes the existing generation-stamped ShopAppearance event. Unity
therefore replaces avatar and skin presentation immediately without reconnecting.
The original protocol header and legacy item protocol values remain unchanged.

Both server targets rebuilt successfully. The focused timer suite passed 4/4 and
the full MapHandler suite passed 89/89. The x64 core passed 94/94, including a real
HSEL socket fixture that observes a distinct post-GameIn appearance refresh. A
Development Player build succeeded with the refreshed 695,296-byte native plugin;
the normal candidate build remained correctly blocked by RemasterBuildGuard.
Real three-server Player run `85792bb2612c49529b51fecc98be1e96` passed HSEL
GameIn, two-native-session movement and Map10 collision. Unity EditMode shop state
checks passed 2/2 for appearance and 4/4 for expiry notices.

The first live-timer implementation covered only ShopInventory. The following
section supersedes that container limitation. Automated Player evidence still
reports `humanAcceptance=false`.

### 2026-09-13 — Original physical shop-container parity

The item persistence and wire bridge now carry all six fields already present in
the fixed 2,728-byte `ItemTotalInfo`: Inventory, Weared, ShopInventory,
PetWearedItem, TitanWearedItem and TitanShopItem. Database container ids 0 through
5 map to the original absolute positions 0-79, 80-89, 390-409, 490-492,
493-499 and 500-503. GameIn, exit persistence, timer persistence and authoritative
item refreshes all use one complete snapshot builder. Invalid extended slots are
rejected before runtime publication and their database rows are retained.

Type-11 expiry now follows the original physical search boundary and order:
ShopInventory, Weared, PetWeared and TitanShopItem. The shared removal primitive
relocates by database id and icon, returns the actual position for DiscardAck,
preserves the empty slot's absolute Position and rejects duplicate ids or icon
mismatches. It is used by both online timer processing and GameIn restoration, so
an item that expired while worn no longer makes admission fail. DiscardAck remains
limited to makeup, decoration, pet equipment and Titan equipment; ordinary shop
equipment can expire without fabricating that acknowledgement. Avatar removal is
also limited to makeup and decoration instead of applying to every catalog entry.

The x86 MapServer and focused test targets built successfully. The complete
MapHandler suite passed 92/92, Player-related tests passed 49/49 and the focused
cross-container set passed 6/6. The x64 Unity core remained green at 94/94, and
the Player plugin SHA-256 remained
`F305C5A0E5EBFC0223687241C5964FC359415BA60D596D20586A288A685EE55F`.
Standalone Player run `c168e4206cb848918296ab7fa68417d5` passed real modern
SQLite Login/Agent/Map HSEL GameIn, two-native-session movement and Map10
collision. It is automated runtime evidence and still records
`humanAcceptance=false`.

Two release boundaries remained open at this point. The next section closes skin
kinds 265/266. The current table still accepts the new container values without a
schema revision; an old MapServer would delete container 2-5 rows during its full
rewrite. Production rollout must therefore add an exact writer/schema feature gate
and use a stop-the-world upgrade with a database backup, or introduce a side table
for rolling compatibility. SQLite is covered here; the required real MSSQL round
trip and rollback proof remains pending.

### 2026-09-13 — Original skin-table expiry and observer publication

MapServer now requires and parses the real `SkinSelectItemList.bin` and
`CostumeSkinItemList.bin` resources before production startup. The loader verifies
the MHFile payload size and checksum, the normal six-field and costume four-field
row shapes, numeric bounds and duplicate row ids. It keeps the two tables separate,
then applies the original category-wide removal rule: kind 265 walks every normal
row and kind 266 walks every costume row, clearing matching values from the five
Hat/Mask/Dress/Shoulder/Shoes skin slots.

GameIn restoration removes an expired physical skin item and its used-item row,
persists the cleared skin array in the same transaction and exposes that state in
the following GameInAck. It sends no ShopItemUseEnd packet on admission. Online
expiry commits the physical item, used row and skin state together before
publishing one ItemExt/SkinItemDiscardAck packet: category 73, protocol 20,
player object id and exactly five little-endian WORD values. It then sends the
authoritative local inventory and modern appearance refresh before Item/106
ShopItemUseEnd. Skin expiry does not fabricate Item/DiscardAck.

The original source reaches DiscardSkinItem from two call sites after freeing the
used-item object, but no old-server packet capture proves that two acknowledgements
were observable. The modern lifetime-safe path therefore publishes one idempotent
state transition. To preserve the original QuickSend visibility rule, the skin ack
is delivered once to every active map connection, deduplicated for AgentServer
multiplexing; its object id identifies the affected player. The two-connection test
proves both the player and an observer receive the update. A database write failure
rolls back the physical item, used row and old skin values and publishes nothing.

The x86 MapServer and handler targets built successfully. The complete MapHandler
suite passed 94/94; the combined MapHandler, skin-transition and shop-layout set
passed 133/133. The x64 Unity core remained green at 94/94 and the staged Player
plugin SHA-256 remained
`F305C5A0E5EBFC0223687241C5964FC359415BA60D596D20586A288A685EE55F`.
Standalone Player run `d37ae7121dc34828b9618101eb782c4e` passed real modern
SQLite Login/Agent/Map HSEL GameIn, two-native-session movement and Map10
collision. This remains automated evidence with `humanAcceptance=false`.

The complete x86 CTest registry then ran in one invocation: 12,632 passed,
9 were conditionally skipped and 0 failed out of 12,641 registered tests in
486.59 seconds. The full log is
`modern/scratch/2026-09-13-unity-skin-expiry/full-ctest.log`. The skips are one
Release-only Map10 smoke, one deploy-manifest check and seven MSSQL-dependent
integration tests; they are recorded as missing environment evidence rather than
counted as acceptance.

The local default MSSQL instance and ODBC 18 path were then exercised through
Windows integrated authentication against the dedicated `mxh_test` database. Six
tests passed: shared-memory connection, modern login/character round trip, money
upsert, three-slot pool use, login-audit lockout, and a new shop-state test covering
all five skin values, used rows, stale-CAS rejection and explicit transaction
rollback. The first TCP attempt against `localhost:1433` timed out because that
endpoint is not listening; `(local)` with port zero is the verified local transport.
Synthetic rows are deleted by the tests. No credential was introduced or logged.

The three-server smoke runner now has a guarded `mssql_odbc` mode. It accepts the
connection only through a named environment variable, requires that the value
explicitly select `mxh_test`, never puts that value in its summary, and refuses to
start if fixed fixture ids 111/222 are occupied. Each run uses unique account and
character names. Normal completion and interpreter-exit cleanup delete only rows
owned by those identities; the test does not scan or truncate shared tables.

SQLite regression run `fde7672e5c6f49d48b7183d342b073b7` and MSSQL run
`9c869267570f4ae59fd50b1542a1bf06` both passed the real Unity Player plus modern
Login/Agent/Map executables, HSEL client transport, two native sessions, movement
broadcast and authoritative Map10 collision correction. All three servers were
still alive before owned-process cleanup. A post-run query found zero smoke
accounts and zero fixture characters in `mxh_test`. Missing or non-test MSSQL
configuration fails before migration or account creation. Both records remain
automated runtime evidence with `humanAcceptance=false`.

An empty-account MSSQL Player run then exposed a separate durability defect:
character creation inserted `character_info`, but each starter-equipment insert
omitted MSSQL's required `updated_at` column and discarded the write result.
AgentServer now creates the character and all starter equipment in one database
transaction, supplies `CURRENT_TIMESTAMP`, rolls back on any row or commit failure
and returns CharacterMakeNack instead of acknowledging partial state. Run
`c8bce28d28c04769854032f2f7314431` passed through the real Player and three
servers with character `UnityNewc8bce28d` and equipment `(1,11000)`, `(2,23000)`,
`(3,27000)`. Post-run counts for smoke accounts, characters and fixture equipment
were all zero. The complete x86 build passed, as did all 51 registered
AgentHandler and AgentHandlerHackShield tests.

The exact writer/schema deployment gate remains open and must be implemented as
separate operational work. No database DDL version was changed in this
skin-expiry slice; the MSSQL unit test only adds `character_data` when an old test
table lacks the already-required column.

### 2026-09-13 — Exact MapServer writer/database deployment contract

The deployment launcher now requires the selected MapServer to report the exact
`mxh-map-writer-v2:item-containers-0-5,shop-skin-5,used-items,starter-equipment-atomic`
contract before database migration or process launch. The shared DB tool probes
the complete item-container columns, the versioned `character_data` shop-state
blob and atomic starter-equipment columns after migration, then reports the same
contract. A missing table or column, an older binary without the command, or a
case-sensitive contract mismatch stops startup before any server is launched.

The deployment manifest records this contract and SHA-256 values for DBTool,
LoginServer, AgentServer and MapServer. Automated tests prove an unmigrated or
damaged SQLite schema fails closed and a migrated schema matches MapServer exactly.
The dedicated `mxh_test` MSSQL schema reported the same contract, after which the
production launcher started and stopped all three real server executables on
isolated ports under run id `mssql-writer-contract-20260913`. The complete x86
build also rebuilt every locale-specific MapServer with the contract.

This closes the exact writer/schema feature mismatch identified for containers
2-5 and skin state. A stop-the-world production backup receipt, rollback exercise
and current full resource-inventory reconciliation are still required before a
production rollout. The contract does not change schema version 1 and does not
claim release or human visual acceptance.

The first reconciliation pass explains the complete 241-file release-manifest
delta: 159 generated DDS files (65 loading images and 94 minimaps), 81 loose HFL
files (three recovered real files plus the documented synthetic placeholder set)
and one recovered Map10 STM, totaling 186,577,734 bytes. Every path, size and
SHA-256 is recorded in
`modern/scratch/2026-09-13-unity-skin-expiry/playdh-extras.json`. These files are
known derivatives previously written into canonical PlayDH, so they are not added
to the frozen original-resource manifest and production integrity continues to
fail closed. Relocating them into the `unity-remaster-v1` derivative overlay and
updating all consumers remains open; no source resource was moved or deleted in
this audit.

Release validation of the existing `unity/baselines/playdh-current.json.gz`
still fails with 107 explicit blockers. They comprise generated/unknown HFL
provenance, unresolved source selections and maps without a logical TTB source;
the validator does not convert any of them into release-ready assets. This means
the 241-file physical relocation is only one part of the resource gate, and the
remaining map evidence must be resolved from PAK/source records rather than by
promoting placeholders.

### 2026-09-13 — Map10 and Map2 authoritative terrain source binding

The unified `playdh-current-source-selection.json` now selects Map.pak entry 265
for Map10 HFL and entry 268 for Map10 STM. The loose 137,030-byte Map10 HFL is
content-proven as a generated 16x16 placeholder and is excluded; the selected
1,188,682-byte PAK HFL is 513x513 and matches the source ID and SHA-256 already
embedded in `Map10.mxhasset`. The loose Map10 STM and PAK entry are byte-identical,
so the PAK source is selected explicitly instead of relying on loose-file order.

The same rule promotes the existing Map2 evidence into the unified baseline:
Map.pak entry 583, SHA-256
`d49dd4b864f09e3880124d31633d1b0860175805a3273284988c2b421d9e31e2`,
is the selected 513x513 HFL, while the generated loose Map2 HFL is rejected.
Both HFL sources carry explicit verified-original attestations bound to their
container entry, exact bytes and Unity descriptors.

The regenerated compressed baseline retained inventory SHA-256
`c3f54ffad61641e7de0475178914ab5fabd76975487af6fb8d14851b79544e3a`.
Integrity validation passed, and all 5,372 loose files/PAK containers rehashed
without drift. Release blockers fell from 107 to 104 and unresolved selections
from 59 to 56. The new binding verifier fails if a descriptor source is absent,
unselected, hash-mismatched or an HFL lacks verified-original provenance. Six
Unity provenance CTests passed, including real Map2/Map10 bindings. Unity EditMode
terrain-import tests passed 3/3 for both maps' source identity, 513x513 grids,
coordinate bounds and upward winding.

Both `.mxhterrain` descriptors remain `releaseReady=false`: this source closure
does not yet prove final materials, static object layout, gameplay equivalence or
human visual acceptance. The remaining 104 blockers stay explicit.

### 2026-09-13 — canonical PlayDH cleanup and derivative overlays

The earlier 241-file paragraph is superseded by byte-level classification. All
81 loose HFL files are generator-produced development placeholders; none is an
original loose terrain. The 159 generated loading/minimap DDS files, 81 HFLs and
the byte-identical loose Map10 STM were moved out of canonical PlayDH into
`unity/source-overlays/unity-remaster-v1/development-placeholders/PlayDH`.
`overlay-manifest.json` records all 241 paths, sizes and SHA-256 values with
`releaseAllowed=false`; its inventory digest is
`a6af69a42c0d0352bc393d1a7839c58cc401b2c63e3ba8553c4ea4c23a9afe60`.

A second comparison found 74 frozen-manifest paths whose tracked bytes had been
replaced by generated or polished variants. Every frozen byte sequence existed
independently under both deployment resource roots and matched the frozen size
and SHA-256. The replacement variants were retained under
`unity/source-overlays/unity-remaster-v1/baseline-replacements/PlayDH`; its
manifest records the replacement hash and the exact restored source. Canonical
PlayDH was then restored from one verified deployment copy.

The canonical root is back to the frozen 5,131-file profile. The release-profile
check passes, the regenerated Unity audit has 19,500 physical sources and 19,495
logical assets, integrity validation passes, and all 5,131 source files rehash
without drift. Its inventory SHA-256 is
`9f4c4d23060e1374883ac13c2ac4470f6565590f7606f7aa3efe69f8096ac762`.
Explicit selections resolve every duplicate. The remaining 46 release blockers
are honest content gaps: 28 maps lack an original HFL source and 18 maps lack a
logical TTB source. No placeholder or replacement variant was promoted to a
release-ready original.

The pack-first filesystem policy and focused native/resource regression passed
14/14 tests; the overlay/source-selection Python tests passed 7/7. This closes
canonical contamination and provenance ambiguity, but it does not close the 46
missing-map blockers or visual/player acceptance.

### 2026-09-13 — shifted-count Map.pak recovery source

Read-only follow-up found a distinct 1,135,536,849-byte Map.pak at
`D:/MX/client/Package/3DData/Map.pak`, SHA-256
`00fab5a3ebae755cda6a500f254d60b21c4d64fb0664744a6de6ad20091f68a3`.
It is a valid writer variant rather than an empty archive: the standard count at
offset 4 is zero, the 11,267-entry count is at offset 92, and entries begin at
offset 96. A strict sequential walk parsed all 11,267 entries and ended exactly
at the container size with no truncation or invalid header.

The archive contains four HFLs currently missing from the canonical audit:
Map3 (`b9ae0e18...`, 1,189,592 bytes), Map103 (`60333632...`, 1,184,392),
Map198 (`0087ca63...`, 1,184,002), and Map203 (`4315f3be...`, 3,685,568).
They were extracted without changing the container into
`unity-remaster-v1/recovered-originals`, with entry indices, offsets, sizes,
hashes and container provenance recorded. The audit now accepts explicitly
named additional roots and records `overlay:<label>` source types; source-byte
verification fails if a required root is omitted. All four entries have
verified-original attestations bound to the container SHA-256 and entry index.
Map3, Map103 and Map198 are byte-identical independent copies of already
selected canonical PAK entries. Map203 is a new logical source and closes that
map's missing-HFL blocker. The composite baseline therefore has 19,504 physical
sources, 19,496 logical assets, eight resolved duplicates and 45 release
blockers. All 5,135 canonical-plus-recovered source files rehash successfully.

`unpack_pak.py` now detects both 92-byte standard and 96-byte shifted-count
headers. Nine unit tests, including a synthetic shifted archive, pass; the real
archive dry-run parsed 11,267/11,267 and selected exactly four target HFLs.
The resource auditor's additional-root and fail-closed rehash behavior is
covered by 13 unit tests. Registered CTest `mxh_unity_composite_source_bytes`
rehashes the complete two-root profile and passes.

### 2026-09-13 — Unity item-use vertical slice

The x64 C ABI is now version `0x00010008` and exposes one ordinary inventory-use
command plus its correlated response event. Unity sends the exact legacy
`UseSyn` payload (one little-endian `uint16` inventory position). The native core
allows one pending use request, matches the echoed position, rejects malformed
or mismatched replies, fails a missing reply at the session timeout, and updates
the visible HP/MP snapshot only from the authoritative 20-byte `UseAck`.

MapServer now follows a successful `UseAck` with `TotalInfoLocal`, so Unity
replaces its 124-slot inventory from authoritative server state after the item is
consumed. The server test verifies that the refreshed slot is empty. The x64
core suite passed 94/94 tests, including real-socket Ack/Nack and request
correlation. The two focused MapHandler item-use tests passed, and Unity EditMode
passed 87/91 with zero failures; the four skips remain explicit real-server
fixtures. The trade panel now enables non-empty carried slots 0–79 and routes a
click to this server-authoritative command while keeping worn slots out of the
ordinary-use path. The staged DLL SHA-256 is
`d0e56f7a6e1d39271e1ae90f5021ab4aca16c9d89d6cc3b86f68796dffb16d5d`.

This closes the programmatic command/state path and the player input binding. A
real-server consumable item run remains required for Map10 slice acceptance.

### 2026-09-13 — Unity equipment move vertical slice

The x64 C ABI is now version `0x00010009`. Unity can submit an inventory/equipment
move using source and target positions in the original 0–89 position space. The
native core serializes the exact 22-byte authoritative `ItemBase` already held in
its snapshot, appends the two-byte target, allows one pending move, and matches
`MoveAck`/`MoveNack` by database identity and target before publishing a result.
Malformed, mismatched and timed-out responses fail the session rather than
mutating visible equipment from ambiguous data.

The trade panel now cycles through shop, consumable-use and equipment modes.
Equipment mode uses a two-click source/target flow and supports both inventory
to worn and worn to inventory moves. Only positions 0–89 participate. Equipment
kind, level and attribute legality remain exclusively authoritative in the
existing MapServer path; Unity does not reproduce or weaken those rules.

The rebuilt x64 suite passed 94/94 tests. Nine existing client/MapServer move
tests passed, covering authoritative item fields, appearance refresh, valid
equip, persistence, wrong-slot rejection, level rejection and invalid crossed
equipment swaps. Unity EditMode passed 88/92 with zero failures and four explicit
real-server skips. The current staged DLL supersedes the prior item-use-only DLL
and has SHA-256
`e3a3471cd50a9c7b78e21fc24e794b36b023cfa83e6d0b5aaa7ede5a8de7af57`.

An isolated real-three-server SQLite run (`0a7d52b068bc43608fc4b2f25d9ca736`)
then executed the native Unity client sequence: load worn weapon 11000, unequip
slot 81 to inventory slot 0, disconnect, reconnect and verify the unequipped
state, equip slot 0 back to slot 81, disconnect, reconnect and verify the worn
state. The targeted Unity test passed in 1.82 seconds and the final database row
is exactly `(container=1, slot=1, db_idx=9001, item_idx=11000)`. All three owned
server processes remained live until cleanup. The structured summary records
`equipment.persistencePassed=true`.

This is programmatic E4 evidence on SQLite. Visible human appearance review and
the corresponding MSSQL run remain required for full Map10 acceptance.

### 2026-09-13 — real three-server consumable persistence

The isolated smoke harness now has an `--item-use` fixture. It seeds item 1 from
the real `ItemList.bin` into Map10 inventory slot 0, enters through the real
Login/Agent/Map executables, submits the Unity native `UseSyn`, requires the
correlated success event and authoritative empty inventory refresh, disconnects,
then reconnects and requires the slot to remain empty.

The first diagnostic run proved the server transaction and exposed that the
smoke harness was still launching a MapServer executable linked before the new
post-use `TotalInfoLocal` change. After rebuilding the actual
`mxh_map_server_CHINA.exe`, run `ed4047d1e94c42468fef9695f995d470`
passed. The server applied the real item effect (`hp+=50`), the Unity test saw
the Ack and empty-slot refresh, the reconnect retained the empty inventory, and
the final `modern_player_item` query returned no rows for player 111. Its
structured summary reports `itemUse.persistencePassed=true` and all owned
servers remained live until cleanup.

This closes SQLite programmatic E4 for ordinary consumable use. Visible player
review and MSSQL remain open.

### 2026-09-13 — real two-client UTF-8 chat and sender echo

The Unity native core now emits a local `CHAT_MESSAGE` event only after the
extended-chat packet has been accepted by the client transport. The event keeps
the submitted request id, local player id, exact UTF-8 text and `ChatProtocol::All`.
This restores the original client-visible sender echo while preserving the
AgentServer rule that broadcasts the server-routed packet only to the other
players on the same map. Failed sends do not produce an echo.

Isolated SQLite/HSEL run `3aa3df3d379049ea91c3fe2ac71c7e2d` entered two
independent native sessions on Map10 and submitted the exact text
`墨香 Map10 双人聊天`. The sender received the correlated local event and the
observer received an uncorrelated server-routed event with byte-equivalent text.
The MapServer log records `Chat(All) from player=72111` followed by
`Chat broadcast players=2 target_conns=1`. The targeted Unity test
`RealTwoClientsSeeExactUtf8ChatAndSenderLocalEcho` passed in 0.852 seconds; the
whole fixture recorded 84 passed, 0 failed and 6 intentionally skipped tests.
All three server processes remained live through cleanup.

The x64 native suite remains 94/94 passing. The staged plugin SHA-256 is
`A9311C8FE117AE59DFAA4AE3EEA573880FC1D37C81A578E8852BF1CB9110B0D7`.
This closes programmatic two-client Map10 world-chat delivery and sender echo on
SQLite. A second isolated run, `ffcd421090f54914b6798ded1771830f`, passed the
same two-native-session test against the dedicated local MSSQL `mxh_test`
database through ODBC and HSEL. The exact UTF-8 test passed in 0.892 seconds;
MapServer recorded player 111, two same-map players and one remote target. Both
temporary accounts and both characters were queried after teardown and returned
zero rows, proving cleanup completed. The three servers were live immediately
before owned-process cleanup.

Programmatic Map10 world-chat delivery and sender echo are therefore closed on
both required database backends. Human IME/UI review and additional chat scopes
remain open.

### 2026-09-13 — MSSQL Unity commerce, inventory and map-transfer closure

The real Unity/MSSQL acceptance sweep found two backend-dialect defects in
MapServer persistence. `persist_player_money` and
`persist_player_position_locked` sent SQLite `strftime` and `ON CONFLICT`
syntax to SQL Server. The first failing sale run
`59184ebaf6f44fcab68539001686ab32` removed item 53343 but retained money 1000;
its Map log records SQL Server error 42000. The first failing transfer run
`2bc2b9c96af04ac8b6f0fd8323b482f5` retained Map10 position and character-map
rows. These failures are preserved as regression evidence rather than hidden.

Both helpers now select explicit SQL Server `MERGE ... WITH (HOLDLOCK)` queries
with UTC `SYSUTCDATETIME`, while retaining the existing SQLite queries and the
same fixed-width bound values. The rebuilt China MapServer passed the five
focused SQLite money/position tests and these isolated real MSSQL/HSEL runs:

- discard `e87952c93cf746d9924961979aed2ee6`: item 53343 absent after reconnect;
- equipment `d4d73964307c4ac5816a05d23174e7f5`: weapon 11000 returned to worn slot 81 after two reconnects;
- item use `8715225c9fef440198ddccca79cec288`: consumable item 1 absent after reconnect;
- sale `bc84e1f7ee1c4b7db4fcc63d9b8a6b6a`: item absent and money persisted 1000 to 1250 with no database or disconnect-save error;
- purchase `39040722bced4e6fbe0dab4d7d54fb85`: item 53343 persisted and money became 99,999,000 with the corrected position saver active;
- Map10 to Map2 transfer `3770699fa2a0420cb0b76f15e2572b17`: position `(2,7211,43329)`, character map 2 and money 4242 survived reconnect.

The same transfer passed a fresh SQLite regression under run
`c44c171b5d324de7ab6156d35a6bae63`. Every run reported all owned real server
processes live immediately before cleanup. This closes programmatic dual-db
persistence for these vertical slices. Visible human operation and production
deployment acceptance remain open.

### 2026-09-13 — atomic purchase and sale persistence

Purchase and sale now persist the complete carried inventory and money inside
one adapter transaction before publishing a success response. An item-write,
money-write or commit failure rolls the transaction back, restores the prior
runtime inventory and money, and publishes only BuyNack or SellNack. An
uncertain commit or rollback disconnects the database and marks MapServer as
draining instead of continuing with an unprovable state. The purchase rollback
also fixes partial insertion when a multi-item purchase runs out of slots.

The focused handler suite injects a `modern_player_state` write failure after
inventory serialization and proves sale returns one Nack, retains the item and
retains the prior money. The real SQLite purchase fixture was corrected to
contain the production item table; it now verifies all three purchased rows and
the money row in the same successful transaction. Four focused handler tests
pass. Fresh post-change real-server runs pass sale on SQLite
`7be7c3ecfee141af985706c907e6fe8e` and MSSQL
`05bd9c5043484819892fcbc1fc5f9539`, plus purchase on SQLite
`7712f0389719442b851605439f654330` and MSSQL
`f6b254e17d8849a5a369a3c44e49439a`. All four retained correct money and item
state across reconnect with the servers live until cleanup.

The complete x86 Debug build then succeeded for every configured target and
locale. The first 12,649-test sweep exposed one older price test that had never
connected its SQLite adapter or created an item table; its previous green result
depended on ignored persistence failures. The fixture now uses the production
money/item table shapes and verifies the item rows. After the correction, a
fresh complete sweep passed 12,649/12,649 with zero failures in 454.88 seconds;
the full log is
`modern/out/unity-remaster/commerce-atomicity-full-ctest.log`. Ten explicit
environment-gated tests remain skipped in the default run. The dedicated local
MSSQL entry subsequently enabled the relevant gates and passed 6/6, including
shared-memory ODBC, modern schema, purchase money, pooled slots, login lockout
and shop-state compare-and-swap rollback.

### 2026-09-13 — MSSQL quest and experience persistence dialect

A post-commerce SQL audit found that quest-log upserts and monster-kill
experience upserts still used SQLite-only `ON CONFLICT` syntax on every backend.
Both MapServer paths now select SQL Server `MERGE ... WITH (HOLDLOCK)` with UTC
timestamps while preserving their existing SQLite statements. The fixed-width
bound parameter layouts and gameplay calculations are unchanged.

The handler tests now expose the adapter backend and capture executed SQL. A
quest start and a monster death under the MSSQL dialect both prove that the
production path emits `MERGE` and contains no `ON CONFLICT`; five focused quest
and experience tests pass, including SQLite persistence and relogin restoration.
A new real ODBC test executes the exact production quest and experience MERGE
statements against dedicated `mxh_test`, reads back quest state 2, accepted time
123456, level 3 and experience 4567, then deletes its rows. The explicit MSSQL
suite now reports 7 passed and one intentional legacy-backup skip; writer
contract verification also passes. Real Unity quest completion and combat-level
progression remain separate gameplay acceptance gates.

### 2026-09-13 — real Unity quest acceptance on both databases

The isolated Unity harness now has a dedicated `--quest` fixture. It enters
Map10 through the real Login/Agent/Map executables, submits quest 1 with the
native `StartSyn` command, requires the server's `StartAck`, disconnects, then
reconnects and requires the authoritative `TotalInfo` snapshot to restore the
active state. The harness independently queries `modern_player_quest_log` and
accepts only the exact `(quest_id=1,state=1)` row.

SQLite run `9046d9b87b5a4438a1ed4f9d96c08e4f` and MSSQL ODBC run
`0f0e492ea62e4f3da57cd35542289891` both passed. Each Unity result contains 91
tests, 82 passed, zero failed and nine intentionally environment-gated skips;
`RealQuestAcceptancePersistsAcrossReconnect` passed in both runs. Both Map logs
record quest 1 `StartSyn` and `QUEST_START_ACK`, all owned servers remained live
until cleanup, and both structured summaries report
`quest.persistencePassed=true`. The MSSQL fixture also deletes quest/subquest
rows before removing its temporary character and account.

This closes programmatic quest acceptance and relogin restoration on both
required database backends. Quest completion/reward, visible HUD interaction,
and human acceptance remain open.

### 2026-09-13 — real Unity quest completion/reward and authoritative acceptance

Quest reward delivery is now one authoritative transaction. MapServer snapshots
the actor, quest log and connected-player state, applies every final-subquest
item reward in script order, then commits quest rows, item rows and player
progress before publishing EndAck. A write failure restores runtime state and
returns EndNack; an uncertain commit or rollback drains the server. Focused
failure injection proves that a failed player-state write leaves the quest
Complete, restores money and inventory, rolls back once and permits a clean
retry. The runtime adapter also corrected canonical script semantics: TALKTONPC
and USEITEM are one-shot events whose second value is context, and a single Hunt
trigger consumes its matching COUNT requirement. Multiple Hunt triggers that
execute ADDCOUNT for the same counter now produce one shared runtime count with
an explicit allowed-monster set; unrelated monsters and independently numbered
counters remain separate, matching the legacy ChangeSubQuestValue/COUNT flow.

The Unity harness now provides `--quest-reward`. It seeds canonical quest 173 at
a legal level and legal prior progress, then Unity performs the real final kill
against Map10 monster object 50023/kind 73, observes completion, submits EndSyn,
receives the authoritative reward refresh and EndAck, reconnects, and verifies
Rewarded state plus item 414 quantity 30. SQLite run
`255d07e5444f4620a791e9ce91cc991a` and MSSQL ODBC run
`71c3756e752a4496afdc8463c469bfc6` both passed. Each Unity result contains 92
tests, 82 passed, zero failed and ten explicit environment skips. Both database
summaries contain quest `(173,3)`, subconditions Talk 38 `1/1`, Hunt 73 `30/30`,
Talk 38 `1/1`, reward `(414,30)`, and progress `(level=49,exp=750010,money=0)`;
all owned servers remained live until cleanup. Diagnostic run
`768dcfb8b9f3440184e65b002ab64628` was retained as the evidence that exposed the
old TALKTONPC interpretation and is not acceptance evidence.

Quest acceptance now maps the first script LEVEL limit into the runtime
definition, validates it against the authoritative actor level, and persists the
new log inside a database transaction before sending StartAck. A persistence
failure restores the prior in-memory log and sends StartNack; commit uncertainty
drains and disconnects the database. Thirteen adapter tests, including a direct
load of the frozen canonical QuestScript.bin for quest 173, and four focused StartSyn
handler tests pass. This closes the previously observed level-gate and
Ack-before-commit defects. It does not yet constitute a full quest-from-start
visible run: the completion fixture starts from controlled legal prior progress,
and NPC dialogue/HUD operation plus human acceptance remain open.

Filtered quest counters now remain server-authoritative in the modern runtime.
The adapter maps ADDCOUNTFQW to an exact equipped-item requirement, ADDCOUNTFW
to the equipped weapon type from ItemList.bin, ADDCOUNTLEVELGAP to the legacy
asymmetric player/monster level-gap limits, and ADDCOUNTMONLEVEL to the inclusive
monster-level range. The MapServer kill event supplies the actor level, defeated
monster level, weapon-slot item and resolved WeaponType; Unity supplies none of
these decisions. Synthetic coverage rejects mismatched weapon items, weapon
types and level ranges before proving every matching variant advances its
counter and reaches completion. Real Unity fixtures for representative shipped
quests using each filtered variant remain a separate acceptance step.
### 2026-09-13：NPC 任务对话协议语义修复

- 原版 `MP_PROTOCOL_QUEST` 与 `QuestManager.cpp` 证明任务对话使用 Quest 子协议 24，`SEND_QUEST_IDX` 前两个 `WORD` 分别承载 NPC 语义索引与任务索引；场景 `NPC_SPEECH_SYN` 的实时对象 ID 不能代替脚本中的 `@TALKTONPC` 索引。
- modern 服务端已移除普通 NPC Speech 对任务进度的隐式推进，并实现 Quest/NpcTalk 的四字节权威入口。入口要求 NPC 已在当前地图生成且玩家距离不超过 500 个原游戏单位；事件按 quest id 隔离，只推进当前活动阶段；写入任务日志成功后才发变更通知，写入失败恢复内存快照并返回 Nack。
- C ABI 新增 `MXH_UNITY_COMMAND_QUEST_NPC_TALK=18`，Unity C# 提供 `SubmitQuestNpcTalk(npcIndex, questId, snapshot)`；x64 核心按原版小端布局发送 `npcIndex:u16 + questId:u16`。
- 验证：quest manager 45/45、MapHandler 155/155、x64 Unity core 94/94；Unity 6000.6.0f1 BatchMode 脚本编译退出码 0。尚未形成从 NPC 对话到奖励领取的可见真人验收，因此完整任务流程仍为 Partial。

任务对话 UI 随后已进入实际 `ConnectionValidation` 场景：选中服务器生成的 NPC 时，如果存在服务器下发的活动任务，面板显示 NPC 名称、任务 ID、继续与关闭按钮；继续按钮调用 `SubmitQuestNpcTalk`，等待独立的 `QUEST_NPC_RESPONSE` 后显示确认或拒绝。NpcTalk Ack/Nack 不再被普通 Quest 解码器误当成状态更新，因此 NPC ID 不会覆盖当前 Quest ID。当前 UI 仍使用恢复中的任务 ID 文案；原版对话正文与多活动任务选项需要由 QuestScript/NPC 对话资源清单驱动，不能以当前开发文案作为视觉验收完成证据。

本步验证：Unity 场景生成器日志返回码 0，场景包含 `QuestDialoguePanel`；Unity EditMode 91 PASS、11 条因环境/显式条件 SKIP、0 FAIL，其中任务面板定向用例 1/1 PASS；x64 Unity core 94/94；MapHandler 155/155。已暂存到 Unity Plugins 的 x64 DLL SHA-256 为 `84D4CDBB81C5DDCE1A09BB6AE1D4DD2E5E12DF08413E424D1EF208E355BE294D`。

### 2026-09-13：原始任务文本目录与多任务 NPC 选择

- 新增 `modern/tools/export_unity_quest_interactions.py`，从只读 `QuestScript.bin`、`QuestString.bin`、`questnpclist.bin` 与 `StaticNpc.bin` 导出带四项来源 SHA-256 的稳定 UTF-8 JSON。QuestScript/QuestString 按严格 Big5 解码；NPC 名称目录对历史混合坏字节使用替换解码并保留来源哈希。
- 当前目录含 1,230 个 NPC/任务/阶段/地图交互，Quest 173 标题恢复为“嵩山的殭屍”，Map10 NPC 554/572 分别关联任务 57/60 与 180。连续导出 SHA-256 均为 `19cba863020187243b7d0690358fca651f1c332576b6c23bb655978db1bb3024`。
- Unity 运行时拒绝未知 schema、缺失来源哈希、重复完整身份键和统计不一致。跨地图复用 NPC 索引的合法条目以地图与坐标区分；对话选项按任务/阶段去重，并只展示服务器活动任务及其服务器阶段。`QuestStateController` 现保留多个同时活动的任务状态。
- `QuestDialoguePanel` 已写入 `QuestChoices` 下拉选择，正文显示恢复出的原始标题和描述；提交仍走服务器权威 NpcTalk 校验。
- 验证：目录/对话/多任务状态定向 EditMode 6/6；全量 EditMode 95 PASS、11 个既有显式 SKIP、0 FAIL；场景更新 BatchMode return code 0。正式 `--strict` 导出因 12 条未解析标题按预期失败，因此此项仍为 E3/PARTIAL，不能进入正式资源包，也尚未形成真人可见 E4 证据。

资源追查进一步确认：当前客户端、部署目录、modern 数据与源码配套目录中的四份 `QuestString.bin` 均为同一 SHA-256 `6ebdb69e...de20a3`。历史恢复目录另有一份 146,362 字节、SHA-256 `3ac196c4...18a44` 的候选，但解析后同样没有任务 152、200–203 的 `$SUBQUESTSTR`。QuestScript 证明 200–203 是 99 级串联活动任务，不能从触发器动作臆造标题。

Map10 NPC 任务对话随后完成真实三服双数据库验证。夹具将任务 180 保持 Accepted，仅预置阶段 0 的 NPC 41 对话完成，并把角色放到原始 NPC 572 坐标 `(3500,46900)`。Unity 提交 `NpcTalk(572,180)`、收到专用 Ack、断开并重连。SQLite 运行 `b82abd6584654eaa9f7f630f9f15e97a` 与 MSSQL 运行 `cadb382e56294de1bc26695fc7d27046` 均保持阶段 0/1 为 `1/1`、阶段 2/3 为 `0/1`，证明没有跨阶段提前推进；两次运行的 Login/Agent/Map 在清理前均存活。新增测试使全量 EditMode 为 95 PASS、12 个环境显式 SKIP、0 FAIL。该证据属于自动 E4；玩家实际点击新任务下拉面板的真人可见验收仍未完成。

原版 NPC 对话内容也已从正确资源链恢复。`QuestScript.bin` 的 `#NPCSCRIPT` 提供 NPC 与页号，`Npc_Script.bin` 把页号映射到 `Npc_Msg.bin` 消息和 `Npc_HyperText.bin` 操作。导出清单现包含七项来源哈希、原始消息 ID、保留控制标记的 raw 文本、供 Unity 显示的清理文本，以及超链接文字 ID/类型/目标。NPC 572 的任务 180 阶段 1 被逐项核对为页 3、消息 2230“面目全非，幾乎無法辨識誰是誰。”、操作 678“搜身找尋名牌”。Unity 面板显示该原始台词与按钮文字，提交仍由服务器核验。

`Npc_Script.bin` 在 NPC 216 的第 3 页缺少闭合花括号。原版客户端资源允许这种历史坏格式；导出器因此改用 `$NPC`/`$PAGE` 行边界解析，而不是把后续页面吞进坏块。该修复恢复了任务 38/285 的页面 4 和 9，包括消息 12473/12478 与“說來聽聽”“將物品遞過去，接收報酬”。

任务入口超链接随后补齐全部标题缺口。`Npc_Script.bin` 中 `#HYPERLINK <textId> 4 <questId>` 是原版任务入口，`Npc_HyperText.bin` 提供对应显示标题：任务 152 为“[中原 Lv.19]採花賊”，任务 200–203 为“玄武藍寶箱任務”。目录为每条记录标注 `titleSource=NpcHyperTextQuestLink`，没有把触发器动作改写成标题。当前 1,230 条交互标题全部有原始来源。

任务 152 的 NPC 264 页面 16、19、23 在当前 `Npc_Script.bin` 中缺失，但只读的配套表保留了连续内容：页面 15 结束于消息 8362，随后 `Npc_Msg.bin` 的 8363、8366、8370 分别对应三次任务交互，`Npc_HyperText.bin` 的 480、481、482 分别为“我去找回來”“遞上花”“遞上水瓶”。导出器仅在 Npc_Script、Npc_Msg、Npc_HyperText 三项 SHA-256 与已审计版本完全一致时恢复这三页，并标注 `pageSource=RecoveredContiguousNpcMessageSequence` 与证据说明；任一哈希变化或正文/操作缺项时不恢复。

正式 `--strict` 导出现在通过：1,230 条交互、0 条未解析标题、0 条未解析呈现，3 条呈现明确分类为连续配套表恢复。连续两次导出 SHA-256 均为 `cd3a784ff97df88e4e236f2621d947da7abde449450d98acbc25413bcd2c06a5`。新增 Python 门禁测试 3/3 PASS，覆盖精确哈希启用、任一哈希变化禁用及缺少配套内容时拒绝部分恢复；Unity 定向 6/6、全量 EditMode 95 PASS/12 个环境显式 SKIP/0 FAIL。任务交互资源的 strict 门禁已关闭，玩家实际看到并点击这些页面的 E4 验收仍未完成。

### 2026-09-13：任务对话 Player 可见闭环

真实 Player 验收首次揭示 `QuestDialoguePanel` 被整体设为 inactive，导致控制器无法订阅目标选择事件，也不会加载目录；此前直接调用控制器的 EditMode 测试没有覆盖这个场景生命周期错误。面板现保持启用，由 `CanvasGroup` 控制显示、交互和射线阻挡，隐藏时控制器继续接收 NPC 选择事件。测试新增断言，确认隐藏后宿主对象仍 active。

开发版 Player 探针现可显式执行 `--quest-npc`：登录真实 modern Login/Agent/Map 三服，进入 Map10，等待任务 180 状态，打开 NPC 572 的实际面板，记录可见标题/正文/操作，点击操作并等待专用 Quest NpcTalk Ack，最后截图及核对持久化。SQLite 运行 `908dc105ec464f09b4c827c897d52672` PASS；画面显示 NPC“不知名的屍體”、任务“血教的移動?”、正文“面目全非，幾乎無法辨識誰是誰。”、操作“搜身找尋名牌”和反馈“任务步骤已更新”。数据库仅将任务 180 子阶段 1 的 NPC572 计数从 0/1 推进至 1/1，其余阶段保持不变；三个服务清理前均存活。截图 SHA-256 为 `1c7c0ab8e89a17d11d195c7ccb5c2ca85890095e512622b588679010d8d90081`。

修复后定向 QuestDialogue EditMode 2/2 PASS，全量 EditMode 95 PASS/12 个环境显式 SKIP/0 FAIL。该运行是自动化 Player E4 证据，证明实际画面与服务器状态闭环；仍不能代替玩家对布局、可读性和美术质量的真人验收。当前截图也明确显示连接验证界面和商店开发占位仍存在，不能作为 Map10 完整切片或重制画质完成证据。

随后将探针收紧为必须选择服务器实际生成的 NPC 实体，发现此前的 Player E4 仍通过测试身份直接打开面板，未覆盖场景实体。真实链路暴露两个独立问题：服务器实体在 Map10 视觉异步加载期间到达时会被注册表丢弃；场景的 `monsterPrefab`、`npcPrefab`、`groundDropPrefab` 均为空，因此即使事件保留，也会按“无审计 Prefab 不实例化”的规则拒绝生成。注册表现按地图代次缓存加载期间的实体事件，视觉就绪后按到达顺序重放，断线和换图清空缓存；定向测试 1/1 PASS。

原生事件补充了已有保留字段语义：`NPC_ADDED.argument0` 继续是稳定生成 NPC ID，`reserved0` 携带 `NpcChxList` 使用的视觉 kind，不改变结构尺寸或协议头。x64 core 96/96 PASS。真实三服诊断运行 `5bbbf42b6b77405d923225441750e893` 确认 NPC 572 的视觉 kind 为 54，原始 `NpcChxList.bin` 将其映射为 `N054.chx`，`npc.pak` 条目 65 确实存在该资源。下一步必须导出并绑定 N054 的 CHX→MOD→纹理/动画依赖闭包，不能使用随机胶囊或角色模型冒充正式 NPC。由于实际实体尚未生成，`908dc105...` 只能证明真实面板、命令、三服及持久化闭环，不能单独证明玩家点击了场景 NPC。当前全量 Unity EditMode 为 97 PASS、12 个环境显式 SKIP、0 FAIL（109 total）。

上段 PAK 条目号与闭包状态已由完整导出清单纠正：`N054.chx` 是 `npc.pak` entry 311，不是浏览器显示的筛选序号 65；其内容只引用 `dead_m2.mod` entry 103，且 `$MOTION_NUM 0`。MOD 的作者期材质名为 `dead_m2.tga`，运行包实际提供同基名 `dead_m2.dds` entry 102；其余六个 PAK 和散落目录均无 `dead_m2.tga`。三项原始 SHA-256 及解析规则已写入 Unity 派生目录的 `provenance.json`，原始字节未修改。

Unity 现在按服务器 `visualKind` 选择审计 prefab，仅为 kind54 绑定 N054，未匹配 kind 不会错误显示这具尸体。导入模型含 1 个 mesh、884 顶点、2 个 face group；没有骨骼或动作依赖。运行时只在导入视觉缺少 Collider 时按渲染 bounds 添加选择碰撞体。NPC/Monster 的旧编码名称在 C++ 边界转换为 UTF-8，无法解码的显示名降为空串，防止无效线上字节越过 UTF-8 C ABI 并中断后续事件。

真实 Development Player 运行 `6bd9f83e65a141e38a6eee626f96942b` 通过：服务器 NPC572 实体以 visualKind54 实例化，正式 `TargetSelectionController.Select` 打开原始任务180对话，按钮提交后收到专用 Ack，SQLite 重连确认阶段0/1=`1/1`、阶段2/3=`0/1`。报告记录 `questNpcVisualKind=54`、原始标题/正文/操作及“任务步骤已更新”；截图 SHA-256 为 `65f997a00637d47f678e1af372768c2e1ed5dd26144b545452781867b756ee73`。x64 core 94/94、Unity kind 定向2/2、全量 EditMode 98 PASS/12 SKIP/0 FAIL。该结果关闭自动实际实体选择链路，仍不代表真人操作、布局或现代高精度美术验收。

### 2026-09-13：Map10 全怪物静态视觉闭包

真实 Map10 服务器日志确认初始 228 个怪物只使用 kind 73、102、103、104、105、219。`MonsterList.bin` 将它们映射到 `L073/L102/L103/L104/L105/L219.chx`；六个 CHX 均从 `monster.pak` 重新导出，各自声明 LOD2/LOD1/LOD0 和 12 个 ANM。Unity 当前选用 LOD2 作为静态运行样板，六个模型分别保留 37–76 根骨骼、480–841 个顶点及原始权重。73/105/219 使用包内 DDS，102/103/104 使用包内 TIF；后者通过可重复工具转换为无损 RGBA PNG，并在每个 `provenance.json` 中同时记录原始与派生 SHA-256。

`stage_unity_entity_visuals.py` 现在从完整 PAK 提取清单校验源哈希，生成每个 kind 的模型、纹理和 12 动作依赖清单。模型导入器支持审计 DDS 及 TIF→PNG 派生；原始 MOD 中完全缺少法线的眼球子网格按三角形重建法线，部分法线长度错误仍拒绝导入。`ServerEntityRegistry` 分别按 NPC/Monster visualKind 查找 prefab，未匹配项保持不实例化；MonsterAdd 的 C ABI `reserved0` 已改为 monster kind，并由原生 burst 测试锁定。

真实 Development Player 运行 `741eb9806f5b4d9780439dd2887c8659` 通过，报告直接枚举注册表子实体：`materializedMonsterCount=228`，distinct kinds 精确为 `[73,102,103,104,105,219]`；同次运行仍完成 NPC572/quest180 原始对话与重连持久化。截图 SHA-256 `769e4d91624235de5376f9e4d0920a7c6c30830bd8f2a5255a974d2f6ce8b1c7`。x64 core 94/94，Unity EditMode 99 PASS/12 SKIP/0 FAIL，六份派生哈希复核通过。当前只完成静态 LOD2 实例化；12 动作的语义映射、LOD 切换、战斗动画时序、性能与真人画质验收仍未关闭。

### 2026-09-13：Map10 怪物原始 ANM 运行时接入

六种 Map10 怪物的 72 个 ANM 已由 `mxh_unity_motion_export` 全部转换并纳入每种 visual kind 的来源清单；导出结果保留原始 track、key、首末帧和源 SHA-256。`MapVisualSetup` 将每种怪物的 12 个 `ImportedMotion` 与对应 LOD2 模型一起写入场景。`ServerMonsterAnimation` 使用同一个 30 FPS legacy integer-frame driver 对导入模型的骨骼和权重采样，只更新实例自有 mesh 顶点，不修改服务器权威 GameObject 根坐标。真实 Player 运行 `2181705b2e6541108f857448f8a648cd` 报告 228/228 实例均挂载动画组件，distinct visual kinds 仍精确为六种；任务180闭环同时保持通过。截图 SHA-256 `0672d49647f7fa576b5e2de502ee341ff172b0698079be24f1608dab9621b017`。

运行时已经为初始/移动、生命下降和归零提供动作切换入口；死亡动作完成后才隐藏实体。当前槽位语义采用待进一步与可信旧客户端运行证据核对的候选映射（1待机、2移动、4攻击、8受击、9死亡），因此只把“全部实例成功加载并持续采样待机 ANM”记为通过，不能把攻击/受击/死亡时序宣称为1:1完成。原客户端源码目录在当前工作区实际只剩 CMakeLists，无法作为槽位证据。Unity 定向根坐标测试1/1、全量 EditMode 100 PASS/12 SKIP/0 FAIL；测试确认移动和死亡采样不会改变权威根坐标，死亡播完才停用实例。

### 2026-09-13：服务器权威施法释放与怪物受击链路

Unity x64 C ABI 现将服务端 Skill `StartAck` 严格解码为释放事件，将
`SingleResult` 严格解码为命中事件；截断、尾随字节、零 ID 和负伤害均拒绝。
Unity 只在命中事件包含正伤害时驱动目标怪物的候选受击动作 8，生命归零仍仅由
独立 `LifeNotify` 驱动死亡动作，且资源动画不移动服务器权威根坐标。

真实 Player 诊断发现 AgentServer 把全部 Skill 消息当作世界广播，并按
`object_id` 排除发送者，导致 MapServer 已生成的 `StartAck` 无法返回施法者。
AgentServer 现将 `StartAck/StartNack` 精确路由到所属角色；`SkillObjectAdd` 等
世界事件仍排除发送者。服务端 handler 全量 156/156 通过，x64 Unity core 全量 97/97 通过。

Development Player 运行 `7a933c1c764e4b5ba480abbb5289cbe8` 通过真实
SQLite、HSEL 客户端传输和 modern Login/Agent/Map 三服。报告记录施法者111、
技能1、服务器 skill object 80000、目标50023、伤害7、命中结果1，并确认释放
事件先于命中且目标进入动作8；228个 Map10 怪物仍全部实例化并挂载动画。截图
SHA-256 为 `EA4B493A4E131EAB54669737EA1D64A24960BB0CD3CE07065DAF0B02DFFA2DB2`。
Unity 全量 EditMode 为100 PASS、12个显式环境 SKIP、0 FAIL；暂存核心 DLL
SHA-256 为 `0156552811A0F81008AC573F2A8309FF9A9802A76A671FA7B9AD39771B808623`。

该结果是自动 Player E4 证据，不是人物施法动作、特效、音频或真人手感验收。
该缺口已由下述 2026-09-13 MapServer 攻击闭环取代；本段保留为当时画面证据的范围说明。

### 2026-09-13：玩家生命事件桥接与 MonsterList 战斗定义接入

C ABI 升至 v1.12。Unity 核心严格接收原版 Character `LifeAck(1)` 与
`ShieldAck(5)` 的四字节有符号增量；LifeAck 只允许当前本地角色，按服务器增量更新
权威快照并发出 `PLAYER_LIFE`，越界或错误长度进入协议失败。x64 核心 98/98 通过，
Unity EditMode 100 通过、12 个真实服务器用例按条件跳过，暂存 DLL SHA-256 为
`FD0F3D13CF13DD5EBA741F55A3227634615D937DC9C38108B0825A6F6F521AC5`。

现有 `MonsterCatalog` 已扩展为读取 `MonsterList.bin` 的等级、生命、护盾、经验、
攻击上下限、防御、移动速度、领域/搜索参数、主动攻击标记以及两个攻击技能和概率。
规范 PlayDH 实测解析 384 行，Map10 的 kind 73、102、103、104、105、219 全部存在且
战斗字段有效。MapServer 的生产资源配置现在把 MonsterList 作为必需输入，AIGroup
生成的实例从该目录复制真实模板；五种地区宏构建均通过，MapHandler 157/157 通过。
攻击状态机仍需用对应 SkillList 的 SkillRange/DelayTime 完成距离、冷却、伤害、死亡
与持久化闭环，因此本项仍不构成怪物攻击玩法验收。

### 2026-09-13：MapServer 怪物对玩家权威攻击闭环

`tick_monster_ai` 现使用 MonsterList 的搜索周期、攻击组合和攻击技能，并以
SkillList 的 `SkillRange`、`DelayTime` 执行追击与严格冷却判断。实际命中沿用原版
怪物物理攻击、玩家物防、连击/武功护盾比例和最低 1 点生命伤害规则；服务器同步
更新 Player actor、PlayerInfo，并发送 SkillObjectAdd/SingleResult、ShieldAck、
LifeAck、CharacterDie 和 SkillObjectRemove。死亡玩家不会继续成为仇恨目标或收到
重复死亡包；缺失技能定义时状态机失败关闭并返回出生点。

确定性测试覆盖了冷却等于边界不攻击、下一毫秒命中、生命负增量、技能对象消息、
死亡消息和死亡后不重复攻击。`mxh_server_handler_tests` 为 158/158 PASS，
MapHandler 子集为 100/100 PASS；KOR/CHINA/JAPAN/HK/TL 五个 MapServer 目标均构建
成功。当前闭环尚未覆盖怪物属性伤害、异常状态、宠物/泰坦目标、活动伤害倍率和
现场 Boss 的位置/自身目标技能分支，不能据此宣称全部怪物战斗 1:1 完成。

真实 Unity D3D11 Player 隔离三服运行 `8415fc6872364d888a986341bc417f83`
进一步闭合了客户端表现链：玩家111以340生命进入 Map10，在44178/13253攻击真实
怪物50023；服务器技能对象80000先释放，目标随后收到1点暴击伤害并驱动原始候选
受击动作，怪物反击使玩家首个观测生命从340降至278（负增量-62），截图时原生
快照与屏幕 HUD 同为162/340。228/228怪物及六种 visual kind 保持实例化，Player
退出码为0。截图 SHA-256 为
`645E87DC1AD63DA2313BC60225811DBEDEF2C351CC24CA0ADF5401E5767CAD2B`。

此证据是自动 E4/PARTIAL：它证明真实服务器、C ABI、Unity事件和现有HUD之间的
双向战斗数据链，不代表人物模型、人物受击/死亡动作、血条最终美术或真人操作质量
已经验收。当前截图仍是开发验证布局，不能作为阶段C的现代高精度画面确认样板。

死亡事件桥接增量：C ABI 与 C# 版本同步为 1.13，新增 PLAYER_DEATH(38)，
携带受害者与攻击者 ID；接收有效 CharacterDie 后将本地生命快照归零。
消息处理限定在当前世界状态并校验长度与身份。真实 socket 夹具验证事件与快照，
原生核心 99/99 PASS；2026-09-13 06:54 UTC Unity EditMode 为100 PASS、0 FAIL、
12 SKIP。DLL 已重新暂存。此增量尚未完成独立 Player 死亡流程、人物死亡动作、
复活交互或死亡后的输入验收，不作为上述功能完成证据。

异常死亡包回归进一步覆盖7字节截断、9字节超长、攻击者与包头不一致、
非当前玩家受害者和零攻击者五类真实 socket 输入；均进入协议错误状态且不发布
PLAYER_DEATH。原生核心回归现为100/100 PASS。此测试不替代换图过期事件或
独立 Player 死亡表现验证。

死亡输入增量：核心在生命快照为零时拒绝普通移动/停止、技能和定时路线/停止，
返回 WRONG_STATE；定时 hello 仍可建立同步。死亡 socket 用例追加普通移动、停止、
有效技能请求被拒绝且位置不变的断言，完整原生回归100/100 PASS，DLL重新暂存。
这属于客户端命令约束；服务端死亡操作校验、定时移动死亡专项、复活及Player表现
仍需独立验收，不能把本次结果当作完整死亡流程通过。

服务端审查确认普通移动已有 actor.is_alive 校验；技能扣MP入口此前没有。
现于会话身份校验与MP预留的同一 players_mu_ 临界区要求 actor 活跃且存活，
死亡/非活跃施法返回 StartNack(3)，不扣MP、不创建技能对象。
回归使用HP=0、MP=50的角色请求真实技能，断言仅返回Nack且MP仍为50；
server_handler 160/160 PASS，CHINA MapServer重建成功。尚未完成部署或独立
Player死亡后的攻击验收；已经发出的技能与并发死亡结算还需单独检查。

死亡限制合入后的独立Player正常战斗回归：Unity6000.6.0f1 x64 Player重建成功，
三服运行ID `8bf9c4a4ad37419f8be25a6809cda751`，SQLite/HSEL/Direct3D11通过。
实际玩家111在Map10释放技能1，技能对象80000命中怪物50023并造成1点伤害；
观测玩家生命340→285（-55），截图时HUD与快照为115/340。228只怪物与六类
外观实例存在。报告在 `modern/out/unity-remaster/three-server/8bf9c4a4ad37419f8be25a6809cda751/report.json`。
此次未请求移动测试，未到达玩家死亡或复活，humanAcceptance=false；仅支持
正常战斗链未被死亡限制破坏的结论，不支持完整死亡或视觉质量验收。

独立Player增量：移动运行 `6f899287f6374230a831d3ec8f7a4825` 通过SQLite真实三服、
两个原生会话与Map10实际阻挡碰撞校正。新增 `--combat-timeline --player-death`
验证模式等待怪物实际击杀玩家，要求死亡事件、零生命快照及死亡后移动/技能拒绝。
Player重建后运行 `dfb8b9ab12da4a8a9f87f5ab748d2ab9` PASS，四项死亡验证布尔值均为true，
截图时生命快照为0。证据位于对应three-server运行目录。此为自动运行证据，
没有验证人物死亡动作、复活UI、复活持久化或真人体验，不能作为完整死亡流程验收。
