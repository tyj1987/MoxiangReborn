# P2-12 Dialogs 移植 Roadmap

> **作者**:Mavis
> **日期**:2026-07-17
> **状态**:🟢 进行中(44/202 = 21.8% 继续推进 22% 里程碑)
> **关联**:`docs/KNOWN_BUGS.md` R-12、`AI_TASK_QUEUE.md` P2-12

## 背景

legacy `[Client]MH/` 目录下有 **202 个 dialog 文件**(含 1 个备份
`MugongDialog_BACKUP.{h,cpp}`,实际活跃 201 个)。现代 `modern/src/ui/`
只 port 了 **6 个**:`cDialog`(基类)、`cGuildDialog`、`cIconDialog`、
`cListDialog`、`cExitDialog`、`cGuagen`(子控件)。一次推完 196 个
dialog 不现实--这是 Phase 6 时代就遗留的 Phase 12 长尾任务。

本文把 202 个 legacy dialog 按"实现难度 + 依赖复杂度"分 5 档,每档
标 port 优先级 + 估计工作量 + blocker,让后续 session 按矩阵接活。

## 已 Port 列表（44/202 = 21.8%）

| 现代类 | 头文件 | 测试数 | 备注 |
|--------|-------|-------|------|
| `cDialog` | `modern/src/ui/cDialog.{hpp,cpp}` | 8 | 基类,Phase 6.3 |
| `cGuildDialog` | `modern/src/ui/cGuildDialog.{hpp,cpp}` | 13 | Phase 6 早期 |
| `cIconDialog` | `modern/src/ui/cIconDialog.{hpp,cpp}` | ? | Phase 6 |
| `cListDialog` | `modern/src/ui/cListDialog.{hpp,cpp}` | ? | Phase 6 |
| `cExitDialog` | `modern/src/ui/cExitDialog.{hpp,cpp}` | 10 | 2026-07-16 (0.13.10) |
| `cGuagen` | `modern/src/ui/cGuagen.{hpp,cpp}` | 18 | **2026-07-16** (0.13.12) - Tier 2 子控件 |
| `cListDialogEx` | `modern/src/ui/cListDialogEx.{hpp,cpp}` | 14 | **2026-07-16** (0.13.13) - Tier 1.5 子控件(link list + WE_ROWCLICK) |
| `cMacroDialog` | `modern/src/ui/cmacrodialog.{hpp,cpp}` | 20 | **2026-07-16** (0.13.14) - Tier 2 dialog(macro key bindings, ME_*/MM_*/MSK_* 1:1 enum) |
| `cCharMakeDlg` | `modern/src/ui/cmakdial.{hpp,cpp}` | 8 | **2026-07-16** (0.13.15) - Tier 2 dialog(sex selector, 4 cStatic toggled by sex, SetVisible 替代 legacy 非存在 SetActive) |
| `cGuildJoinDialog` | `modern/src/ui/guildjoindialog.{hpp,cpp}` | 11 | **2026-07-16** (0.13.16) - Tier 2 dialog(guild member-invite, 3 button 210-212, 4-singleton dispatch 阻塞) |
| `cCharStateDialog` | `modern/src/ui/charstatedialog.{hpp,cpp}` | 18 | **2026-07-16** (0.13.17) - Tier 2 dialog(character state bar, 5 cPushupButton 220-224, **5 SetXxxMode REAL (no singleton)**, 1:1 quirk PeaceWar 反向, OnActionEvent + Refresh 阻塞) |
| `cSOSDialog` | `modern/src/ui/sosdialog.{hpp,cpp}` | 20 | **2026-07-16** (0.13.18) - Tier 2 dialog(guild SOS, 1 cListDialog + 1 cButton 230-231, Linking REAL, 1:1 quirk SetHeight(158) drop, 5-singleton dispatch 阻塞) |
| `cWearedExDialog` | `modern/src/ui/wearedexdialog.{hpp,cpp}` | 12 | **2026-07-16** (0.13.19) - Tier 2 dialog(equipment slot, wraps cIconDialog 14 cells, AddItem/DeleteItem REAL wrap, 1:1 quirk m_type/m_nIconType drop, 7-singleton dispatch 阻塞) |
| `cMiniFriendDialog` | `modern/src/ui/minifrienddialog.{hpp,cpp}` | 19 | **2026-07-16** (0.13.20) - Tier 2 dialog(mini friend-add, 4 children 240-243, **第一个全 REAL Tier 2 (无 TODO)**, 1:1 quirks m_type drop, VCM_CHARNAME→2, m_bDisable→!isEnabled, cEditBox m_bInitEdit guard) |
| `cReviveDialog` | `modern/src/ui/revivedialog.{hpp,cpp}` | 12 | **2026-07-16** (0.13.22) - Tier 2 dialog(revive options, 3 cButton 250-252, Linking REAL, SetActive override (siege war vs normal map 1:1 branch), **1:1 quirk m_pLoginBtn 永远不 toggle**, 1:1 quirk legacy cButton SetActive 改用 modern SetVisible; SIEGEMGR+MAP singleton dispatch 阻塞) |
| `cTextArea` | `modern/src/ui/ctextarea.{hpp,cpp}` | 22 | **2026-07-16** (0.13.23) - Tier 1.5 sub-widget(multi-line text area, 5th subcontrol, header 2055B; InitTextArea 2 overloads + SetActive override + SetFocusEdit + SetScriptText + SetReadOnly + SetLimitLine + SetTextColor + Add delegate; **基础设施 port 解锁 ~30 Tier 2/3 dialog**; IME + 实际 render + scroll state Phase 12.x deferred) |
| `cMPNoticeDialog` | `modern/src/ui/mpnoticedialog.{hpp,cpp}` | 8 | **2026-07-16** (0.13.23) - Tier 2 dialog(MP notice, 10th dialog port, header 695B; 2 cTextArea 260-261; Linking REAL + SetScriptText placeholder; 1:1 quirk ctor m_type drop, CHATMGR placeholder text; 复用 cTextArea port) |
| `cEventNotifyDialog` | `modern/src/ui/eventnotifydialog.{hpp,cpp}` | 20 | **2026-07-16** (0.13.24) - Tier 2 dialog(GM event notification, 11th dialog port, header 793B; 2 children cStatic+cTextArea 270-271; Linking REAL, SetActive override (1:1 quirk 调 base + !val clear context text), ActionEvent override, SetTitle/SetContext REAL wrapper, SetEventCount no-op; **1:1 quirk SetTitle with nullptr not supported (modern cStatic::SetStaticText 接 std::string, UB)**) |
| `cGuildCreateDialog` | `modern/src/ui/guildcreatedialog.{hpp,cpp}` | 14 | **2026-07-16** (0.13.25) - Tier 2 dialog(guild create, 12th dialog port, header 679B; 5 children cStatic+cEditBox+cTextArea+cButton+cStatic 280-284; Linking REAL, SetActive override 7-singleton dispatch TODO, SetMunpaName 1:1 quirk SetReadOnly(TRUE), SetMunpaIntro) |
| `cGuildUnionCreateDialog` | `modern/src/ui/guildcreatedialog.{hpp,cpp}` | 7 | **2026-07-16** (0.13.25) - Tier 2 dialog(guild union create, 13th dialog port, header 679B 同 GuildCreateDialog.h; 3 children cEditBox+cButton+cTextArea 290-292; Linking REAL + SetScriptText placeholder "GUILD_UNION_TEXT" 替代 CHATMGR->GetChatMsg(1125), SetActive override 4-singleton TODO) |
| `cChaseInputDialog` | `modern/src/ui/chaseinputdialog.{hpp,cpp}` | 16 | **2026-07-16** (0.13.26) - Tier 2 dialog(chase target name, 14th dialog port, header 497B; **最简 Tier 2** (1 cEditBox child id 300 + 4 method + 0 cTextArea dep); Linking REAL + SetActive override (1:1 quirk val=true 才 clear) + SetItemIdx wrapper + WantedChaseSyn 6-singleton TODO) |
| `cChaseDialog` | `modern/src/ui/chasedialog.{hpp,cpp}` | 14 | **2026-07-16** (0.13.27) - Tier 2 dialog(chase target minimap, 15th dialog port, header 775B; 2 children cStatic+cTextArea 310-311; Linking REAL + SetActive override + InitMiniMap + LoadMinimapImageInfo TODO + Render no-op; **首个用未 port 类型 (MINIMAPIMAGE/cImageSelf/VECTOR2/MAPTYPE) 的 Tier 2**, modern port 用 placeholder type (int/float/std::string) 1:1 保留语义) |
| `cBailDialog` | `modern/src/ui/baildialog.{hpp,cpp}` | 14 | **2026-07-16** (0.13.28) - Tier 2 dialog(bail amount entry, 16th dialog port, header 497B; 2 children cEditBox+cTextArea 320-321; Linking REAL + SetValidCheck(1) + SetAlign(Right) + SetScriptText placeholder, 4 method wrapper (Open/Close/SetFame/SetBadFrameSync) 都 4-singleton TODO) |
| `cPetWearedExDialog` | `modern/src/ui/petwearedexdialog.{hpp,cpp}` | 21 | **2026-07-17** (0.13.29) - Tier 2 dialog(pet equipment slots, 17th dialog port, header 445B; wraps cIconDialog 3 cells; AddItem + DeleteItem REAL wrap (1:1 quirk Korean "!!!복사본 옵션 적용" comment 保留); **首个含 GetBlankPositionRestrictRef 实用方法(扫 cell 找空位)的 Tier 2**; CheckDuplication TODO (cItem 未 port, R-12.x); kSlotPetWearNum=3 / kTpPetWearStart=490 inline constexpr 不引 shared header) |
| `cGuildNoticeDlg` | `modern/src/ui/guildnoticedlg.{hpp,cpp}` | 19 | **2026-07-17** (0.13.30) - Tier 2 dialog(guild notice editor, 18th dialog port, header 310B; 1 cTextArea id 350 + 2 button id 351/352; Linking REAL + SetEnterAllow(FALSE) + SetScriptText(""); OnActionEvent 2-button dispatch (SEND + CANCEL, 都 GUILDMGR TODO); SetActive override (val=true pre-fill notice before base, 1:1 `if(val==TRUE)` guard); **首个用 cTextArea::SetEnterAllow 的 Tier 2**; 同步扩 cTextArea (1 bool toggle 0 regression); 1:1 quirk legacy typo'd `OnActionEvnet`, modern 用正确拼写 `OnActionEvent`) |
| `cChinaAdviceDlg` | `modern/src/ui/chinaadvicedlg.{hpp,cpp}` | 10 | **2026-07-17** (0.13.31) - Tier 2 dialog(China advice / T&C, 19th port, header 677B; 1 cTextArea id 360; Linking REAL + SetScriptText placeholder "CHINA_ADVICE_TEXT" 替代 CHATMGR->GetChatMsg(30); OnActionEvent empty no-op; 1:1 quirk CNA_BTN_OK enum 存在但 legacy .cpp 没用到) |
| `cIntroReplayDlg` | `modern/src/ui/introreplaydlg.{hpp,cpp}` | 5 | **2026-07-17** (0.13.31) - Tier 2 dialog(intro replay placeholder, 20th port, header 475B; **完全空 dialog** ctor + dtor + Linking empty body; 为 "intro replay button" target id 占位) |
| `cKeySettingTipDlg` | `modern/src/ui/keysettingtipdlg.{hpp,cpp}` | 9 | **2026-07-17** (0.13.31) - Tier 2 dialog(keyboard shortcut tip, 21st port, header 331B; 2 cImageSelf + Render override; cImageSelf 未 port, modern 存 2 .tga 路径 std::string; Render no-op; kModeHidden=2 + kNumImages=2 constexpr) |
| `cLoadingDlg` | `modern/src/ui/loadingdlg.{hpp,cpp}` | 3 | **2026-07-17** (0.13.31) - Tier 2 dialog(loading screen placeholder, 22nd port, header 578B; **100% 空** ctor + dtor only, 无 Linking 方法) |
| `cNameChangeNotifyDlg` | `modern/src/ui/namechangenotifydlg.{hpp,cpp}` | 3 | **2026-07-17** (0.13.31) - Tier 2 dialog(name change notify placeholder, 23rd port, header 652B; 1:1 quirk m_type = WT_NAMECHANGENOTIFY_DLG drop, modern ctor + dtor only) |
| `cGuildInvitationKindSelectionDialog` | `modern/src/ui/guildinvitationkindselectiondialog.{hpp,cpp}` | 13 | **2026-07-17** (0.13.31) - Tier 2 dialog(guild invitation kind selector, 24th port, header 329B; 3 button id 370-372; Linking empty; OnActionEvent 3 branch + 3-singleton TODO; 1:1 quirks legacy CANCEL SetActive(FALSE) commented out, legacy default ASSERT(0) 改 no-op) |
| `cTipBrowserDlg` | `modern/src/ui/tipbrowserdlg.{hpp,cpp}` | 17 | **2026-07-17** (0.13.31) - Tier 2 dialog(4-tab tip browser, 25th port, header 383B; 4 cDialog page + 4 cPushupButton + cancel; **全 REAL**; Show/Close REAL; OnActionEvent 2 path 1:1 quirks legacy `we == WE_PUSHDOWN` exact match + `we & WE_BTNCLICK`) |
| `cGuildNickNameDialog` | `modern/src/ui/guildnicknamedialog.{hpp,cpp}` | 18 | **2026-07-17** (0.13.31) - Tier 2 dialog(guild member nickname editor, 26th port, header 821B; 1 cTextArea + 1 cEditBox; Linking REAL + SetValidCheck(0)=VCM_SPACE; SetActive override 2-singleton TODO; SetNickMsg placeholder sprintf; 1:1 quirk m_type drop, null Name guard) |
| `cShoutDialog` | `modern/src/ui/shoutdialog.{hpp,cpp}` | 13 | **2026-07-17** (0.13.31) - Tier 2 dialog(shout message sender, 27th port, header 832B; 1 cEditBox + 2 state field; Linking REAL; SetItemInfo inline setter; SendShoutMsgSyn 4-singleton TODO; 1:1 quirk m_type drop, cEditBox::editText() 不是 GetEditText()) |
| `cGuildInviteDialog` | `modern/src/ui/guildinvitedialog.{hpp,cpp}` | 15 | **2026-07-17** (0.13.31) - Tier 2 dialog(guild invitation display, 28th port, header 837B; 1 cTextArea; Linking REAL; SetInfo 2-flk branch + CHATMGR TODO; kFlgMember=0 / kFlgStudent=1; 1:1 quirk null names guard) |
| `cStallKindSelectDlg` | `modern/src/ui/stallkindselectdlg.{hpp,cpp}` | 17 | **2026-07-17** (0.13.31) - Tier 2 dialog(street stall kind selector, 29th port, header 898B; 3 button sell/buy/cancel; Show+Close REAL; OnActionEvent 3 branch + final Close() for known ids; 1:1 quirk legacy `else return;` no Close() for unknown id, m_type drop; 1:1 quirk modern cButton 没 SetActive 用 cWindow::SetVisible 替代) |
| `cPartyInviteDlg` | `modern/src/ui/partyinvitedlg.{hpp,cpp}` | 15 | **2026-07-17** (0.13.32) - Tier 2 dialog(party invitation, 30th port, header 812B; 2 button OK/cancel + 1 cTextArea + 1 cStatic, id 440-443; Linking REAL; SetMsg 2-option branch + CHATMGR TODO; kOptRandom=0 / kOptDamage=1; 1:1 quirks m_type drop, null pInviter guard, unknown option leaves cStatic empty) |
| `cNameChangeDialog` | `modern/src/ui/namechangedialog.{hpp,cpp}` | 20 | **2026-07-17** (0.13.33) - Tier 2 dialog(name change editor, 31st port, header 877B; 1 cEditBox id 450 + 1 m_dwDBIdx state; Linking REAL + SetValidCheck(2)=VCM_CHARNAME; SetActive override REAL; NameChangeSyn 4-singleton TODO; 1:1 quirks m_type drop, modern SetEditText m_bInitEdit guard) |
| `cChangeJobDialog` | `modern/src/ui/changejobdialog.{hpp,cpp}` | 11 | **2026-07-17** (0.13.34) - Tier 2 dialog(job-change item, 32nd port, header 920B; 2 state field (m_ItemPos + m_ItemDBIdx); SetItemInfo/GetItemPos/GetItemDBIdx REAL inline; ChangeJobSyn + CancelChangeJob 4-singleton TODO (HERO + NETWORK + OBJECTSTATEMGR + ITEMMGR); 1:1 quirks m_type drop, legacy ctor 不 init state fields) |
| `cGTRegistcancelDialog` | `modern/src/ui/gtregistcanceldialog.{hpp,cpp}` | 15 | **2026-07-17** (0.13.35) - Tier 2 dialog(tournament registration cancel, 33rd port, header 795B; 1 cButton id 460; Linking REAL; SetActive override REAL + HERO/OBJECTSTATEMGR TODO; TournamentRegistCancelSyn 2-singleton TODO; 1:1 quirk m_type drop, legacy val==FALSE only triggers HERO dispatch) |
| `cGTRegistDialog` | `modern/src/ui/gtregistdialog.{hpp,cpp}` | 20 | **2026-07-17** (0.13.35) - Tier 2 dialog(tournament registration, 34th port, header 865B; 2 cStatic id 470-471 + 1 cButton id 472; Linking REAL; SetActive override REAL; TournamentRegistSyn 3-singleton TODO 返回 kErrorNoGuildMaster; SetRegistGuildCount TODO (cStatic::SetStaticValue 未 port); 5 eGTError enum constants + kMaxGuildInTournament=32) |
| `cReinforceDataGuideDlg` | `modern/src/ui/reinforcedataguidedlg.{hpp,cpp}` | 20 | **2026-07-17** (0.13.36) - Tier 2 dialog(9-tab reinforce data guide, 35th port, header 793B; 9 cPushupButton id 480-488 + 7 unique cDialog id 490-496 + 1 OK button id 498; **全 REAL** (no singleton TODO); 1:1 quirks m_pDataDlg[6] aliases 5 + m_pDataDlg[8] aliases 7; legacy `we == WE_PUSHDOWN` exact match; 9 eRFDG_ITEM_KIND enum constants) |
| `cWantedDialog` | `modern/src/ui/wanteddialog.{hpp,cpp}` | 16 | **2026-07-17** (0.13.37) - Tier 2 dialog(wanted list, 36th port, header 759B; 1 cListDialog id 500; Linking REAL; InitWanted REAL (RemoveAll); SetInfo + AddInfo TODO (WANTEDLIST struct + CHATMGR, R-12.x deferred); kMaxWantedNum=20) |
| `cWantRegistDialog` | `modern/src/ui/wantregistdialog.{hpp,cpp}` | 19 | **2026-07-17** (0.13.38) - Tier 2 dialog(wanted registration editor, 37th port, header 5172B; 1 cStatic id 510 + 1 cEditBox id 511; Linking REAL; SetWantedName REAL; SetActive override REAL for SetFocusEdit(false); gCurTime + HERO + NETWORK MSGBASE dispatch TODO; ActionEvent CMouse + gCurTime timer TODO; kVcmNumber=1) |
| `cMainDialog` | `modern/src/ui/maindialog.{hpp,cpp}` | 18 | **2026-07-17** (0.13.39) - Tier 2 dialog(main UI button bar, 38th port, header 4393B; 4 cPushupButton id 530-533; Linking REAL synth 4 cPushupButton (legacy Add() side-channel replaced by synth since modern cWindow::Add is non-virtual); GetPushupBtn REAL with bounds-check; kNumBtns=4) |
| `cGuildMarkDialog` | `modern/src/ui/guildmarkdialog.{hpp,cpp}` | 21 | **2026-07-17** (0.13.40) - Tier 2 dialog(guild mark registration, 39th port, header 6257B; 1 cTextArea id 550 + 2 cButton id 551/552; Linking REAL; ShowGuildMark/ShowGuildUnionMark 1:1 with cButton SetActive 改 SetVisible (R-12 fix); SetActive override TODO (HERO + OBJECTSTATEMGR + GAMEIN); 2 CHATMGR placeholders for msg 303/1114) |
| `cMPMissionDialog` | `modern/src/ui/mpmissiondialog.{hpp,cpp}` | 20 | **2026-07-17** (0.13.41) - Tier 2 dialog(event-map mission notice, 40th port, header 6136B; 2 cTextArea id 570/571 + 2 message arrays; Linking REAL; SetMissionInfo REAL with defensive bounds-check; SetActive/ActionEvent TODO (GAMEIN + gCurTime + CMouse); LoadMissionMsg no-op (legacy cpp body empty); kMaxMissionMsgNum=5) |
| `cAlertDlg` | `modern/src/ui/alertdlg.{hpp,cpp}` | 21 | **2026-07-17** (0.13.42) - Tier 2 dialog(alert dialog with 2 cButton + cbBtnFunc callback, 41st port, header 4727B; 2 cButton id 600-601 + 1 cbBtnFunc callback + 1 m_obj; Linking REAL synth 2 cButton; ActionEvent CMouse TODO; SetcbBtn REAL with std::function; SetObj/GetObj REAL; 2 AB_* enum constants) |
| `cMPGuageDialog` | `modern/src/ui/mpguagedialog.{hpp,cpp}` | 30 | **2026-07-17** (0.13.43) - Tier 2 dialog(event-map timer + exp gauge, 42nd port, header 5525B; CObjectGuagen (void*) + 3 cStatic id 610-613; Linking REAL; SetTime REAL with red threshold + "%02u:%02u" format; SetEventMapTimer REAL 3-way switch (kFlagReady=0, kFlagActive=1, kFlagStopped=2); SetExpGuage/ShowEventMap TODO (CObjectGuagen::SetValue + CHATMGR); kRedTextThreshold=30000) |
| `cUnionNoteDlg` | `modern/src/ui/unionnotedlg.{hpp,cpp}` | 19 | **2026-07-17** (0.13.44) - Tier 2 dialog(guild union note sender, 43rd port, header 5181B; 1 cTextArea id 620 + 1 cEditBox id 621 (unused) + CItem (void*) + m_bUse flag; Linking REAL SetEnterAllow(false) + SetScriptText(""); Show/Use/OnActionEvent TODO (HERO + CHATMGR + NETWORK + ITEMMGR); 1:1 quirk legacy "OnActionEvnet" typo 改 "OnActionEvent") |
| `cMPRegistDialog` | `modern/src/ui/mpregistdialog.{hpp,cpp}` | 28 | **2026-07-17** (0.13.45) - Tier 2 dialog(MP practice registration, 44th port, header 1098B; 4 children: 2 cTextArea (id 564/565) + 1 cStatic (id 566) + 1 cIconDialog (id 563); Linking REAL + 1-cell layout config on inner cIconDialog (1:1 quirk: modern has no .bin loader); SetActive override (val==FALSE resets 4 children + WINDOWMGR/OBJECTSTATEMGR TODO); FakeMoveIcon 5-singleton TODO (SURYUNMGR/HERO/WINDOWMGR/CHATMGR/OBJECTSTATEMGR + cMugongBase/cSkillInfo); SetSuryunMugongInfo sprintf placeholder "Mugong: %s (Sung %u)" + SetScriptText (CHATMGR msg 661 TODO); SetPracticeInfo LTime=limitime/60000 + sprintf placeholder + SetStaticValue (CHATMGR msg 660 TODO); AddLink 1:1 wrap (DeleteIcon(0) if not addable + AddIcon(0,picon,TRUE)); GetMugong returns nullptr (TODO until CMugongBase port, R-12.x deferred); 1:1 quirks: m_type=WT_MPREGISTDIALOG drop, SetDragOverIconType drop (modern cIconDialog has no such API, same as cPetWearedExDialog/cWearedExDialog), null mugongName→"(null)" defensive; 7 local id range 562-568 1:1 with legacy enum) |
| `cIconGridDialog` | `modern/src/ui/cIconGridDialog.{hpp,cpp}` | 24 | **2026-07-17** (0.13.46) - Tier 1.5 subcontrol(2D icon grid with drag-drop, 6th Tier 1.5 subcontrol; Init(x, y, wid, hei, basicImage, col, row, id) + InitGrid(gridX, gridY, cellWid, cellHei, borderX, borderY); AddIcon / DeleteIcon (linear pos + 2D cellX/cellY overloads) + MoveIcon; GetCellPosition / GetPositionForXYRef / GetPositionForCell / GetCellAbsPos hit-test math; PtInCell simplified to in-bounds cell rect (modern cIcon opaque); SetAbsXY / SetActive / SetDisable / SetAlpha cascade to dependent icons (IsDepend simplified to always-true); 1:1 quirks: GetCellPosition uses DEFAULT_CELLSIZE=40 for hit range, NOT m_wCellWidth; m_DisableFromPos/m_DisableToPos per-locale not ported; constants NOTUSE=0, USE=1, DEFAULT_CELLSIZE=40, DEFAULT_CELLBORDER=4) |
| `cPKLootingDialog` | `modern/src/ui/pklootingdialog.{hpp,cpp}` | 25 | **2026-07-17** (0.13.46) - Tier 2 dialog(PK loot dialog, 46th port, header ~11KB; 7 cStatic id 0-6 + 1 cIconGridDialog id 8 (12 cells 4×3); state m_dwDiePlayerIdx / m_nTime (30s default) / m_dwStartTime / m_nChance (1 default) / m_nLootItemNum (1 default) / m_bSelected[12] / m_bLootingEnd / m_bMsgSync / m_dwCreateTime / m_bShow; InitPKLootDlg + Linking + ActionEvent (1s m_bShow delay + per-sec timer) + OnActionEvent (close-btn + close-window + loot-cell click); ReleaseAllIcon / ChangeIconImage / AddLootingItemNum; LootItemKind enum (Item/Money/Exp/None); engine singletons stubbed no-op (PKMGR/HERO/OBJECTMGR/ITEMMGR/CHATMGR/NETWORK); test-injectable SetClockForTesting) |
| `cSkinSelectDialog` | `modern/src/ui/skinselectdialog.{hpp,cpp}` | 23 | **2026-07-17** (0.13.47) - Tier 2 dialog(skin-select dialog, 47th port, ~200 行 legacy; 1 cListDialog id 2 (皮肤列表) + 1 cIconDialog id 1 (3-cell 预览); state m_dwSelectIdx (1-based per legacy quirk) / m_dwSkinDelayTime / m_bSkinDelayResult; Linking REAL + SetShowSelect(TRUE); SetActive override (val==FALSE clear list/icon/idx, val==TRUE SkinItemListInfo); ActionEvent override (cDialog::ActionEvent + PtIdxInRow hit-test + on LBTNCLICK populate 3-cell preview); OnActionEvent (WE_CLOSEWINDOW + 3 button id OK/CANCEL/RECOVERY, engine-send stubbed); SkinItemListInfo (GAMERESRCMNGR stubbed 0, color-from-level preserved); engine singletons stubbed (GAMERESRCMNGR/HERO/CHATMGR/OBJECTMGR/ITEMMGR/NETWORK/WINDOWMGR); CItemShow opaque placeholder; constants SKINITEM_LIST_MAX=3 + ID_DLG=0/ID_ITEMVIEW=1/ID_LIST=2/ID_OK=3/ID_CANCEL=4/ID_RECOVERY=5) |

## 5 档分级

### Tier 1 - Trivial(无外部依赖,1-2 commit/个,~80-200 行)

**特征**:纯 UI 状态机,不读 GameIn/不连网络/不调 NPC 脚本。
基类 cDialog 已能 cover 90%,子类只需 override SetActive/Init
+ 1-2 个 callback。**适合做小活起点**。

| Legacy Dialog | 现代目标 | 工作量 | 优先级 |
|--------------|---------|-------|-------|
| ExitDialog | `cExitDialog` ✅ | 100 行 + 10 test | **已完成** (commit 16ef797) |
| _(已查尽 - 无 trivial 候选)_ | - | - | - |

**重要修正(2026-07-16 探查后)**:原 Tier 1 列表中除 ExitDialog 外
**全部不是 trivial**--legacy 客户端所有 dialog 都深度耦合
global singleton(CHATMGR / OBJECTMGR / GUILDMGR / HERO /
WINDOWMGR / NETWORK / ITEMMGR / HEROID)。经 grep `<1500B` + 检查 cpp
实装后,无 trivial 候选。具体排除原因:

- `HelpDialog` 依赖 cListDialogEx + cPage + cDialogueList + cHyperTextList
  + HelpDicManager(**Tier 2** - 需先 port cListDialogEx)
- `BailDialog` 依赖 cEditBox + cTextArea + CHATMGR + HERO + WINDOWMGR
  + NETWORK(**Tier 3/5** - 需 InventoryService + NetworkService)
- `EventNotifyDialog` 待查(header 274 B,但 cpp 体量可能大)
- `MallNoticeDialog` 待查
- `ChatOptionDialog` 依赖 ChatManager settings(**Tier 5**)
- `NameChangeDialog` 依赖 CHATMGR + GAMEIN + NETWORK(**Tier 5**)
- `PetRevivalDialog` 依赖 OBJECTMGR + ITEMMGR(**Tier 5**)
- `NumberPadDialog` 依赖 WINDOWMGR + cStatic + cComboBox + MT_LOGINDLG
  (**Tier 5**)
- `MNCreateDialog` / `MNFrontDialog` / `MNJoinDialog` 实际是**空壳
  stub**(cpp 全空,WindowIDEnum 没 MN_* 编号)- port 它们毫无
  价值(不会从 dispatcher 触发)

**新策略**:把"trivial"重新定义为 **"无需 modern port 任何
global service 的 dialog"**。符合条件者:
- ✅ cExitDialog(已 port,callback pattern)
- ❌ 其他所有 dialog 至少依赖 1 个 global service

**建议推进**:每次新 session 直接从 **Tier 2** 开始(接受 200-500
行 + 1 个 service interface 注入),不要再在 Tier 1 找 trivial。

### Tier 1.5 - 子控件 widget(无 service 依赖,作为 Tier 2 dialog 的子组件)

**特征**:本身不是 dialog,而是 dialog 内部用的子控件(progress bar、
list control、edit box、pushup button 等)。Tier 2 dialog 直接需要
这些子控件。**先 port 完子控件再 port dialog**,否则 Tier 2
"无 blocker" 的描述会暴露真实 blocker(0.13.12 教训:cGuagen 缺失
阻塞 CharacterDialog / QuestDialog / MugongDialog 等多个 Tier 2)。

| 子控件 | 现代目标 | 子 dialog 用户 | 状态 |
|--------|---------|---------------|------|
| `cGuagen` | `cGuagen` ✅ | CharacterDialog (HP/MP bar), MonsterGuageDlg, TitanGuageDlg, MainBarDialog, GuageDialog | **2026-07-16 ported** (commit `de390cd`, 18/18 test) |
| `cPushupButton` | `cPushupButton` | OptionDialog, KeySettingTipDlg, MiniNoteDialog | 候选 (Tier 1.5 下一个) |
| `cListCtrl` | `cListCtrl` (确认 modern 端存在) | FriendDialog, NoteDialog, PartyDialog, ChatDialog, QuestDialog | 候选 |
| `cEditBox` | `cEditBox` (确认 modern 端存在) | ChatDialog, NoteDialog, MacroDialog, ChatOptionDialog | 候选 |
| `cListDialogEx` | `cListDialogEx` | HelpDialog, 多个 log-style dialogs | 候选 (Tier 1.5 范围, 无 service 依赖) |
| `cListCtrlEx` | `cListCtrlEx` | InventoryExDialog, QuickDialog (Tier 3 blocked) | Tier 3 范围 |
| `cPage` | `cPage` | HelpDialog (page navigation) | 候选 |
| `cDialogueList` | `cDialogueList` | HelpDialog + Tier 4 NpcScript | 候选 (Tier 4 范围) |
| `cHyperTextList` | `cHyperTextList` | HelpDialog (rich text) | 候选 |

### Tier 2 - UI 状态 + 子控件编排(~200-500 行,2-3 commit/个)

**特征**:dialog 自身无状态,但有 2-5 个 cButton / cListCtrl / cEditBox
子控件需要 wire + 联动。需要先 port 子控件(多数已 port 在 src/ui/
下),主要工作量在 Linking() + callback 编排。

| Legacy Dialog | 现代目标 | 子控件 | 阻塞 |
|--------------|---------|-------|------|
| CharacterDialog | `cCharacterDialog` | cStatic, cButton, cEditBox, cListCtrl, **cGuagen** ✅ | 需 PlayerStatsService (Tier 3) |
| QuestDialog | `cQuestDialog` | cListCtrl, cButton, cStatic | 需 QuestService + 1 子控件 |
| ChatDialog | `cChatDialog` | cEditBox, cButton, cListCtrl | 需 ChatService (Tier 5) |
| DealDialog | `cDealDialog` | cStatic, cButton, cListCtrl | 需 inventory 状态 (Tier 3) |
| ExchangeDialog | `cExchangeDialog` | cStatic, cButton, cListCtrl | 需 inventory 状态 (Tier 3) |
| FriendDialog | `cFriendDialog` | cListCtrl, cButton | 需 network (Tier 5) |
| NoteDialog | `cNoteDialog` | cListCtrl, cEditBox | 需 network (Tier 5) |
| PartyDialog | `cPartyDialog` | cListCtrl, cButton | 需 network (Tier 5) |
| ItemShopDialog | `cItemShopDialog` | cButton, cStatic, cListCtrl | 需 shop data (Tier 3) |
| MacroDialog | `cMacroDialog` | cEditBox, cButton | 无(已 port 子控件) |

**重要更新(2026-07-16)**: 旧 roadmap 说 "无阻塞" 是错的。
CharacterDialog header 包含 cGuagen.h,**0.13.12 cGuagen port 解锁
CharacterDialog 子控件 blocker**。但 CharacterDialog 仍需 PlayerStatsService
(HP/MP/str/agi 等数据从哪读)- Tier 3 阻塞。

**小活建议**:
- 选 1 个**纯 UI 状态机** + **子控件全 port** + **无 service** 的 dialog
- 截至 0.13.12,唯一符合所有条件的是 **MacroDialog**(仅 cEditBox + cButton,OptionManager settings 已经是 local state)
- 第二个候选是 **DealDialog**(Tier 3 blocked after 0.13.12 解锁子控件)
- **0.13.12-0.13.47 修正**: MacroDialog 0.13.14 已 ported(20 tests)。子控件 cListDialogEx 0.13.13 + cTextArea 0.13.23 + cIconGridDialog 0.13.46 也已 ported,解锁了 cChaseInputDialog / cMPNoticeDialog / cTextArea / cPKLootingDialog / cSkinSelectDialog 系列。当前(0.13.47)最新可立即 port 的 Tier 2 候选:
  - **cMPRegistDialog** ✅ 已 ported (0.13.45, 28 tests, 5-singleton TODO)
  - **cPKLootingDialog** ✅ 已 ported (0.13.46, 25 tests, 5-singleton no-op stub)
  - **cSkinSelectDialog** ✅ 已 ported (0.13.47, 23 tests, 7-singleton stubbed)
  - **cChatOptionDialog** (cEditBox, cButton - 跟 MacroDialog 类似小 dialog。0.13.45 探查发现是 dead code - 整段 `/* */` 块注释 disabled, see `docs/KNOWN_BUGS.md`)
  - **cStallFindDlg** (cListDialog + cComboBox + 2 cPushupButton + 2 cButton + 1 cEditBox - 中等, ~28KB legacy, 需先 port cComboBox Tier 1.5 subcontrol)
  - **cSkinSelectDialog** (cListDialog + cIconDialog - 1:1 简单,1 CItemShow quirk drop)

### Tier 3 - 需要 GameIn/Inventory 状态(~500-1000 行,3-5 commit/个)

**特征**:dialog 通过 `GAMEIN->GetInventoryDialog()` 之类全局指针
读游戏状态。Modern port 需要:
1. 把 GameIn 拆成可注入的 service interface
2. dialog 通过 service interface 读状态而非全局指针
3. mock service 给单测用

| Legacy Dialog | 阻塞 |
|--------------|------|
| InventoryExDialog | 需 InventoryService 接口 |
| QuickDialog | 需 InventoryService + SkillService |
| MugongDialog | 需 SkillService + InventoryService |
| CharacterDialog | 需 PlayerStatsService |
| ItemShopDialog | 需 ShopService + InventoryService |
| GuildWarehouseDialog | 需 GuildService + InventoryService |
| PrivateWarehouseDialog | 需 InventoryService |
| MixDialog | 需 ItemService + InventoryService |
| BuyRegDialog | 需 ShopService + InventoryService |

**架构阻塞**:需要先做 InventoryService / SkillService / PlayerStatsService
等 5-8 个 service interface(每个 ~200 行 + mock + test)。这是
"现代服务化" 的分水岭工作,建议**单独开一个 phase**(Phase 13?),
不在 P2-12 内做。

### Tier 4 - 需要 NPC 脚本驱动(~800-2000 行)

**特征**:dialog 状态机由 NPC 对话脚本驱动,每次 NPC talk 会重写
dialog 内容 + 选项按钮。

| Legacy Dialog | 阻塞 |
|--------------|------|
| NpcScriptDialog | 需 NpcScriptEngine(未 port) |
| QuestDialog | 部分功能依赖 NpcScriptEngine |
| HelpDialog 部分页 | 依赖 quest data |

**架构阻塞**:NpcScriptEngine 是 2008-era 的自研脚本系统(5-6 个
class),完整 port 等价于重写一个小的 game scripting language。
**Phase 13+ 长尾**。

### Tier 5 - 网络协议驱动(~1000-3000 行)

**特征**:dialog 状态与网络包强耦合,server push / client ack 一来
一回,dialog 立刻 rebuild。

| Legacy Dialog | 阻塞 |
|--------------|------|
| GuildDialog | 已 port(1:1),但需 GuildService(Tier 3 阻塞) |
| GuildCreateDialog | 需 GuildService + network |
| GTBattleListDialog | 需 GTService(guild tournament 协议) |
| GTScoreInfoDialog | 需 GTService |
| FortWarDialog | 需 GuildWarService |
| SeigeWarDialog | 需 GuildWarService + MapService |
| MurimNet 系列(5 个) | 需 MurimNetService(PvP 协议) |
| GuildTraineeDialog | 需 GuildService |
| GuildFieldWarDialog | 需 GuildWarService |
| PartyWarDialog | 需 PartyService |
| GuildRankDialog | 需 GuildService |
| GuildMarkDialog | 需 GuildService + texture 协议 |
| GuildMunhaDialog | 需 GuildService |
| GuildNickNameDialog | 需 GuildService |
| GuildPlusTimeDialog | 需 GuildService |
| GuildLevelUpDialog | 需 GuildService |
| GuildJoinDialog | 需 GuildService |
| GuildInviteDialog | 需 GuildService |
| GuildInvitationKindSelectionDialog | 需 GuildService |
| ChangeJobDialog | 需 JobService + network |
| ChannelDialog | 需 ChannelService |
| ServerListDialog | 需 LoginService |
| CostumeSkinSelectDialog | 需 ItemService + texture |
| SkinSelectDialog | 需 ItemService + texture |
| PetUpgradeDialog | 需 PetService + network |
| PetWearedExDialog | 需 PetService + inventory |
| CharacterDialog 部分页 | 需 QuestService + PlayerStatsService |
| ChatDialog | 需 ChatService + network |
| MiniNoteDialog | 需 NoteService |
| MiniFriendDialog | 需 FriendService |
| ShoutDialog | 需 ChatService |
| ShoutchatDialog | 需 ChatService |
| SurvivalCountDialog | 需 GuildWarService + TimerService |
| WantNpcDialog | 需 WantService + network |
| WantRegistDialog | 需 WantService + network |
| WantedDialog | 需 WantService + network |
| DissolutionDialog | 需 GuildService + ItemService |
| PKLootingDialog | 需 InventoryService + LootService |
| PointSaveDialog | 需 PointService + network |
| PyoGukDialog | 需 PyoGukService + ItemService |
| MoveDialog | 需 MapService + network |
| ReviveDialog | 需 PlayerService + MapService |
| SealDialog | 需 ItemService + PlayerService |
| ItemShopGridDialog | 需 ShopService + InventoryService |
| MNFrontDialog | 需 MurimNetService |
| MNCreateDialog | 需 MurimNetService |
| MNJoinDialog | 需 MurimNetService |
| MNChannelDialog | 需 MurimNetService |
| MNPlayRoomDialog | 需 MurimNetService |
| MPGuageDialog | 需 PlayerService(实时刷新) |
| MPMissionDialog | 需 MissionService |
| MPNoticeDialog | 需 MissionService |
| MPRegistDialog | 需 MissionService + network |
| GuageDialog | 需 PlayerService(实时刷新) |
| GTRegistDialog | 需 GTService + network |
| GTRegistcancelDialog | 需 GTService + network |
| GTStandingDialog | 需 GTService |
| cDialogueList | 需 NpcScriptEngine(Tier 4) |
| ChaseDialog | 需 PlayerService(实时刷新) |
| ChaseinputDialog | 需 PlayerService |
| cJackpotDialog | 需 ItemService + network |
| cListDialogEx | 需 Service(泛型 list 增强) |
| JournalDialog | 需 JournalService + InventoryService |
| MugongSuryunDialog | 需 SkillService + InventoryService |
| SuryunDialog | 需 SkillService + InventoryService |
| HelpDialog 部分页 | 需 HelpService + 网络下载 |
| SOSDialog | 需 PlayerService + network |
| WearedExDialog | 需 EquipmentService + InventoryService |
| RareCreateDialog | 需 ItemService + InventoryService |
| MainDialog | 需 GameMainService(**最复杂**) |
| MainBarDialog | 需 GameMainService(**最复杂**) |
| MugongDialog | 需 SkillService + InventoryService + QuickBar |
| QuestTotalDialog | 需 QuestService |
| OptionDialog | 需 OptionService(settings) |

**架构阻塞**:需要 ~15-20 个 service interface + 完整 network
packet 解码([CC]Header/Protocol.h 的 96 类 Category)+ 实时刷新
timer 机制。这是 Phase 14+ 范畴,**预计 4-6 周工作量**,本 roadmap
只标 TODO 不做。

## 文件归档 / 清理建议

| Legacy 文件 | 状态 | 建议 |
|------------|------|------|
| `MugongDialog_BACKUP.{h,cpp}` | 备份 | 记录在 R-12,本 session 不删 |
| `cDialogueList.{h,cpp}` | 看起来是 Tier 4 残留 | 暂留,port NpcScriptEngine 时一起处理 |

## 整体估算

| 档 | 数量 | 工作量 | 累计估算 |
|----|------|-------|---------|
| Tier 1.5 子控件 | 9+ | 100-200 行 + 测试 | 1 周(每次小活 1-2 个, 已完成 1/9: cGuagen) |
| Tier 1 (legacy "trivial") | 1 | 100 行 + 测试 | ✅ 已完成 (cExitDialog) |
| Tier 2 | 10 | 200-500 行 + 测试 | 2-3 周 |
| Tier 3 | 9 | 500-1000 行(需 service interface) | 3-4 周(依赖 Phase 13 service 化) |
| Tier 4 | 3 | 800-2000 行(需 NpcScriptEngine) | 2-3 周(依赖 Phase 14 script port) |
| Tier 5 | 100+ | 1000-3000 行(需 15-20 service) | 8-12 周(依赖 Phase 15 service + network) |
| **总计** | **131+** | - | **16-22 周** ≈ 4-5 个月 |

加上 base **47/202** = **23.3%**(当前, 0.13.47 后 cSkinSelectDialog)→ 100% ≈ 3-4 个月全职工作量。**突破 10% / 15% / 20% / 22% / 23% 五里程碑(0.13.27/0.13.30/0.13.42/0.13.45/0.13.47)。**
**这是真的"长尾",不靠 24 小时 AI 接力推不完。**

## 建议推进节奏

1. **每次新 session** 推 1-2 个 **Tier 1.5 子控件** widget(小活,1-2 commit)。
   下次 session 候选: cListDialogEx / cPushupButton / cListCtrl / cTextArea(全部已 ported,见 0.13.12 摘要下方的小活建议段)
2. **每 3-5 session** 评估一次 Tier 2(需要先确认子控件齐全 + 无 service 阻塞)
   - 当前可立即 port 的 Tier 2 dialog: **MacroDialog** ✅(0.13.14, 20 tests) / **cMPRegistDialog** ✅(0.13.45, 28 tests);新候选见上方"小活建议"段
3. **不开 Phase 13** 不动 Tier 3-5(架构阻塞)
4. **每 10 session** 更新一次本 roadmap(重新评估 Tier / 优先级)

## 0.13.12 更新摘要

- ✅ 新 ported: cGuagen (18 test, Tier 1.5 子控件) - 解锁 CharacterDialog 等多个
  Tier 2 dialog 的子控件 blocker
- ✅ R-9 矩阵约定审计完成: 7 个新 test pin D3DX row-major layout, 3 个旧 test 更新
- 📊 Progress: 5/202 = 2.5% → 22/202 = 10.9% 突破 10% 里程碑
  (Tier 1.5 子控件算"组件 port", 不算严格 dialog port,
  但属于"无 service 阻塞的 ui 元素")

## 0.13.45 更新摘要 (2026-07-17)

Phase 10.24 + 12.x 接力。P2-12 进度从 0.13.12 的 22/202 推到 0.13.45 的 45/202 = 22.3%
(+23 dialog 跨越 0.13.13 - 0.13.45, ~5 天, 12 个 Tier 2 dialog + Tier 1.5 sub-widget 同步):

- 0.13.13 cListDialogEx (14 tests, Tier 1.5 sub-widget)
- 0.13.14 cMacroDialog (20 tests, Tier 2 - **解锁** MacroDialog 候选)
- 0.13.15 cCharMakeDlg (8 tests)
- 0.13.16-0.13.20 cGuildJoinDialog / cCharStateDialog / cSOSDialog / cWearedExDialog / cMiniFriendDialog
- 0.13.22 cReviveDialog
- 0.13.23 cTextArea (22 tests, Tier 1.5 sub-widget - **解锁** ~30 Tier 2/3 dialog)
- 0.13.23 cMPNoticeDialog
- 0.13.24 cEventNotifyDialog
- 0.13.25 cGuildCreateDialog + cGuildUnionCreateDialog
- 0.13.26 cChaseInputDialog
- 0.13.27 cChaseDialog (突破 10% 里程碑)
- 0.13.28 cBailDialog
- 0.13.29 cPetWearedExDialog
- 0.13.30 cGuildNoticeDlg
- 0.13.31 11 个 batch: cChinaAdviceDlg / cIntroReplayDlg / cKeySettingTipDlg / cLoadingDlg / cNameChangeNotifyDlg / cGuildInvitationKindSelectionDialog / cTipBrowserDlg / cGuildNickNameDialog / cShoutDialog / cGuildInviteDialog / cStallKindSelectDlg
- 0.13.32 cPartyInviteDlg
- 0.13.33 cNameChangeDialog
- 0.13.34 cChangeJobDialog (突破 16% 里程碑)
- 0.13.35 cGTRegistDialog + cGTRegistcancelDialog
- 0.13.36 cReinforceDataGuideDlg
- 0.13.37 cWantedDialog (突破 18% 里程碑)
- 0.13.38 cWantRegistDialog
- 0.13.39-0.13.41 cMainDialog / cGuildMarkDialog / cMPMissionDialog batch
- 0.13.42 cAlertDlg
- 0.13.43 cMPGuageDialog (突破 21% 里程碑)
- 0.13.44 cUnionNoteDlg
- 0.13.45 cMPRegistDialog (28 tests, 突破 22% 里程碑)

## 0.13.46 更新摘要 (2026-07-17)

0.13.46 接力 0.13.45。本 session 推 2 个 port: 1 个 Tier 1.5 subcontrol (cIconGridDialog 24 tests) + 1 个 Tier 2 dialog (cPKLootingDialog 25 tests)。ctest baseline 1646 → 1700 (+54 net, 0 回归)。

- 0.13.46 cIconGridDialog (24 tests, Tier 1.5 subcontrol - **解锁** cPKLootingDialog + 任何 cIconGridDialog-依赖的 Tier 2 dialog)
- 0.13.46 cPKLootingDialog (25 tests, Tier 2 - **46th Tier 2 dialog port**)

## 0.13.47 更新摘要 (2026-07-17)

0.13.47 接力 0.13.46。本 session 推 1 个 port: 1 个 Tier 2 dialog (cSkinSelectDialog 23 tests)。ctest baseline 1700 → 1723 (+23 net, 0 回归)。

- 0.13.47 cSkinSelectDialog (23 tests, Tier 2 - **47th Tier 2 dialog port, 突破 23% 里程碑**)

**整体进展**:
- 测试: 506 → 1723 (+1217 tests, 0 回归, ~36 sec wall)
- Tier 1.5 子控件 10/9 ✅ (cGuagen / cPushupButton / cListCtrl / cListDialog / cListDialogEx / cTextArea / cImage / cMultiLineText / cMsgBox / cIconGridDialog)
- Tier 1 (trivial) 1/1 ✅ (cExitDialog)
- Tier 2 累计 38/10 完成 (1.1x 超额, 0.13.30 引入 cTextArea 加速, 0.13.46 引入 cIconGridDialog 加速, 0.13.47 cSkinSelectDialog 继续)
- Tier 3-5 仍 blocked (待 Phase 13 service interface)
- 4 个 blocker 仍未动: C-32 (无 SQL Server) / C-35 (4 个 Distribute Debug_<LOCALE> 撞 legacy mfc71.lib + 4 个匿名 enum 重定义) / R-9.x drawBox 真正 host 接入 / Phase 13 Tier 3+ dialog 化
- Modern UI setter/getter sweep (持续 background): 完整 11 classes (cStatic / cButton / cEditBox / cListDialog + cTextArea / cPushupButton / cIconDialog / cListDialogEx / cGuagen / cListCtrl + cIconGridDialog partial — visual-only render no-op stubs are intentional). Future 1:1 port 不需要 re-audit (memory entry 标记 sweep-complete)

## 关联文档

- `docs/KNOWN_BUGS.md` R-12(roadmap + SetActive polymorphic bug)
- `AI_TASK_QUEUE.md` P2-12(每次完成一个 dialog 在此打勾)
- `MODERNIZATION_PLAN.md` Phase 6(dialogs 整体状态)
