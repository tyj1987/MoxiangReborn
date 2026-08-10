# CHANGELOG - åŽ†å²å®Œæˆé¡¹

> å®Œæ•´ commit åŽ†å²: git log --oneline (1 commit = 1 sub-deliverable)ã€‚
> æœ¬æ–‡ä»¶ä¿ç•™ 2026-08 é‡æž„å‰ ROADMAP çš„å…³é”®åŽ†å² [x] é¡¹æ‘˜è¦, ç”¨äºŽå›žæº¯ã€‚

æœ€è¿‘é‡æž„: 2026-08-06 - æŠŠè€ ROADMAP (434 è¡Œ) ç æˆè§„åˆ’æ–‡æ¡£ (158 è¡Œ) + æœ¬ CHANGELOGã€‚

﻿
## 2026-08-10 - cItemShopDialog IItemShopService wiring + 5 service-mode tests (M2 advance)

- New service interface `mxh::services::IItemShopService` (catalog + money queries; mirrors the legacy NPC shop table layout) so the modern dialog code reads shop state through an injected service rather than via legacy singletons (ITEMMGR / GameIn->GetCharacterDialog()). Includes a `ShopEntry` value struct (item_id, price, quantity) shared with the dialog via type alias.
- `cItemShopDialog` now takes an optional `IItemShopService*` via `SetShopService()`. When bound: `GetMoney()` reads `service->playerMoney()` (live economy), `TotalPrice()` resolves the entry through `service->getShopEntry(i)` (live catalog), and `Buy()` validates via `service->hasEnoughMoney()` instead of the local `m_money` snapshot. The existing `m_entries` + `m_money` fallback path is preserved so legacy NPC types + unit tests without a service continue to work unchanged.
- 5 new behavior tests in `citemshopdialog_test.cpp` covering the service-mode contract: catalog+money consultation, insufficient funds rejection (no callback fired), out-of-range index rejection (no callback fired), live economy reflection after service mutation, and clean fall-back to local snapshot when the service pointer is cleared. Existing 3 local-mode tests remain unchanged.
- 11783 unit tests now pass (was 11778 + 5 new).
- `scripts/commercial-smoke.ps1` gate stays GREEN: MSSQL_E2E PASS, GUI_CLIENT_SMOKE PASS (all 5 state frames + terrain frame).



## 2026-08-10 - cFriendDialog IFriendService wiring + 4 service-mode tests (M2 advance)

- New service interface `mxh::services::IFriendService` (roster + presence queries; mirrors the legacy FRIENDMGR layout) so the modern dialog code reads the friend list through an injected service rather than via legacy singletons (FRIENDMGR / CHATMGR). Includes a `FriendStatus` enum and `FriendEntry` struct shared with the dialog via type alias (the dialog's `FriendStatus` is now an alias for the service enum so there is one canonical definition).
- `cFriendDialog` now takes an optional `IFriendService*` via `SetFriendService()`. When bound: `IsFriendOnline(id)` reads `service->getStatus(id)` (live presence) and `WhisperSelected()` gates the whisper dispatch on `service->isFriend(id)` so an offline or removed friend cannot be whispered to. The local `m_friends` snapshot remains a fallback for unit tests + legacy NPC types not yet wired.
- 4 new behavior tests in `cfrienddialog_test.cpp` covering the service-mode contract: roster-driven `IsFriendOnline`, whisper gating on roster membership, clean fall-back to local snapshot when the service pointer is cleared, and live presence reflection after service mutation. Existing 3 local-mode tests remain unchanged.
- 11787 unit tests now pass (was 11783 + 4 new).

## 2026-08-09 - tooling hygiene: verify-state-frames.py + gitignore deploy/runtime

- `scripts/verify-state-frames.py` was referenced by `gui-client-smoke.ps1` since 2026-08-09 but had not been committed (prior session oversight). Now tracked so the GUI smoke gate is reproducible from a fresh checkout.
- `deploy/runtime/` (per-process runtime data + logs from `deploy/scripts/start_modern.ps1`) is now in `.gitignore` so the SQLite `*.db` and per-service `*.log` artifacts stay out of the working tree after the smoke runs (matching the other deploy/ subtrees that are already ignored).

## 2026-08-09 — Client GUI smoke visual acceptance (CLIENT-RUNTIME advance)

- Sprite 2D textured quad reordered to CCW in NDC so the default rasterizer
  (FrontCounterClockwise=TRUE, Cull BACK) keeps the textured quad visible.
  The 3D pipeline (drawBox / drawGrid) is already CCW; the 2D path was
  emitting CW triangles which the cull dropped, leaving every login / char
  make / login-form frame at the BeginRender clear color.
- drawTexturedQuad now binds the point sampler (PSSetSamplers(0, 1, &sampler))
  before Draw(); the D3D11 sampler slot was previously undefined so the
  textured PS read zero on the first call. Mirrors the terrain/lit path.
- Device::initialize seeds m_matViewProj with a screen-space ortho (pixel
  -> NDC, Y-flipped) so 2D primitive paths (sprite / font / line / point /
  circle) draw in pixel space even before the first 3D setViewFrustum call.
  Added MatrixScreenOrtho helper in include/mxh/render/math.hpp and 5
  unit tests covering 800x600 mapping, edge corners, zero-size clamp,
  and null pointer no-op.
- gui-client-smoke kStateNames table re-aligned to the modern GameStateId
  enum (End=0 / Intro=1 / Connect=2 / Title=3 / CharSelect=4 / CharMake=5
  / GameLoading=6 / GameIn=7 / MapChange=8 / MurimNet=9) and slot 3 is
  mapped to the legacy name "login" so the verifier and
  capture script keep matching the 1:1 visual reproduction contract.
- CLoginState::dispatch_login_ack now requests Title (state 3) instead of
  CharSelect (state 4) so the GUI smoke test exercises the full
  Connect -> login form -> CharSelect flow. The host main loop auto-
  redirects Title -> CharSelect on the next frame for headless mode, and
  the engine pending-transfer slot preserves the LoginResult for
  CharSelectState::Init to consume.
- gui-client-smoke now PASSes 5/5 state frames (connect, login,
  charselect, charmake, gamein) and the original BGM / create / select /
  game-in markers; full 11778 unit tests remain 0 failures.

See commit "client: GUI smoke per-state visual acceptance (CCW quad + sampler + screen ortho + state names + Title bridge)".

## 2026-08-09 — R-9 headless 帧闭环

- 修复 demo `WM_PAINT` 未验证导致的无限消息循环，headless 3 帧现可自然退出。
- 修复 cube 索引越界及左手坐标系相机方向错误。
- 新增 `RenderDemo.HeadlessFrameAcceptance`，锁定 grid、cube、checker 纹理和深度遮挡。

## 2026-08-09 — C-Tier-3 service wiring progress

- `cMPGuageDialog` now consumes `IPlayerStatsService` for live EXP progress with max-level and overrun handling.
- `cQuickDialog` now validates item and skill bindings through `IInventoryService` and `ISkillService`.
- `cInventoryExDialog` now refreshes its 60-slot view from `IInventoryService` item snapshots.
- `cMugongDialog` now refreshes slot enabled state from `ISkillService` learned-skill state.
- `cCharacterDialog` now refreshes level, current HP, and current MP from `IPlayerStatsService` without guessing shield/attribute mappings.
- Added 6 service-backed UI behavior tests; existing dialog contracts remain green.

## 2026-08-09 — runtime database path hardening

- Login, Agent, and Map server defaults now write SQLite runtime databases under `modern/build/runtime/` instead of the repository root.
- Each server creates the selected SQLite database's parent directory before connecting; explicitly supplied `--db` paths remain supported.
- This prevents future smoke runs from regenerating root `moxian.db*` pollution; existing historical files remain listed for user-confirmed cleanup.

## 2026-08-09 — C-Tier-3 scope correction

- Reconciled the historical Phase C definition: Tier-3 refers to business dialogs that depend on Phase B quest/trade services (historical candidates include QuestDialog, QuestTotalDialog, and DealDialog).
- Existing inventory/skill/player wiring remains valid reusable progress, but is no longer counted as proof of the nine-dialog Tier-3 business acceptance.
- Added `IQuestService` and wired `cQuestDialog::ClaimSelected()` through it; service rejection leaves the quest in `Completed` state. Two UI behavior tests lock the acceptance/rejection paths.
- Added inventory-backed validation to `cDealDialog::AddOwnItem()` through `IInventoryService`; unknown owned items are rejected before entering a trade. One UI behavior test locks the path.
- Added `cQuestTotalDialog::ClaimSelectedQuest()` as the service-backed orchestration path over `cQuestDialog`; the forwarding behavior is covered by a UI test.
- Added inventory-backed validation to `cGuildWarehouseDialog::Store()`; warehouse storage now rejects items absent from the player's inventory when an inventory service is attached.
- Added `ITradeService` as the atomic trade-commit boundary; `cDealDialog::Confirm()` now honors service rejection before marking a deal confirmed, with a dedicated behavior test.

### D4.31 PutOnAvatarItem / TakeOffAvatarItem data plane (2026-08-06)

- D4.31 - 1:1 ports of the validation + mutation + side-effect-emission halves of legacy CShopItemManager::PutOnAvatarItem and ::TakeOffAvatarItem from [Server]Map/ShopItemManager.cpp:1792-2021.
- Adds modern/include/mxh/server/avatar_equip_transition.hpp: free functions put_on_avatar_item / take_off_avatar_item. The data plane captures 13 mutually exclusive failure modes (AvatarMissing / PositionOutOfRange / ItemBaseMissing / UsingItemMissing / ItemBaseMismatch / AvatarEquipMissing / ItemInfoMissing / AvatarMismatch / HatBlockedByDress / WeaponSlotMismatch / ExistingItemInfoMissing / DressEquipMissing / DependentItemInfoMissing) plus the new avatar[24] state and a side-effect list (ShopItemUseParamUpdateToDB + ParamUpdateInMemory) so the orchestrator can dispatch DB writes + m_UsingItemTable mutations.
- 27 tests in modern/tests/unit/server/avatar_equip_transition_test.cpp: every status + the hat/dress interaction, weapon-slot guard (player_inited gate), default-fill mask (new equip's mask overrides weared slots 12..17), dependent-removal mask (old equip mask drives fill on slot clearing), broadcast suppression on item_pos==0, avatar-mismatch + cosmetic take-off default fill, weapon-slot dress replacement, dependent-item info miss, weapon-slot clear.
- See commit <commit d24f1c9b>.`n`n
### D4.32 PutSkinSelectItem data plane (2026-08-06)

- D4.32 - 1:1 port of the data-plane half of legacy CShopItemManager::PutSkinSelectItem from [Server]Map/ShopItemManager.cpp:2638-2704.
- Adds modern/include/mxh/server/skin_select_transition.hpp: SkinEquipSlot enum (Hat=0/Mask/Dress/Shoulder/Shoes=4, Max=5) + kSkinItemListMax=3 + LEGACY_SHOP_ITEM_NOMALCLOTHES_SKIN=265 / COSTUME_SKIN=266 + LEGACY_PART3D_HEADBAND=6 + free function put_skin_select_item(env, current_skin, dw_skin_index, dw_skin_kind, player_level, skin_delay_active). The data plane captures: (a) NullSkinSlots/Fail status when current_skin==nullptr, (b) dwSkinIndex==0 -> Fail, (c) find_skin miss -> Fail, (d) NOMALCLOTHES_SKIN && player_level < skin.dwLimitLevel -> LevelFail (costume-skin bypasses), (e) skin_delay_active -> DelayFail, (f) success path that walks skin.equip_item[0..3] and writes skin_item[Part3DType map] = item_idx with the costume-dress shoes override + headband Part3DType=6 -> Hat special case + unmapped Part3DType -> Hat slot default.
- 13 tests in modern/tests/unit/server/skin_select_transition_test.cpp: every status + the headband/hat special-case + costume-dress shoes override + nomal-clothes no-shoes-override + zero-equip skip + unknown Part3DType default + missing item info skip.
- See commit fc68978f.
### D4.33 DiscardSkinItem / RemoveEquipSkin data plane (2026-08-06)

- D4.33 - 1:1 port of the data-plane half of legacy CShopItemManager::DiscardSkinItem and ::RemoveEquipSkin from [Server]Map/ShopItemManager.cpp:2707-2773.
- Adds modern/include/mxh/server/skin_discard_transition.hpp: SkinDiscardEnv virtual interface (skin_count + skin_at per skin-kind) + free function remove_equip_skin(env, current_skin, dw_skin_kind) -> new_skin + thin wrapper discard_skin_item(env, dw_skin_kind, current_skin). The data plane walks the relevant skin table (NOMALCLOTHES_SKIN / COSTUME_SKIN only), iterates each entry's wEquipItem[3], and clears any matching wSkinItem[i] to 0. Mirrors the legacy two-level nested loop exactly: outer table walk, inner skin-slot walk, innermost wEquipItem[j] compare, with continue-on-null + skip-on-zero-slot + skip-on-zero-entry semantics.
- 12 tests in modern/tests/unit/server/skin_discard_transition_test.cpp: every status path + matching/non-matching slot + multiple table entries + nomal-vs-costume kind isolation + zero-entry skip + zero-slot skip + null-table-entry continue + discard_skin_item wrapper.
- See commit d5342ebe.
### D4.34 AddUsingShopItem data plane (2026-08-06)

- D4.34 - 1:1 port of the data-plane half of legacy CShopItemManager::AddUsingShopItem from [Server]Map/ShopItemManager.cpp:2775-2781.
- Adds modern/include/mxh/server/add_using_shop_item.hpp: free function add_using_shop_item_decision(row, dw_item_index, already_present) -> decision { status, entry }. The data plane captures the 3 mutually exclusive failure modes (KeyZero / AlreadyPresent / Ok) plus the constructed UsingShopItemEntry that the legacy code would have written into the pool + table. The orchestrator applies the decision via ShopItemManager::add_using_item(entry).
- 4 tests in modern/tests/unit/server/add_using_shop_item_test.cpp: zero-key reject, already-present reject, ok + key recorded, key-may-differ-from-icon (legacy: AddShopItem.ShopItem.ItemBase.wIconIdx is the canonical key but the legacy signature accepts an arbitrary dwItemIndex).
- See commit 61cf965c.
### D4.35 CheckEndTime side-effect dispatcher (2026-08-06)

- D4.35 - 1:1 port of the per-row side-effect chain that legacy CShopItemManager::CheckEndTime applies to each expired shop item in [Server]Map/ShopItemManager.cpp (steps: DiscardItemAttempt + BumpDup + BroadcastUseEnd + ShopItemDeleteToDB + LogItemMoney in legacy order).
- Adds modern/include/mxh/server/check_end_time_side_effect.hpp: free function check_end_time_side_effect(row, player_id, dup_slot) -> vector<CheckEndTimeStep>. Each step carries the kind + w_icon_idx + item_pos + db_idx + dup_slot the legacy code would have read out of the SHOPITEMWITHTIME row. The orchestrator applies each step to its respective subsystem (ITEMMGR / ShopItemManager / NetBase / DBThread / LogManager) without re-reading the legacy body.
- 4 tests in modern/tests/unit/server/check_end_time_side_effect_test.cpp: 4-step chain when dup_slot==None + 5-step chain with BumpDup inserted after DiscardItemAttempt + db_idx flows through every step + all 5 dup slots (Incantation/Charm/Herb/Sundries/PetEquip) are accepted.
- See commit 1730a8c2.
### D4.36 PutOnAvatarItem side-effect dispatcher (2026-08-06)

- D4.36 - 1:1 port of the tail-side-effects that legacy CShopItemManager::PutOnAvatarItem applies after mutating avatar[] in [Server]Map/ShopItemManager.cpp:1792-1922 (the SEND_AVATARITEM_INFO broadcast gated by ItemPos != 0 + the CalcAvatarOption(bCalcStats) recompute).
- Adds modern/include/mxh/server/put_on_avatar_side_effect.hpp: free function put_on_avatar_side_effect_plan(transition, dw_item_index) -> plan { effects }. Maps the AvatarEquipTransition.send_avatar_info + recalculate_avatar_option + calc_stats flags into the ordered PutOnAvatarSideEffect list (BroadcastAvatarInfo, RecomputeAvatarOption) so the orchestrator can apply them via PACKEDDATA_OBJ->QuickSend + the runtime player stat recompute hook without re-reading the legacy body.
- 5 tests in modern/tests/unit/server/put_on_avatar_side_effect_test.cpp: broadcast-only when send_info true + recompute-only when recalc true + both when both flags set + empty plan when no flags + calc_stats=false propagates to both effects.
- See commit 669665f9.
### D4.37 TakeOffAvatarItem side-effect dispatcher (2026-08-06)

- D4.37 - 1:1 port of the tail-side-effects that legacy CShopItemManager::TakeOffAvatarItem applies after mutating avatar[] in [Server]Map/ShopItemManager.cpp:1925-2021 (the SEND_AVATARITEM_INFO broadcast + CalcAvatarOption(bCalcStats) recompute).
- Adds modern/include/mxh/server/take_off_avatar_side_effect.hpp: free function take_off_avatar_side_effect_plan(transition, dw_item_index) -> plan { effects }. Maps the AvatarEquipTransition.send_avatar_info + recalculate_avatar_option + calc_stats flags into the ordered TakeOffAvatarSideEffect list (BroadcastAvatarInfo, RecomputeAvatarOption) so the orchestrator can apply them via PACKEDDATA_OBJ->QuickSend + the runtime player stat recompute hook without re-reading the legacy body. Mirrors the PutOnAvatarItem dispatcher except the broadcast is unconditional on success in the legacy code.
- 5 tests in modern/tests/unit/server/take_off_avatar_side_effect_test.cpp: both effects on success + broadcast-only + recompute-only + empty when no flags + calc_stats=false propagation.
- See commit 01360adf.
### D4.38 PutSkinSelectItem side-effect dispatcher (2026-08-06)

- D4.38 - 1:1 port of the success-path side-effect chain that legacy [Server]Map/ItemManager.cpp:6534-6544 + 6562-6574 applies after PutSkinSelectItem / RemoveEquipSkin succeed (InitSkinDelay + StartSkinDelay + CharacterSkinInfoUpdate + SEND_SKIN_INFO broadcast).
- Adds modern/include/mxh/server/skin_select_side_effect.hpp: free function skin_select_success_side_effect_plan() -> plan { send_broadcast, effects[3] }. The plan captures the 3-step chain (StartSkinDelay, CharacterSkinInfoUpdate, BroadcastSkinInfo) so the orchestrator can route each step to the runtime player / DBThread / PACKEDDATA_OBJ subsystems without re-reading the legacy body.
- 2 tests in modern/tests/unit/server/skin_select_side_effect_test.cpp: success plan emits 3 steps in legacy order + plan is idempotent across calls.
- See commit 5b22e6f8.### D4.27 DiscardAvatarItem data plane (2026-08-06)

- D4.27 - 1:1 port of the data-plane half of legacy CShopItemManager::DiscardAvatarItem(WORD ItemIdx, WORD ItemPos) from [Server]Map/ShopItemManager.cpp.
- Adds modern/include/mxh/server/discard_avatar_item.hpp: AvatarEquipRow struct (Position + Item[24] mask, mirrors legacy AVATARITEM) + kAvatarDefaultFillStart/End constants (12..18 = [Weared_Hair, Weared_Gum)) + free function discard_avatar_item(equip, idx, current_avatar) -> new_avatar. The 4 no-op conditions are: missing equip, position out of range, avatar[pos] != item_idx, position at sentinel.
- Adds modern/src/server/discard_avatar_item.cpp: implementation with the clear-and-default-fill loop matching the legacy collapsed iteration.
- 11 tests in modern/tests/unit/server/discard_avatar_item_test.cpp: 4 no-op conditions + matching-slot clear + default-fill zeros->ones + non-zero masked slots preserved + all masked slots preserved + non-weared slots untouched + Weared_Gwun boundary check + position-at-Max sentinel test.
- See commit a96c6547: server: D4 discard_avatar_item data plane - AvatarEquip clear + default-fill + 11 tests.

### D4.26 IsPetSummonItem/IsTitanCallItem/IsTitanEquipItem + GetItemKindType + playtime_decrement (2026-08-06)

- D4.26 - 1:1 ports of three ItemKind predicates + GetItemKindType + the CheckEndTime PLAYTIME decrement arithmetic, all as pure data-plane free functions.
- Adds modern/include/mxh/server/item_kind_predicates.hpp: is_pet_summon_item (ItemKind == QUEST_PET or SHOP_PET), is_titan_call_item (ItemKind == TITAN_PAPER), is_titan_equip_item (ItemKind & TITAN_EQUIP_UMBRELLA = bit 7), all with nullptr -> false.
- Adds modern/include/mxh/server/get_item_kind_type.hpp: get_item_kind_type(info, kind_out, type_out) - writes (ItemKind, ItemType) on hit, zeros on miss.
- Adds modern/include/mxh/server/playtime_decrement.hpp: playtime_decrement(remtime, last_check, now) -> {new_remaintime, elapsed_clamped_ms} with the 30s clamp + underflow-to-zero arithmetic.
- 22 tests across modern/tests/unit/server/item_kind_predicates_test.cpp (10) + get_item_kind_type_test.cpp (3) + playtime_decrement_test.cpp (9).
- See commits e5b27b1c (item_kind_predicates) + 2dab9b8d (get_item_kind_type) + c26c1d72 (playtime_decrement).

### D4.25 IsDupItem data plane (2026-08-06)

- D4.25 - 1:1 port of legacy CItemManager::IsDupItem(WORD wItemIdx) from [Server]Map/ItemManager.cpp. Pure data plane: returns true iff the inventory is allowed to stack duplicates of the item.
- Adds modern/include/mxh/server/is_dup_item.hpp: legacy item-kind constants (13 always-dup kinds: YOUNGYAK*4 + EXTRA*7 + SHOP_CHARM + SHOP_HERB), Sundries exception (SimMek / CheRyuk / Shout idx), 30-entry Incantation non-dup list (TownMove15 / MemoryMove15 / MemoryMoveExtend* / ShowPyoguk* / Tracking* / Extend* / CharacterSlot*), Skin no-dup (NOMALCLOTHES_SKIN / COSTUME_SKIN), free function is_dup_item + inline wrappers is_rare_option_item (legacy IsRareOptionItem) and is_option_item (legacy IsOptionItem).
- Adds modern/src/server/is_dup_item.cpp: implementation with incantation_is_non_dup helper for the 30-entry switch.
- 19 tests in modern/tests/unit/server/is_dup_item_test.cpp: 4 always-dup kinds + 7 extra kinds + shop charm/herb + Sundries 5 branches + 30-entry Incantation non-dup list + Incantation LimitLevel+SellPrice gate + Skin 3 branches + NullInfo + UnknownKind + 2 precedence tests + 3 IsRareOptionItem/IsOptionItem wrapper tests.
- See commits 535742c2 (16 tests) + 56637dfb (3 wrapper tests).


### D4.24 AddDupParam/DeleteDupParam/IsDupAble data plane (2026-08-06)

- D4.24 - 1:1 port of legacy CShopItemManager::AddDupParam / DeleteDupParam / IsDupAble from [Server]Map/ShopItemManager.cpp:2286-2620. Splits the legacy per-bit OR / XOR-when-both-set / block semantics across 5 dup categories (charm / herb / incantation / sundries / pet_equip) into a pure data plane.
- Adds modern/include/mxh/server/dup_param.hpp: DupCounters struct (5 counter fields) + DupParamIndices struct (5 index fields) + DupParamLookup virtual class (returns 0 by default) + SundrySideEffects struct (set/clear bStreetStall booleans) + 5 inner namespaces (charm_dup / herb_dup / incantation_dup / sundries_dup / pet_equip_dup) with bit constants matching [CC]Header/CommonGameDefine.h:2874-2930 (DONTDUP_* flag values 1:1 with legacy) + 3 free functions (add_dup_param, delete_dup_param, is_dup_able).
- Adds modern/src/server/dup_param.cpp: implementation with apply_or (OR, idempotent for Add) and apply_delete (XOR-when-both-set, clears bit only if both counter and dup_param have it). Sundries street-stall side effect captured in SundrySideEffects struct.
- 33 tests in modern/tests/unit/server/dup_param_test.cpp: 32 named tests covering AddDupParam (NoIndices, CharmBitsOrIntoCounter, CharmOrIsIdempotent, IncantationBitsOrIntoCounter, SundriesBitsOrAndTriggersStreetStall, SundriesBitsWithoutStreetStallHasNoSideEffect, PetEquipBitsOrIntoCounter, AllFiveCategoriesTogether, LookupReturnsZeroIsNoOp) + DeleteDupParam (NoIndices, CharmBitsClearedWhenBothSet, CharmBitsUntouchedWhenCounterEmpty, IncantationBitsClearedWhenBothSet, SundriesClearsAndTriggersClearStreetStall, SundriesNoClearWhenCounterEmpty, PetEquipBitsClearedWhenBothSet, AllFiveCategoriesTogether) + IsDupAble (EmptyCountersAllDupEnabled, CharmOverlapBlocks, CharmNoOverlapAllows, HerbOverlapBlocks, IncantationOverlapBlocks, SundriesOverlapBlocks, PetEquipOverlapBlocks, NoIndicesReturnsTrue, LookupReturnsZeroIsTrue, AnyCategoryOverlapBlocks, NoOverlapReturnsTrueWithAllIndicesSet, DisjointBitsAllAcrossFiveCategoriesReturnsTrue).
- See commit 529e1843: server: D4 AddDupParam/DeleteDupParam/IsDupAble data plane - 5 dup categories + 33 tests.

### D4.23 CalcAvatarOption data plane (2026-08-06)

- D4.23 CalcAvatarOption - 1:1 port of legacy CShopItemManager::CalcAvatarOption() from [Server]Map/ShopItemManager.cpp:2024-2183 (avatar stat accumulator).
- Adds modern/include/mxh/game/avatar_item_option.hpp: AvatarItemOption struct (25 fields, 39 bytes under pack(1), byte-identical to legacy AVATARITEMOPTION) + AvatarSlot enum (eAvatar_Hat..WeeMaxAmgi, Max=24) + EAvatarCount=24 constant.
- Adds modern/include/mxh/server/avatar_calc.hpp: calc_avatar_option(avatar[24], ItemManager&) - walks 24 slots, skips wIconIdx<2 (cosmetic empty/base-skin) and ItemInfo misses, accumulates 25 field deltas with the legacy >0 / ==1 predicates and uint16->uint8 narrow casts.
- 24 tests in modern/tests/unit/game/avatar_calc_test.cpp: layout size + enum indices + empty/cosmetic-base/unknown slots + single-field accumulator + 25-field all-fields pass + multi-slot accumulator + bKyungGong/NaeruykspendbyKG flag-only semantics + LimitSimMek truthy predicate + uint16->uint8 truncation + 24-slot full sweep.
- See commit 7e49a5a5: server: D4.23 CalcAvatarOption data plane + AVATARITEMOPTION struct + 24 tests.

### D4.22 CheckAvatarEndtime data plane (2026-08-06)

- D4.22 CheckAvatarEndtime - 1:1 port of legacy CShopItemManager::CheckAvatarEndtime() from [Server]Map/ShopItemManager.cpp:1142-1180 (avatar-only counterpart to CheckEndTime, runs unconditionally every tick the avatar-event loop calls it).
- Adds modern collect_avatar_realtime_expired(PackedTime now, out) data plane function (header in modern/include/mxh/server/shop_item_manager.hpp, impl in modern/src/server/shop_item_manager.cpp). Predicate is byte-identical to collect_realtime_expired (SellPrice == eShopItemUseParam_Realtime AND curtime > EndTime); the side-effect chain (DiscardItem + SendMsgDwordToPlayer(MP_ITEM_SHOPITEM_USEEND) + ShopItemDeleteToDB + LogItemMoney) is the orchestrator's responsibility and is documented in the implementation comment.
- 8 tests in modern/tests/unit/server/shop_item_manager_test.cpp ShopItemManagerCheckAvatarEndTime suite: NoRows / PlaytimeRowsSkipped / FutureEndTime / EqualEndTime / PastEndTime / MixedRows / DoesNotMutateTable / MatchesCollectRealtimeExpiredExactly.
- See commit 668ec566: server: D4.22 CheckAvatarEndtime data plane + 8 tests (legacy 4-step side-effect chain documented).

### build: mxh_compat_tests linker fix (2026-08-06)

- mxh_compat_tests previously failed to link because item_effects.cpp (in mxh_compat) references mxh::game::ItemManager::try_get, which lives in src/game/item_manager.cpp. The test target linked mxh_compat + gtest_main but not mxh_game, leaving 81 tests Not Run.
- Fix: add mxh_game to mxh_compat_tests target_link_libraries in modern/tests/unit/CMakeLists.txt. 1 line change. 81 previously Not-Run compat tests now run + pass.
- See commit f385b8d7: build: fix mxh_compat_tests linker error - link mxh_game (ItemManager::try_get).


### E2 T2 wire golden round-trip coverage (2026-08-06)

- E2 T2 wire golden round-trip - å¯¹æ‰€æœ‰ 84 ä¸ª golden .bin æ–‡ä»¶å„åŠ ä¸€ä¸ª TEST(WireFormatGolden, RoundTrip_X)ï¼Œè§£æžå­—èŠ‚åˆ° Packet structï¼Œå† wire_bytes() é‡ç¼–ç ï¼Œæ–­è¨€å­—èŠ‚å®Œå…¨ç›¸åŒã€‚ 11 ä¸ªåŽŸ wire format æµ‹è¯• + 84 ä¸ª golden round-trip = 95 tests, 95 PASSED, 0 FAILEDã€‚ é”ä½çŽ°ä»£ wire encoder ä¸Ž legacy [Server]*/4DyuchiNET_Latest å­—èŠ‚çº§ 1:1ã€‚
- åŠ  find_golden_dir() helper (server/golden ç­‰è·¯å¾„å€™é€‰) + filesystem/fstream/string å¤´æ–‡ä»¶ã€‚

### E1 T1 parse test subdir expansion (2026-08-06)

- E1 T1 parse test subdir expansion - deploy Server/ + QuestScript/ + PlayDH non-Client (top-level + EffectScript/Map/QuestScript/SkillArea) å…¨éƒ¨ read_mh_bin parse ok = true + size match + 256MB é™ã€‚
- æ€»æ•° 89 -> 268 parse test entries across 5 suites (MxhResourceParse 60 + MxhResourceParseClient 29 + MxhResourceParseServer 108 + MxhResourceParseQuestScript 6 + MxhResourceParsePlayDh 65). 7 ä¸ª 14-byte placeholder stub è¢«è¿‡æ»¤ (header.file_size = 0 ä¼šå¤±è´¥ EXPECT_GT)ã€‚
- ä¸Ž SHA-256 manifest é”å®šçš„ 303 records ä¸¥æ ¼å¯¹é½ï¼šæ¯ä¸ªæœ‰ SHA-256 è®°å½•çš„ .bin éƒ½æœ‰å¯¹åº” parse testã€‚



---

## æˆªæ­¢ 2026-08-06 å·²å®Œæˆé¡¹

### E1 T1 èµ„æºå­—èŠ‚çº§éªŒè¯ (2026-08-05)

- E1 T1 payload SHA-256 byte-level verification - 3 ä¸ª manifest (deploy + PlayDH + PlayDH/Client), 146 records, 117 .bin æ–‡ä»¶ SHA-256 + byte-size + header.type é”å®šã€‚3/3 VerifyManifest_* PASSã€‚
- E1 T1 mechanical expansion - 89 deploy/Resource .bin å…¨éƒ¨ read_mh_bin parse ok = true + size match + 256MB é™ã€‚

### Phase D çŽ©æ³•/æ•°å€¼ 1:1

- D1.1 + D1.2 + D1.3 SkillList.bin parser + call-site - SkillInfo æ‰©åˆ° 60+ å­—æ®µ 1:1 legacy SKILLINFO, SkillListParser è§£ç  MHFile packed-text, 1817 entries load æˆåŠŸ 0 parse_errorsã€‚MapHandler ç”¨çœŸ SkillList.bin è€Œéž 4-skill hardcodeã€‚
- D2 BattleFactory 1:1 - 14 compute_* å‡½æ•° (critical/decisive/player-phy/player-attr/exp/point/phy-defence/received-dmg/monster-phy/monster-attr/titan-phy/titan-attr) + 13 legacy_* æµ‹è¯•ã€‚
- D3.x QuestManager (D3.5-D3.7):
  - D3.5 QuestScriptLine ç«¯åˆ°ç«¯ parser - 9 event tokens + 6 limit tokens + &Limit + @Event + *Execute layout
  - D3.6 QuestTrigger runtime evaluator - 10 tests, EndQuest fix (args[0] 1-arg subquest ç´¢å¼•)
  - D3.7 QuestScript subquest block parser - 12 tests
  - D3.3 / D3.4 QuestExecute data-plane dispatch (5+4 QuestGroup + 4 dispatch tests)
- D5 MurimNet 1:1 wire - Channel + PlayRoom + 60 åè®®ä»£ç  + 9 wire serializer + 7 short wire + runtime.broadcast_chat sink + MurimNetCrypt + 139 testsã€‚
- D6.1 æ•°å€¼ baseline - 7 OBJECTKIND / 6 MonsterAI / 14B MonsterTotalInfo / 22B ItemBase / 124 æ§½ / 2728B ItemTotalInfo / 3775B GameInAck hero payload / 4 ItemEffect å…¬å¼ / 3 default MonsterTemplate å…¨éƒ¨ 1:1 é”æ­»ã€‚
- D6.2 Distributer recipient + party-exp pure decisions - legacy null-party tie no-op, 50/50 tie, float allocation, zero-send gate. 10 new tests, 45/45 focused PASSã€‚
- D6.2 FieldBossMonsterManager 1:1 - 19 tests (channel é…ç½®/å‡ºæ€ª/é‡ç½®/å•é£ž/é™é¢‘)ã€‚
- D6.2 ChooseOne tie-break + SetPlusTotalDamage += semanticsã€‚
- D6.3 BossMonsterManager 1:1 - 19 tests (register/spawn/erase/damage/live_count)ã€‚
- D6.4 cMonsterSpeechManager 1:1 - 15 testsã€‚
- D6.5 ExperienceCurve 1:1 - 15 tests (CharacterExpPoint.bin reader, add_exp é™ä¸€æ¬¡å‡çº§)ã€‚
- D6.6 state_param + summon_monster + titan_item_manager 1:1 - 31 testsã€‚
- D6.7 distribute_network_msg_parser + common_network_msg_parser 1:1 - 19 testsã€‚
- D6.x ItemList.bin 1:1 parser - ITEM_INFO 77 fields, 56/60 token MHFile packed-text, 9887 rows 0 parse errorsã€‚Shared decode_mhfile_text_payload helperã€‚
- R-8 item_effects real lookup via ItemManager - ItemManager.init_from_bin / get / try_get / exists / sizeã€‚resolve_item_effect_with_manager è¯» LifeRecover / LifeRecoverRate / NaeRyukRecover / NaeRyukRecoverRateã€‚12 tests PASSã€‚
- R-8 call-site MapHandler.load_item_list - 3 testsã€‚
- B6.1 HSEL YHLibrary ABI æ ¡æ­£ - 79 crypto tests, non-virtual dtor / 2 const virtual getters / protected version/type fields / non-virtual CHSEL_STREAMã€‚

### R-2 HackShield è·¯ç”± + tick

- R-2 AgentHandler HackShield routing - cat==HackShield (67) é€šè¿‡ mxh::server::parse_hackshield_message, conn_user_levels_ + conn_hs_states_ + hackshield_disconnect_pending_, superuser (>=5) ç«‹å³ send_guid_reqã€‚10 testsã€‚
- R-2.1 AgentHandler auto-populate user_level from chr_log_info (proto=9) - 3 testsã€‚
- R-2.2 AgentHandler::tick_hackshield() server-side periodic recheck - 4 tests (empty/grace/non-superuser/mixed)ã€‚
- bug: Crypt::encrypt_crc/decrypt_crc è¿”å›ž 0 è€Œéž HselStream CRC char - fixed + 4 testsã€‚

### MurimNet 1:1 å…³é”®

- commit 83d53c1b - MurimNetChannel.for_each_channel + PlayRoomManager.for_each_room + SendMsg_ChannelList/PlayRoomList
- commit ce931c51 - wire-byte serializer (MSG_CHANNEL_BASEINFO/MSG_PLAYROOM_BASEINFO/#pragma pack(1) 9 size é”å®š)
- commit 845294ec - SendMsg_ChannelList/PlayRoomList 8 tests (size = GetMsgLength, Category=38, Protocol=34/35, title 63 å­—èŠ‚)

### Phase C UI 1:1 port - ä¸»è¦ batches (Batch 1.1-1.10 + Batch 2.1-2.80)

å®Œæ•´ 109 dialog åˆ—è¡¨ (æŒ‰ phase commit ç´¢å¼•):
- Batch 1.1 - 1.10 (10 dialog): cNumberPadDialog / cPartyWarDialog / cWantedDialog / cMPNoticeDialog / cReviveDialog / cMiniFriendDialog / cGuildInviteDialog / cChatOptionDialog / cStallKindSelectDlg / cDebugDlg (331 tests)
- Batch 2.1 - 2.7 (7 dialog): cMPGuageDialog / cAlertDlg / cChinaAdviceDlg / cLoadingDlg / cKeySettingTipDlg / cIntroReplayDlg / cNameChangeNotifyDlg (210 tests)
- Batch 2.32 - 2.80: 30+ dialog (cCharChangeDlg, cBailDialog, cChaseInputDialog, cObjectStateManager ç­‰, æ¯ dialog 10-25 tests)
- æ”¶å£ fixes: cChatOption checked event bits, cListDialogEx legacy event bits, cPushupButton sticky click state, cListItem m_maxLine=0 inconsistencyã€‚

### Phase B æœåŠ¡ç«¯ E2E

- B.2.1 CLoginState - 38B legacy payload å­—èŠ‚çº§
- B.2.2 CCharSelectState - 8B ListSyn + 889B ListAck + è‡ªåŠ¨é€‰ç¬¬ä¸€ä¸ª + SelectSyn
- B.2.3 CInGameState - 3775B SEND_HERO_TOTALINFO GameInAck
- B.2.4 state machine çœŸæŽ¥ net äº‹ä»¶
- B.2.5 MoxianClientE2E headless tool - CreateProcessW å¯ 3 server + C++ çŠ¶æ€æœºè·‘ Login -> CharSelect -> InGame (60s timeout)

### Phase A å®¢æˆ·ç«¯

- f80f6e0 - MoxianClient skeleton (DX11 + WinMain + 4 sprite + cImage <-> IDISpriteObject)
- CMainGame 1:1 port - eGAMESTATE 0..9 byte-for-byte + 9 state stub + CMainTitle (14 å­—æ®µ 1:1 surface + Init è¯» MHVerInfo.ver)

---

## è€ ROADMAP ä¸­çš„ Phase 0-12 å™ªéŸ³

å‡¡æ—§è¯æåŠ"Phase 0-12 å·²å®Œæˆ""35.1% complete""Qoder IDE Quest" ç­‰, å…¨éƒ¨åºŸå¼ƒã€‚å‚è§ ROADMAP Â§6ã€‚


### D3 quest event dispatcher bridge (2026-08-06)

- D3 runtime bridge - æŠŠ legacy CQuestManager::AddQuestEvent -> CQuestGroup::AddQuestEvent -> sub-condition åŒ¹é… 1:1 ç§»æ¤åˆ° modern runtimeã€‚æ–°å¢ž mxh::server::QuestEvent {kind,target_id,delta} + dispatch_quest_event(QuestLog&, const QuestEvent&)ï¼ŒæŒ‰ QuestLog é¡ºåºéåŽ†ï¼Œåªä¿®æ”¹ Accepted çŠ¶æ€ï¼ˆä¸Ž legacy group å¯¹ terminal quest çš„ guard ä¸€è‡´ï¼‰ï¼Œæ¯ä¸ªåŒ¹é… sub-quest ç´¯åŠ  delta å¹¶ clamp åˆ° targetï¼Œè¿”å›ž std::vector<QuestEventChange> {quest_id, previous_state, state, updated_subs}ã€‚
- 6 ä¸ªæ–°è¡Œä¸ºé”å®šæµ‹è¯•ï¼šDispatchQuestEvent.UpdatesEveryMatchingActiveQuestInLogOrder / CompletesQuestWhenFinalConditionMatches / UpdatesAllMatchingSubsWithinQuest / IgnoresNonMatchingAndTerminalQuests / ZeroDeltaAndNoneKindAreNoOps / DoesNotMutateOtherQuestsã€‚mxh_quest_manager_tests: 37 -> 43 tests PASS, 0 regressions; æœåŠ¡ç«¯ ctest 313 é¡¹ä¸­ 2 flaky (LoginServerFixture Weather/GuildFieldWar) é‡è·‘ 100% é€šè¿‡ã€‚
- è§ commit 9512a082: server: D3 quest event dispatcher bridge (legacy AddQuestEvent->QuestGroup semantics)ã€‚



### D4 use_shop_item_decision data plane (2026-08-06)

- D4.20 use_shop_item_decision - æŠŠ legacy CShopItemManager::UseShopItem ä¸­*å®žé™…*è½åˆ° SHOPITEMBASE å­—èŠ‚ä¸Šçš„éƒ¨åˆ† 1:1 æå–ä¸º modern è‡ªç”±å‡½æ•°ï¼šInvalidIcon / ItemInfoMissing / AlreadyInUse ä¸‰é“ guard + Realtime/Playtime/Continue/SellPrice==0 å››ä¸ª BeginTime/Remaintime åˆ†æ”¯ + ItemKind åˆ° dup counter (Incantation/Charm/Herb/Sundries/PetEquip) çš„è·¯ç”±ã€‚
- æ–°å¢ž inline å¸¸é‡ SHOP_ITEM_USE_PARAM_REALTIME/PLAYTIME/CONTINUE + LEGACY_SHOP_ITEM_* (257/258/259/260/261/262/263/264/300/310ï¼Œ1:1 å¼•ç”¨ [CC]Header/CommonGameDefine.h:698-713)ã€‚
- 8 ä¸ªæ–°è¡Œä¸ºé”å®šæµ‹è¯•ï¼šRejectsEmptyIcon / RejectsMissingItemInfo / RejectsAlreadyInUse / RealtimeBranchEncodesEndTimeInRemaintime / PlaytimeBranchConvertsRarityToMilliseconds / ContinueBranchHasNoExpiry / ZeroSellPriceRoutesSundriesAndNoTimer / RoutesHerbToHerbDupã€‚mxh_shop_item_manager_tests: 91 -> 99 tests PASS, 0 regressions (ctest -R ShopItemManager|UseShopItemDecision 100% é€šè¿‡)ã€‚
- æ³¨é‡Šè§£é‡Šï¼šlegacy UseShopItem æ•´æ®µè¢« `/* ... */` æ³¨é‡Šï¼ˆ"ìž„ì‹œë¡œ ë†ˆìŒ - ì„±ëŒ€"ï¼‰ï¼Œä¸å¯ 1:1 å¤çŽ°ï¼›æœ¬å‡½æ•°åªè¦†ç›–èƒ½è½åˆ° SHOPITEMWITHTIME è¡Œçš„ guard + BeginTime/Remaintime éƒ¨åˆ†ã€‚
- è§ commit dcb05173: server: D4 use_shop_item_decision data plane (legacy UseShopItem guard + BeginTime/Remaintime)ã€‚



### D4.21 CheckEndTime realtime branch data plane (2026-08-06)

- D4.21 CheckEndTime realtime branch - æŠŠ legacy CShopItemManager::CheckEndTime ä¸­ Realtime æ¨¡å¼ (`SellPrice == eShopItemUseParam_Realtime`) çš„æ•°æ®é¢ 1:1 æå–ä¸º modern æ–¹æ³•: collect_realtime_expired(PackedTime now, out) / consume_realtime_expired(PackedTime now)ã€‚predicate ä¸Ž legacy `curtime > stTIME(Remaintime)` ä¸¥æ ¼ä¸€è‡´ï¼›åªæ‰«æ `Param == SHOP_ITEM_PARAM_STORED_TIME` çš„è¡Œï¼ˆplaytime/continue/one-shot è·³è¿‡ï¼‰ã€‚
- 7 ä¸ªæ–°è¡Œä¸ºé”å®šæµ‹è¯•: NoRowsNoExpirations / PlaytimeRowsAreSkipped / FutureEndTimeIsNotExpired / EqualEndTimeIsNotExpired / PastEndTimeIsCollected / MixedRowsSelectsOnlyStoredTimeExpired / ConsumeIsIdempotentOnAlreadyExpiredRowsã€‚mxh_shop_item_manager_tests: 99 -> 106 tests PASS, 0 regressionsã€‚
- è§ commit 3e21470e: server: D4.21 CheckEndTime realtime branch data plane (legacy SellPrice==Realtime sweep)ã€‚




### D3.8 quest_group_run_pending (2026-08-06)

- D3.8 quest_group_run_pending - ? legacy CQuestGroup::Process() ???? 1:1 ??? modern ????:?? quest_group_process(state) ? dispatch list,? (quest, event) ??? triggers_by_quest ?,??? quest ?????? trigger (legacy CQuestTrigger::OnQuestEvent ? "first matching condition wins" ??),?? std::vector<QuestGroupApplyResult> {quest_idx, subquest_idx, status, changed}?
- 7 ????????:NoEventsProducesEmptyResult / AppliesFirstMatchingTriggerAndAdvancesSubCount / SkipsQuestWithNoTriggerEntry / FiresFirstConditionMatchAndIgnoresTheRest / FansOutAcrossMultipleQuests / ConditionFailedLeavesQuestUntouched / IdempotentAfterEventsDrained?mxh_quest_group_tests: 23 -> 30 tests PASS, 0 regressions?
- ? commit 9e5ba572: server: D3.8 quest_group_run_pending (legacy CQuestGroup::Process tick link)?



### D3.9 subquest counter access (2026-08-06)

- D3.9 subquest counter access - ? legacy CQuestGroup::GetSubQuestValue + CQuestGroup::ChangeSubQuestValue ???? 1:1 ??? modern ????:get_sub_quest_value(state, questIdx, subIdx) ?? subQuestData[subIdx] ? 0xFFFFFFFFu sentinel(legacy "no such quest" marker),change_sub_quest_value(state, questIdx, subIdx, kind) ?? QUEST_VALUE_ADD/MINUS(legacy eQuestValue_Add/Minus from QuestDefines.h:128-129),Minus clamp ? 0,unknown kind ???
- ?? inline ?? QUEST_VALUE_ADD=0u / QUEST_VALUE_MINUS=1u / QUEST_SUB_QUEST_VALUE_NOT_FOUND=0xFFFFFFFFu?get_quest ? const ??????? lookup?
- 8 ????????:GetMissingQuestReturnsSentinel / GetExistingButUnsetSubQuestReturnsZero / GetReturnsStoredCount / AddIncrementsCount / MinusDecrementsCount / MinusClampsAtZero / UnknownKindIsRejected / MissingQuestReturnsFalse?mxh_quest_group_tests: 30 -> 38 tests PASS, 0 regressions?
- ? commit f5a7d89b: server: D3.9 subquest counter access (legacy GetSubQuestValue + ChangeSubQuestValue data plane)?



### D3.10 quest end lifecycle tests (2026-08-06)

- D3.10 quest end lifecycle tests - ????? quest_group_end_quest / quest_group_end_subquest ? 7 ???????,?? legacy CQuestGroup::EndQuest(questIdx, subQuestIdx) + CQuestGroup::EndSubQuest ??????:missing-quest ?? false?repeat=0 ?? complete / clear active subs / ? subQuestData?repeat=1 ?? quest ??? reset counter?idx==0 ? end_subquest ??? quest->data + quest->time?
- mxh_quest_group_tests: 38 -> 45 tests PASS, 0 regressions?
- ? commit 3616baac: server: D3.10 lock legacy EndQuest + EndSubQuest semantics (7 lifecycle tests)?





### D3.11 weapon-filtered AddCount (2026-08-06)

- D3.11 weapon-filtered AddCount - ? legacy CQuestGroup::AddCountFromWeapon + AddCountFromQWeapon ???? 1:1 ??? modern ????:add_count_from_weapon(state, questIdx, subIdx, max, requiredWeaponKind, playerWeaponKind) ? `playerWeaponKind == requiredWeaponKind` ???? quest_group_add_count;add_count_from_q_weapon(state, ...) ??????? weapon item-idx???????????? false ??? counter(legacy "???" 1:1 quirk)?
- ????????? quest_group_add_count ? max-bound clamp ??,?? + ???????
- 6 ????????:AddCountFromWeaponMismatchedKindDoesNothing / AddCountFromWeaponMatchingKindIncrements / AddCountFromWeaponClampsAtMax / AddCountFromQWeaponMismatchedItemDoesNothing / AddCountFromQWeaponMatchingItemIncrements / AddCountFromWeaponMissingQuestReturnsFalse?mxh_quest_group_tests: 45 -> 51 tests PASS, 0 regressions?
- ? commit 67dcac95: server: D3.11 weapon-filtered AddCount (legacy AddCountFromWeapon + AddCountFromQWeapon data plane)?





### D3.12 weapon-filtered TakeQuestItem (2026-08-06)

- D3.12 weapon-filtered TakeQuestItem - ? legacy CQuestGroup::TakeQuestItemFromWeapon + TakeQuestItemFromQWeapon ???? 1:1 ??? modern ????:take_quest_item_from_weapon(state, quest, sub, item, num, prob, requiredKind, playerKind) ? `playerKind == requiredKind` ?????? quest_group_take_quest_item;take_quest_item_from_q_weapon(state, ...) ????? weapon item-idx????????? false ??? m_QuestItemTable(1:1 quirk)?
- ? D3.11 ? AddCount weapon-filtered ??:????? `*TAKEQUESTITEMFW` / `*TAKEQUESTITEMFQW` QuestScriptLoader execute ??,?? runtime hook (QUESTMGR->TakeQuestItem) ? caller ???
- 5 ????????:TakeQuestItemFromWeaponMismatchedKindDoesNothing / TakeQuestItemFromWeaponMatchingKindInserts (+1 sub increment + m_QuestItemTable ????) / TakeQuestItemFromQWeaponMismatchedItemDoesNothing / TakeQuestItemFromQWeaponMatchingItemInserts (+1 sub increment) / TakeQuestItemFromWeaponMissingQuestStillInsertsItem (legacy "? item table ? ChangeSubQuestValue" 1:1 quirk, add_count return void-cast)?mxh_quest_group_tests: 51 -> 56 tests PASS, 0 regressions?
- ? commit c197573b: server: D3.12 weapon-filtered TakeQuestItem (legacy TakeQuestItemFromWeapon + TakeQuestItemFromQWeapon)?





### D3.13 weapon-filtered execute dispatch (2026-08-06)

- D3.13 weapon-filtered execute dispatch - ? D3.11/12 ? quest_group_add_count_from_weapon / quest_group_add_count_from_q_weapon / quest_group_take_quest_item_from_weapon / quest_group_take_quest_item_from_q_weapon ????? apply_count_execute / apply_item_execute ? QuestScriptLoader dispatch ??:
  - apply_count_execute ?? player_weapon_kind / player_weapon_item ???? 0 ??;AddCountFQW / AddCountFW ?? case ???? UnsupportedContext,????????????(?? 0 ?? "? context" -> gate ???? -> ???? Applied + changed=false,? legacy "missing context ? no-op" ??)?
  - apply_item_execute ??? player_weapon_kind / player_weapon_item;TakeQuestItemFQW / TakeQuestItemFW ?? case ??????,?? GiveItem/TakeItem/GiveMoney/TakeMoney/TakeExp/TakeSExp/RandomTakeItem ??? UnsupportedContext(?? player inventory / ?? / ?? context,??? quest_group ???)?
- ???? MissingQuestAndWeaponContextAreExplicit ??:`*ADDCOUNTFW 1 2 3` ?? 0 context ??? Applied + changed=false;???? "match context ? changed=true" ? sanity check?
- mxh_quest_execute_tests + mxh_quest_group_tests ????(77/77),0 regressions?
- ? commit e6dfaf1a: server: D3.13 extend apply_count_execute / apply_item_execute with player weapon context (D3.11/12 wiring)?



### D4 UseShopItemUpdateToDB data plane (2026-08-06)

- D4 UseShopItemUpdateToDB - 1:1 port of the SQL builders from legacy [Server]Map/MapDBMsgParser.cpp:676-706 (ShopItemUpdatetimeToDB / ShopItemUpdateUseInfoToDB / ShopItemParamUpdateToDB). Splits the legacy sprintf-then-Query monolith into a pure data plane (this header) + a DbThread::execute_async dispatch layer (orchestrator), so the SQL format is testable in isolation and byte-equal to the legacy sprintf output.
- Adds modern/include/mxh/server/shop_item_update_sql.hpp: STORED_SHOPITEM_UPDATETIME / UPDATEUSEINFO / UPDATEPARAM constants matching the legacy MapDBMsgParser.h STORED_SHOPITEM_* macros (verbatim "dbo.MP_SHOPITEM_*") plus three inline SQL builders: build_shop_item_update_time_sql(chid, item, remain_ms) -> "EXEC dbo.MP_SHOPITEM_Updatetime ..." (3 args), build_shop_item_update_use_info_sql(chid, dbidx, param, remain_ms) -> "EXEC dbo.MP_SHOPITEM_UpdateUseInfo ..." (4 args), build_shop_item_update_param_sql(chid, dbidx, param) -> "EXEC dbo.MP_SHOPITEM_UpdateParam ..." (3 args).
- 14 tests in modern/tests/unit/server/shop_item_update_sql_test.cpp: BasicFormat / ZeroArguments / LargeUnsignedArguments / StoredProcedureNameMatchesLegacy / TwoCommasImpliesThreeArguments / HasExecPrefix (for the time_sql builder); BasicFormat / FourArgumentsThreeCommas / ZeroParamAndRemain (for the use_info_sql builder); BasicFormat / ThreeArgumentsTwoCommas (for the param_sql builder); NoHexPrefix / NoLeadingZeros / Deterministic (cross-builder invariants).
- See commit bbebdb18: server: D4 UseShopItemUpdateToDB data plane - 3 SQL builders + 14 tests.
### D4 CalcShopItemOption data plane (2026-08-06)

- D4 CalcShopItemOption - 1:1 port of legacy CShopItemManager::CalcShopItemOption() from [Server]Map/ShopItemManager.cpp:1246-1691 (the shop-item stat accumulator). Splits the legacy monolithic function into a pure data plane (this commit) + an orchestrator half (legacy m_pPlayer->SetExtraSlotCount / STATSMGR->CalcCharStats / etc. hooks stay in the orchestrator).
- Adds modern/include/mxh/game/shop_item_option.hpp: ShopItemOption struct (38 fields, 124 bytes under pack(1), byte-identical to legacy SHOPITEMOPTION) + ESkinItemCount=6 constant. Static-asserts sizeof(ShopItemOption) == 124.
- Adds modern/include/mxh/server/calc_shop_item_option.hpp: inline legacy enum constants (eSHOP_ITEM_CHARM/HERB/INCANTATION/MAKEUP/DECORATION/SUNDRIES + eIncantation_SkPointRedist/StatePoint/InvenExtend/PyogukExtend/MugongExtend/CharacterSlot/MixUp) + CalcShopItemOptionInfo view struct (24 fields subset of legacy ITEM_INFO) + CalcShopItemOptionEnv base class (override event_rate_active for plustime gates) + CalcShopItemOptionStatus enum (Ok / InvalidIcon / NullStats / ItemInfoMissing) + CalcShopItemOptionSideEffects struct (new_protect_item_idx + 4 locale-bounded expansion flags) + free function calc_shop_item_option(stats, w_idx, b_add, param, info, env, current_protect_item_idx, out_side_effects).
- Adds modern/src/server/calc_shop_item_option.cpp: full implementation of the 4 branches (Incantation / Charm / Herb / Sundries) with the +/- calc = (bAdd ? 1 : -1) accumulator semantics, the clamp-to-zero underflow patterns, the locale-bounded incantation side-effect flags, and the plustime gates (legacy gEventRate / gEventRateFile replaced by env.event_rate_active).
- 57 tests in modern/tests/unit/server/calc_shop_item_option_test.cpp: layout size + Avatar/SkinItem array sizes + 4 early returns + 12 incantation branches (MixUp add/remove/clamp, GenGol/StatePoint, Life/SkillPoint, CheRyuk/ProtectCount add/remove, LimitJob/EquipLevelFree add/remove, InvenExtend side-effect) + 22 charm branches (GenGol/MinChub/Cheryuk/SimMek + Life/Shield/NeagongDamage/WoigongDamage + NaeRyuk/AddSung + LimitJob/LimitGender/LimitLevel/LimitGenGol/LimitMinChub + plustime gating on LimitCheRyuk/LimitSimMek/ItemGrade + non-plustime branches + RangeType/Plus_MugongIdx/Plus_Value/AllPlus_Kind + RangeAttackMin/RangeAttackMax/CriticalPercent/PhyDef + NaeRyukRecover + AttrFire street-stall decoration) + 4 herb branches (Life/Shield/Naeryuk with zero-clamp semantics) + 2 sundries branches (HK_LOCAL bStreetStall).
- See commit 68061fe2: server: D4 CalcShopItemOption data plane - SHOPITEMOPTION struct + 57 tests.
### T2 wire-format coverage 100% (2026-08-06)

- T2 protocol byte-level compat was at 95.1% (77/81 categories) due to a hardcoded kTotalCategories=81 in wire_format_coverage_test.cpp. The legacy Protocol.h has 77 real categories (MP_SERVER=1..MP_FORTWAR=77, MP_MAX=78), so the test was reporting 4 "missing" categories 78-81 that don't exist in the protocol.
- Fix: kTotalCategories = 77 (matches the actual legacy count). Coverage is now 100% (77/77). All 84 wire-format goldens (parse + re-encode byte-equal) cover every real category.
- See commit 8b17de1d: test: wire_format_coverage kTotalCategories 81 -> 77.
### D4 UpdateLogoutToDB data plane (2026-08-06)

- D4 UpdateLogoutToDB - 1:1 port of legacy CShopItemManager::UpdateLogoutToDB() from [Server]Map/ShopItemManager.cpp:1195-1238. Splits the per-row iteration into a pure data plane (this commit) + an orchestrator half (the legacy function calls ShopItemUpdatetimeToDB / g_DB.Query via g_pServerMsg).
- Adds modern/include/mxh/server/update_logout_to_db.hpp: UpdateLogoutToDBInfo struct (sell_price / item_kind / melee_attack_min -- 3-field subset of legacy ITEM_INFO) + UpdateLogoutRowDecision struct (Action enum: Persist / Skip / Drop + new_remaintime + new_last_check) + free function update_logout_to_db_decision(current_remaintime, last_check_time, g_cur_time, info, env). Reuses CalcShopItemOptionEnv for the plustime gate.
- Adds modern/src/server/update_logout_to_db.cpp: 1:1 implementation of the per-row legacy logic -- drop non-PLAYTIME rows, plustime gate (Charm + MeleeAttackMin + env.event_rate_active), clamp checktime to 30000 ms, decrement Remaintime with underflow-clamp-to-zero.
- 12 tests in modern/tests/unit/server/update_logout_to_db_test.cpp: RealTimeItemIsDropped / PlayTimeDecrementsByElapsedTime / RemaintimeUnderflowClampsToZero / PlustimeActiveDecrementsNormally / PlustimeInactiveSkipsRemaintimeUpdate / PlustimeRemaintimeZeroAlwaysDrops / PlustimeDisabledWhenMeleeAttackMinZero / CheckTimeCapClampsToThirtySeconds / ClockSkewYieldsZeroChecktime / PlustimeRemaintimeZeroWithActiveRateDrops + 2 integration tests.
- See commit cb30fba6: server: D4 UpdateLogoutToDB data plane - per-row PLAYTIME decrement + plustime gate + 12 tests.

















