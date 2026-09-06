# Changelog

## Unreleased

### Phase 0 §6.5 — GameInAck 后客户端稳定性 (2026-09-05 session)

- **Root cause fix**: `mxh_client` 32-bit 2 GB user-mode ceiling → `target_link_options(mxh_client PRIVATE /LARGEADDRESSAWARE)` 让 32-bit PE 用 4 GB user-mode on x64 Windows (commit `469cfbdb`).
- Diagnostic infrastructure: VEH (Vectored Exception Handler) via `AddVectoredExceptionHandler` + multi-layer try-catch + `MLOG_HEAP` macro via `GetProcessMemoryInfo` / PSAPI + `mx_find_throw_site` DIA SDK RVA→file:line tool.
- `std::stof` → `std::from_chars` in `monster_catalog` / `item_list_parser` / `skill_list_parser` to avoid `std::invalid_argument` throw.
- Tighter `request_friend_add[_by_name]` try-catch no longer calls `ex.what()` (avoids catch handler re-throw).
- Added `m_dispatchEnabledForTest` + `SetDispatchForTest(bool)` + `HandleMessageForTest(Message)` hooks in CInGameState / CLoginState / CCharMake / CCharSelectState for §6.4 protocol-burst test path.
- `capture-gamein.ps1 -NoAutoExit -ClientTimeoutSeconds 600`: 3/3 literal 10-min runs (`gfix-20260905-143403-31` / `144503-53` / `20260906-010124-99`) all `state-gamein.tga` 3,145,746 bytes captured, WS 1.49 GB stable, no `.dmp`, no `veh.log`. `exit_code=-1` is the capture-script force-kill at the 10-min cap, not a crash. Required `--debug-ui-bounds` to skip release visual gate (Phase 5 G10 concern, separate work item).

### Phase 0 §6.4 — Protocol-burst regression (2026-09-05)

- `protocol_burst_test.cpp` covers 4 burst scenarios on `CInGameState`: `GameInAckThenImmediateMonsterAddProducesNoCrash` (50 MonsterAdd in one frame), `DuplicateMonsterAddIsIdempotent`, `OutOfOrderMonsterAckStillPopulates`, `EntityBeforeGameInDoesNotPromoteState` (silent-ingress defense).

### Phase 1 §7.2 — Login error matrix (2026-09-05)

- 12/14 unit-level tests covering 5 boundary cases (17-byte truncation, UTF-8 round-trip, empty fields, invalid port), 4 Nack paths (LoginNackTriggersFailWith / LoginNackIsIdempotent / LoginAckAfterNackDoesNotRecover / FailWithIsIdempotent), 2 disconnect paths (DisconnectBeforeAckTriggersFailWith / DisconnectAfterAckIsIgnored), and 1 no-password-persist test (PersistedFileDoesNotContainPasswordField — locks §7.2 #14 by reading the on-disk JSON bytes and asserting no "password" / "passwd" / "PASS" / "Password" field exists).
- 2 remaining items (重复注册, 登录超时) require real LoginServer timing-out / duplicate-account race; covered by `mxh_client_e2e` re-write (separate work item).

### Phase 1 §7.3 — Character flow dispatch hook (2026-09-05)

- 11 unit-level tests covering 4 state machines (CInGameState / CLoginState / CCharMake / CCharSelectState) with `SetDispatchForTest` / `HandleMessageForTest` / `m_dispatchEnabledForTest` opt-in flag. Each test uses the real `on_message` dispatch path (not a mock) so the receive queue + per-handler logic is exercised. 3 e2e items (角色列表与数据库一致, 重登持久化, 选择后进入真实 GameLoading — partially covered by StateTransfer.GameEntryRequestRoundTrip typed payload) require DB round-trip.

### Test baseline (2026-09-05 final)

- `mxh_client_tests`: **322/322 PASS** in 16.6 sec (from 299 → 322, +23 new tests).
- Full `ctest` (excl. `MoxianClientE2E` pre-existing race): **12,416/12,416 PASS** in 113.71 sec, 5 pre-existing skips (MSSQL E2E + D:\[SWorking] gated + deploy manifest).
- 54 commits ahead of origin in `codex/runtime-recovery-pve`.

### Governance and provenance

- Protected 12 unreachable Git commits with backup refs and a verified bundle.
- Promoted the 8,642-file legacy recovery snapshot to a read-only reference area.
- Added SHA-256 manifests for the current PlayDH tree and recovered SWorking profile.
- Removed historical phase/session documents that were no longer authoritative.
- Replaced the corrupt mixed-encoding ignore file and expanded binary attributes.
- Removed exact untracked duplicate UI header copies and the generated restoration baseline.

### Runtime foundation

- Added explicit `playdh-current` and `sworking-2008-reference` resource profiles.
- Removed the server launcher’s implicit source-recovery scratch fallback.
- Changed the local acceptance server default map to Map10.
- Added atomic client settings persistence for display, audio and last-account preferences.
- Added typed GameLoadingCoordinator transfer handling and progress/error state.
- Character-list parsing now retains appearance, equipment, level and map fields.
- Client startup now rejects a profile missing required Map10, entity, UI or audio resources.
- Removed the synthetic placeholder dialog from the product path.

### Still in progress

- Launcher/update UI, display transition, complete character presentation, loading, map fidelity, live UI binding, collision, effects, SFX and full legacy comparison remain open.
- Phase 1 §7.2 剩 2 项 e2e (重复注册, 登录超时) + Phase 1 §7.3 剩 2 项 e2e (角色列表与数据库一致, 重登持久化) + Phase 2-3, 5 + Phase 4, 6.3 24h soak, 7 PVE — all blocked on real server / DB / 24h soak / 外部 VM.
