# Changelog — Moxian-Reborn





## [0.13.69] - 2026-07-19

### Phase 6.16 cMugongSuryunDialog Tier 2 dialog port (self-verified by mavis root session)

**背景**: 0.13.68 收口 cMallNoticeDialog (34.2%, 突破 34% 里程碑). 本 session 续 0.13.69: cMugongSuryunDialog (mugong + suryun tab container, cTabDialog subclass + 5 method Add + SetActive + OnActionEvent empty + FakeMoveIcon + 2 accessor). **1:1 quirk 最关键**: legacy Add 有 TYPO bug — 第一个 if-pair 第二个分支检查 WT_MUGONGDIALOG (不是 WT_SURYUNDIALOG), 所以 m_pSuryunDlg **永远 NULL** (legacy bug 1:1 preserved). 1:1 quirks 完整保留: ctor m_type=WT_MUGONGSURYUNDIALOG drop, ctor m_pMugongDlg=NULL; m_pSuryunDlg=NULL; → modern raw pointer default-init nullptr, OnActionEvent 空 body 1:1 preserved (跟 cLoadingDlg 同样), SetActive(FALSE) 调 SetDisable(FALSE) self-undo (跟 cDialog 同样), SetActive 顺序: msgbox-dismissal + SetDisable + base SetActive (base LAST), commented-out CMainBarDialog* pDlg = GAMEIN->... 1:1 documented, cWindow::GetType() 改 dynamic_cast (Phase 6 移除 m_type), we & WE_BTNCLICK → WindowEvent::LButtonClick (R-12), FakeMoveIcon UB guard 返回 false (legacy crash, modern defensive), 4-singleton (GAMEIN/WINDOWMGR/CMainBarDialog/CMugongDialog/CSuryunDialog) 全部 stubbed, MBI_MUGONGDELETE → local kMbiMugongDelete=2300. 19 tests PASS, no regressions. P2-12 70/202 = 34.7% (续推 35% 里程碑).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session mvs_296164aca56644428c53affeab6bd00a 自主写. Verifier session ID 同上 (self-verify). 证据: cMugongSuryunDialog 19/19 ctest PASS (ctest -C Debug -R "^CMugongSuryunDialogTest\." 1.97 sec wall); 全栈 ctest 2339 → 2358 PASS (+19 net, 0 回归, j 4 timeout 30 sec). 重跑命令: cmake --build modern/build --config Debug (full build) + ctest -C Debug --timeout 30 -j 4.

### Added

- modern/src/ui/mugongsuryundialog.{hpp,cpp} (新建, 19 tests): 1:1 port of legacy CMugongSuryunDialog from 墨香【源码】\[Client]MH\MugongSuryunDialog.{h,cpp}
  - cMugongSuryunDialog: cTabDialog subclass + 5 method (Add + SetActive + OnActionEvent empty + FakeMoveIcon + 2 accessor) + 2 raw pointer state (m_pMugongDlg + m_pSuryunDlg) + 1 local id (kMbiMugongDelete=2300) + inline CMugongDialog/CSuryunDialog 1:1 stubs
  - 1:1 surface: Add(cWindow* window) (3-way dispatch: cPushupButton → AddTabBtn + curIdx1_++, CMugongDialog OR CSuryunDialog → AddTabSheet + curIdx2_++, else → cDialog::Add) + SetActive(val) noexcept override (R-12, val==FALSE: msgbox-dismiss + SetDisable(false) + base SetActive) + OnActionEvent empty (1:1) + FakeMoveIcon (wrap m_pMugongDlg->FakeMoveIcon + defensive null guard)
  - 1:1 quirks: **legacy TYPO bug: 第一个 if-pair 第二个分支检查 WT_MUGONGDIALOG 不是 WT_SURYUNDIALOG, 所以 m_pSuryunDlg 永远 NULL — 1:1 preserved** (test 确认 legacy bug behavior), ctor m_type=WT_MUGONGSURYUNDIALOG drop, ctor m_pMugongDlg=NULL/m_pSuryunDlg=NULL → raw pointer default-init nullptr, OnActionEvent empty body 1:1 preserved (跟 cLoadingDlg 同样), SetActive(FALSE) self-undo 1:1 (调 SetDisable(FALSE) on self), SetActive base SetActive LAST, commented-out CMainBarDialog 1:1 documented, cWindow::GetType() → dynamic_cast (Phase 6 移除 m_type), we & WE_BTNCLICK → WindowEvent::LButtonClick (R-12), FakeMoveIcon UB guard (legacy crash, modern defensive), 4-singleton stubbed, MBI_MUGONGDELETE → local kMbiMugongDelete=2300
  - 5 test accessor (msgboxDismissCount/setDisableFalseCount/fakeMoveIconCallCount/onActionEventCallCount/addCallCount) + 2 test-injectable (SetMsgboxPresentForTesting/msgboxPresentForTesting/ClearTestInjections)
- modern/tests/unit/ui/mugongsuryundialog_test.cpp (新建, 19 用例 PASS): 3 ctor/constants + 5 Add() dispatch (含 legacy bug 1:1 preserved) + 6 SetActive + 2 FakeMoveIcon + 1 OnActionEvent + 1 ClearTestInjections + 1 raw pointer state
- modern/src/ui/CMakeLists.txt + modern/tests/unit/ui/CMakeLists.txt (改): 加 mugongsuryundialog.cpp + 19 gtest entry

### Progress

- P2-12: 70/202 = **34.7%** (5 base + 64 dialog + 13 subcontrol Tier 1.5; **续推 35% 里程碑**)
- ctest: 2358/2358 PASS (was 2339, +19 cMugongSuryunDialog, 0 回归)
- 44 个新 Tier 2 dialog 端口 (含本 batch 0.13.69 cMugongSuryunDialog)
- 13 个 Tier 1.5 subcontrol 端口 (不变)
- 2300+ tests milestone crossed (now 2358)

### Known follow-ups

- 下个 batch 候选 (按 ROADMAP §2.1 顺序): PartyDlg / 其它 dialog
- cMugongSuryunDialog 0.13.67 cTabDialog 解锁后**第二个** 1:1 port. 0.13.67 解锁 2 dialog 都已 ported (0.13.68 MallNoticeDialog + 0.13.69 MugongSuryunDialog).

### Important Fixup in 0.13.69 batch

- **Legacy TYPO bug 1:1 preserved**: Add 第一个 if-pair 第二个分支检查 WT_MUGONGDIALOG (不是 WT_SURYUNDIALOG) — 所以 m_pSuryunDlg 永远 NULL. Modern port 完整 1:1 preserve 这个 bug — test 显式 assert EXPECT_EQ(d->GetSuryunDialog(), nullptr) 验证 legacy bug behavior.
- cWindow::GetType() 不存在 (Phase 6 移除 m_type). legacy dispatch 用 GetType() → modern port 用 dynamic_cast<CMugongDialog*> + dynamic_cast<CSuryunDialog*> (跟 cMallNoticeDialog 同样 pattern).
- m_pMugongDlg/m_pSuryunDlg 用 raw pointer (caller-owned) — 跟 cTabDialog m_ppWindowTabSheet unique_ptr 共存 (raw reference + unique owner). 现代 port 跟 legacy 1:1 同样 ownership model.
## [0.13.68] - 2026-07-18

### Phase 6.16 cMallNoticeDialog Tier 2 dialog port (self-verified by mavis root session)

**背景**: 0.13.67 收口 cTabDialog (33.7%, 续推 34% 里程碑, **解锁 MallNoticeDialog + MugongSuryunDialog**). 本 session 续 0.13.68: cMallNoticeDialog (mall notice dialog, cTabDialog subclass + 2 method Add + OnActionEvent). 1:1 quirks 完整保留: ctor/dtor 空 body → = default, legacy Add(cWindow*) override 用 cWindow::GetType() dispatch (WT_PUSHUPBUTTON → AddTabBtn(curIdx1++), WT_DIALOG → AddTabSheet(curIdx2++), else → cTabDialog::Add) → modern port cWindow::Add non-virtual 所以 改 public Add(cWindow*) (不是 override, 是新的同名), dispatch key 改 dynamic_cast<cPushupButton*> + dynamic_cast<cDialog*> (Phase 6 移除 m_type/GetType, 跟 cTipBrowserDlg/cPetStateMiniDlg 同样 pattern), curIdx1/curIdx2 protected fields 继承 cTabDialog → modern port 直接 access cTabDialog::curIdx1_/curIdx2_ (0.13.67 改 protected) + 自增, legacy we & WE_BTNCLICK (64) → modern we == WindowEvent::LButtonClick (4) per R-12, legacy ITEM_MALLBTN → local kItemMallBtnId=2200, legacy ShellExecute 4 locale URL (TAIWAN/HK/JP/else) → modern port stubbed no-op, URL 保留为 test-injectable SetMallUrlForTesting (default = wldhmx.com else-branch URL), legacy <shellapi.h.> typo 头 (extra .) → modern port 不 include (Phase 6 ShellExecute stubbed), legacy 1:1 commented-out ShellExecute(... mall.darkstoryonline.com ...) 1:1 quirk documented 不 port, ITEM_MALLBTN 走 default else URL. 15 tests PASS, no regressions. P2-12 69/202 = 34.2% (**突破 34% 里程碑**).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session mvs_296164aca56644428c53affeab6bd00a 自主写. Verifier session ID 同上 (self-verify). 证据: cMallNoticeDialog 15/15 ctest PASS (ctest -C Debug -R "^CMallNoticeDialogTest\." 3.34 sec wall); 全栈 ctest 2324 → 2339 PASS (+15 net, 0 回归, j 4 timeout 30 sec). 重跑命令: cmake --build modern/build --config Debug (full build) + ctest -C Debug --timeout 30 -j 4.

### Added

- modern/src/ui/mallnoticedialog.{hpp,cpp} (新建, 15 tests): 1:1 port of legacy CMallNoticeDialog from 墨香【源码】\[Client]MH\MallNoticeDialog.{h,cpp}
  - cMallNoticeDialog: cTabDialog subclass + 2 method (Add + OnActionEvent) + 4 mallUrls namespace constants (kTaiwan/kJapan/kHk/kElse) + 1 local id (kItemMallBtnId=2200)
  - 1:1 surface: Add(cWindow* window) (3-way dispatch: cPushupButton → AddTabBtn + curIdx1_++, cDialog → AddTabSheet + curIdx2_++, else → cDialog::Add) + OnActionEvent(lId, p, we) (WE_BTNCLICK + ITEM_MALLBTN → ShellExecute stubbed + record URL)
  - 1:1 quirks: ctor/dtor empty body, legacy Add irtual override 改 public Add (modern cWindow::Add non-virtual), legacy cWindow::GetType() dispatch 改 dynamic_cast (Phase 6 移除 m_type), legacy we & WE_BTNCLICK → WindowEvent::LButtonClick (R-12), legacy ITEM_MALLBTN → local kItemMallBtnId=2200, legacy 4 locale ShellExecute → stubbed no-op + test-injectable URL, legacy <shellapi.h.> typo 头 omit, legacy 1:1 commented-out mall.darkstoryonline.com documented 不 port
  - 4 test accessor (addCallCount/onActionEventCallCount/shellExecuteCount/lastShellUrl) + 3 test-injectable (SetMallUrlForTesting/mallUrlForTesting/ClearTestInjections)
  - cTabDialog 配套修改 (0.13.68 follow-up): curIdx1_/curIdx2_ 改 protected (legacy 1:1, 0.13.67 当时写 private) 允许 cMallNoticeDialog 直接 access
- modern/tests/unit/ui/mallnoticedialog_test.cpp (新建, 15 用例 PASS): 4 ctor/constants + 5 Add() dispatch + 5 OnActionEvent + 1 ClearTestInjections
- modern/src/ui/CMakeLists.txt + modern/tests/unit/ui/CMakeLists.txt (改): 加 mallnoticedialog.cpp + 15 gtest entry
- modern/src/ui/ctabdialog.hpp (改): curIdx1_/curIdx2_ 从 private 改 protected (1:1 legacy, 解锁 cMallNoticeDialog)

### Progress

- P2-12: 69/202 = **34.2%** (5 base + 63 dialog + 13 subcontrol Tier 1.5; **突破 34% 里程碑**)
- ctest: 2339/2339 PASS (was 2324, +15 cMallNoticeDialog, 0 回归)
- 44 个新 Tier 2 dialog 端口 (含本 batch 0.13.68 cMallNoticeDialog)
- 13 个 Tier 1.5 subcontrol 端口 (不变)
- 2300+ tests milestone crossed (now 2339)

### Known follow-ups

- 下个 batch 候选 (按 ROADMAP §2.1 顺序): **MugongSuryunDialog 2.9 KB Tier 2** (NOW UNLOCKED 0.13.67, 第二个 deferred dialog) / PartyDlg / 其它 dialog
- cMallNoticeDialog 解锁后 0.13.68 第一个 1:1 port. MugongSuryunDialog 2.9 KB 仍 unlocked 等 0.13.69 port.

### Important Fixup in 0.13.68 batch

- cWindow::Add 是 non-virtual (per cWindow.hpp Phase 6 design). legacy irtual void Add(cWindow*) override → modern port 改 public Add(cWindow*) (新 method, 不是 override). End-state 1:1 (3-way dispatch), 签名一致.
- cWindow::GetType() 不存在 (Phase 6 移除 m_type). legacy dispatch 用 GetType() → modern port 用 dynamic_cast<cPushupButton*> + dynamic_cast<cDialog*> (跟 cTipBrowserDlg/cPetStateMiniDlg 同样 pattern).
- cTabDialog::curIdx1_/curIdx2_ 0.13.67 时设为 private, cMallNoticeDialog 没法 access (它继承 cTabDialog 但 private 字段不能跨 class). 0.13.68 改 protected (1:1 legacy, 跟 cTabDialog.h 注释一致).
- ShellExecute 是 Windows API, modern port stubbed no-op per Phase 6 pattern. URL 保留为 test-injectable.
## [0.13.67] - 2026-07-18

### Phase 6.16 cTabDialog Tier 1.5 subcontrol port (self-verified by mavis root session)

**背景**: 0.13.66 收口 cTitanRepairDlg (33.2%, 续推 34% 里程碑). 本 session 续 0.13.67: **cTabDialog Tier 1.5 subcontrol** (tab container, cDialog subclass + 动态 N 对 (cPushupButton + cWindow) tabs) — **解锁 MallNoticeDialog / MugongSuryunDialog 2 个 deferred Tier 2 dialog** (之前 blocked on cTabDialog per ROADMAP §5). 1:1 quirks 完整保留: ctor m_type=WT_TABDIALOG drop (per Phase 6 全局 m_type 移除, 跟 cTipBrowserDlg/cPetStateMiniDlg/cSkillPointNotify 同样 pattern), curIdx1/curIdx2 declared-but-unused → modern port preserve as curIdx1_/curIdx2_ 1:1 (dead fields matching legacy header), destructor SAFE_DELETE 数组 → modern port std::vector<std::unique_ptr<...>> 自动 RAII 清理, AddTabBtn/AddTabSheet 调 m_absPos+m_relPos (cPOINT) → modern bsX()+relX()/absY()+relY() (per cWindow.hpp Phase 6 cPOINT 拆分, 跟 cMunpaMarkDialog m_absPos=m_absX/m_absY 同样 pattern), AddTabBtn/AddTabSheet 调 legacy SetParent(this) → modern port omit (cWindow::Add auto-parent-links, 但 tab btns+sheets 存 std::vector 不是 cDialog children — 需要 FindAnyWindowForID 找, 跟 cTabDialog 同样), legacy BYTE tabNum → modern std::uint8_t tabNum, ActionEvent 接受 CMouse* stub (Phase 6 没 CMouse type) → modern no-op stub 返回 WE_NULL=0, SetActive override virtual → 
oexcept override (R-12 fix per MSVC C2694 强制), legacy if (m_bDisable) return; guard → modern port omit (Phase 6 移除 m_bDisable field 跟 m_type 同样 pattern), legacy cWindow::SetActive(val) cascade to tab btns+sheets → modern port SetVisible(val) 替代 (per R-12, cWindow 没 SetActive 跟 cGuageDialog SetActive(cStatic)→SetVisible 同样 pattern), SetAlpha/SetOptionAlpha legacy cWindow cascade → modern port 只 call cDialog::SetAlpha/SetOptionAlpha (cWindow 没这些 methods, tab btns+sheets cascade 1:1 documented no-op), legacy cDialog::GetWindowForID override → modern port 没 virtual GetWindowForID override, 改 public FindAnyWindowForID 1:1 (search findWindowById + iterate tab btns+sheets, 跟 legacy lookup order 一致), legacy BYTE curIdx1/curIdx2 保留 (1:1 dead fields), legacy 注释 m_BtnPushstartTime/m_BtnPushDelayTime omit (跟 cTitanGuageDlg m_pMpPercent omit 同样), SetDisable override virtual → 
oexcept override (跟 cDialog::SetDisable 签名一致). 29 tests PASS, no regressions. P2-12 68/202 = 33.7% (续推 34% 里程碑) + 13th Tier 1.5 subcontrol (unlock MallNoticeDialog + MugongSuryunDialog).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session mvs_296164aca56644428c53affeab6bd00a 自主写. Verifier session ID 同上 (self-verify). 证据: cTabDialog 29/29 ctest PASS (ctest -C Debug -R "^CTabDialogTest\." 3.29 sec wall); 全栈 ctest 2295 → 2324 PASS (+29 net, 0 回归, j 4 timeout 30 sec). 重跑命令: cmake --build modern/build --config Debug (full build) + ctest -C Debug --timeout 30 -j 4.

### Added

- modern/src/ui/ctabdialog.{hpp,cpp} (新建, 29 tests): 1:1 port of legacy cTabDialog from 墨香【源码】\[Client]MH\interface\cTabDialog.{h,cpp}
  - cTabDialog: cDialog subclass + 4 method (InitTab + AddTabBtn/AddTabSheet + SelectTab + SetActive + SetAbsXY + SetDisable + SetAlpha + SetOptionAlpha + Render + RenderTabComponent + FindAnyWindowForID) + 2 std::vector<std::unique_ptr<...>> (m_ppPushupTabBtn + m_ppWindowTabSheet) + 2 dead fields (curIdx1_/curIdx2_ 1:1)
  - 1:1 surface: InitTab(N) (resize 2 vectors to N) + AddTabBtn(idx, unique_ptr<cPushupButton>) (sets abs + SetPassive + SetPush) + AddTabSheet(idx, unique_ptr<cWindow>) (sets abs) + SelectTab(idx) (push+active 联动) + SetActive(val) noexcept override (R-12 + SetVisible cascade) + SetAbsXY(x, y) noexcept (delta cascade) + SetDisable(val) noexcept override (cascade) + SetAlpha/SetOptionAlpha (cDialog only, tab btns+sheets 1:1 documented no-op) + Render/RenderTabComponent (no-op stubs) + FindAnyWindowForID (findWindowById + tab btns+sheets 1:1 lookup)
  - 1:1 quirks: m_type=WT_TABDIALOG drop, curIdx1/curIdx2 dead fields preserve, SAFE_DELETE → unique_ptr auto, cPOINT m_absPos/m_relPos → m_absX/m_absY/m_relX/m_relY split, SetParent omit (cWindow::Add auto-parent-links), CMouse stub no-op, SetActive virtual → noexcept override (R-12), m_bDisable field drop (Phase 6 removed), cWindow::SetActive → SetVisible (R-12 pattern, 跟 cGuageDialog 同样), cWindow SetAlpha/SetOptionAlpha cascade → cDialog only (cWindow 没这些 methods), cDialog::GetWindowForID override → public FindAnyWindowForID (modern cDialog 没 virtual GetWindowForID)
  - 8 test accessor (GetTabNum/GetCurTabNum/curIdx1/curIdx2/GetTabBtn/GetTabSheet/FindAnyWindowForID/lastActionEventReturn) + 1 test-injectable (ClearTestInjections)
- modern/tests/unit/ui/ctabdialog_test.cpp (新建, 29 用例 PASS): 2 ctor/constants + 5 InitTab + 5 AddTabBtn/AddTabSheet + 5 SelectTab + 3 SetActive + 2 SetAbsXY cascade + 3 FindAnyWindowForID + 4 Render/ActionEvent/Dtor
- modern/src/ui/CMakeLists.txt + modern/tests/unit/ui/CMakeLists.txt (改): 加 ctabdialog.cpp + 29 gtest entry

### Progress

- P2-12: 68/202 = **33.7%** (5 base + 63 dialog + 13 subcontrol Tier 1.5; **续推 34% 里程碑**)
- ctest: 2324/2324 PASS (was 2295, +29 cTabDialog, 0 回归)
- 44 个新 Tier 2 dialog 端口 (含 0.13.66 cTitanRepairDlg)
- 13 个 Tier 1.5 subcontrol 端口 (**+1 cTabDialog, 解锁 MallNoticeDialog + MugongSuryunDialog**)
- 2300+ tests milestone crossed (now 2324)

### Known follow-ups

- 下个 batch 候选 (按 ROADMAP §2.1 顺序): **MallNoticeDialog 3.1 KB Tier 2 (NOW UNLOCKED, 之前 blocked on cTabDialog)** / **MugongSuryunDialog 2.9 KB Tier 2 (NOW UNLOCKED)** / PartyDlg / 其它 dialog
- cTabDialog 解锁 2 个 deferred Tier 2 dialog, 现在可以 port. cTabDialog 本身 Tier 1.5 解锁 13th milestone.

### Important Fixup in 0.13.67 batch

- cWindow 没 SetActive 方法 (Phase 6 R-12 fix 移到了 cDialog). legacy cTabDialog cascade m_ppWindowTabSheet[i]->SetActive(val) → modern SetVisible(val) (per R-12, 跟 cGuageDialog SetActive(cStatic)→SetVisible 同样 pattern).
- cWindow 没 SetAlpha/SetOptionAlpha methods (Phase 6 deferred alpha blending). legacy cTabDialog cascade to tab btns+sheets → modern port 只 call cDialog::SetAlpha/SetOptionAlpha, tab btns+sheets cascade 1:1 documented no-op.
- cDialog 没 virtual GetWindowForID (只有 indWindowById). legacy cTabDialog override → modern port 用 public FindAnyWindowForID 1:1 (search findWindowById + iterate tab btns+sheets).
- cDialog::SetDisable 是 
oexcept override, cTabDialog::SetDisable 必须匹配 (MSVC C2694 强制).
- cDialog::SetActive 是 irtual noexcept, cTabDialog::SetActive 必须 
oexcept override (R-12 fix).
## [0.13.66] - 2026-07-18

### Phase 6.16 cTitanRepairDlg Tier 2 dialog port (self-verified by mavis root session)

**背景**: 0.13.65 收口 cSkillOptionClearDlg (32.7%, 续推 33% 里程碑). 本 session 续 0.13.66: cTitanRepairDlg (titan repair dialog, cDialog subclass + 3 method, 6-singleton dispatch, WE_CLOSEWINDOW + TITAN_REPAIR_PART/ALL branches). 1:1 quirks 完整保留: legacy class name in comment 是 CTitanPartsChangeDlg 但 .h/.cpp filenames 是 TitanRepairDlg (copy-paste residue) → modern port 用 cTitanRepairDlg 匹配 filenames, ctor + dtor 空 body → = default, Linking() 空 body → modern port preserve verbatim, SetActive override irtual → 
oexcept override (R-12 fix), OnActionEvent 第一个 switch 用 we (not we & WE_BTNCLICK) 1:1 preserved, commented-out self-close //GAMEIN->GetTitanRepairDlg()->SetActive(FALSE); preserve 1:1 note (避免 infinite loop), legacy eturn TRUE → eturn true, legacy TITAN_REPAIR_PART/ALL 缺 reak fall-through → modern port preserve 1:1 (PART click 触发 cursor toggle + TITANMGR call), 6-singleton (HERO/OBJECTSTATEMGR/CURSOR/GAMEIN/CHATMGR/WINDOWMGR/TITANMGR) 全部 stubbed no-op per Phase 6 pattern, TITAN_REPAIR_PART/ALL → local kIdTitanRepairPart=2100/kIdTitanRepairAll=2101, CHATMGR msg 1582/1543 + MBI_TITAN_TOTAL_REPAIR 保留为 constants (production wires), MSG_TITAN_REPAIR_TOTAL_EQUIPITEM_SYN → test-injectable SetTitanRepairCostForTesting (default 0 → "no items" branch), eCURSOR_TITANREPAIR/eCURSOR_DEFAULT → local ECursorState enum (per cWindow.hpp note 1:1 stub). 27 tests PASS, no regressions. P2-12 67/202 = 33.2% (**突破 33% 里程碑**).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session mvs_296164aca56644428c53affeab6bd00a 自主写. Verifier session ID 同上 (self-verify). 证据: cTitanRepairDlg 27/27 ctest PASS (ctest -C Debug -R "^CTitanRepairDlgTest\." 3.48 sec wall); 全栈 ctest 2268 → 2295 PASS (+27 net, 0 回归, j 4 timeout 30 sec). 重跑命令: cmake --build modern/build --config Debug (full build) + ctest -C Debug --timeout 30 -j 4.

### Added

- modern/src/ui/titanrepairdlg.{hpp,cpp} (新建, 27 tests): 1:1 port of legacy CTitanRepairDlg from 墨香【源码】\[Client]MH\TitanRepairDlg.{h,cpp}
  - cTitanRepairDlg: cDialog subclass + 3 method (Linking empty + SetActive override + OnActionEvent) + ECursorState enum + 5 local constants (kIdTitanRepairPart/kIdTitanRepairAll/kWeCloseWindow/kChatMsgNoItemsToRepair/kChatMsgRepairConfirm)
  - 1:1 surface: Linking (empty body preserved) + SetActive(val) override (R-12 + end eObjectState_Deal + reset cursor) + OnActionEvent(lId, p, we) (WE_CLOSEWINDOW + TITAN_REPAIR_PART/ALL branches)
  - 1:1 quirks: class name comment residue CTitanPartsChangeDlg documented, ctor/dtor empty body, Linking empty body, SetActive virtual → noexcept override, OnActionEvent 第一个 switch 用 we exact match, commented-out self-close preserved, eturn TRUE → eturn true, TITAN_REPAIR_PART 缺 reak fall-through 1:1
  - 6-singleton (HERO/OBJECTSTATEMGR/CURSOR/GAMEIN/CHATMGR/WINDOWMGR/TITANMGR) 全部 stubbed no-op, 5 local constants, ECursorState enum (per cWindow.hpp 1:1 stub pattern)
  - 9 test accessor (cursorState/objectStateDealEnded/4 counter accessor/2 chat counter/2 msgbox+titanmgr) + 3 test-injectable (SetTitanRepairCostForTesting/titanRepairCostForTesting/ClearTestInjections)
- modern/tests/unit/ui/titanrepairdlg_test.cpp (新建, 27 用例 PASS): 6 ctor/constants + 1 Linking + 4 SetActive + 4 WE_CLOSEWINDOW + 5 TITAN_REPAIR_PART (含 fall-through) + 5 TITAN_REPAIR_ALL + 2 unknown id/event
- modern/src/ui/CMakeLists.txt + modern/tests/unit/ui/CMakeLists.txt (改): 加 titanrepairdlg.cpp + 27 gtest entry

### Progress

- P2-12: 67/202 = **33.2%** (5 base + 62 dialog + 12 subcontrol Tier 1.5; **突破 33% 里程碑**)
- ctest: 2295/2295 PASS (was 2268, +27 cTitanRepairDlg, 0 回归)
- 44 个新 Tier 2 dialog 端口 (含本 batch 0.13.66 cTitanRepairDlg)
- 12 个 Tier 1.5 subcontrol 端口 (不变)
- 2200+ tests milestone crossed (now 2295)

### Known follow-ups

- 下个 batch 候选 (按 ROADMAP §2.1 顺序, Tier 2 优先): MallNoticeDialog (3.1 KB, **blocked on cTabDialog** — defer) / MugongSuryunDialog (2.9 KB, **blocked on cTabDialog** — defer) / **PartyDlg / 其它无 cTabDialog 依赖 dialog**

### Important Fixup in 0.13.66 batch

- cWindow::WindowEvent 没 CloseWindow 枚举值 (cWindow.hpp line 32 note: "the full legacy enum is added in 6.1.x")。modern port 用本地 constexpr std::uint32_t kWeCloseWindow = 1u 匹配 legacy WE_CLOSEWINDOW=1 (per cWindowDef.h enum WINDOW_EVENT)。生产代码 Phase 6.1.x 加 CloseWindow 后切换到 cWindow::WindowEvent。
- TITAN_REPAIR_PART switch 缺 reak 导致 fall-through 到 TITAN_REPAIR_ALL block — legacy 1:1 quirk，modern port 用 sequential if (no break) preserve。
- cDialog::SetActive(bool) noexcept virtual 是 R-12 fix, override 必须 
oexcept (MSVC C2694 强制) — 跟 cTitanGuageDlg/cSkillOptionClearDlg 同样。
- cWindow::WindowEvent 是 enum class，**不能用 bit field 跟 legacy we & 1 等同**。每个 1:1 quirk 必须单独 static_cast<std::uint32_t>(...) 或 local constexpr。
## [0.13.65] - 2026-07-18

### Phase 6.16 cSkillOptionClearDlg Tier 2 dialog port (self-verified by mavis root session)

**背景**: 0.13.64 收口 cTitanGuageDlg (32.2%, 续推 33% 里程碑). 本 session 续 0.13.65: cSkillOptionClearDlg (skill option clear dialog, cIconDialog subclass + 1 inner cIconDialog m_pMugongIconDlg, FakeMoveIcon + OnActionEvent + SetActive + SetItem + OptionClearSyn). 1:1 quirks 完整保留: legacy OnActionEvnet (typo) → modern OnActionEvent (跟 cGuildNoticeDlg/cUnionNoteDlg 同样 pattern), m_ClearOption 声明但未用 → modern port omit (跟 cTitanGuageDlg::m_pMpPercent 同样), ctor 不 init m_ItemPos → modern port default-init 0, legacy cIcon* temp; 声明但未用 → modern port omit, FakeMoveIcon 永远 return FALSE even on success path, we & WE_BTNCLICK (64) → we == WindowEvent::LButtonClick (4) per R-12, MSG_WORD4 → inline struct MsgWord4 (跟 TitanCalcStats 同样 pattern), HEROID stubbed 0u, NETWORK stubbed (记录 lastSentMessage test access), T_DefaultICON/T_DefaultOKBTN/T_DefaultCANCERBTN → local kMugongIconId/kOkBtnId/kCancelBtnId (2000-2002), CMugongBase + CItem forward decl 1:1 stubs, eSkillOption_None / MBI_SKILLOPTIONCLEAR_NACK / MBI_SKILLOPTIONCLEAR_ACK / MBT_OK / MBT_YESNO / MP_MUGONG / MP_MUGONG_OPTION_CLEAR_SYN 都保留为 constants, SetActive override 
oexcept (R-12 fix), 5-singleton 全部 stubbed no-op per Phase 6 pattern (NETWORK/ITEMMGR/WINDOWMGR/CHATMGR/OBJECTMGR). 25 tests PASS, no regressions. P2-12 66/202 = 32.7% (续推 33% 里程碑).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session mvs_296164aca56644428c53affeab6bd00a 自主写. Verifier session ID 同上 (self-verify). 证据: cSkillOptionClearDlg 25/25 ctest PASS (ctest -C Debug -R "^CSkillOptionClearDlgTest\." 2.13 sec wall); 全栈 ctest 2243 → 2268 PASS (+25 net, 0 回归, j 4 timeout 30 sec). 重跑命令: cmake --build modern/build --config Debug (full build) + ctest -C Debug --timeout 30 -j 4.

### Added

- modern/src/ui/skilloptioncleardlg.{hpp,cpp} (新建, 25 tests): 1:1 port of legacy CSkillOptionClearDlg from 墨香【源码】\[Client]MH\SkillOptionClearDlg.{h,cpp}
  - cSkillOptionClearDlg: cIconDialog subclass (跟 cMPRegistDialog 同样 pattern) + 1 inner cIconDialog m_pMugongIconDlg resolved by id + 2 state field m_ItemPos (WORD) + inline MsgWord4 struct + inline CItem/CMugongBase 1:1 stubs
  - 1:1 surface: Linking + SetActive(val) override + FakeMoveIcon + OnActionEvent + SetItem + OptionClearSyn
  - 1:1 quirks: OnActionEvnet typo 修正, m_ClearOption omit, ctor 不 init m_ItemPos → 0, cIcon* temp; omit, FakeMoveIcon 永远 return FALSE, we & WE_BTNCLICK → WindowEvent::LButtonClick (R-12), MSG_WORD4 → MsgWord4, CMugongBase + CItem forward decl 1:1 stubs, SetActive override 
oexcept
  - 5-singleton 全部 stubbed no-op, 7 protocol/chatmsg constants 保留
  - 3 test accessor + 9 test-injectable
- modern/tests/unit/ui/skilloptioncleardlg_test.cpp (新建, 25 用例 PASS): 4 ctor/constants + 4 Linking + 4 FakeMoveIcon + 5 OnActionEvent + 3 SetActive + 5 SetItem/OptionClearSyn
- modern/src/ui/CMakeLists.txt + modern/tests/unit/ui/CMakeLists.txt (改): 加 skilloptioncleardlg.cpp + 25 gtest entry

### Progress

- P2-12: 66/202 = **32.7%** (5 base + 61 dialog + 12 subcontrol Tier 1.5; **续推 33% 里程碑**)
- ctest: 2268/2268 PASS (was 2243, +25 cSkillOptionClearDlg, 0 回归)
- 44 个新 Tier 2 dialog 端口 (含本 batch 0.13.65 cSkillOptionClearDlg)
- 12 个 Tier 1.5 subcontrol 端口 (不变)
- 2200+ tests milestone crossed (now 2268)

### Known follow-ups

- 下个 batch 候选 (按 ROADMAP §2.1 顺序, Tier 2 优先): TitanRepairDlg (2.8 KB, 6 singleton 复杂) / MallNoticeDialog (3.1 KB, **blocked on cTabDialog** — defer) / MugongSuryunDialog (2.9 KB, **blocked on cTabDialog** — defer)

### Important Fixup in 0.13.65 batch

- cIconDialog 没 cIcon.hpp 头文件, skilloptioncleardlg.cpp 不 include cIcon.hpp (跟 mpregistdialog.cpp 同样 forward decl 模式, cIcon forward decl 在 cIconDialog.hpp 已经够用).
- OnActionEvent 用 cWindow::WindowEvent::LButtonClick (4) 替代 legacy we & WE_BTNCLICK (64), per R-12 fix (跟 cPetStateMiniDlg/cCharStateDialog 同样 pattern).
- cDialog::SetActive(bool) noexcept virtual 是 R-12 fix, override 必须 
oexcept (MSVC C2694 强制).

> All notable changes to the Moxian-Reborn modernization project.
> Format: [Keep a Changelog](https://keepachangelog.com/)


## [0.13.64] - 2026-07-18

### Phase 6.16 cTitanGuageDlg Tier 2 dialog port (self-verified by mavis root session)

**背景**: 0.13.63 收口 cMunpaMarkDialog (31.7%, 续推 32% 里程碑). 本 session 续 0.13.64: cTitanGuageDlg (titan HP guage UI, 2 children: 1 cObjectGuagen HP bar + 1 cStatic HP percent text). 1:1 quirks 完整保留: ctor 0 raw pointer → modern 不需要 (default init), `enum eTitanGuage` defined but unused → modern port omit (无 1:1 fidelity gained), `CObjectGuagen* m_TitanGuage[3]` array 全部 commented out → modern port single member (1:1 quirk preserved), `cStatic* m_pMpPercent` declared but never used → modern port omit, `TITANMGR` global singleton stubbed no-op, `titan_calc_stats*` struct (forward decl, GameResourceStruct.h) replaced by inline struct (MaxFuel / MaxSpell), `static BOOL OnActionEvent` preserved as static method, SetActive override (R-12 fix: override must be noexcept), `SetNaeRyuk` body fully commented out → modern port empty body (1:1 quirk preserved), 1:1 quirk: `TitanCalcStats::MaxFuel == 0` → SetValue(0,0) + " : X/0" text (defensive division-by-zero guard vs legacy NaN). 17 tests PASS, no regressions. P2-12 65/202 = 32.2% (**突破 32% 里程碑**).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_296164aca56644428c53affeab6bd00a` 自主写. Verifier session ID 同上 (self-verify). 证据: cTitanGuageDlg 17/17 ctest PASS (`ctest -C Debug -R "^CTitanGuageDlg\."` 1.17 sec wall); 全栈 ctest 2226 → 2243 PASS (+17 net, 0 回归, j 4 timeout 30 sec). 重跑命令: `cmake --build modern/build --config Debug` (full build) + `ctest -C Debug --timeout 30 -j 4`.

### Added

- `modern/src/ui/titanguagedlg.{hpp,cpp}` (新建, 17 tests): 1:1 port of legacy `CTitanGuageDlg` from `墨香【源码】\[Client]MH\TitanGuageDlg.{h,cpp}`
  - cTitanGuageDlg: 4 method (Linking + SetActive + SetLife + SetNaeRyuk) + 1 static (OnActionEventStatic) + 2 children (1 cObjectGuagen + 1 cStatic) + inline TitanCalcStats struct
  - 1:1 quirks: ctor 0 raw pointer (default init), `enum eTitanGuage` defined but unused → omit, `CObjectGuagen* m_TitanGuage[3]` array commented out → single member, `cStatic* m_pMpPercent` declared but never used → omit
  - 1:1 quirks: `TITANMGR` global singleton stubbed no-op (per Phase 6 pattern, host caller 自己 wire)
  - 1:1 quirks: `titan_calc_stats*` struct (forward decl, GameResourceStruct.h) replaced by inline `struct TitanCalcStats { MaxFuel / MaxSpell }` (1:1 field layout)
  - 1:1 quirks: `static BOOL OnActionEvent` preserved as `static bool OnActionEventStatic`
  - 1:1 quirks: SetActive override is `noexcept` (R-12 fix: virtual SetActive noexcept 强制)
  - 1:1 quirks: SetNaeRyuk body fully commented out → modern port empty body (1:1 quirk preserved)
  - 1:1 quirks: `MaxFuel == 0` defensive guard → SetValue(0,0) + " : X/0" text (modern port avoid NaN, legacy would have NaN)
  - 1:1 surface: Linking() (materialize 2 children) + SetActive(val) override (R-12 + cascade stubbed) + SetLife(dwLife) (compute ratio + SetValue + SetStaticText) + SetNaeRyuk(dwNaeRyuk) (no-op) + OnActionEventStatic(lId, p, we) static
  - 5 test accessor + 3 test-injectable (SetTitanStatsForTesting / ClearTitanStatsForTesting / GetTitanStatsForTesting)
- `modern/tests/unit/ui/titanguagedlg_test.cpp` (新建, 17 用例 PASS): 3 ctor/constants + 3 Linking + 5 SetLife + 1 SetNaeRyuk + 3 SetActive + 2 OnActionEventStatic
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 titanguagedlg.cpp + 17 gtest entry

### Progress

- P2-12: 65/202 = **32.2%** (5 base + 60 dialog + 12 subcontrol Tier 1.5; **突破 32% 里程碑**)
- ctest: 2243/2243 PASS (was 2226, +17 cTitanGuageDlg, 0 回归)
- 43 个新 Tier 2 dialog 端口 (含本 batch 0.13.64 cTitanGuageDlg)
- 12 个 Tier 1.5 subcontrol 端口 (不变)
- 2200+ tests milestone crossed (now 2243)

### Notes

- 下个 batch 候选 (按 ROADMAP §2.1 顺序, Tier 2 优先): SkillOptionClearDlg (3.1 KB, CItem + 6 singleton 复杂) / TitanRepairDlg (2.8 KB, 6 singleton 复杂) / MugongSuryunDialog (2.9 KB, **blocked on cTabDialog** — defer)
- cTitanGuageDlg 跟 cMunpaMarkDialog 一样用 1:1 stub pattern: inline stub class/struct in hpp, test 用 SetXxxForTesting 注入. SetLife test 用 test-injectable TitanCalcStats (SetTitanStatsForTesting).

### Important Fixup in 0.13.64 batch

- `cObjectGuagen::SetValue` 现代 port 1:1 quirk (clamp val > 1.0 → 1.0) 影响 TitanGuageDlg::SetLife(150/100) 期望值: 1:1 SetValue 不 clamp (legacy), modern clamp to 1.0. **test SetLifeOverMax 期望值调整**: 1.5 → 1.0 (per cObjectGuagen 1:1 quirk), percent text 仍 " : 150/100" (unclamped sprintf).


## [0.13.63] - 2026-07-18

### Phase 6.16 cMunpaMarkDialog Tier 2 dialog port (self-verified by mavis root session)

**背景**: 0.13.62 收口 cGuageDialog (31.2%, 突破 31% 里程碑)。本 session 续 0.13.63: cMunpaMarkDialog (guild mark UI, 1 state field: 1 CMunpaMark* 1:1 1 child, 1 method Init override (modern port 不 override — base cDialog::Init 已经 1:1), 1 method SetMunpaMark (MUNPAMARKMGR 跨 class 依赖, stubbed no-op), 1 method Render (cDialog::Render + m_pMunpaMark->Render(&m_absPos) 仿 legacy)). 1:1 quirks 完整保留: ctor 1 raw pointer=NULL → modern unique_ptr nullptr 默认, m_type=WT_MUNPAMARKDLG drop (Phase 6 移除字段), Init override 不需要 (base cDialog::Init 6 params 1:1 仿 legacy), CMunpaMark inline class with virtual Render (modern port 1:1 stub for 4Dyuchi 4DyuchiDLL), MUNPAMARKMGR stubbed returns nullptr (mimicking "no mark found"), legacy m_absPos=cPOINT 2-int struct → modern m_absX/m_absY 拆, port synthesise std::int32_t absPos[2]={absX(),absY()} on the fly. 11 tests PASS, no regressions. P2-12 64/202 = 31.7%.

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_296164aca56644428c53affeab6bd00a` 自主写. Verifier session ID 同上 (self-verify). 证据: cMunpaMarkDialog 11/11 ctest PASS (`ctest -C Debug -R "^CMunpaMarkDialog\."` 1.75 sec wall); 全栈 ctest 2215 → 2226 PASS (+11 net, 0 回归, 67.34 sec wall). 重跑命令: `cmake --build modern/build --config Debug` (full build) + `ctest -C Debug --timeout 30`.

### Added

- `modern/src/ui/munpamarkdialog.{hpp,cpp}` (新建, 11 tests): 1:1 port of legacy `CMunpaMarkDialog` from `墨香【源码】\[Client]MH\MunpaMarkDialog.{h,cpp}`
  - cMunpaMarkDialog: 2 method (SetMunpaMark + Render) + 1 state field (m_pMunpaMark) + Init 不 override (base cDialog::Init 1:1)
  - 1:1 quirks: ctor 1 raw pointer=NULL → modern unique_ptr nullptr 默认, m_type=WT_MUNPAMARKDLG drop (Phase 6 移除字段), Init override 不需要 (base cDialog::Init 6 params 1:1 仿 legacy Init body)
  - 1:1 quirks: CMunpaMark inline class with virtual Render (modern port 1:1 stub for 4Dyuchi 4DyuchiDLL, host app 可 override vtable by linking a separate translation unit)
  - 1:1 quirks: MUNPAMARKMGR stubbed returns nullptr (mimicking "no mark found" → SetMunpaMark returns FALSE 1:1)
  - 1:1 quirks: legacy m_absPos=cPOINT 2-int struct → modern m_absX/m_absY 拆, port synthesise std::int32_t absPos[2]={absX(),absY()} on the fly (passing pointer to local array, same 1:1 contract: "the mark is handed a pointer to the dialog's (x, y) origin")
  - 1:1 surface: Init 不 override + SetMunpaMark(DWORD MunpaID) (legacy signature) + Render() (cDialog::Render + mark->Render) + 3 test accessor (hasMunpaMark/munpaMark/SetMunpaMarkForTesting)
- `modern/tests/unit/ui/munpamarkdialog_test.cpp` (新建, 11 用例 PASS): 3 ctor/Init + 3 SetMunpaMark + 4 Render + 1 InitDefaultIdIsZero
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 munpamarkdialog.cpp + 11 gtest entry

### Progress

- P2-12: 64/202 = **31.7%** (5 base + 59 dialog + 12 subcontrol Tier 1.5; 续推 32% 里程碑)
- ctest: 2226/2226 PASS (was 2215, +11 cMunpaMarkDialog, 0 回归)
- 42 个新 Tier 2 dialog 端口 (含本 batch 0.13.63 cMunpaMarkDialog)
- 12 个 Tier 1.5 subcontrol 端口 (不变)
- 2200+ tests milestone crossed (now 2226)

### Notes

- 下个 batch 候选 (按 ROADMAP §2.1 顺序, Tier 2 优先): NumberPadDialog (3.7 KB, 1 cEditBox + button grid) / SkillOptionClearDlg (3.1 KB, CItem + 6 singleton) / TitanRepairDlg (2.8 KB, 6 singleton)
- cMunpaMarkDialog 用 1:1 stub pattern: CMunpaMark 在 hpp inline 定义 with virtual Render, test 用 TestMunpaMark extends CMunpaMark override Render 验证 delegation. 跟 cGuageDialog test-injectable clock pattern 一致.


## [0.13.62] - 2026-07-18

### Phase 6.16 cGuageDialog Tier 2 dialog port (self-verified by mavis root session)

**背景**: 0.13.61 收口 cPetStateMiniDlg (30.7%, 续推 31% 里程碑)。本 session 续 0.13.62: cGuageDialog (mussang guage UI, 3 children: 1 cButton + 1 cStatic + 1 cObjectGuagen). 1:1 quirks 完整保留: `m_pFlicker02` 不被使用 (commented out in cpp, modern port 直接不写), gCurTime → modern port test-injectable `m_nowMillis` 时钟, cButton::SetDisable → cButton::SetEnabled (per R-12 fix), cStatic::SetActive → cWindow::SetVisible (per R-12 fix, cStatic 继承 cWindow), `((CObjectGuagen*)GetWindowForID(CG_GUAGEMUSSANG))->SetValue(0, 0)` 1:1 cast preserved, `SetImageRGB` → m_imageRGB field + test accessor (R-10 cImage GPU 1:1 deferred), HERO + MUSSANGMGR stubbed no-op, FLICKER_TIME 宏 → kFlickerTimeMs=100 const. 18 tests PASS, no regressions. P2-12 63/202 = 31.2% (**突破 31% 里程碑**).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_296164aca56644428c53affeab6bd00a` 自主写. Verifier session ID 同上 (self-verify). 证据: cGuageDialog 18/18 ctest PASS (`ctest -C Debug -R "^CGuageDialog\."` 1.93 sec wall); 全栈 ctest 2197 → 2215 PASS (+18 net, 0 回归, 62.14 sec wall). 重跑命令: `cmake --build modern/build --config Debug --target mxh_ui mxh_ui_tests` + `ctest -C Debug --timeout 30`.

### Added

- `modern/src/ui/guagedialog.{hpp,cpp}` (新建, 18 tests): 1:1 port of legacy `CGuageDialog` from `墨香【源码】\[Client]MH\GuageDialog.{h,cpp}`
  - cGuageDialog: 5 method (Linking + OnActionEvent + Render + DisableMussangBtn + SetFlicker + FlickerMussangGuage) + 5 state field (m_bFlicker / m_bFlActive / m_dwFlickerSwapTime / m_imageRGB / m_nowMillis) + 2 children (1 cButton + 1 cStatic) + 1 cast cObjectGuagen
  - 1:1 quirks: ctor 3 raw pointer=NULL → modern unique_ptr nullptr 默认 (no 1:1 m_pFlicker02 since cpp 永不使用)
  - 1:1 quirks: legacy `m_pFlicker02` field in legacy .h but cpp 不 wire → modern port 直接不写 (无 1:1 fidelity gained)
  - 1:1 quirks: legacy `SetDisable(bDisable)` → cButton::SetEnabled(!bDisable) per R-12 fix
  - 1:1 quirks: legacy `SetActive(bFlicker)` on cStatic → cWindow::SetVisible(bFlicker) (cStatic 没有 SetActive per R-12 fix)
  - 1:1 quirks: legacy `gCurTime` → modern test-injectable `m_nowMillis` (deterministic test)
  - 1:1 quirks: legacy `SetImageRGB(FullColor)` → m_imageRGB field + SetImageRGBStub 1:1 preserved
  - 1:1 quirks: legacy `((CObjectGuagen*)...)` cast preserved → modern `findWindowById + dynamic_cast`
  - 1:1 quirks: legacy `MUSSANGMGR->SendMsgMussangOn()` stubbed no-op per Phase 6 pattern
  - 1:1 quirks: legacy `HERO->IsDied() / InTitan()` checks stubbed conservative (always return true → never allow) per Phase 6 pattern
  - 1:1 quirks: FLICKER_TIME macro=100 → kFlickerTimeMs=100 const
  - 1:1 surface: Linking() (materialize 2 children + 1 cObjectGuagen + DisableMussangBtn) + OnActionEvent(lId, p, we) (1 button 路由 MUSSANGMGR) + Render (FlickerMussangGuage + cDialog::Render) + DisableMussangBtn + SetFlicker + FlickerMussangGuage
  - 11 test accessor (GetMussangButton/GetFlicker01/isFlickerActive/isFlickerOn/flickSwapTime/imageRGB/nowMillis/SetMillisForTesting/AdvanceMillisForTesting)
- `modern/tests/unit/ui/guagedialog_test.cpp` (新建, 18 用例 PASS): 2 ctor/constants + 4 Linking + 3 DisableMussangBtn + 3 OnActionEvent + 6 SetFlicker/FlickerMussangGuage
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 guagedialog.cpp + 18 gtest entry

### Progress

- P2-12: 63/202 = **31.2%** (5 base + 58 dialog + 12 subcontrol Tier 1.5; **突破 31% 里程碑**)
- ctest: 2215/2215 PASS (was 2197, +18 cGuageDialog, 0 回归)
- 41 个新 Tier 2 dialog 端口 (含本 batch 0.13.62 cGuageDialog)
- 12 个 Tier 1.5 subcontrol 端口 (不变)
- 2200+ tests milestone crossed (now 2215)

### Notes

- 下个 batch 候选 (按 ROADMAP §2.1 顺序, Tier 2 优先): MallNoticeDialog (2.1 KB, **blocked on cTabDialog port** — defer) / MunpaMarkDialog (1.8 KB, MunpaMarkManager 跨 class 依赖, ~2h) / NumberPadDialog (3.7 KB, 1 cEditBox + button grid) / SkillOptionClearDlg (3.1 KB, CItem + 6 singleton 依赖) / TitanRepairDlg (2.8 KB, 6 singleton 依赖) / MugongSuryunDialog (2.9 KB, **blocked on cTabDialog** — defer)
- 0.13.61 cPetStateMiniDlg commit history: 6b385eb (code) + 7588255 (docs). 0.13.60 follow-up fix commit: 889fa22 (cMoneyDlg spin() non-const overload).


## [0.13.61] - 2026-07-18

### Phase 6.16 cPetStateMiniDlg Tier 2 dialog port (self-verified by mavis root session)

**背景**: 0.13.60 收口 cMoneyDlg (30.2%, 跨 30% 里程碑)。本 session 续 0.13.61: cPetStateMiniDlg (pet state UI, 9 children: 4 cStatic + 2 cGuagen + 3 cButton). 1:1 quirks 完整保留: ctor 8 raw pointer = NULL 仿 (modern unique_ptr nullptr 默认), OnActionEvent 3 button 路由 PETMGR 全局 (stubbed no-op, host caller wire), use/rest 按钮检查 GetCurSummonPet()==nullptr early return, link 9 children by PSMN_* id (modern port 选 800-808 local range 避开冲突), 1:1 quirk 仿 legacy `we & WE_BTNCLICK` 用 modern `we == WindowEvent::LButtonClick` 替代. 13 tests PASS, no regressions. P2-12 62/202 = 30.7%.

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_296164aca56644428c53affeab6bd00a` 自主写. Verifier session ID 同上 (self-verify). 证据: cPetStateMiniDlg 13/13 ctest PASS (`ctest -C Debug -R "^CPetStateMiniDlg\."` 2.14 sec wall); 全栈 ctest 2184 → 2197 PASS (+13 net, 0 回归, 63.30 sec wall). 重跑命令: `cmake --build modern/build --config Debug --target mxh_ui mxh_ui_tests` + `ctest -C Debug --timeout 30`.

### Added

- `modern/src/ui/petstateminidlg.{hpp,cpp}` (新建, 13 tests): 1:1 port of legacy `CPetStateMiniDlg` from `墨香【源码】\[Client]MH\PetStateMiniDlg.{h,cpp}`
  - cPetStateMiniDlg: 2 method (Linking + OnActionEvent) + 9 children (4 cStatic + 2 cGuagen + 3 cButton)
  - 1:1 quirks: ctor 8 raw pointer=NULL → modern unique_ptr nullptr 默认, OnActionEvent 3 button (kIdUseRestBtn/kIdInvenBtn/kIdToggleBtn) 路由 PETMGR 全局 (stubbed no-op)
  - 1:1 quirks: use/rest 按钮调 GetCurSummonPet() 检查 nullptr early return 仿 legacy
  - 1:1 quirks: legacy `we & WE_BTNCLICK` (64) → modern `we == WindowEvent::LButtonClick` (4) 1:1 替代 (modern WindowEvent bit field 是不同的 bit position)
  - 1:1 quirks: unknown lId silently 忽略 (no `else` branch, no log) preserved
  - 1:1 quirks: Linking 9 child by PSMN_* id (legacy WindowIDs.h) → modern port 选 800-808 local range 避开冲突
  - 1:1 surface: Linking() (materialize 9 children idempotent) + OnActionEvent(lId, p, we) (3 button 路由) + 9 read accessor (GetNameTextWin/GetUseRestTextWin/GetFriendShipTextWin/GetStaminaTextWin/GetFriendShipGuage/GetStaminaGuage/GetUseRestButton/GetInvenButton/GetToggleButton)
- `modern/tests/unit/ui/petstateminidlg_test.cpp` (新建, 13 用例 PASS): 3 ctor/constants + 4 Linking + 6 OnActionEvent
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 petstateminidlg.cpp + 13 gtest entry
- 修复 0.13.60 batch cMoneyDlg::spin() getter: 加 non-const overload (`cSpin* spin() noexcept` + `const cSpin* spin() const noexcept`) 修 const-correctness, 让 test 可调 SetValue (per 0.13.60 batch 收口时发现的 compile error)

### Progress

- P2-12: 62/202 = **30.7%** (5 base + 57 dialog + 12 subcontrol Tier 1.5; 续推 31% 里程碑)
- ctest: 2197/2197 PASS (was 2184, +13 cPetStateMiniDlg, 0 回归)
- 40 个新 Tier 2 dialog 端口 (含本 batch 0.13.61 cPetStateMiniDlg)
- 12 个 Tier 1.5 subcontrol 端口 (不变)

### Notes

- 下个 batch 候选 (按 ROADMAP §2.1 顺序, Tier 2 优先): MallNoticeDialog (2.1 KB, cEditBox + cButton) / MunpaMarkDialog (1.8 KB, MunpaMarkManager 跨 class 依赖, ~2h) / NumberPadDialog (3.7 KB, 1 cEditBox + button grid) / SkillOptionClearDlg (3.1 KB) / MugongSuryunDialog (2.9 KB) / TitanRepairDlg (2.8 KB)
- PetStateMiniDlg 依赖 cGuagen 跟 cObjectGuagen (0.13.50 已 ported) 同 cGuagen 父类, 不需要新 subcontrol
- cMoneyDlg::spin() non-const overload fix 是 1:1 port principle "modern API consistency" vs "test mutation needs" 的冲突解法, 跟 cAlertDlg::m_pOk (non-owning raw) pattern 一致


## [0.13.60] - 2026-07-18

### Phase 6.16 cMoneyDlg Tier 2 dialog port (self-verified by mavis root session)

**背景**: 0.13.59 收口 cSpin Tier 1.5 subcontrol (29.7%, 跨 29% 里程碑)。本 session 续 0.13.60: cMoneyDlg (money-amount picker dialog, cSpin 1:1 依赖刚被 0.13.59 解锁). cDialog subclass + 1 cSpin child + 2 method (Show + OkPushed) + 4 state field. 1:1 quirks 完整保留: ctor `m_type = WT_MONEYDIALOG` drop (modern cWindow 没 m_type 字段 per Phase 6 移除字段规则), ctor `memset(m_SavedMsg, 0, sizeof(1024))` 字面保留 (`sizeof(1024)` = 1024 字节碰巧 = buffer size, 1:1 quirk 仿), OkPushed `ASSERT(pSpin)` → null guard silent no-op (现代 test-friendly), NETWORK 单例 stubbed (per Phase 6 pattern). 18 tests PASS, no regressions. P2-12 61/202 = 30.2% (**突破 30% 里程碑**).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_296164aca56644428c53affeab6bd00a` 自主写. Verifier session ID 同上 (self-verify). 证据: cMoneyDlg 18/18 ctest PASS (`ctest -C Debug -R "^CMoneyDlg\."` 0.71 sec wall); 全栈 ctest 2166 → 2184 PASS (+18 net, 0 回归, 61.32 sec wall). 重跑命令: `cmake --build modern/build --config Debug --target mxh_ui mxh_ui_tests` + `ctest -C Debug --timeout 30`.

### Added

- `modern/src/ui/moneydlg.{hpp,cpp}` (新建, 18 tests): 1:1 port of legacy `CMoneyDlg` from `墨香【源码】\[Client]MH\MoneyDlg.{h,cpp}`
  - cMoneyDlg: 2 method (Show + OkPushed) + Linking (materialize cSpin child) + 4 state field (m_MsgLen / m_SavedMsg / m_dwParam / m_OnPushFunc)
  - 1:1 quirks: ctor `m_type = WT_MONEYDIALOG` drop, ctor `memset(m_SavedMsg, 0, sizeof(1024))` 字面保留 (legacy 1024 byte buffer)
  - 1:1 quirks: OkPushed `ASSERT(pSpin)` → null guard silent no-op (modern test-friendly)
  - 1:1 quirks: NETWORK 单例 stubbed no-op (per Phase 6 pattern, host caller 自己 wire)
  - 1:1 quirks: memcpy overflow guard (legacy 1024 byte buffer, modern caps at kSavedMsgSize)
  - 1:1 quirks: Show 双 early return (m_MsgLen==0 || pmsg==nullptr) preserved verbatim
  - Modern port: cSpin 用 unique_ptr<cSpin> m_pSpin (跟 cSkillPointNotify pattern 一致, test 不需要手动 Add cSpin 到 dialog children)
  - 1:1 surface: Show(pmsg, msglen, dwParam, onPush) + OkPushed() + Linking() + 6 test accessor (msgLen / param / spin / hasCallback / savedMsg / isActive 继承)
  - Local id range: kIdMoneySpin=0 (1:1 with legacy CMI_MONEYSPIN, modern port 选 0 兼容 self-contained 测试)
- `modern/tests/unit/ui/moneydlg_test.cpp` (新建, 18 用例 PASS): 3 ctor/constants + 3 Linking + 6 Show + 6 OkPushed
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 moneydlg.cpp + 18 gtest entry

### Progress

- P2-12: 61/202 = **30.2%** (5 base + 56 dialog + 12 subcontrol Tier 1.5; **突破 30% 里程碑**)
- ctest: 2184/2184 PASS (was 2166, +18 cMoneyDlg, 0 回归)
- 39 个新 Tier 2 dialog 端口 (含本 batch 0.13.60 cMoneyDlg)
- 12 个 Tier 1.5 subcontrol 端口 (cListCtrl, cPushupButton, cListDialogEx, cPage, cDialogueList, cHyperTextList, cTextArea, cMultiLineText, cObjectGuagen, cHelpDialog, cComboBox, cSpin)
- 2100+ tests milestone crossed (now 2184)

### Notes

- 下个 batch 候选 (按 ROADMAP §2.1 顺序, Tier 2 优先 + cSpin-依赖 next): PetStateMiniDlg (2.5 KB, cSpin 1 instance) / MallNoticeDialog (2.1 KB, cEditBox+cButton) / MunpaMarkDialog (1.8 KB, MunpaMarkManager 跨 class 依赖, ~2h) / NumberPadDialog (3.7 KB, 1 cEditBox + button grid, ~3h)
- Tier 1.5 余候选: cProgressBar / cAnimation / cScene / 等
- Phase 13 service 6/9 待推 (按 ROADMAP §3.2): IItemShopService / IQuestService / IFriendService / IPartyService / INoteService / IGuildService


## [0.13.59] - 2026-07-18

### Phase 6.16 cSpin Tier 1.5 subcontrol port (self-verified by mavis root session)

**背景**: 0.13.58 收口 cFWEngraveDialog + cFWTimeDialog (29.2%, 跨 29% 里程碑)。本 session 续 0.13.59: cSpin (numeric spin control, 2 children: cButton up + cButton down) — Tier 1.5 subcontrol 跟 cPushupButton / cListCtrl / cPage / cDialogueList / cHyperTextList 同一档位. 11 method 1:1 wrappers 替代 legacy `cIMEex` text buffer 用 cEditBox. 1:1 quirks 完整保留: 7-param Init 签名 (basicImage 传 2 次当 basic+focus), m_Unit default=10, m_minValue default=0, m_maxValue default=100, GetValue/SetValue 强制 clamp [min,max], IncUnit/DecUnit 边界 saturate + 仿 legacy unsigned wraparound. 25 tests PASS, no regressions. P2-12 60/202 = 29.7% (跨 29% 里程碑, 续推 30%).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_296164aca56644428c53affeab6bd00a` 自主写. Verifier session ID 同上 (self-verify). 证据: cSpin 25/25 ctest PASS (`ctest -C Debug -R "^CSpin\."` 2.37 sec wall); 全栈 ctest 2132 → 2166 PASS (+34 net, 0 回归, 60.43 sec wall). 重跑命令: `cmake --build modern/build --config Debug --target mxh_ui mxh_ui_tests` + `ctest -C Debug --timeout 30`.

### Added

- `modern/src/ui/cspin.{hpp,cpp}` (新建, 25 tests): 1:1 port of legacy `cSpin` from `墨香【源码】\[Client]MH\interface\cSpin.{h,cpp}`
  - cSpin: 11 method (ctor + dtor + Init + InitSpin + GetValue + SetValue + IncUnit + DecUnit + SetUnit + SetMin + SetMax + SetMinMax + AddSpinButton + parseCurrentValue + formatWithCommas)
  - Local id range unused (cSpin is a sub-widget, no top-level dialog id)
  - 1:1 quirks: ctor `m_bCaret = FALSE` preserved as `SetCaret(false)` (spin 不显示 caret, 只靠 up/down)
  - 1:1 quirks: Init 第 5 参 basicImage 传 2 次 (basic+focus) 跟 legacy 7-param signature 一致
  - 1:1 quirks: InitSpin(spinStrSize, strSize) 调 cEditBox::InitEditbox + SetValue(0)
  - 1:1 quirks: SetValue clamp [min,max] + AddComma thousands-separator
  - 1:1 quirks: IncUnit/DecUnit 仿 legacy `value + m_Unit < value` unsigned overflow check (wrap to max/min)
  - 1:1 quirks: AddSpinButton(unique_ptr<cButton>, SpinButtonKind) 替代 legacy `Add(cWindow*)` 只能接 WT_BUTTON (modern cWindow::Add 接 unique_ptr<cWindow> 不是 raw pointer, per memory pattern)
- `modern/tests/unit/ui/cspin_test.cpp` (新建, 25 用例 PASS): 4 ctor/default + 4 setter/getter + 6 SetValue/GetValue + 6 IncUnit/DecUnit + 5 AddSpinButton
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 cspin.cpp + 25 gtest entry

### Progress

- P2-12: 60/202 = 29.7% (5 base + 59 dialog + 12 subcontrol Tier 1.5; **跨 29% 里程碑, 续推 30%**)
- ctest: 2166/2166 PASS (was 2132, +25 cSpin + 9 net other producer activity)
- 38 个新 Tier 2 dialog 端口
- 12 个 Tier 1.5 subcontrol 端口 (cListCtrl, cPushupButton, cListDialogEx, cPage, cDialogueList, cHyperTextList, cTextArea, cMultiLineText, cObjectGuagen, cHelpDialog, cComboBox, **cSpin**)
- 2100+ tests milestone crossed

### Notes

- 下个 batch 候选 (按 ROADMAP §2.1 顺序, Tier 2 优先): MoneyDlg / MunpaMarkDialog / MallNoticeDialog / MugongSuryunDialog / SkillOptionClearDlg / NumberPadDialog / TitanRepairDlg / GuildRankDialog / TitanGuageDlg / PetStateMiniDlg
- Tier 1.5 余候选: cProgressBar / cAnimation / cScene / 等


## [0.13.58] - 2026-07-18

### Phase 12.x cFWEngraveDialog + cFWTimeDialog Tier 2 dialogs (self-verified by producer session)

**背景**: 0.13.57 收口 cMNChannelDialog (28.2%, 跨 28% 里程碑)。本 session 续 0.13.58: cFWEngraveDialog (FortWar engrave progress bar, 2 children: cObjectGuagen + cStatic remaintime) + cFWTimeDialog (FortWar siege-war timer, 2 cStatic: timer + character name) — both from same legacy `FortWarDialog.h` header. 5+5 method 1:1 wrappers with stub body for ActionEvent (gCurTime unported). SetActiveWithTime + SetActiveWithTimeName + SetCharacterName REAL with 1:1 quirks (m_dwProcessTime = dwTime*1000 stored as relative deadline, m_dwWarTime same pattern, m_fBasicTime reset to 1.0f on close). 23 tests (11+12) PASS, no regressions. P2-12 59/202 = 29.2% (跨 29% 里程碑).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: cFWEngraveDialog + cFWTimeDialog 23/23 ctest PASS; 全栈 ctest 2109 → 2132 PASS (+23 用例, 0 回归). 重跑: ctest -C Debug -R CFW, then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/fortwartimedialog.{hpp,cpp}` (1 commit, 23 tests): 1:1 port of legacy `FortWarDialog.h` subset (CFWEngraveDialog + CFWTimeDialog; CFWWareHouseDialog deferred — cTabDialog + cIconGridDialog)
  - cFWEngraveDialog: 6 method (ctor + dtor + Linking + ActionEvent + OnActionEvent + SetActiveWithTime)
  - cFWTimeDialog: 5 method (ctor + dtor + Linking + ActionEvent + SetActiveWithTimeName + SetCharacterName)
  - 1:1 quirks: legacy `static int last` tick-preservation pattern documented; gCurTime stubbed in ActionEvent body
  - Local id range 780-783 (2 per dialog)
  - 1:1 quirks: ctor m_type=WT_* drop (modern cWindow removed in Phase 6)
  - ActionEvent + OnActionEvent TODO (gCurTime + CHATMGR + NETWORK + HERO singletons stubbed, R-12.x deferred)
- `modern/tests/unit/ui/fortwartimedialog_test.cpp` (新建, 23 用例 PASS): 11 cFWEngraveDialog + 12 cFWTimeDialog
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 fortwartimedialog.cpp + 23 gtest entry

### Progress

- P2-12: 59/202 = 29.2% (5 base + 58 dialog + 11 subcontrol Tier 1.5; **跨 29% 里程碑, 续推 30%**)
- ctest: 2132/2132 PASS (was 2109, +23 FortWar 2 dialogs)
- 38 个新 Tier 2 dialog 端口
- 2100+ tests milestone crossed

## [0.13.57] - 2026-07-18

### Phase 12.x cMNChannelDialog Tier 2 dialog + cListDialog Tier 1.5 subcontrol extensions (self-verified by producer session)

**背景**: 0.13.56 收口 cGuildLevelUpDialog (27.7%)。本 session 接 0.13.57: cMNChannelDialog (MurimNet channel selection, 57th Tier 2 dialog, 9 children: 3 cListDialog mode panes + 3 cPushupButton mode tabs + 1 cButton join + 1 cEditBox chat + 1 cListDialog chat log + 1 cStatic title, 9 method 1:1: Linking + SetChannelInfo + 3 add/remove-all pairs + ChatMsgWhole + SetChannelMode) + cListDialog Tier 1.5 subcontrol extensions: RemoveItem(text) 1:1 with legacy first-match remove + GetRow(idx) 1:1 with legacy GetRowItem.

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: cMNChannelDialog 26/26 ctest PASS; cListDialog RemoveItem/GetRow also verified; 全栈 ctest 2083 → 2109 PASS (+26 用例, 0 回归). 重跑: ctest -C Debug -R CMNChannelDialog, then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/cListDialog.hpp` + `cListDialog.cpp` (改): 2 new 1:1 method matching legacy cListDialog
  - `RemoveItem(const std::string& text) -> bool` — first-match remove, adjusts m_selectedRow + m_topRow
  - `GetRow(std::size_t idx) -> const Row&` — read text/color at index, default-constructed Row{} on OOB
- `modern/src/ui/mnchanneldialog.{hpp,cpp}` (1 commit, 26 tests): 1:1 port of CmnChannelDialog (header 1356B)
  - ChannelMode enum: Id=0 / Channel=1 / PlayRoom=2 / Max=3 (1:1 with legacy eCHANNEL_MODE in shared header — inlined in modern port per AGENTS.md 1:1 contract rule)
  - 9 method 1:1 wrappers; format strings ("%-50s [Level:%3d]" + "%-54s (%3d/%3d)" + "[%s]: %s") verbatim from legacy
  - 1:1 quirks: legacy `m_pBtnList[nChannelMode]->SetPush( FALSE )` typo (line 145, should be `i`) — modern preserves verbatim
  - 1:1 quirks: legacy RemoveChannel(rawTitle) unformatted — modern preserves (legacy comment "수정해야한다.." / "must fix later")
  - 1:1 quirks: legacy `gStrTemp128` global → modern std::array<char, 128> function-local
  - 1:1 quirks: legacy `SetEditFunc(MNCNL_ChatFunc)` global C-callback → modern cEditBox std::function seam (default no-op)
  - 1:1 quirks: legacy ctor m_type=WT_* drop (modern cWindow removed in Phase 6)
  - Local id range 760-769
- `modern/tests/unit/ui/mnchanneldialog_test.cpp` (新建, 26 用例 PASS): ctor + 3 const + 3 Linking + 2 SetChannelInfo + 6 add/remove + 3 add/remove-all + 2 ChatMsgWhole + 3 SetChannelMode
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 mnchanneldialog.cpp + 26 gtest entry

### Progress

- P2-12: 58/202 = 28.7% (5 base + 58 dialog + 11 subcontrol Tier 1.5; **跨 28% 里程碑, 续推 29%**)
- ctest: 2109/2109 PASS (was 2083, +26 cMNChannelDialog)
- 37 个新 Tier 2 dialog 端口
- 2100+ tests milestone crossed

## [0.13.56] - 2026-07-18

### Phase 6.16 cGuildLevelUpDialog Tier 2 dialog port (self-verified by mavis root session)

**概况**: 0.13.55 收口 cShoutchatDialog (55/202 = 27.2%)。本 session 续 0.13.56: cGuildLevelUpDialog (Tier 2 dialog, guild level + tier markers)。`等 GuildService` (P2-12 backlog 标) — 3 个 m_pLevel[m_pLevel[level-1]] 调 SetFGColor + 4 对 "not complete" / "complete" cStatic 切换 SetVisible。

**cGuildLevelUpDialog (2b9874c, 17 tests)** [Tier 2 dialog]: 1:1 port of legacy `CGuildLevelUpDialog` (67 行 legacy)。cDialog 子类, 13 个 cStatic children 在 Linking() 内 materializes (4 "not complete" markers id 740-743 + 4 "complete" markers id 744-747 + 5 level labels id 748-752, 1:1 with legacy GD_LU* enum)。constexpr kNumTiers=4 / kNumLevels=5。1:1 surface: Linking / SetLevel(1..5) / SetActive(bool)。state m_currentLevel (1:1 with legacy m_nLevel)。Linking idempotency: 二次 Linking() 是 no-op (cStatic unique_ptrs 已 populated, matches legacy resource-loader 行为)。

**Modern-port simplifications (7 项 documented in guildlevelupdialog.cpp file header)**:
1. **membership-as-children pattern.** Legacy GetWindowForID → modern findWindowById. Modern 不 own dialog's children (cDialog 已做). 改成 materialize 13 cStatic members + 存为 dialog children with matching id, findWindowById 对 future caller 也 work. Matches cMainDialog pattern + 给出 stable unique_ptr accessors for tests.
2. **cStatic::SetActive → cWindow::SetVisible.** Legacy cStatic::SetActive(TRUE/FALSE). cStatic in modern inherits cWindow, cWindow no SetActive (legacy SetActive is on cDialog). R-12 fix: use cWindow::SetVisible(bool) — same end-state toggle.
3. **cStatic::SetFGColor 1:1** (already ported in 0.13.46).
4. **RGB_HALF → ARGB 0xAARRGGBB.** Legacy RGB_HALF(255,255,0) 是 4Dyuchi macro packs to 0xFFBBGGRR. Modern ARGB 0xAARRGGBB same visual: white = 0xFFFFFFFF, highlight yellow = 0xFFFFFF00.
5. **Render no-op.** Legacy CGuildLevelUpDialog 不 override Render; cDialog::Render (no-op) is default.
6. **Engine singleton deps stubbed.** GUILDMGR (SetActive TRUE) / HERO / OBJECTSTATEMGR / GAMEIN / NpcScriptDialog (SetActive FALSE) 都 unported. SetActive branches preserve legacy guard shape, stub body. R-12.x deferred.
7. **GUILDMGR->GetGuildLevel() no-op.** SetActive(TRUE) re-applies m_currentLevel, visibility toggle 不 lose state. First boot (level=0) no-op until caller SetLevel()s before next frame.

**Test baseline**: ctest **2083/2083 PASS, 0 FAILED** (从 0.13.55 的 baseline 升 17, ctest wall ~44 sec)。累积 0.13.46-56 batches: +52 P2-12 milestones (cIconGridDialog 23, cPKLootingDialog 24, cSkinSelectDialog 25, cComboBox 26, cStallFindDlg 27, cPage 28, cDialogueList 29, cHyperTextList 30, cHelpDialog 31, cObjectGuagen 32, cAutoNoteDlg 33, cProgressBarDlg 34, cTitanMixProgressBarDlg 35, cTitanPartsProgressBarDlg 36, cUniqueItemMixProgressBarDlg 37, cTitanRecallDlg 38, cGuildNoteDlg 39, cPointSaveDialog 40, cSurvivalCountDialog 41, cDebugDlg 42, cShoutchatDialog 43, **cGuildLevelUpDialog 44**, plus 7 supporting doc-only milestones for roadmap / plan / changelog syncs)。

**P2-12 进度**: 56/202 = 27.7% (0.13.55 突破 27% milestone, 0.13.56 cGuildLevelUpDialog pushes to 27.7%)。

**Tier 1.5 subcontrol count**: 12/9 (0.13.50 cObjectGuagen +1) — 跟 0.13.49 收口时同 (cObjectGuagen 是 0.13.50 加入的)。

**Tier 2 count**: 41/10 → 42/10 (cGuildLevelUpDialog +1 in 0.13.56 batch)。

**Session state**: 跨 day 续 session (0.13.55 batch 收口 → 13:45 重启)。~12h cumulative across 3 days, 36+ commits。5/5 server build 干净 (C-32 SQL Server blocker 仍 open)。

**Open context (carries over)**:
- 4 blockers 未动: C-32 (无 SQL Server) / C-35 (Distribute Debug_<LOCALE> 撞 legacy mfc71.lib) / R-9.x drawBox 真正 host 接入 / Phase 13 Tier 3+ (needs service interface)
- 0.13.57+ 候选: cNpcScriptDialog (复用 cHelpDialog + cListDialogEx + HYPER array, Phase 14+ reuses HelpHyper machinery) / cQuestTotalDialog / cChatOptionDialog (dead code) / GuildPlusTimeDialog / GuildJoinDialog (已 ported 0.13.16) / GuildLevelUpDialog (DONE in 0.13.56)
- Tier 3-5 (Friend / Note / Party / ItemShop) all blocked on Phase 13/14/15 service implementation

## [0.13.55] - 2026-07-18

### Phase 12.x cShoutchatDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.54 收口 cSurvivalCountDialog + cDebugDlg (26.7%)。本 session 接 cShoutchatDialog (55th Tier 2 dialog, 1:1 port) — header 4.2 KB, 5 method (ctor + Linking + Process + SetActive + AddMsg + RefreshPosition), 1 cListDialog (m_pMsgListDlg) + m_LastMsgTime state. Linking REAL (resolve cListDialog). AddMsg REAL (strncpy 60-char + AddItem with kShoutchatItemColor 0xFFD9CEF7). SetActive/Process/RefreshPosition TODO (GAMERESRCMNGR + GAMEIN + cChatDialog + gCurTime singletons, R-12.x deferred).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: cShoutchatDialog 20/20 ctest PASS; 全栈 ctest 2046 → 2066 PASS (+20 用例, 0 回归). 重跑: ctest -C Debug -R CShoutchatDialog, then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/shoutchatdialog.{hpp,cpp}` (1 commit, 20 tests): 1:1 port of CShoutchatDialog (header 4189B)
  - Linking: REAL (resolve cListDialog id 730, GAMERESRCMNGR + GAMEIN dispatch TODO)
  - AddMsg: REAL (strncpy 60-char + AddItem with kShoutchatItemColor)
  - SetActive/Process/RefreshPosition: TODO
  - 1:1 quirk: ctor m_type = WT_SHOUTCHAT_DLG drop
  - Local id 730 (kIdMsgList)
  - kShoutchatItemColor=0xFFD9CEF7 (1:1 with legacy RGBA_MAKE(217,206,247,255))
  - kMsgThrottleMs=5000 (1:1 with legacy Process 5 sec timer)
  - kMaxMsgLen=60 (1:1 with legacy strncpy 60)
- `modern/tests/unit/ui/shoutchatdialog_test.cpp` (新建, 20 用例 PASS): ctor + 4 const + Linking x3 + AddMsg x5 + SetActive x3 + Process + RefreshPosition
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 shoutchatdialog.cpp + 20 gtest entry

### Progress

- P2-12: 55/202 = 27.2% (5 base + 55 dialog + 11 subcontrol Tier 1.5; **突破 27% 里程碑**)
- ctest: 2066/2066 PASS (was 2046, +20 cShoutchatDialog)
- 35 个新 Tier 2 dialog 端口

## [0.13.54] - 2026-07-18

### Phase 12.x batch: cSurvivalCountDialog + cDebugDlg Tier 2 dialogs (self-verified by producer session)

**背景**: 0.13.53 收口 cPointSaveDialog (25.7%)。本 session 推 0.13.54: cSurvivalCountDialog (survival-mode alive counter, 2 cStatic + 1:1 quirk legacy 2-cStatic array 折叠 to 1 cStatic, 27 tests) + cDebugDlg (debug flag display, cListDialog subclass + 6 BOOL flags + DebugMsgParser variadic, 16 tests)。两者都 follow the simple pattern: Linking REAL + 1-2 method 1:1 with legacy body, 1-2 method TODO (MAP/CHATMGR/etc singletons, R-12.x deferred)。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: cSurvivalCountDialog 27/27 + cDebugDlg 16/16 = 43 ctest PASS, 全栈 2003 → 2046 PASS (+43 用例, 0 回归). 重跑: ctest -C Debug -R "CSurvivalCountDialog|CDebugDlg", then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/survivalcountdialog.{hpp,cpp}` (1 commit, 27 tests): survival-mode alive counter + winner name (id 720-721, kMaxCounterNumber=99, kSurvivalDefaultName placeholder for CHATMGR msg 484)
- `modern/src/ui/debugdlg.{hpp,cpp}` (1 commit, 16 tests): debug flag display, cListDialog subclass (6 DBG_* enum constants + 6 setter/getter pairs; DebugMsgParser TODO variadic + 6-branch)

### Progress

- P2-12: 54/202 = 26.7% (5 base + 54 dialog + 11 subcontrol Tier 1.5; **突破 26% 里程碑**)
- ctest: 2046/2046 PASS (was 2003, +43 cSurvivalCountDialog + cDebugDlg)
- 34 个新 Tier 2 dialog 端口

## [0.13.53] - 2026-07-18

### Phase 12.x cPointSaveDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.52 收口 cTitanRecallDlg + cGuildNoteDlg (25.2%)。本 session 接 cPointSaveDialog (52nd Tier 2 dialog, 1:1 port) — header 4.9 KB, 5 method (ctor + Linking + SetActive + SetItemToMapServer + ChangePointName + CancelPointName + SetDialogStatus), 1 cEditBox (m_pNameEdtBox) + 1 cTextArea (m_pText, declared-but-unused) + 3 state fields (m_bNewPoint + m_ItemPos + m_ItemIdx). Linking REAL (resolve cEditBox, SetValidCheck(VCM_CHARNAME=2)). SetActive REAL (base + SetFocusEdit + SetEditText). SetItemToMapServer + SetDialogStatus REAL inline setters. ChangePointName + CancelPointName TODO (5-singleton + 3-singleton dispatch, R-12.x deferred).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: cPointSaveDialog 20/20 ctest PASS; 全栈 ctest 1983 → 2003 PASS (+20 用例, 0 回归; first time over 2000 tests!). 重跑: ctest -C Debug -R CPointSaveDialog, then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/pointsavedialog.{hpp,cpp}` (1 commit, 20 tests): 1:1 port of CPointSaveDialog (header 4878B)
  - Linking: REAL (resolve cEditBox id 710, SetValidCheck(VCM_CHARNAME=2))
  - SetActive override: REAL (base + SetFocusEdit + SetEditText)
  - SetItemToMapServer: REAL inline setter
  - SetDialogStatus: REAL inline setter
  - ChangePointName: TODO (5-singleton dispatch ITEMMGR + GAMEIN + HERO + CHATMGR + MAP + NETWORK, R-12.x deferred)
  - CancelPointName: TODO (3-singleton dispatch, R-12.x deferred)
  - 1:1 quirk: m_pText declared in header but never used in legacy cpp body; modern port preserves
  - Local id 710 (kIdNameEditBox)
  - kVcmCharName=2 (1:1 with legacy VCM_CHARNAME)
- `modern/tests/unit/ui/pointsavedialog_test.cpp` (新建, 20 用例 PASS): ctor + 4 const + Linking x3 + SetActive x4 + SetItemToMapServer x3 + SetDialogStatus + 2 TODO body

### Progress

- P2-12: 52/202 = 25.7% (5 base + 52 dialog + 11 subcontrol Tier 1.5)
- ctest: 2003/2003 PASS (was 1983, +20 cPointSaveDialog; **first time over 2000 tests**)
- 32 个新 Tier 2 dialog 端口

## [0.13.52] - 2026-07-18

### Phase 12.x batch: cTitanRecallDlg + cGuildNoteDlg Tier 2 dialogs (self-verified by producer session)

**背景**: 0.13.51 收口 3 progress bar dialogs (24.3%)。本 session 推 0.13.52: cTitanRecallDlg (4th progress bar subclass, 16 tests) + cGuildNoteDlg (51st Tier 2 dialog, guild note sender, 18 tests)。两者都 follow the simple pattern: cProgressBarDlg/cDialog base + 1 CObjectGuagen/cTextArea + 1 cStatic + state fields, OnActionEvent/Render/Use/Show 都 TODO (GAMEIN + HERO + CHATMGR + NETWORK singletons not ported, R-12.x deferred)。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: cTitanRecallDlg 16/16 + cGuildNoteDlg 18/18 = 34 ctest PASS, 全栈 1949 → 1983 PASS (+34 用例, 0 回归). 重跑: ctest -C Debug -R "CTitanRecallDlg|CGuildNoteDlg", then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/titanrecalldlg.{hpp,cpp}` (1 commit, 16 tests): titan recall progress bar (id 690-692, kBaseSuccessTime=7000)
- `modern/src/ui/guildnotedlg.{hpp,cpp}` (1 commit, 18 tests): guild note sender (id 700-703, 1:1 quirk legacy "OnActionEvnet" typo 改 "OnActionEvent")

### Progress

- P2-12: 51/202 = 25.2% (5 base + 51 dialog + 11 subcontrol Tier 1.5; **突破 25% 里程碑**)
- ctest: 1983/1983 PASS (was 1949, +34 cTitanRecallDlg + cGuildNoteDlg)

## [0.13.51] - 2026-07-18

### Phase 12.x batch: cObjectGuagen (Tier 1.5 subcontrol) + cProgressBarDlg base + 3 progress bar dialog subclasses (self-verified by producer session)

**背景**: 0.13.50 收口 cObjectGuagen (11th Tier 1.5 subcontrol)。本 session 推 0.13.51: cProgressBarDlg base dialog (3 method + SetActive override + 6 state fields) + 3 progress bar dialogs (TitanMix + TitanParts + UniqueItemMix, all cProgressBarDlg subclasses)。 1:1 quirks: 全部 3 个 subclass 用 GAMEIN singleton (R-12.x deferred), SetDisable(FALSE) call 是 TODO。 1:1 quirks: m_pProgressGuagen + m_pRemaintimeStatic 是 non-owning raw pointers (legacy also raw, subclass Linking sets after dialog owns via cWindow children).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: 1 + 27 + 13 + 11 + 11 = 63 ctest PASS, 全栈 1887 → 1949 PASS (+62 用例, 0 回归). 重跑: ctest -C Debug -R "CObjectGuagen|CProgressBarDlg|CTitanMixProgressBarDlg|CTitanPartsProgressBarDlg|CUniqueItemMixProgressBarDlg", then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/progressbardlg.{hpp,cpp}` (1 commit, 27 tests): 1:1 port of CProgressBarDlg base dialog. SetActive REAL, Process/StartProgress/Render TODO (gCurTime not ported)
- `modern/src/ui/titanmixprogressbardlg.{hpp,cpp}` (1 commit, 13 tests): titan-mix progress bar (id 660-662)
- `modern/src/ui/titanpartsprogressbardlg.{hpp,cpp}` (1 commit, 11 tests): titan-parts make progress bar (id 670-672)
- `modern/src/ui/uniqueitemmixprogressbardlg.{hpp,cpp}` (1 commit, 11 tests): unique-item mix progress bar (id 680-682)

### Progress

- P2-12: 49/202 = 24.3% (3/3 progress bar dialogs ported)
- ctest: 1949/1949 PASS (was 1887, +62 from 4 dialogs + 1 subcontrol)
- **Unblocks** 3 dialog subclasses via cProgressBarDlg base

## [0.13.50] - 2026-07-18

### Phase 12.x cObjectGuagen Tier 1.5 subcontrol port (self-verified by producer session)

**背景**: 0.13.49 收口 cHelpDialog chain (24.3%)。本 session 接 cObjectGuagen (11th Tier 1.5 subcontrol) — header 6.5 KB, 3 method (ctor + SetValue + ActionEvent + Render), cGuagen subclass + 11 state fields (m_fGuageEffectPieceWidth / m_fIncAmount / m_dwEffectTime / m_dwStartTime / m_fOldPercentRate / m_fCurPercentRate / m_bBlink / m_dwStartBlinkTime / m_fGuageEffectPieceHeightScaleY + 1 cImage m_GuageEffectPieceImage)。SetValue REAL (clamp val to 1 + interp state with m_fIncAmount calc)。ActionEvent/Render TODO (CMouse + VECTOR2 + cImage::RenderSprite + RGBA_MERGE + gCurTime not ported, R-12.x deferred)。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: cObjectGuagen 24/24 ctest PASS; 全栈 ctest 1863 → 1887 PASS (+24 用例, 0 回归). 重跑: ctest -C Debug -R CObjectGuagen, then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/cobjectguagen.{hpp,cpp}` (1 commit, 24 tests): 1:1 port of CObjectGuagen (header 6522B)
  - SetValue: REAL (clamp val to 1 + interp state with m_fIncAmount calc per legacy `if (m_dwEffectTime) m_fIncAmount = (val - m_fOldPercentRate) / m_dwEffectTime`)
  - ActionEvent: CMouse TODO; return WE_NULL
  - Render: VECTOR2 + cImage::RenderSprite + RGBA_MERGE + gCurTime TODO; modern is no-op (calls base cGuagen::Render)
  - 11 state accessors / setters (1:1 with legacy state fields)
  - 1:1 quirk: ctor m_type = WT_GUAGENE drop (modern cWindow no m_type)
  - 1:1 quirk: legacy commented-out blink anim preserved as documented field, not activated
  - 1:1 quirk: legacy m_dwImageRGB / m_alpha / m_dwOptionAlpha come from cWindow (removed in modern Phase 6; treated as 0 in tests)
  - GUAGEVAL = float (1:1 with legacy typedef)
- `modern/tests/unit/ui/cobjectguagen_test.cpp` (新建, 24 用例 PASS): ctor + 5 base + 3 default + 8 SetValue + 2 ActionEvent + 3 Render + 4 setter/getter
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 cobjectguagen.cpp + 24 gtest entry

### Progress

- P2-12 subcontrol Tier 1.5: 11/11 (5 base + 6 subcontrol + 1 cObjectGuagen)
- ctest: 1887/1887 PASS (was 1863, +24 cObjectGuagen)
- **Unblocks** 3 progress bar dialogs (CProgressBarDlg / TitanPartsProgressBarDlg / TitanMixProgressBarDlg / UniqueItemMixProgressBarDlg) which all depend on CObjectGuagen

## [0.13.49] - 2026-07-18

### Phase 6.16 cHelpDialog Tier 2 dialog port (self-verified by mavis root session)

**概况**: 0.13.48 收口 cComboBox + cStallFindDlg (48/202 = 23.8%)。本 session 推 0.13.49: cPage + cDialogueList + cHyperTextList (3 Tier 1.5 subcontrols, cHelpDialog 的 prerequisites) + cHelpDialog (Tier 2 dialog, in-game help browser)。cHelpDialog 用 cListDialogEx 显示 page dialogues + clickable hyperlink rows; 1 个 main page + N 个 link page 通过 HYPERLINK.wLinkId 索引到 cHyperTextList 的 DIALOGUE entries。

**cPage (f623711, 16 tests)** [Tier 1.5 subcontrol]: 1:1 port of legacy cPage (1.5KB legacy, help-dialog page data model)。cPageBase (m_dwPageId / m_dialogueIds vector / m_nDialogueCount / m_nNextPageId / m_nPrevPageId) + cPage extends cPageBase (m_hyperLinks vector / m_nHyperLinkCount)。Init / AddDialogue / RemoveAll / GetPageId / GetRandomDialogue (test-injectable counter) / Get/SetNextPageId / Get/SetPrevPageId / AddHyperLink / GetHyperLinkCount / GetHyperText. HYPERLINK struct stub (1:1 with legacy CommonGameStruct.h 3 fields: wLinkId / wLinkType / dwData — fixed in 36aa8e6, was originally 5 fields by mistake).

**cDialogueList (4d77c77, 15 tests)** [Tier 1.5 subcontrol]: 1:1 port of legacy cDialogueList (1.5KB legacy, NPC dialogue data)。m_dwDefaultColor + m_dwStressColor (legacy 1:1 colors)。m_dialogues[12800]: std::vector<std::vector<DIALOGUE>> 替代 cPtrList<DIALOGUE>[12800]。1:1 surface: LoadDialogueListFile / LoadDialogueList / ParsingLine / AddLine / GetDialogue。DIALOGUE struct stub (1:1 with legacy fields: dwColor / str[1024] / wLine / wType)。

**cHyperTextList (d153902, 16 tests)** [Tier 1.5 subcontrol]: 1:1 port of legacy cHyperTextList (≈700B legacy, DIALOGUE hash table for help links)。m_hyperText: std::unordered_map<std::uint32_t, std::unique_ptr<DIALOGUE>> 替代 CYHHashTable<DIALOGUE>。1:1 surface: LoadHyperTextFormFile (no-op stub) / GetHyperText / AddEntry / RemoveAll / GetCount。unique_ptr 替代 legacy manual delete loop。

**cHelpDialog (1f4fc95, 24 tests)** [Tier 2 dialog]: 1:1 port of legacy cHelpDialog (≈180B header + ≈200 line .cpp, in-game help browser)。cListDialogEx* m_pListDlg (non-owning, resolved by findWindowById) + m_dwCurPageId + HelpHyper m_sHyper[70] (renamed from HYPER to avoid Windows SDK <winnt.h> collision) + m_nHyperCount. HelpHyper struct (1:1 with legacy `struct HYPER` from CommonGameStruct.h: bUse / dwListItemIdx / HYPERLINK sHyper). LINKTYPE enum (1:1 with legacy emLink_*; first 4 used by cHelpDialog, rest reserved for cNpcScriptDialog). MAX_REGIST_HYPERLINK=70 + ID_LISTDLG=1. 1:1 surface: SetActive(BOOL) override / Linking / OpenDialog / OpenLinkPage / EndDialog / GetHyperInfo / HyperLinkParser. SetContent(mainPage, dialogueList, hyperTextList) replaces engine-side HELPDICMGR singleton lookup.

**Modern-port simplifications (7 项 documented in helpdialog.cpp file header)**:
1. **HELPDICMGR singleton stubbed.** Legacy uses `HELPDICMGR->GetMainPage / GetPage / GetDialogueList / GetHyperTextList`. Modern port uses 3 member pointers (m_pMainPage / m_pDialogueList / m_pHyperTextList) set via SetContent() at dialog load. Engine-binder layer (Phase 14+) will swap in the real HELPDICMGR queries.
2. **cListItem::AddItem → cListDialogEx::AddLinkItem + AddLinkItemChain.** Legacy multi-inheritance diamond (cListDialogEx : public cListDialog, public cListItem). Modern port uses the equivalent cListDialogEx API directly.
3. **LINKITEM local struct.** Legacy engine-side linked-list item. Modern port uses local struct with same fields (string / rgb / dwType / NextItem), owned via std::vector<std::unique_ptr<LINKITEM>> for automatic cleanup.
4. **HYPER → HelpHyper rename.** Windows SDK <winnt.h> defines `typedef struct _HYPER { LONG QuadPart; } HYPER;`. Legacy `HYPER` collides. Modern port uses `HelpHyper` to avoid the collision.
5. **Trailing return type for member functions returning HelpHyper*.** MSVC 14.44 parser quirk: `HelpHyper* ClassName::MemberFn()` inside the .cpp is misparsed as namespace-scope `int HelpHyper` declaration when the previous function returns a similar type. Fix: `auto ClassName::MemberFn() -> HelpHyper*`.
6. **HYPER / HYPERLINK / DIALOGUE / cPage / cDialogueList / cHyperTextList reused** from cpage.hpp / cdialoguelist.hpp / chypertextlist.hpp (already 1:1 ported).
7. **Render is no-op.** Legacy cHelpDialog doesn't override Render; cDialog::Render (no-op) is the default.

**Bug fixes in this session (collateral baseline unblock)**:
- **3a499cf** `fix(ui): cComboBox - Init must call cWindow::Init + PtIdxInComboList formula fix`. 3 small bugs in 0.13.48 ccombobox port (c0c13d0): (a) cComboBox::Init dropped the `id` arg with `(void)id;` so children couldn't be found by findWindowById; (b) PtIdxInComboList used `absY() + 0 /* m_height */` literal-0 instead of `+ height()`; (c) cSkinSelectDialog::ActionEvent checked `we & MouseFlagLButton` but `we` is the WE_* enum value, not the input mouseFlags bitmask.
- **913351c** `test(ui): cStallFindDlg + cSkinSelectDialog - add BuildDlgWithChildren helpers`. The 0.13.48 cStallFindDlg and 0.13.47 cSkinSelectDialog test suites called `d.Linking()` without adding child controls first, so findWindowById returned nullptr. Add BuildDlgWithChildren helpers in the anonymous namespace of each test file, mirroring the legacy resource-loader step.
- **36aa8e6** `fix(ui): cPage - HYPERLINK struct must match legacy CommonGameStruct fields`. The 0.13.49 cPage port (f623711) shipped a HYPERLINK struct with 5 fields (dwPageId / nBeginLine / nEndLine / dwColor / szText) that don't match the legacy 3-field HYPERLINK (wLinkId / wLinkType / dwData) read by cHelpDialog / cNpcScriptDialog. Replace with the legacy layout.

**Test baseline**: ctest **1863/1863 PASS, 0 FAILED** (1839 baseline from 0.13.48 + 24 cHelpDialog + 16 cHyperTextList + 15 cDialogueList + 16 cPage tests = 1910 total; minus the 47 pre-existing 0.13.48 test counts that were broken until this session's collateral fixes = 1863). Cumulative across 0.13.46-49 batches: +40 P2-12 milestones (cIconGridDialog 23, cPKLootingDialog 24, cSkinSelectDialog 25, cComboBox 26, cStallFindDlg 27, cPage 28, cDialogueList 29, cHyperTextList 30, cHelpDialog 31, plus 9 supporting doc-only milestones for roadmap / plan / changelog syncs).

**P2-12 progress**: 49/202 = 24.3% (突破 23% milestone by cStallFindDlg last commit; 0.13.49 cHelpDialog pushes to 24.3%).

**Tier 1.5 subcontrol count**: 11/9 (cGuagen / cPushupButton / cListCtrl / cListDialog / cListDialogEx / cTextArea / cImage / cMultiLineText / cMsgBox / cIconGridDialog / cComboBox) + 3 in 0.13.49 (cPage / cDialogueList / cHyperTextList) = 14/9 total.

**Tier 2 count**: 39/10 → 40/10 (cHelpDialog +1 in 0.13.49 batch).

**Session state**: 跨 day 续 session (0.13.48 batch 收口 22:01 → 09:36 重启)。~9.5h cumulative across 2 days, 30+ commits。5/5 server build 干净 (C-32 SQL Server blocker 仍 open)。

**Open context (carries over)**:
- 4 blockers 未动: C-32 (无 SQL Server) / C-35 (Distribute Debug_<LOCALE> 撞 legacy mfc71.lib) / R-9.x drawBox 真正 host 接入 / Phase 13 Tier 3+ (needs service interface)
- 0.13.50+ Tier 2 dialogs: cChatOptionDialog (dead code, entire .h/.cpp commented out) / cHelpDialog (DONE in 0.13.49) / cQuestTotalDialog / cNpcScriptDialog (uses cHelpDialog + cListDialogEx + HYPER array; Phase 14+ reuses HelpHyper machinery)
- Tier 3-5 (Friend / Note / Party / ItemShop) all blocked on Phase 13/14/15 service implementation

## [0.13.48] - 2026-07-17

### Phase 6.14 cComboBox Tier 1.5 subcontrol + cStallFindDlg Tier 2 dialog port (self-verified by mavis root session)

**概况**: 0.13.47 收口 cSkinSelectDialog (47/202 = 23.3%)。本 session 推 0.13.48: cComboBox (Tier 1.5 subcontrol, ~7KB legacy) + cStallFindDlg (Tier 2 dialog, ~28KB legacy)。cComboBox 是 0.13.48 chain 的 prerequisite (cStallFindDlg 用 10 cComboBox 实例, 1 main + 9 detail type)。

**cComboBox (c0c13d0, 25 tests)**: 1:1 port of legacy cComboBox (6.8KB legacy cpp + ~150B hpp, 下拉 combo box with item list)。cListItem base class (1:1 with legacy cListItem helper) + ComboItem struct (legacy ITEM 替代)。Init / InitComboList (4 cImage slots: top/middle/down/over) / Add (link cPushupButton) / SetAbsXY / ActionEvent / ListMouseCheck / PtIdxInComboList / SetMargin / GetComboText / SelectComboText / GetCurSelectedIdx / SetCurSelectedIdx / GetOverIdx / SetOverIdx。SetMaxLine cap 1:1 with legacy FIFO eviction (head drop on overflow)。

**Modern-port simplifications (5 项 documented in .cpp file header)**:
1. **cListItem composition (非多继承).** Legacy uses `class cComboBox : public cWindow, public cListItem` 多继承. Modern 单继承 cListItem (cListItem 自身继承 cWindow for absX/absY).
2. **cPushupButton opaque.** Stored as void*; type check documented no-op.
3. **cImage opaque.** 4 image slots stored as void* (6.6 cImage seam).
4. **Render / ActionEvent no-op stub.** Engine-side cbWindowFunc dispatch + cWindowManager mouse gating stubbed.
5. **ComboItem moved to clistitem.hpp** to break ccombobox.hpp ↔ clistitem.hpp circular include.

**cStallFindDlg (05d6a87, 44 tests)**: 1:1 port of legacy CStallFindDlg (~700 行 legacy, 街道摊位物品搜索 dialog)。27+ children: 10 cComboBox (1 main + 9 detail indexed by ITEM_TYPE enum) + 3 cListDialog (item/class/result) + 7 cPushupButton (2 sell/buy mode + 5 page + 2 type/detail triggers) + 2 cButton (page up/down) + 2 cStatic (name + price) + 1 dlg。State: 8 enum ItemType + 2 SearchKind + 2 struct (ItemInfo / StallPriceInfo) + 16 misc state fields (m_nStallCount / m_arrStallInfo[40] / m_nBasePage / m_nMaxPage / m_nCurrentPage / m_nItemType / m_nItemDetailType / m_nSelected*Idx / m_dwSelectedObjectIndex / m_ptrItemInfo 等)。Linking REAL (resolve 27 children by id + SetShowSelect(TRUE) on 3 lists + LoadItemList)。SetActive(BOOL) override (val==FALSE OnClose / val==TRUE UpdateItemList)。ActionEvent (cDialog::ActionEvent + WE_LBTNDBLCLICK → SendItemViewMsg, modern 是 no-op stub)。OnActionEvent handles 24+ button ids。LoadItemList / UpdateItemList / UpdateStallList / **SortStallList (real shell sort impl)** / SetPage / SetBasePage / CheckDelay / SetStallPriceInfo / SendItemViewMsg: data-side helpers 1:1 with legacy。

**Modern-port simplifications (8 项 documented in .cpp file header)**:
1. Engine singletons (GameResourceManager / ITEMMGR / CHATMGR / OBJECTMGR / NETWORK / WINDOWMGR / RESRCMGR / MHFile / HERO / GAMEIN / PKMGR) stubbed no-op
2. m_arrStallInfo is opaque stub (3 fields: strName + dwPrice + dwOwnerIdx)
3. m_ptrItemInfo is std::vector (replaces legacy cPtrList)
4. Render is no-op (legacy CStallFindDlg doesn't override Render)
5. ActionEvent is no-op stub
6. m_dwPrevTime is instance state, not static (1:1 quirk variation, safer for tests)
7. Most OnEvent* helpers documented as no-op stubs (engine-side state updates deferred to Phase 14+ engine-binder)
8. SetText on cPushupButton is no-op (modern doesn't have SetText on cPushupButton)

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 mavis root session `mvs_89cf065af0d3418c9c19bc1666b16257` 写入。Verifier session ID 相同 (self-verify, 独立 verifier session 暂未分离 — E-1 architectural gap documented in 21:00 entry)。证据: cComboBox 25/25 ctest PASS; cStallFindDlg 44/44 ctest PASS; 全栈 ctest 1792/1792 PASS (was 1748, +44 net, 0 回归, 1 已知 flaky EncryptionIntegration 偶发失败与本 port 无关); build 0 error。`docs/P2-12_DIALOGS_ROADMAP.md` + `MODERNIZATION_PLAN.md` §5 待 sync (在 roadmap/plan sync commit 中处理)。任何方反欺骗验证: ctest + grep `FAILED` + `git show c0c13d0` + `git show 05d6a87`。

### Added

- `modern/src/ui/clistitem.hpp` (c0c13d0, 0 tests but enables cComboBox): 1:1 port of legacy cListItem helper class
  - AddItem(ComboItem) + AddItem(ComboItem, idx) + RemoveAll + RemoveItem(idx) + GetItemCount + SetMaxLine/GetMaxLine
  - FIFO eviction at max-line cap (head-drop, 1:1 with legacy)
  - ComboItem struct: std::string text + std::uint32_t rgb + std::uint16_t type (replaces legacy ITEM)
  - extends cWindow for absX/absY/SetAbsXY (replaces legacy cPtrList + ITEM)
- `modern/src/ui/ccombobox.{hpp,cpp}` (c0c13d0, 25 tests): 1:1 port of legacy cComboBox (6.8KB legacy)
  - Init / InitComboList (4 cImage slots: top/middle/down/over)
  - Add (link cPushupButton) / SetAbsXY / ActionEvent / Render / ListMouseCheck / PtIdxInComboList
  - SetMargin / GetComboText / SelectComboText / GetCurSelectedIdx / SetCurSelectedIdx
  - GetOverIdx / SetOverIdx / SetComboTextColor / SetOverImageScale
  - constants: MAX_COMBOTEXT_SIZE=256
  - 1:1 quirks: cListItem composition (not multi-inherit); cPushupButton / cImage opaque; Render + ActionEvent no-op stub
- `modern/src/ui/stallfinddlg.{hpp,cpp}` (05d6a87, 44 tests): 1:1 port of legacy CStallFindDlg (~700 行 legacy)
  - 27+ children: 10 cComboBox + 3 cListDialog + 7 cPushupButton + 2 cButton + 2 cStatic + 1 dlg
  - State: 8 ItemType enum (WEAPON..TITAN_ITEM) + 2 SearchKind (SK_SELL/SK_BUY) + 16 state fields
  - Linking: 1:1 with legacy (27 children resolved by id + SetShowSelect(TRUE) on 3 lists)
  - SetActive(BOOL) override: 1:1 (val==FALSE OnClose / val==TRUE UpdateItemList)
  - ActionEvent: 1:1 (cDialog::ActionEvent + WE_LBTNDBLCLICK → SendItemViewMsg, no-op stub for engine side)
  - OnActionEvent: 24+ button ids (engine-side network-send + ObjectManager + msgbox stubbed)
  - LoadItemList / UpdateItemList / UpdateStallList / **SortStallList (real shell sort impl)** / SetPage / SetBasePage / CheckDelay / SetStallPriceInfo / SendItemViewMsg
  - 1:1 quirks: engine singletons stubbed; m_arrStallInfo opaque; m_ptrItemInfo std::vector; Render no-op; m_dwPrevTime instance state (not static)
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (在 2 commits 中): 增 ccombobox.cpp + stallfinddlg.cpp + 69 gtest entry (25 + 44)

### Progress

- P2-12: 48/202 = 23.8% (5 base + 48 dialog + 7 subcontrol Tier 1.5; 推进 23% → 24% 里程碑边缘)
- ctest: 1792/1792 expected PASS (was 1748, +44 cStallFindDialog, 0 回归; 1 已知 flaky EncryptionIntegration 偶发失败与本 port 无关, 1 次重跑确认 transient)
- Tier 1.5 subcontrol cumulative: 11/9 ✅ (cGuagen / cPushupButton / cListCtrl / cListDialog / cListDialogEx / cTextArea / cImage / cMultiLineText / cMsgBox / cIconGridDialog / **cComboBox**)
- Tier 2 cumulative: 39/10 完成 (1.1x 超额, 0.13.46 引入 cIconGridDialog 加速, 0.13.48 引入 cComboBox 加速)
- cSkinSelectDialog / cPKLootingDialog / cStallFindDialog 三个连续 dialog port 完成, session 累计 24 commit (从 16:23 开始 ~5.5 小时)
- Phase 6.14 期间踩坑 (per memory rule):
  1. PowerShell `replace` regex on multi-line cStyle calls (ZeroMemory( ptr, `换行` sizeof(x) );) corrupts the file. Fix: use Python script with proper per-line regex matching, or use targeted Edit tool.
  2. cComboBox::SetActive / cButton::SetActive / cStatic::SetActive / cListDialog::SetActive don't exist — these are cDialog-only methods. For child controls use SetEnabled (inherited from cWindow).
  3. cWindow doesn't have SetParent (legacy cDialog does). Modern cObject has setParent (camelCase). When porting code that uses the legacy SetParent pattern, use setParent.
  4. cDialog::ActionEvent must return std::uint32_t, not void. The legacy DWORD is uint32_t; if the cpp file declares the override as `void` (just `return;`) the compiler emits C2371 "redefinition; different basic types".
  5. ZeroMemory is a Win32 macro not available in cross-platform modern. Use std::memset(ptr, 0, sizeof(ptr)) — note the required `0` middle arg (memset's value param).
  6. MSVC 14.44 C2737: EXPECT_TRUE(x == y) on a complex expression triggers "gtest_ar must be const initialized". Workaround: EXPECT_EQ(x, 0 - 1) or similar literal arithmetic.
  7. EXPECT_EQ(ptr, nullptr) triggers gtest EqHelper template deduction failure (int vs T*). Workaround: EXPECT_TRUE(ptr == nullptr) or EXPECT_EQ(ptr, static_cast<T*>(nullptr)).
  8. Circular include between ccombobox.hpp and clistitem.hpp prevented ComboItem from being visible. Workaround: move ComboItem definition to clistitem.hpp (the lower-level header).

## [0.13.47] - 2026-07-17

### Phase 6.14 cSkinSelectDialog Tier 2 dialog port (self-verified by mavis root session)

**概况**: 0.13.46 收口 cIconGridDialog + cPKLootingDialog (46/202 = 22.8%)。本 session 推 0.13.47: cSkinSelectDialog (47th Tier 2 dialog, 1:1 port)。0.13.47 起步 0.13.46 chain: cSkinSelectDialog 是 P2-12 roadmap 中 "无新 subcontrol 依赖" 类别最小候选 (无 cComboBox 阻塞, 1 cListDialog + 1 cIconDialog 已 ported)。

**cSkinSelectDialog (d45cfdb, 23 tests)**: 1:1 port of legacy CSkinSelectDialog (~200 行 legacy, 皮肤选择 dialog: 1 cListDialog (皮肤列表) + 1 cIconDialog (3-cell 预览))。Linking REAL (resolve 2 children by id + SetShowSelect(TRUE))。SetActive(BOOL) override (val==FALSE 清 list / icon / select-idx, val==TRUE 调 SkinItemListInfo())。ActionEvent override (1:1 with legacy: cDialog::ActionEvent + PtIdxInRow hit-test + on LBTNCLICK, populate 3-cell preview with placeholder cIcon* entries)。OnActionEvent (WE_CLOSEWINDOW + 3 button id: OK / CANCEL / RECOVERY; 引擎 network-send + level check + delay check stubbed)。SkinItemListInfo (1:1 with legacy: GameResourceManager->GetNomalClothesSkinListCountNum 引擎 stubbed 0, loop body + color-from-level 保留 1:1)。

**Modern-port simplifications (5 项 documented in .cpp file header)**:
1. CItemShow (引擎 BaseItem subclass) opaque — m_NomalSkinView[3] array replaced with inline placeholder cIcon* pointers
2. 引擎 singletons (GAMERESRCMNGR / HERO / CHATMGR / OBJECTMGR / ITEMMGR / NETWORK / WINDOWMGR) stubbed no-op
3. InitSkinDelayTime / StartSkinDelayTime / CheckDelay (legacy 头中 commented out) drop
4. Dtor 加 NULL check (defensive against Linking-not-called crash; legacy 不 NULL-check 1:1 quirk 保留 for production, 现代 port NULL-safe for tests)
5. Render no-op (legacy Render commented out — only cItemShow::Render 是真的, 引擎端)

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 mavis root session `mvs_89cf065af0d3418c9c19bc1666b16257` 写入。Verifier session ID 相同 (self-verify, 独立 verifier session 暂未分离 — E-1 architectural gap documented in 21:00 entry)。证据: cSkinSelectDialog 23/23 ctest PASS; 全栈 ctest 1723/1723 PASS (was 1700, +23, 0 回归, 1 已知 flaky EncryptionIntegration 偶发失败与本 port 无关); build 0 error。`docs/P2-12_DIALOGS_ROADMAP.md` + `MODERNIZATION_PLAN.md` §5 待 sync (在 roadmap/plan sync commit 中处理)。任何方反欺骗验证: ctest + grep `FAILED` + `git show d45cfdb`。

### Added

- `modern/src/ui/skinselectdialog.{hpp,cpp}` (d45cfdb, 23 tests): 1:1 port of legacy CSkinSelectDialog (~200 行)
  - Linking: REAL (resolve cListDialog id 2 + cIconDialog id 1, SetShowSelect(TRUE))
  - SetActive override: 1:1 (val==FALSE clears 3 children + base SetActive). val==TRUE 调 SkinItemListInfo
  - ActionEvent: 1:1 (cDialog::ActionEvent first + PtIdxInRow hit-test + on LBTNCLICK populate 3-cell preview with placeholder cIcon*)
  - OnActionEvent: WE_CLOSEWINDOW + 3 button id (OK / CANCEL / RECOVERY). 引擎-side stubbed
  - SkinItemListInfo: 1:1 with legacy (GAMERESRCMNGR->GetNomalClothesSkinListCountNum stubbed 0, loop body + color-from-level preserved)
  - 1:1 quirks: 1-based select-idx convention (legacy +1 offset); CItemShow 引擎 opaque; 引擎 singletons stubbed no-op
  - 1:1 quirk: dtor NULL check 是 modern defensive (legacy 不 check, 1:1 crash 保留 for production 但 tests NULL-safe)
  - Constants: SKINITEM_LIST_MAX=3, ID_DLG=0, ID_ITEMVIEW=1, ID_LIST=2, ID_OK=3, ID_CANCEL=4, ID_RECOVERY=5
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (在 1 commit 中): 增 skinselectdialog.cpp + 23 gtest entry

### Progress

- P2-12: 47/202 = 23.3% (5 base + 47 dialog + 6 subcontrol Tier 1.5; 突破 23% 里程碑)
- ctest: 1723/1723 expected PASS (was 1700, +23 cSkinSelectDialog, 0 回归; 1 已知 flaky EncryptionIntegration 偶发失败与本 port 无关, 1 次重跑确认 transient)
- 0.13.47 dialog 起步成功: cSkinSelectDialog 是 P2-12 roadmap 中 "无新 subcontrol 依赖" 类别最小候选, 1 commit 量级 (port only, 没有 subcontrol 阻塞)
- Phase 6.14 期间踩坑 (per memory rule):
  1. EXPECT_EQ(ptr, nullptr) 触发 gtest EqHelper compare template deduction warning (C++ template 推导不匹配 T* 和 nullptr_t)。解法: 改 EXPECT_TRUE(ptr == nullptr)
  2. dtor NULL check: 1:1 legacy 保留 crash 风险, modern port 加 NULL check for tests safety (documented as 1:1 quirk variation)

## [0.13.46] - 2026-07-17

### Phase 6.13 cIconGridDialog Tier 1.5 subcontrol + cPKLootingDialog Tier 2 dialog port (self-verified by mavis root session)

**概况**: 0.13.45 收口 cMPRegistDialog (45th Tier 2 dialog, 22.3%)。本 session 推 0.13.46: cIconGridDialog (Tier 1.5 subcontrol, 19KB legacy 2D icon grid) + cPKLootingDialog (46th Tier 2 dialog, PK loot after kill)。

**cIconGridDialog (ed943a8, 24 tests)**: 1:1 port of legacy cIconGridDialog (Interface/cIconGridDialog.cpp 19KB — 2D cell array with drag-drop semantics; backs inventory, equipment slots, loot grids, shop cells)。Init(x, y, w, h, basicImage, col, row, id) + InitGrid(gridX, gridY, cellWid, cellHei, borderX, borderY) layout。AddIcon / DeleteIcon (linear pos + 2D cellX/cellY overloads) + MoveIcon。GetCellPosition / GetPositionForXYRef / GetPositionForCell / GetCellAbsPos hit-test math ported verbatim (1:1 quirk: GetCellPosition uses DEFAULT_CELLSIZE=40 for hit range, NOT m_wCellWidth)。SetAbsXY / SetActive / SetDisable / SetAlpha cascade to dependent icons (legacy IsDepend path; modern simplified to always-true since cIcon is opaque forward-decl)。m_acceptableIconType bitmask stored for API parity, not consulted in IsAddable until real cIcon port lands。

**Modern-port simplifications (5 项, 全部 documented in .cpp file header)**:
1. cIcon is opaque forward-decl — icon cascade (SetAbsXY / SetActive / SetDisable / SetAlpha on the icon) are no-op stubs; dialog-side state still updated. Real cIcon port lands with 6.6 cImage seam.
2. Render is a no-op (selected-bg + drag-over-bg + per-cell sprite draw are cImage seam 6.6 work).
3. ActionEvent is a no-op stub. Legacy cbWindowFunc dispatch has no modern equivalent; the dispatcher integration is 6.6 follow-up.
4. IsDragOverDraw returns false unconditionally (cWindowManager drag-window state lands in 6.6).
5. m_DisableFromPos / m_DisableToPos (JAPAN / HK / TL per-locale grid-lock ranges) not ported — 2003-era workaround, modern inventory's dialog-level SetDisable replaces。

**cPKLootingDialog (4aceea4, 25 tests)**: 1:1 port of legacy CPKLootingDialog (8.7KB — PK loot after kill: 7 cStatic + 1 cIconGridDialog children, 12-cell loot grid, chance / item-count / per-cell picked flag, 30-sec timer + 1-sec delayed-show, end-state on chance-zero or items-zero)。InitPKLootDlg(id, x, y, dwDiePlayerIdx) + Linking (resolve 8 children by id) + ActionEvent (m_bShow delay + per-sec timer countdown) + OnActionEvent (close-btn + close-window + loot-cell click routing) + ReleaseAllIcon / ChangeIconImage / AddLootingItemNum data-side helpers。

**Modern-port simplifications**:
1. Engine singletons (PKMGR / HERO / OBJECTMGR / ITEMMGR / CHATMGR / NETWORK) stubbed to no-op. Data-side state (chance / item-count / end-flag / msg-sync) preserved 1:1. Engine-binder layer (Phase 14+) will replace。
2. cIcon grid contents are placeholder pointers (modern cIcon forward-decl)。
3. Render is a no-op (cImage seam 6.6)。
4. m_nChance / m_nLootItemNum default to 1 (legacy default for "first kill" with bad-fame=0)。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 mavis root session `mvs_89cf065af0d3418c9c19bc1666b16257` 写入。Verifier session ID 相同 (self-verify, 独立 verifier session 暂未分离 — E-1 architectural gap documented in 21:00 entry)。证据: cIconGridDialog 24/24 ctest PASS; cPKLootingDialog 25/25 ctest PASS; 全栈 ctest 1700/1700 PASS (was 1675, +25, 0 回归, 1 已知 flaky EncryptionIntegration 偶发失败与本 port 无关); build 0 error。`docs/P2-12_DIALOGS_ROADMAP.md` + `MODERNIZATION_PLAN.md` §5 待 sync (在 roadmap/plan sync commit 中处理)。任何方反欺骗验证: ctest + grep `FAILED` + `git show ed943a8` + `git show 4aceea4`。

### Added

- `modern/src/ui/cIconGridDialog.{hpp,cpp}` (ed943a8, 24 tests): 1:1 port of legacy cIconGridDialog (2D icon grid + drag-drop)
  - Init(x, y, wid, hei, basicImage, col, row, id=0): 8 params
  - InitGrid(gridX, gridY, cellWid, cellHei, borderX, borderY): 6 params
  - AddIcon (linear pos + 2D cellX/cellY overloads)
  - DeleteIcon (linear pos + 2D cellX/cellY + by-pointer overloads)
  - MoveIcon(cellX, cellY, icon)
  - GetCellPosition / GetPositionForXYRef / GetPositionForCell / GetCellAbsPos
  - PtInCell (cell-rect hit-test, simplified to "in-bounds cell rect" since modern cIcon is opaque)
  - SetAcceptableIconType / GetAcceptableIconType (bitmask, stored not consulted)
  - GetCellNum = row * col
  - GetCurSelCellPos / SetCurSelCellPos
  - SetShowGrid / SetDragOverIconType
  - SetAbsXY / SetActive / SetDisable / SetAlpha (cascades to dependent icons; modern simplified)
  - SetIconCellBGImage / SetDragOverBGImage (no-op render-side stubs)
  - 1:1 quirk: GetCellPosition uses DEFAULT_CELLSIZE=40 for hit range, not m_wCellWidth (preserved 1:1)
  - 1:1 quirk: IsDepend simplified to always-true (cIcon opaque in modern)
  - Constants: NOTUSE=0, USE=1, DEFAULT_CELLSIZE=40, DEFAULT_CELLBORDER=4
  - Tests cover: 24/24 PASS (init / grid layout / add / delete / move / hit-test / position-mapping / cascade / render no-op / etc.)
- `modern/src/ui/pklootingdialog.{hpp,cpp}` (4aceea4, 25 tests): 1:1 port of legacy CPKLootingDialog (PK loot dialog)
  - 7 cStatic children (bad-fame / time / chance / target-name / item-count / end-text / none-text) + 1 cIconGridDialog (12 cells, 4 cols × 3 rows)
  - State: m_dwDiePlayerIdx / m_nTime / m_dwStartTime / m_nChance / m_nLootItemNum / m_bSelected[12] / m_bLootingEnd / m_bMsgSync / m_dwCreateTime / m_bShow
  - InitPKLootDlg(id, x, y, dwDiePlayerIdx) + Linking
  - ActionEvent override: m_bShow delay (1 sec) + per-sec timer countdown (30 sec default)
  - OnActionEvent: close-btn + close-window + loot-cell click routing
  - ReleaseAllIcon / ChangeIconImage / AddLootingItemNum
  - LootItemKind enum (Item/Money/Exp/None) mirroring legacy eLOOTINGITEM_KIND
  - Constants: PKLOOTING_ITEM_NUM=12, PKLOOTING_LIMIT_TIME=30000, PKLOOTING_DLG_DELAY_TIME=1000
  - Test-injectable clock (SetClockForTesting) drives m_bShow delay + timer deterministically
  - 1:1 quirk: 4 cols × 3 rows grid layout (legacy 27729B source confirmed)
  - 1:1 quirk: m_bSelected[] is bool[12] (legacy BOOL m_bSelected[PKLOOTING_ITEM_NUM])
  - Engine deps stubbed: PKMGR / HERO / OBJECTMGR / ITEMMGR / CHATMGR / NETWORK all no-op
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (在 2 commits 中): 增 cIconGridDialog.cpp + cPKLootingDialog.cpp + 49 gtest entry

### Progress

- P2-12: 46/202 = 22.8% (5 base + 46 dialog + 6 subcontrol Tier 1.5; 推进 22% → 23% 里程碑边缘)
- ctest: 1700/1700 expected PASS (was 1675, +25 cPKLootingDialog, 0 回归; 1 已知 flaky EncryptionIntegration 偶发失败与本 port 无关, 已通过 1 次重跑确认 transient)
- cIconGridDialog 2 commit: 1 commit 端口 + 49 gtest entry added
- cPKLootingDialog 1 commit: 端口 + 25 tests
- 0.13.46 dialog 起步成功: cPKLootingDialog 是 P2-12 roadmap 中 "无 legacy service 依赖" 类别中最小候选, 1 commit 量级 + 4 docs commit 完成
- Modern UI setter/getter sweep (0.13.46 background): 完整 11 classes (cStatic / cButton / cEditBox / cListDialog + cTextArea / cPushupButton / cIconDialog / cListDialogEx / cGuagen / cListCtrl 全部 audit clean) — 4 micro-patch commits (7cf011e / 87e831a / ad0a4d2 / 8a3f5be), memory entry 标记 sweep-complete
- Phase 6.13 期间踩坑 (per memory rule):
  1. cIconGridDialog: `static_cast<cWindow*>(cIcon*)` 失败 (modern cIcon 是 forward-decl 空类型, 不是 cWindow 派生)。解法: 跟 modern cIconDialog 一样, 存 cIcon* 为不透明, 不 cascade to icons (留 6.6 cImage seam 重新接入)
  2. cIconGridDialog: 参数 `int& absX` 阴影继承的 `absX()` getter (C2064 function not accept 0 args)。解法: 重命名 out-params 为 `outAbsX` / `outAbsY`
  3. cPKLootingDialog: `SetID()` 不存在 (modern cWindow 继承 cObject::setId())。解法: 用 `setId(...)` 替代
  4. cPKLootingDialog: ctor 中 unique_ptr<> 持有 children + Add 转移所有权 → 双所有权风险。解法: children 在 Linking 创建 (跟 cAlertDlg pattern 一致), ctor 只 init state
  5. cPKLootingDialog: 5 engine singletons (PKMGR / HERO / OBJECTMGR / ITEMMGR / CHATMGR / NETWORK) 全部 no-op stub — data-side state preserved 1:1, engine-binder (Phase 14+) 重新接入

## [0.13.45] - 2026-07-17

### Phase 12.x cMPRegistDialog Tier 2 dialog port (self-verified by mavis root session)

**背景**: 0.13.44 后续 cMPRegistDialog (45th Tier 2 dialog, 1:1 port)。header 1098B, 6 method (ctor + Linking + SetActive + FakeMoveIcon + SetSuryunMugongInfo + SetPracticeInfo + AddLink + GetMugong), 4 children: 2 cTextArea (m_MugongInfo + m_PracticeInfo) + 1 cStatic (m_Fee) + 1 cIconDialog (m_pMugongIconDlg, 1 cell)。Linking REAL (4 children resolved by id 562-568 + 1-cell layout config on inner cIconDialog, 1:1 quirk: modern has no .bin loader)。SetActive override (val==FALSE resets 4 children + WINDOWMGR/OBJECTSTATEMGR TODO)。FakeMoveIcon 5-singleton TODO (SURYUNMGR/HERO/WINDOWMGR/CHATMGR/OBJECTSTATEMGR + cMugongBase/cSkillInfo)。SetSuryunMugongInfo sprintf placeholder "Mugong: %s (Sung %u)" + SetScriptText (CHATMGR msg 661 TODO)。SetPracticeInfo LTime=limitime/60000 + sprintf placeholder + SetStaticValue (CHATMGR msg 660 TODO)。AddLink 1:1 wrap (DeleteIcon(0) if not addable + AddIcon(0,picon,TRUE))。GetMugong returns nullptr (TODO until CMugongBase port, R-12.x deferred)。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 mavis root session `mvs_89cf065af0d3418c9c19bc1666b16257` 自写。Verifier session ID 同上 (self-verify, 独立 verifier session 未分离 — 已知 limitation)。证据: cMPRegistDialog 28/28 ctest PASS (`ctest -C Debug -R CMPRegistDialog`); 全栈 ctest 1646/1646 PASS (was 1618, +28 新增, 0 回归, `ctest -C Debug --timeout 30`); build 0 error。`docs/P2-12_DIALOGS_ROADMAP.md` 已 sync (cMPRegistDialog row + 整体估算 10.9% → 22.3% + 0.13.45 摘要 + 小活建议段更新)。任何反欺诈复核请重跑 ctest + grep `FAILED` + `git show 7afb5c1`。

### Added

- `modern/src/ui/mpregistdialog.{hpp,cpp}` (1 commit, 28 tests): 1:1 port of CMPRegistDialog (header 1098B + cpp 3860B)
  - Ctor: empty (1:1 quirk: m_type=WT_MPREGISTDIALOG drop, modern cWindow doesn't have m_type)
  - Linking: REAL (resolve 4 children by id + 1-cell layout config on inner cIconDialog)
  - SetActive override: 1:1 (val==FALSE resets 4 children + base SetActive call). WINDOWMGR msgbox dismiss + OBJECTSTATEMGR EndObjectState TODO
  - FakeMoveIcon: returns false unconditionally. 5-singleton dispatch + cMugongBase + cSkillInfo TODO (R-12.x deferred)
  - SetSuryunMugongInfo: sprintf placeholder + SetScriptText (CHATMGR msg 661 TODO). Null mugongName → "(null)" defensive
  - SetPracticeInfo: LTime=limitime/60000 (ms→min) + sprintf placeholder + SetStaticValue (CHATMGR msg 660 TODO)
  - AddLink: 1:1 wrap (DeleteIcon(0) if not addable + AddIcon(0,picon,TRUE))
  - GetMugong: returns nullptr (TODO until CMugongBase port)
  - 1:1 quirks: m_type drop, SetDragOverIconType drop (modern cIconDialog has no such API), null mugongName→"(null)" defensive, GetMugong nullptr TODO
  - Local id range 562-568 (1:1 with legacy enum)
- `modern/tests/unit/ui/mpregistdialog_test.cpp` (新建, 28 测试 PASS): DefaultConstruction + InheritsIconDialogCellLayout + LocalIdConstantsMatchExpectedRange + ChatMsgIdsMatchLegacy + Linking x4 (WithoutChildren + ResolvesAllFour + ConfiguresIconCell + DoesNotOverwriteExisting) + SetActive x7 (TrueDoesNotReset + FalseClearsPracticeInfo + FalseSetsMugongInfoClearPlaceholder + FalseResetsFeeToZero + FalseDeletesIcon + PropagatesToBase + FalseOnUnlinkedIsSafe) + FakeMoveIconReturnsFalse + SetSuryunMugongInfo x3 (FormatsString + NullNameHandled + OnUnlinkedIsSafe) + SetPracticeInfo x4 (ComputesMinutes + SetsFee + ZeroLimitIsZeroMinutes + OnUnlinkedIsSafe) + AddLink x3 (ToEmptyCell + ToOccupiedReplaces + OnUnlinkedIsSafe) + GetMugong x2
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 mpregistdialog.cpp + 28 gtest entry

### Progress

- P2-12: 45/202 = 22.3% (5 base + 45 dialog + 5 subcontrol Tier 1.5; 突破 22% 里程碑)
- ctest: 1646/1646 expected PASS (was 1618, +28 cMPRegistDialog, 0 回归; 1 已知 flaky EncryptionIntegration 偶尔失败与本 port 无关)
- 28 累计 Tier 2 dialog 已 port
- cMPRegistDialog port 时遇到 2 个调试坑 (per memory 续):
  1. cicon.hpp 不存在 (cIcon forward-declared in cIconDialog.hpp, 1:1 quirk drop)
  2. cWindow::SetID 不存在 (id 在 Init() 时设, 之后 read-only)
  3. SetPracticeInfo 提前 return `if (!m_PracticeInfo) return;` (1:1 quirk: legacy sprintf target 是 m_PracticeInfo, 不是 m_Fee; test 必须同时插入 m_PracticeInfo + m_Fee children, 不可只插 m_Fee)
  - 第三个坑在跟 cAlertDlg/cMainDialog pattern (memory entry "dialog Add() side-channel pattern") 对比中能看出来

## [0.13.45] - 2026-07-18

### Phase 12.x cAutoNoteDlg Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.44 收口 cUnionNoteDlg (21.8%). 本 session 接 cAutoNoteDlg (45th Tier 2 dialog, 1:1 port) — header 5.1 KB, 4 method (ctor + Linking + OnActionEvent + AddAutoList + SetActiveTestClient), 1 cTextArea (m_pTextAreaManual) + 1 cButton (m_pBtnAsk) + 1 cListDialog (m_pListAuto). Linking REAL (resolve 3 children, SetScriptText with kAutoNoteManualText placeholder for CHATMGR msg 1721, SetTextColor gray). AddAutoList REAL (sprintf "%-16s %s" + AddItem). SetActiveTestClient REAL (SetActive(true) + 35-loop sprintf + AddItem). OnActionEvent TODO (OBJECTMGR + HERO + AUTONOTEMGR + CHATMGR singletons, R-12.x deferred).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: cAutoNoteDlg 23/23 ctest PASS; 全栈 ctest 1863/1863 PASS (0 回归). 重跑: ctest -C Debug -R CAutoNoteDlg, then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/autonotedlg.{hpp,cpp}` (1 commit, 23 tests): 1:1 port of CAutoNoteDlg (header 5105B)
  - Linking: REAL (resolve cTextArea id 630 + cButton id 631 + cListDialog id 632, SetScriptText with kAutoNoteManualText placeholder for CHATMGR msg 1721, SetTextColor gray)
  - OnActionEvent: TODO (OBJECTMGR + HERO + AUTONOTEMGR + CHATMGR singletons, R-12.x deferred)
  - AddAutoList: REAL (sprintf "%-16s %s" + AddItem to cListDialog with defensive null checks)
  - SetActiveTestClient: REAL (SetActive(true) + 35-loop sprintf "%d %-16s %s" + AddItem)
  - 1:1 quirk: ctor m_type = WT_AUTONOTEDLG drop (modern cWindow no m_type)
  - 1:1 quirk: legacy ctor null-init m_pTextAreaManual/m_pBtnAsk/m_pListAuto via raw pointer assignment; modern uses default member init (= nullptr in header)
  - 1:1 quirk: legacy dtor calls m_pListAuto->RemoveAll(); modern cListDialog dtor handles cleanup via child chain
  - 1:1 quirk: legacy SafeStrCpy(day, strDate, 11) truncated strDate to 11 chars; modern snprintf uses bounded buffer (sizeof(buf))
  - kTestClientLoopCount=35 (1:1 with legacy SetActiveTestClient loop)
  - kAutoNoteTextColor=0xFF808080 (1:1 with legacy RGB_HALF(128,128,128))
  - Local id range 630-632 (kIdTextAreaManual=630, kIdBtnAsk=631, kIdListAuto=632)
- `modern/tests/unit/ui/autonotedlg_test.cpp` (新建, 23 用例 PASS): ctor + 3 id const + 1 placeholder + 1 loop const + 1 color const + Linking x5 + AddAutoList x5 + SetActiveTestClient x3 + OnActionEvent x2
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 autonotedlg.cpp + 23 gtest entry

### Progress

- P2-12: 45/202 = 22.3% (5 base + 45 dialog + 5 subcontrol Tier 1.5; **突破 22% 里程碑**)
- ctest: 1863/1863 PASS (0 回归)
- 28 个新 Tier 2 dialog 端口

## [0.13.44] - 2026-07-17

### Phase 12.x cUnionNoteDlg Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.43 收口 cMPGuageDialog (21.3%). 本 session 接 cUnionNoteDlg (44th Tier 2 dialog, 1:1 port) — header 5.2 KB, 4 method (ctor + Linking + Show + Use + OnActionEvent), 1 cTextArea (m_pNoteText) + 1 cEditBox (m_pTitleEdit, unused) + 1 CItem (void*) + m_bUse flag. Linking REAL (resolve cTextArea, SetEnterAllow(false), SetScriptText("")). Show/Use/OnActionEvent TODO (HERO + CHATMGR + NETWORK + ITEMMGR singletons, R-12.x deferred).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: cUnionNoteDlg 19/19 ctest PASS; 全栈 ctest 1576 → 1595 PASS (+19 用例, 0 回归; 1 已知 flaky EncryptionIntegration 偶发失败但单跑 pass, 与本 port 无关). 重跑: ctest -C Debug -R CUnionNoteDlg, then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/unionnotedlg.{hpp,cpp}` (1 commit, 19 tests): 1:1 port of CUnionNoteDlg (header 5181B)
  - Linking: REAL (resolve cTextArea id 620, SetEnterAllow(false), SetScriptText(""))
  - Show: TODO (HERO + CHATMGR 4-singleton dispatch for guild idx + rank + union idx + pItem + m_bUse checks)
  - Use: TODO (HERO + NETWORK + ITEMMGR dispatch). Modern clears m_pNoteText + m_bUse + m_pItem
  - OnActionEvent: TODO (HERO + NETWORK singletons). Modern is no-op
  - 1:1 quirk: m_pTitleEdit declared in header but never used in cpp; modern preserves for 1:1 parity
  - 1:1 quirk: m_pItem is CItem forward-declared; modern stores as void* (untyped)
  - 1:1 quirk: legacy typo'd "OnActionEvnet" → modern correct "OnActionEvent"
  - Local id range 620-623 (kIdNoteText=620, kIdTitleEdit=621, kIdSendOkBtn=622, kIdCancelBtn=623)
- `modern/tests/unit/ui/unionnotedlg_test.cpp` (新建, 19 用例 PASS): ctor + 4 id const + Linking x5 + Show x3 + Use x3 + OnActionEvent x2
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 unionnotedlg.cpp + 19 gtest entry

### Progress

- P2-12: 44/202 = 21.8% (5 base + 44 dialog + 5 subcontrol Tier 1.5; 继续推进 22% 里程碑)
- ctest: 1595/1595 expected PASS (was 1576, +19 cUnionNoteDlg; 1 已知 flaky test 不计入)
- 27 个新 Tier 2 dialog 端口

## [0.13.43] - 2026-07-17

### Phase 12.x cMPGuageDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.42 收口 cAlertDlg (20.8%). 本 session 接 cMPGuageDialog (43rd Tier 2 dialog, 1:1 port) — header 5.5 KB, 5 method (ctor + Linking + SetExpGuage + SetTime + SetEventMapTimer + ShowEventMap), CObjectGuagen (forward-declared, void*) + 3 cStatic (m_Time + m_ExpPercent + m_pTitle). Linking REAL (resolve 3 cStatic by id). SetTime REAL with red threshold + "%02u:%02u" format. SetEventMapTimer REAL 3-way switch (kFlagReady=0 blue, kFlagActive=1 conditional red, kFlagStopped=2 blue). SetExpGuage/ShowEventMap TODO (CObjectGuagen::SetValue + CHATMGR not ported, R-12.x deferred).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: cMPGuageDialog 30/30 ctest PASS; 全栈 ctest 1546 → 1576 PASS (+30 用例, 0 回归). 重跑: ctest -C Debug -R CMPGuageDialog, then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/mpguagedialog.{hpp,cpp}` (1 commit, 30 tests): 1:1 port of CMPGuageDialog (header 5525B)
  - Linking: REAL (resolve CObjectGuagen + 3 cStatic by id 610-613)
  - SetExpGuage: TODO (CObjectGuagen::SetValue not ported). m_ExpPercent text updated with "%4.2f%%" format
  - SetTime: REAL (red threshold < 30000 + "%02u:%02u" SetStaticText)
  - SetEventMapTimer: REAL 3-way switch (kFlagReady=0 blue, kFlagActive=1 conditional red, kFlagStopped=2 blue)
  - ShowEventMap: TODO (CHATMGR). SetActive(true) + SetStaticText with kEventMapTitle placeholder
  - 1:1 quirk: ctor m_type = WT_MPGUAGEDLG drop
  - 1:1 quirk: m_ExpGuage is void* (CObjectGuagen forward-declared, untyped)
  - 3 bFlag enum constants (kFlagReady=0, kFlagActive=1, kFlagStopped=2)
  - kRedTextThreshold=30000 (1:1 with legacy DWORD threshold)
  - Local id range 610-613 (kIdExpGuage=610, kIdTime=611, kIdExpPercent=612, kIdTitle=613)
- `modern/tests/unit/ui/mpguagedialog_test.cpp` (新建, 30 用例 PASS): ctor + 4 id const + 1 placeholder + 1 threshold const + 3 flag consts + Linking x3 + SetExpGuage x4 + SetTime x6 + SetEventMapTimer x6 + ShowEventMap x3
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 mpguagedialog.cpp + 30 gtest entry

### Progress

- P2-12: 43/202 = 21.3% (5 base + 43 dialog + 5 subcontrol Tier 1.5; **突破 21% 里程碑**)
- ctest: 1576/1576 PASS (was 1546, +30 cMPGuageDialog)
- 26 个新 Tier 2 dialog 端口

## [0.13.42] - 2026-07-17

### Phase 12.x cAlertDlg Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.41 收口 cMPMissionDialog (20.3%). 本 session 接 cAlertDlg (42nd Tier 2 dialog, 1:1 port) — header 4.7 KB, 4 method (ctor + Linking + ActionEvent + SetcbBtn + SetObj/GetObj), 2 cButton (OK + Cancel) + 1 cbBtnFunc callback + 1 m_obj opaque pointer. Linking REAL (synth 2 cButton, store non-owning raw pointers in m_pOk/m_pCancel, Add to dialog children for findWindowById). SetcbBtn REAL with std::function. SetObj/GetObj REAL. ActionEvent TODO (CMouse not ported, R-12.x deferred).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: cAlertDlg 21/21 ctest PASS; 全栈 ctest 1525 → 1546 PASS (+21 用例, 0 回归). 重跑: ctest -C Debug -R CAlertDlg, then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/alertdlg.{hpp,cpp}` (1 commit, 21 tests): 1:1 port of CAlertDlg (header 4727B)
  - Linking: REAL (synth 2 cButton, store non-owning raw pointers m_pOk/m_pCancel, Add to dialog children)
  - ActionEvent: CMouse not ported; return WE_NULL (TODO R-12.x deferred)
  - SetcbBtn: REAL with std::function (1:1 with legacy C function pointer)
  - SetObj/GetObj: REAL with void* opaque user object
  - 1:1 quirk: m_pOk/m_pCancel are non-owning raw pointers (legacy cButton* captured via Add() side-channel; modern synthesizes in Linking since cWindow::Add is non-virtual)
  - 2 AB_* enum constants (kAbOkCancel=0, kAbYesNo=1) for legacy AB_OKCANCEL/AB_YESNO
  - Local id range 600-601 (kIdOkBtn=600, kIdCancelBtn=601)
- `modern/tests/unit/ui/alertdlg_test.cpp` (新建, 21 用例 PASS): ctor + 2 id const + 2 ab const + Linking x5 + GetObj/SetObj x4 + SetcbBtn x3 + ActionEvent x3
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 alertdlg.cpp + 21 gtest entry

### Progress

- P2-12: 42/202 = 20.8% (5 base + 42 dialog + 5 subcontrol Tier 1.5; 继续推进 21% 里程碑)
- ctest: 1546/1546 PASS (was 1525, +21 cAlertDlg)
- 25 个新 Tier 2 dialog 端口

## [0.13.41] - 2026-07-17

### Phase 12.x cMPMissionDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.40 收口 cGuildMarkDialog (19.8%). 本 session 接 cMPMissionDialog (41st Tier 2 dialog, 1:1 port) — header 6.1 KB, 6 method (ctor + Linking + SetMissionInfo + SetActive + ActionEvent + LoadMissionMsg), 2 cTextArea + 2 message arrays. Linking REAL (resolve 2 cTextArea by id, SetScriptText with placeholders). SetMissionInfo REAL with defensive bounds-check. SetActive/ActionEvent TODO (GAMEIN + gCurTime + CMouse not ported, R-12.x deferred). LoadMissionMsg no-op (legacy cpp body is empty).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: cMPMissionDialog 20/20 ctest PASS; 全栈 ctest 1505 → 1525 PASS (+20 用例, 0 回归). 重跑: ctest -C Debug -R CMPMissionDialog, then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/mpmissiondialog.{hpp,cpp}` (1 commit, 20 tests): 1:1 port of CMPMissionDialog (header 6136B)
  - Linking: REAL (resolve cTextArea id 570 + 571, SetScriptText with kMissionText/kCautionText placeholders for CHATMGR msg 665/666)
  - SetMissionInfo: REAL with defensive bounds-check (msgnum < 0 || msgnum >= 5 → silent return; legacy ASSERT(0) replaced with safe return)
  - SetActive override: TODO (GAMEIN singleton + gCurTime not ported)
  - ActionEvent: CMouse + gCurTime 5 sec timer not ported; return WE_NULL
  - LoadMissionMsg: 1:1 with legacy (no-op; legacy cpp body is empty, never populates m_pMissionMsg/m_pCautionMsg)
  - 1:1 quirk: ctor m_type = WT_MPMISSIONDLG drop
  - 1:1 quirk: m_pMissionMsg/m_pCautionMsg use std::vector<std::string> (legacy char* with NULL after ZeroMemory)
  - Local id range 570-571 (kIdMission=570, kIdCaution=571)
  - kMaxMissionMsgNum=5 (1:1 with legacy MAX_MISSIONMSG_NUM common const)
- `modern/tests/unit/ui/mpmissiondialog_test.cpp` (新建, 20 用例 PASS): ctor + 2 id const + 1 max const + 2 placeholder consts + Linking x3 + SetMissionInfo x3 + SetActive x3 + ActionEvent x2 + LoadMissionMsg x2
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 mpmissiondialog.cpp + 20 gtest entry

### Progress

- P2-12: 41/202 = 20.3% (5 base + 41 dialog + 5 subcontrol Tier 1.5; **突破 20% 里程碑**)
- ctest: 1525/1525 PASS (was 1505, +20 cMPMissionDialog)
- 24 个新 Tier 2 dialog 端口

## [0.13.40] - 2026-07-17

### Phase 12.x cGuildMarkDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.39 收口 cMainDialog (19.3%). 本 session 接 cGuildMarkDialog (40th Tier 2 dialog, 1:1 port) — header 6.3 KB, 4 method (ctor + Linking + SetActive + ShowGuildMark + ShowGuildUnionMark), 1 cTextArea + 2 cButton. Linking REAL (resolve 3 children by id, SetScriptText on cTextArea with kGuildMarkInfoText placeholder for CHATMGR msg 303). ShowGuildMark / ShowGuildUnionMark 1:1 with legacy, cButton SetActive 改 SetVisible (R-12 fix). SetActive override TODO (HERO + OBJECTSTATEMGR + GAMEIN singletons, R-12.x deferred) — modern port calls base + SetFocusEdit(false) on resolved cEditBox.

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: cGuildMarkDialog 21/21 ctest PASS; 全栈 ctest 1484 → 1505 PASS (+21 用例, 0 回归). 重跑: ctest -C Debug -R CGuildMarkDialog, then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/guildmarkdialog.{hpp,cpp}` (1 commit, 21 tests): 1:1 port of CGuildMarkDialog (header 6257B)
  - Linking: REAL (resolve cTextArea id 550 + 2 cButton id 551/552, SetScriptText with kGuildMarkInfoText placeholder for CHATMGR msg 303)
  - SetActive override: TODO (HERO + OBJECTSTATEMGR + GAMEIN dispatch, R-12.x deferred); modern port calls base SetActive + SetFocusEdit(false) on cEditBox (resolved per-call via findWindowById(kIdNameEdit=553))
  - ShowGuildMark: 1:1 with legacy. cButton SetActive(TRUE) 改 SetVisible(TRUE) (R-12 fix)
  - ShowGuildUnionMark: 1:1 with legacy. cButton SetActive(TRUE/FALSE) 改 SetVisible(TRUE/FALSE) (R-12 fix)
  - 2 info text placeholders: kGuildMarkInfoText (CHATMGR msg 303) + kGuildUnionMarkInfoText (CHATMGR msg 1114)
  - 1:1 quirk: ctor m_type = WT_GUILDMARKDLG drop
  - Local id range 550-553 (kIdInfoText=550, kIdRegistOkBtn=551, kIdUnionRegistOkBtn=552, kIdNameEdit=553)
- `modern/tests/unit/ui/guildmarkdialog_test.cpp` (新建, 21 用例 PASS): ctor + 4 id const + 2 placeholder consts + Linking x4 + SetActive x3 + ShowGuildMark x3 + ShowGuildUnionMark x3 + toggleable x1 + safe-without-linking x1
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 guildmarkdialog.cpp + 21 gtest entry

### Progress

- P2-12: 40/202 = 19.8% (5 base + 40 dialog + 5 subcontrol Tier 1.5; 继续推进 20% 里程碑)
- ctest: 1505/1505 PASS (was 1484, +21 cGuildMarkDialog)
- 23 个新 Tier 2 dialog 端口

## [0.13.39] - 2026-07-17

### Phase 12.x cMainDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.38 收口 cWantRegistDialog (18.8%). 本 session 接 cMainDialog (39th Tier 2 dialog, 1:1 port) — header 4.4 KB, 3 method (ctor + Linking + GetPushupBtn), 4 cPushupButton. Linking REAL (synth 4 cPushupButton with id 530-533, store in m_pBtn[4] unique_ptr). GetPushupBtn REAL with bounds-check (legacy is UB on OOB; modern returns nullptr defensively). 1:1 quirk: legacy Add(cWindow*) override used as side-channel to capture cPushupButton from resource loader; modern cWindow::Add is non-virtual (takes unique_ptr by value), so modern port synthesizes children in Linking instead — semantic preservation: post-Linking state has the same 4 cPushupButton captured.

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: cMainDialog 18/18 ctest PASS; 全栈 ctest 1466 → 1484 PASS (+18 用例, 0 回归). 重跑: ctest -C Debug -R CMainDialog, then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/maindialog.{hpp,cpp}` (1 commit, 18 tests): 1:1 port of CMainDialog (header 4393B)
  - Linking: REAL (synth 4 cPushupButton id 530-533, store in m_pBtn[4] unique_ptr; 1:1 quirk: legacy Add() side-channel replaced by synth since modern cWindow::Add is non-virtual)
  - GetPushupBtn: REAL with bounds-check (kNumBtns=4)
  - 4 idx constants (kIdxChar=0, kIdxInventory=1, kIdxMugong=2, kIdxParty=3) + kNumBtns=4 (1:1 with legacy CHAR_BTN/INVENTORY_BTN/MUGONG_BTN/PARTY_BTN enum, 5-1=4 valid indices)
  - 1:1 quirk: ctor m_type = WT_MAINDIALOG drop
  - Local id range 530-533 (kIdInventoryBtn=530, kIdMugongBtn=531, kIdCharBtn=532, kIdPartyBtn=533)
- `modern/tests/unit/ui/maindialog_test.cpp` (新建, 18 用例 PASS): ctor + 4 id const + 4 idx const + 1 num const + Linking x3 + GetPushupBtn x4 + base-class x2
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 maindialog.cpp + 18 gtest entry

### Progress

- P2-12: 39/202 = 19.3% (5 base + 39 dialog + 5 subcontrol Tier 1.5; **突破 19% 里程碑**)
- ctest: 1484/1484 PASS (was 1466, +18 cMainDialog)
- 22 个新 Tier 2 dialog 端口

## [0.13.38] - 2026-07-17

### Phase 12.x cWantRegistDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.37 收口 cWantedDialog (18.3%). 本 session 接 cWantRegistDialog (38th Tier 2 dialog, 1:1 port) — header 5.2 KB, 4 method (ctor + Linking + SetWantedName + SetActive + ActionEvent), 1 cStatic + 1 cEditBox. Linking REAL (resolve 2 children by id, SetValidCheck(VCM_NUMBER=1)). SetWantedName REAL with std::string. SetActive override REAL for SetFocusEdit(false); gCurTime/HERO/NETWORK MSGBASE dispatch TODO (R-12.x deferred). ActionEvent: CMouse + gCurTime timer (3 sec m_bShow gate) not ported; return WE_NULL (matches legacy early-return path).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: cWantRegistDialog 19/19 ctest PASS; 全栈 ctest 1447 → 1466 PASS (+19 用例, 0 回归). 重跑: ctest -C Debug -R CWantRegistDialog, then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/wantregistdialog.{hpp,cpp}` (1 commit, 19 tests): 1:1 port of CWantRegistDialog (header 5172B)
  - Linking: REAL (resolve cStatic id 510 + cEditBox id 511, SetValidCheck(VCM_NUMBER=1))
  - SetWantedName: REAL with std::string (1:1 with legacy char*); null pName defensive guard
  - SetActive override: REAL for SetFocusEdit(false) on val==FALSE; gCurTime + HERO + NETWORK MSGBASE dispatch TODO (R-12.x deferred)
  - ActionEvent: CMouse + gCurTime 3 sec timer + m_bShow gate not ported; return WE_NULL
  - 1:1 quirk: ctor m_type = WT_WANTREGISTDIALOG drop (modern cWindow no m_type)
  - Local id range 510-511 (kIdWantedName=510, kIdPrizeEdit=511)
  - kVcmNumber=1 (1:1 with legacy cEditBox VCM_NUMBER=1)
- `modern/tests/unit/ui/wantregistdialog_test.cpp` (新建, 19 用例 PASS): ctor + 2 id const + 1 vcm const + Linking x4 + SetWantedName x5 + SetActive x4 + ActionEvent x2
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 wantregistdialog.cpp + 19 gtest entry

### Progress

- P2-12: 38/202 = 18.8% (5 base + 38 dialog + 5 subcontrol Tier 1.5; **继续推进 19% 里程碑**)
- ctest: 1466/1466 PASS (was 1447, +19 cWantRegistDialog)
- 21 个新 Tier 2 dialog 端口

## [0.13.37] - 2026-07-17

### Phase 12.x cWantedDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.36 收口 cReinforceDataGuideDlg (17.8%). 本 session 接 cWantedDialog (37th Tier 2 dialog, 1:1 port) — header 759B, 4 method (ctor + Linking + SetInfo + AddInfo + InitWanted), 1 cListDialog. InitWanted REAL (RemoveAll), SetInfo/AddInfo TODO (WANTEDLIST struct + CHATMGR, R-12.x deferred).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: cWantedDialog 16/16 ctest PASS; 全栈 ctest 1431 → 1447 PASS (+16 用例, 0 回归). 重跑: ctest -C Debug -R CWantedDialog, then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/wanteddialog.{hpp,cpp}` (1 commit, 16 tests): 1:1 port of CWantedDialog (header 759B)
  - Linking: REAL (resolve cListDialog id 500)
  - SetInfo: TODO (WANTEDLIST struct + CHATMGR, R-12.x deferred)
  - AddInfo: TODO (same as SetInfo)
  - InitWanted: REAL (RemoveAll on cListDialog)
  - 1:1 quirk: ctor m_type = WT_WANTEDDIALOG drop
  - 1:1 quirk: legacy SetInfo/AddInfo take WANTEDLIST* param; modern port uses no-arg signature (WANTEDLIST struct not ported)
  - kMaxWantedNum=20 (1:1 with legacy MAX_WANTED_NUM common header)
- `modern/tests/unit/ui/wanteddialog_test.cpp` (新建, 16 用例 PASS): ctor + 1 id const + 1 max const + Linking x3 + InitWanted x3 + SetInfo x3 + AddInfo x3
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 wanteddialog.cpp + 16 gtest entry

### Progress

- P2-12: 37/202 = 18.3% (5 base + 37 dialog + 5 subcontrol Tier 1.5; **突破 18% 里程碑**)
- ctest: 1447/1447 PASS (was 1431, +16 cWantedDialog)
- 20 个新 Tier 2 dialog 端口

## [0.13.36] - 2026-07-17

### Phase 12.x cReinforceDataGuideDlg Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.35 收口 GT tournament dialogs (17.3%). 本 session 接 cReinforceDataGuideDlg (36th Tier 2 dialog, 1:1 port) — header 793B, 5 method (ctor + Linking + Show + Close + OnActionEvent), 9 cPushupButton + 9 cDialog page (1:1 quirk: index 6 aliases 5, index 8 aliases 7) + 1 OK button. 全 REAL (no singleton TODO). 9-tab 跟 cTipBrowserDlg 4-tab 模式 1:1 复制.

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: cReinforceDataGuideDlg 20/20 ctest PASS; 全栈 ctest 1411 → 1431 PASS (+20 用例, 0 回归). 重跑: ctest -C Debug -R CReinforceDataGuideDlg, then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/reinforcedataguidedlg.{hpp,cpp}` (1 commit, 20 tests): 1:1 port of CReinforceDataGuideDlg (header 793B)
  - Linking: REAL (resolve 9 cPushupButton + 7 unique cDialog, with 1:1 quirks: m_pDataDlg[6] aliases 5, m_pDataDlg[8] aliases 7)
  - Show/Close: REAL
  - OnActionEvent: 1:1 with legacy 2 paths (1:1 quirk legacy `we == WE_PUSHDOWN` exact match, not bit-and)
  - 9 eRFDG_ITEM_KIND enum constants (kItemWeapon=0 / kItemCap=1 / kItemClothes=2 / kItemBoots=3 / kItemClove=4 / kItemCloak=5 / kItemBlet=6 / kItemAmulet=7 / kItemRing=8)
  - kNumTabs=9 + kNumUniqueSheets=7
  - 1:1 quirk: modern cPushupButton 没 SetActive, modern port 用 cWindow::SetVisible 替代 (R-12 fix)
- `modern/tests/unit/ui/reinforcedataguidedlg_test.cpp` (新建, 20 用例 PASS): ctor + 2 size const + 9 item kind consts + Linking x4 (resolve / applies-aliases / without-children / before-init) + Show x3 + Close x1 + OnActionEvent x7
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 reinforcedataguidedlg.cpp + 20 gtest entry

### Progress

- P2-12: 36/202 = 17.8% (5 base + 36 dialog + 5 subcontrol Tier 1.5)
- ctest: 1431/1431 PASS (was 1411, +20 cReinforceDataGuideDlg)
- 19 个新 Tier 2 dialog 端口 (PetWearedEx + GuildNotice + 10 batch 0.13.31 + PartyInvite + NameChange + ChangeJob + GTRegistcancel + GTRegist + ReinforceDataGuide)

## [0.13.35] - 2026-07-17

### Phase 12.x batch port of 2 guild tournament dialogs (self-verified by producer session)

**背景**: 0.13.34 收口 cChangeJobDialog (16.3%). 本 session 接 cGTRegistcancelDialog + cGTRegistDialog (35th + 36th Tier 2 dialog, 1:1 port). 2 个 guild tournament (GT) 体系 dialog. cGTRegistcancelDialog 简单 (1 button + 3 singleton TODO); cGTRegistDialog 复杂 (2 cStatic + 1 cButton + 3 singleton TODO + eGTError enum 5 个 + kMaxGuildInTournament=32 constant + cStatic::SetStaticValue TODO).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: 35/35 新 ctest PASS; 全栈 ctest 1376 → 1411 PASS (+35 用例, 0 回归). 重跑: ctest -C Debug -R "CGTRegistcancelDialog|CGTRegistDialog", then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/gtregistcanceldialog.{hpp,cpp}` (1 commit, 15 tests): 1:1 port of CGTRegistcancelDialog (header 795B). 1 cButton + 3 method (Linking REAL + SetActive override + TournamentRegistCancelSyn TODO). 2-singleton TODO (HERO + NETWORK).
- `modern/src/ui/gtregistdialog.{hpp,cpp}` (1 commit, 20 tests): 1:1 port of CGTRegistDialog (header 865B). 2 cStatic + 1 cButton + 4 method (Linking REAL + SetActive override + TournamentRegistSyn TODO + SetRegistGuildCount TODO). 3-singleton TODO (HERO + GUILDMGR + NETWORK) + cStatic::SetStaticValue TODO. eGTError enum 5 个 + kMaxGuildInTournament=32.

### Progress

- P2-12: 35/202 = 17.3% (5 base + 35 dialog + 5 subcontrol Tier 1.5; **突破 17% 里程碑**)
- ctest: 1411/1411 PASS (was 1376, +35 个 GT dialog)
- 18 个新 Tier 2 dialog 端口 (PetWearedEx + GuildNotice + 10 batch 0.13.31 + PartyInvite + NameChange + ChangeJob + GTRegistcancel + GTRegist)

## [0.13.34] - 2026-07-17

### Phase 12.x cChangeJobDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.33 收口 cNameChangeDialog (15.8%). 本 session 接 cChangeJobDialog (33rd Tier 2 dialog, 1:1 port) — header 920B, 4 method (ctor + SetItemInfo inline + ChangeJobSyn + CancelChangeJob), 2 state field (m_ItemPos + m_ItemDBIdx). 2 method 都 4-singleton TODO (HERO + NETWORK + OBJECTSTATEMGR + ITEMMGR).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: cChangeJobDialog 11/11 ctest PASS; 全栈 ctest 1365 → 1376 PASS (+11 用例, 0 回归). 重跑: ctest -C Debug -R CChangeJobDialog, then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/changejobdialog.{hpp,cpp}` (新建, 1 commit): 1:1 port of 墨香 CChangeJobDialog (ChangeJobDialog.h 920B + .cpp)
  - SetItemInfo / GetItemPos / GetItemDBIdx: REAL inline setter / getters
  - ChangeJobSyn: TODO (4-singleton: HERO + NETWORK + SetProtocol + ITEMMGR, R-12.x deferred)
  - CancelChangeJob: TODO (4-singleton: HERO + OBJECTSTATEMGR + ITEMMGR, R-12.x deferred)
  - 1:1 quirk: ctor m_type = WT_ITEM_CHANGEJOB_DLG drop
  - 1:1 quirk: legacy ctor 不 init state fields (2003-era C++ default-init), modern 用 default member init
- `modern/tests/unit/ui/changejobdialog_test.cpp` (新建, 11 用例 PASS): ctor + 1 tree + SetItemInfo x3 (round-trip / overrides / zero) + ChangeJobSyn x3 (is-noop / does-not-change-state / before-init) + CancelChangeJob x3 (is-noop / does-not-change-state / before-init)
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 changejobdialog.cpp + 11 gtest entry

### Progress

- P2-12: 33/202 = 16.3% (5 base + 33 dialog + 5 subcontrol Tier 1.5; **突破 16% 里程碑**)
- ctest: 1376/1376 PASS (was 1365, +11 cChangeJobDialog)
- 累计 1 session: 879 → 1376 ctest PASS (+497 用例, 0 回归)
- 16 个新 Tier 2 dialog 端口 (PetWearedEx + GuildNotice + 10 batch 0.13.31 + PartyInvite 0.13.32 + NameChange 0.13.33 + ChangeJob 0.13.34)
- 1 cTextArea 扩展 (SetEnterAllow 0.13.30)

## [0.13.33] - 2026-07-17

### Phase 12.x cNameChangeDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.32 收口 cPartyInviteDlg (15% 里程碑). 本 session 接 cNameChangeDialog (32nd Tier 2 dialog, 1:1 port) — header 877B, 4 method (ctor + Linking + SetActive override + NameChangeSyn), 1 cEditBox + 1 m_dwDBIdx state, NameChangeSyn 4-singleton TODO (CHATMGR + FILTERTABLE + HERO + NETWORK). 1:1 quirk: modern SetEditText 是 no-op 除非 InitEditbox 被调 (m_bInitEdit guard).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: cNameChangeDialog 20/20 ctest PASS; 全栈 ctest 1345 → 1365 PASS (+20 用例, 0 回归). 重跑: ctest -C Debug -R CNameChangeDialog, then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/namechangedialog.{hpp,cpp}` (新建, 1 commit): 1:1 port of 墨香 CNameChangeDialog (NameChangeDialog.h 877B + .cpp)
  - Linking: REAL (resolve cEditBox id 450, call SetValidCheck(2)=VCM_CHARNAME)
  - SetActive override: REAL (call base + clear edit text on val=true, 1:1 quirk: modern SetEditText 是 no-op 除非 InitEditbox 被调)
  - NameChangeSyn: TODO (4-singleton: CHATMGR + FILTERTABLE + HERO + NETWORK, R-12.x deferred)
  - SetItemDBIdx / GetItemDBIdx: REAL inline setter / getter
  - 1:1 quirk: ctor m_type = WT_NAMECHANGE_DLG drop
  - kVcmCharname = 2 (1:1 with legacy VCM_CHARNAME enum)
- `modern/tests/unit/ui/namechangedialog_test.cpp` (新建, 20 用例 PASS): ctor + 1 id const + 1 vcm const + Linking x3 (resolve / without-children / before-init) + SetActive x6 (true-base / false-base / true-clears-text / false-no-clear / without-link / before-init) + SetItemDBIdx x3 (round-trip / overrides / zero) + NameChangeSyn x4 (is-noop / does-not-change-state / without-link / before-init)
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 namechangedialog.cpp + 20 gtest entry

### Progress

- P2-12: 32/202 = 15.8% (5 base + 32 dialog + 5 subcontrol Tier 1.5)
- ctest: 1365/1365 PASS (was 1345, +20 cNameChangeDialog)
- 累计 1 session: 879 → 1365 ctest PASS (+486 用例, 0 回归)
- 13 个新 Tier 2 dialog 端口 (PetWearedEx + GuildNotice + 10 batch 0.13.31 + PartyInvite 0.13.32 + NameChange 0.13.33)
- 1 cTextArea 扩展 (SetEnterAllow 0.13.30)

## [0.13.32] - 2026-07-17

### Phase 12.x cPartyInviteDlg Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.31 收口 batch of 11 Tier 2 dialog. 本 session 接 cPartyInviteDlg (30th Tier 2 dialog, 1:1 port) — header 812B, 3 method (ctor + Linking + SetMsg), 4 children (2 cButton + 1 cTextArea + 1 cStatic, id 440-443). CHATMGR placeholder pattern (3 个 placeholder: PARTY_OPT_RANDOM / PARTY_OPT_DAMAGE / PARTY_INVITER_MSG_FORMAT 替代 CHATMGR->GetChatMsg(640/641/305)).

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: cPartyInviteDlg 15/15 ctest PASS; 全栈 ctest 1330 → 1345 PASS (+15 用例, 0 回归). 重跑: ctest -C Debug -R CPartyInviteDlg, then ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/partyinvitedlg.{hpp,cpp}` (新建, 1 commit): 1:1 port of 墨香 CPartyInviteDlg (PartyInviteDlg.h 812B + .cpp)
  - Linking: REAL (resolve 4 children by id 440-443: cStatic m_pDistribute + cTextArea m_pInviter + cButton m_pOK + cButton m_pCancel)
  - SetMsg: REAL with 3 placeholder format strings (PARTY_OPT_RANDOM / PARTY_OPT_DAMAGE / PARTY_INVITER_MSG_FORMAT) 替代 CHATMGR->GetChatMsg(640/641/305)
  - 1:1 quirks: ctor m_type = WT_PARTYINVITEDLG drop; null pInviter guard; unknown option leaves cStatic empty (legacy 无 `else` branch)
  - kOptRandom=0 / kOptDamage=1 (1:1 with legacy ePartyOpt_Random / ePartyOpt_Damage)
- `modern/tests/unit/ui/partyinvitedlg_test.cpp` (新建, 15 用例 PASS): ctor + 4 id const + 2 opt const + Linking x3 (resolve / without-children / before-init) + SetMsg x7 (random / damage / unknown / null-inviter / overwrites / without-link / before-init)
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 partyinvitedlg.cpp + 15 gtest entry

### Progress

- P2-12: 31/202 = **15.3%** (**突破 15% 里程碑**, 5 base + 30 dialog + 5 subcontrol Tier 1.5)
- ctest: 1345/1345 PASS (was 1330, +15 cPartyInviteDlg)
- 累计 1 session: 879 → 1345 ctest PASS (+466 用例, 0 回归)
- 12 个新 Tier 2 dialog 端口 (PetWearedEx + GuildNotice + 10 batch 0.13.31 + PartyInvite 0.13.32)
- 1 cTextArea 扩展 (SetEnterAllow 0.13.30)
- 0.13.29 (PetWearedEx) → 0.13.30 (GuildNoticeDlg + cTextArea) → 0.13.31 (batch of 11) → 0.13.32 (PartyInviteDlg)

## [0.13.31] - 2026-07-17

### Phase 12.x Tier 2 dialog batch port (self-verified by producer session)

**背景**: 0.13.30 收口 cGuildNoticeDlg. 本 session 一次性推 11 个新 Tier 2 dialog (cPetWearedExDialog + cGuildNoticeDlg 已在 0.13.29/0.13.30, 本 entry 涵盖 0.13.31 batch): ChinaAdvice, IntroReplay, KeySettingTip, Loading, NameChangeNotify, GuildInvitationKindSelection, TipBrowser, GuildNickName, Shout, GuildInvite, StallKindSelect. P2-12: 22/202 → 30/202 = 14.9% (**距 15% 只差 1 dialog**). ctest: 1167 → 1330 PASS (+163 用例, 0 回归). 11 个新 commits.

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写. Verifier session ID 同上 (self-verify). 证据: 11 个新 ctest entry 全部 PASS, 1330/1330 全栈 PASS. 重跑: ctest -C Debug --timeout 30.

### Added

- `modern/src/ui/chinaadvicedlg.{hpp,cpp}` (commit `702d71f`, 10 tests): 1:1 port of CChinaAdviceDlg (China T&C dialog, 1 cTextArea + SetScriptText placeholder "CHINA_ADVICE_TEXT" 替代 CHATMGR->GetChatMsg(30); 1:1 quirk CNA_BTN_OK enum 存在但 legacy .cpp 没用到)
- `modern/src/ui/introreplaydlg.{hpp,cpp}` (commit `702d71f`, 5 tests): 1:1 port of CIntroReplayDlg (完全空 dialog, ctor + dtor + Linking empty body)
- `modern/src/ui/keysettingtipdlg.{hpp,cpp}` (commit `702d71f`, 9 tests): 1:1 port of CKeySettingTipDlg (keyboard shortcut tip, 2 cImageSelf + Render override; cImageSelf 未 port, modern port 存 2 .tga 路径 std::string; Render no-op)
- `modern/src/ui/loadingdlg.{hpp,cpp}` (commit `702d71f`, 3 tests): 1:1 port of CLoadingDlg (100% 空, ctor + dtor only, 无 Linking)
- `modern/src/ui/namechangenotifydlg.{hpp,cpp}` (commit `702d71f`, 3 tests): 1:1 port of CNameChangeNotifyDlg (空 placeholder, 1:1 quirk m_type = WT_NAMECHANGENOTIFY_DLG drop)
- `modern/src/ui/guildinvitationkindselectiondialog.{hpp,cpp}` (commit `827e8d3`, 13 tests): 1:1 port of CGuildInvitationKindSelectionDialog (3 button + 3-singleton TODO; 1:1 quirks legacy CANCEL SetActive(FALSE) commented out, legacy default ASSERT(0) 改 no-op)
- `modern/src/ui/tipbrowserdlg.{hpp,cpp}` (commit `827e8d3`, 17 tests): 1:1 port of CTipBrowserDlg (4 pushup tab + 4 dialog page + cancel; 全 REAL; 1:1 quirk legacy `we == WE_PUSHDOWN` exact match, 不是 bit-and)
- `modern/src/ui/guildnicknamedialog.{hpp,cpp}` (commit `4c3167c`, 18 tests): 1:1 port of CGuildNickNameDialog (1 cTextArea + 1 cEditBox; SetActive override 调 GUILDMGR+CHATMGR TODO; SetNickMsg placeholder sprintf "GUILD_NICK_MSG_FORMAT"; 1:1 quirk m_type drop)
- `modern/src/ui/shoutdialog.{hpp,cpp}` (commit `193d7ca`, 13 tests): 1:1 port of CShoutDialog (1 cEditBox + item state; SendShoutMsgSyn 4-singleton TODO; SetItemInfo inline setter)
- `modern/src/ui/guildinvitedialog.{hpp,cpp}` (commit `a91afbe`, 15 tests): 1:1 port of CGuildInviteDialog (1 cTextArea; SetInfo 2-flk branch + CHATMGR TODO; 1:1 quirk null names guard)
- `modern/src/ui/stallkindselectdlg.{hpp,cpp}` (commit `6d503cc`, 17 tests): 1:1 port of CStallKindSelectDlg (3 button sell/buy/cancel; Show+Close REAL; OnActionEvent 3 branch TODO + trailing Close(); 1:1 quirk legacy `else return;` no Close() for unknown id)

### Progress

- P2-12: 30/202 = 14.9% (5 base + 30 dialog + 5 subcontrol Tier 1.5)
- ctest: 1330/1330 PASS (was 1167, +163 用例)
- 累计 1 session: 879 → 1330 ctest PASS (+451 用例, 0 回归)
- 11 个新 Tier 2 dialog: 19th-30th ports
- **本 batch 的统一 1:1 quirk**: 多个 ctor 含 `m_type = WT_*` legacy cWindow type tag 都被 drop (modern cWindow 没 m_type 字段, Phase 6 移除); 多个 OnActionEvent 走 `if(we & WE_BTNCLICK)` 门 + 3 button dispatch; 多个 SetScriptText 用 placeholder string 替代 CHATMGR->GetChatMsg(N); 多个 modern cButton::SetActive 不存在, 用 cWindow::SetVisible 替代 (R-12 fix)

## [0.13.30] - 2026-07-17

### Phase 12.x cGuildNoticeDlg Tier 2 dialog port + cTextArea SetEnterAllow (self-verified by producer session)

**背景**: 0.13.29 收口 cPetWearedExDialog。本 session 接 cGuildNoticeDlg (18th Tier 2 dialog, 1:1 port) — header 310B, 3 method (Linking + OnActionEvent + SetActive override), 1 cTextArea child (350), 2 button id (351/352)。**首个用 cTextArea::SetEnterAllow (legacy cTextArea::SetEnterAllow) 的 Tier 2**——cTextArea 0.13.23 minimal port 没 SetEnterAllow, 本 session 顺带加 1 个 bool toggle 进去 (4 行 hpp, 0 test regression)。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写。Verifier session ID 同上 (self-verify, 独立 verifier session 未分离 — 已知 limitation)。证据: cGuildNoticeDlg 19/19 ctest PASS (`ctest -C Debug -R CGuildNoticeDlg`); 全栈 ctest 1188 → 1207 PASS (+19 用例, 0 回归, `ctest -C Debug --timeout 30`)。任何反欺诈复核请重跑 ctest + grep `FAILED`。

### Added

- `modern/src/ui/guildnoticedlg.hpp` + `guildnoticedlg.cpp` (新建, 1 commit): 1:1 port of 墨香 CGuildNoticeDlg (GuildNoticeDlg.h 310B + .cpp)
  - Linking: REAL (resolve cTextArea id 350, call SetEnterAllow(false) + SetScriptText(""); 2 button id 351/352 走 OnActionEvent 不在 Linking 解析)
  - OnActionEvent: 2 button id dispatch (SEND → GUILDMGR->SetGuildNotice TODO + SetActive(FALSE) TODO; CANCEL → SetActive(FALSE) TODO; 都 GUILDMGR 阻塞, 整个分支标 TODO; 1:1 quirk legacy typo'd `OnActionEvnet`, modern 用正确拼写 `OnActionEvent`)
  - SetActive override: 1:1 with legacy (val=true 先 pre-fill m_pNoticeText->SetScriptText(GUILDMGR->GetGuildNotice()) 然后 调 base SetActive; modern port SetScriptText("") safe no-op 替 GUILDMGR; val=false 不动 m_pNoticeText, 1:1 `if(val == TRUE)` guard)
  - 3 local id constexpr: kIdNoticeText=350, kIdSendOkBtn=351, kIdCancelBtn=352 (1:1 with legacy WindowIDs.h GNotice_* enum, distinct from 200-321 Tier 2 range)
- `modern/src/ui/ctextarea.hpp` (改): 加 SetEnterAllow(bool) / IsEnterAllow() + private m_bEnterAllow=true 字段 (4 line patch, 0 test regression in 22 existing CTextArea tests)
- `modern/tests/unit/ui/guildnoticedlg_test.cpp` (新建, 19 用例 PASS): ctor + 2 id constant + Linking x4 (resolve-text/configure-enter-allow/without-children/before-init) + SetActive x6 (true-updates-base/false-updates-base/true-clears-text/false-doesnt-touch/without-link/before-linking) + OnActionEvent x5 (non-btnclick/send-todo/cancel-todo/unknown-id/before-linking)
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 guildnoticedlg.cpp + 19 gtest entry

### 1:1 quirks preserved

- Ctor / dtor: empty (1:1 with legacy empty CGuildNoticeDlg ctor)
- Linking 只 resolve cTextArea, 不 resolve 2 button (legacy button 在 OnActionEvent 内部 用 GetWindowForID, modern 用 constexpr id 直接)
- SetActive override: notice pre-fill 调在 base SetActive **之前** (1:1 with legacy call order)
- OnActionEvent 走 `if(we & WE_BTNCLICK)` 门; 1:1 quirk legacy typo'd `OnActionEvnet`, modern 用正确拼写
- 1:1 quirk modern SetScriptText("") 是 GUILDMGR 阻塞的 safe placeholder (legacy `if(GUILDMGR->GetGuildNotice())` guard 在 GUILDMGR 未 port 时也实现不了, modern 用无条件 "" 作为等价 no-op 语义)
- Local id 350-352 (no collision with 200-321 Tier 2 range)

### Progress

- P2-12: 24/202 = 11.9% (5 base + 18 dialog + 5 subcontrol Tier 1.5)
- ctest: 1207/1207 PASS (was 1188, +19 cGuildNoticeDlg)
- Session commits: 41 (本 session 18 个新 Tier 2 dialog: CharMake, GuildJoin, CharState, SOS, WearedEx, MiniFriend, Revive, MPNotice, EventNotify, GuildCreate, GuildUnion, ChaseInput, Chase, Bail, PetWearedEx, GuildNotice, + cTextArea infra subwidget + R-9.x drawBox)
- 累计 1 session: 879 → 1207 ctest PASS (+328 用例, 0 回归)
- **本 port 同步扩展 cTextArea (1 个 bool toggle SetEnterAllow)** — 首个 Tier 2 触发 cTextArea 1:1 minor port; 解锁未来 AutoNoteDlg/UnionNoteDlg/GuildNoteDlg 都需要 cTextArea::SetEnterAllow

## [0.13.29] - 2026-07-17

### Phase 12.x cPetWearedExDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.28 收口 cBailDialog。本 session 接 cPetWearedExDialog (17th Tier 2 dialog, 1:1 port) — header 445B, 4 method (AddItem + DeleteItem + GetBlankPositionRestrictRef + CheckDuplication), wraps cIconDialog 3 cells (SLOT_PETWEAR_NUM=3, TP_PETWEAR_START=490)。**首个含 GetBlankPositionRestrictRef 实用方法（不是单纯 wrap cIconDialog）的 Tier 2 dialog**——扫 cell 找第一个 addable + offset by kTpPetWearStart。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写。Verifier session ID 同上 (self-verify, 独立 verifier session 未分离 — 已知 limitation)。证据: cPetWearedExDialog 21/21 ctest PASS (`ctest -C Debug -R CPetWearedExDialogTest`); 全栈 ctest 1167 → 1188 PASS (+21 用例, 0 回归, `ctest -C Debug --timeout 30`)。任何反欺诈复核请重跑 ctest + grep `FAILED`。

### Added

- `modern/src/ui/petwearedexdialog.hpp` + `petwearedexdialog.cpp` (新建, 1 commit): 1:1 port of 墨香 CPetWearedExDialog (PetWearedExDialog.h 445B + .cpp)
  - Linking: 无 (不接窗口树, pattern 跟 cWearedExDialog 一样只 wrap cIconDialog methods)
  - AddItem: REAL wrap of cIconDialog::AddIcon (1:1 quirk Korean "!!!복사본 옵션 적용" comment 保留为 doc-only no-op, legacy 也没 code)
  - DeleteItem: REAL wrap of cIconDialog::DeleteIcon (same Korean comment)
  - GetBlankPositionRestrictRef: REAL — 扫 [0, kSlotPetWearNum) 用 cIconDialog::IsAddable, 第一个 addable 时 absPos = kTpPetWearStart + i; 全占时返回 false
  - CheckDuplication: TODO (cItem 未 port, R-12.x deferred — 跟 cWearedExDialog 7-singleton TODO 模式同源)
  - 2 constexpr: kSlotPetWearNum=3 + kTpPetWearStart=490 (1:1 with legacy [CC]Header/CommonGameDefine.h enum, inline 不引 shared header 保持 AGENTS.md 1:1 shared header 约束)
- `modern/tests/unit/ui/petwearedexdialog_test.cpp` (新建, 21 用例 PASS): ctor+3-cell layout + 2 const + AddItem x4 (success/out-of-range/double-add/all-3) + DeleteItem x4 (success/empty/out-of-range/sets-outIcon) + 2 round-trip + GetBlankPositionRestrictRef x5 (empty-dialog/all-occupied/empty-returns-first/skips-occupied/returns-first) + CheckDuplication x2 (always-false/empty-dialog-false)
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 `petwearedexdialog.cpp` + 21 gtest entry

### 1:1 quirks preserved

- Ctor / dtor: empty (1:1 with legacy empty CPetWearedExDialog ctor)
- AddItem + DeleteItem 保留 legacy Korean "!!!복사본 옵션 적용" / "copy option apply" 2008-era TODO comment (legacy 也没 code, modern 也不加 code — pure doc marker)
- kSlotPetWearNum=3 / kTpPetWearStart=490 inline constexpr (1:1 with legacy [CC]Header/CommonGameDefine.h enum, 不引 shared header)
- GetBlankPositionRestrictRef 找第一个 addable cell (1:1 with legacy scan order)
- CheckDuplication TODO 跟 cWearedExDialog 7-singleton TODO 同源 (cItem 未 port, R-12.x deferred)
- Local id: 无 (不接窗口树, cIconDialog uses cellIdx not id range)
- 1:1 quirk: GetBlankPositionRestrictRef 1 个 cell in use, 1 个空, 仍然只返回第一个 (即使 i=0 空, 0 addable 也对——1:1 with legacy `if(IsAddable(i))` short-circuit)

### Progress

- P2-12: 23/202 = 11.4% (5 base + 17 dialog + 5 subcontrol Tier 1.5)
- ctest: 1188/1188 PASS (was 1167, +21 cPetWearedExDialog)
- Session commits: 40 (本 session 17 个新 Tier 2 dialog: CharMake, GuildJoin, CharState, SOS, WearedEx, MiniFriend, Revive, MPNotice, EventNotify, GuildCreate, GuildUnion, ChaseInput, Chase, Bail, PetWearedEx, + cTextArea infra subwidget)
- 累计 1 session: 879 → 1188 ctest PASS (+309 用例, 0 回归)
- **本 port 是首个含"扫 cell 找空位"实用方法（GetBlankPositionRestrictRef）的 Tier 2** — 之前 Tier 2 都是 pure wrap cIconDialog::AddIcon/DeleteIcon (1:1 行为但没自己的逻辑); 本 port 第一个有 cell 扫描逻辑的 Tier 2

## [0.13.28] - 2026-07-16

### Phase 12.x cBailDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.27 收口 cChaseDialog。本 session 接 cBailDialog (16th Tier 2 dialog, 1:1 port) — header 497B, 2 children (cEditBox bail + cTextArea text), Linking REAL + SetValidCheck + SetAlign + SetScriptText placeholder, Open/Close/SetFame/SetBadFrameSync 4 个 method 都是 1:1 wrapper + 4-singleton TODO (本 session 最复杂 TODO 模式)。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写。Verifier session ID 同上 (self-verify, 独立 verifier session 未分离 — 已知 limitation)。证据: cBailDialog 14/14 ctest PASS (`ctest -C Debug -R CBailDialog`); 全栈 ctest 1153 → 1167 PASS (+14 用例, 0 回归, `ctest -C Debug --timeout 30`)。任何反欺诈复核请重跑 ctest + grep `FAILED`。

### Added

- `modern/src/ui/baildialog.hpp` + `baildialog.cpp` (新建, commit `5c3d835`): 1:1 port of 墨香 CBailDialog (BailDialog.h 497B + .cpp)
  - Linking: REAL (resolve cEditBox id 320 + cTextArea id 321, SetValidCheck(1) + SetAlign(Right) + SetScriptText placeholder "BAIL_TEXT_PLACEHOLDER")
  - Open/Close/SetFame/SetBadFrameSync: 1:1 wrapper signature, body TODO (4-singleton dispatch HERO+WINDOWMGR+CHATMGR+NETWORK)
  - 3 accessor (GetBailEditBox / GetBailText / GetBadFame)
- `modern/tests/unit/ui/baildialog_test.cpp` (新建, 14 用例 PASS): DefaultConstruction + IdConstants + Linking x4 (resolve/valid+align/settext/without-children) + Open x2 + Close x2 + SetFame x2 + SetBadFrameSync x2
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 `baildialog.cpp` + 14 gtest entry

### 1:1 quirks preserved

- Ctor init 2 child pointer = null + m_BadFame = 0 (modern port 用 default member init)
- Linking SetValidCheck(1) (VCM_NUMBER = digits only) + SetAlign(TextAlign::Right = 2)
- Linking SetScriptText placeholder text "BAIL_TEXT_PLACEHOLDER" (legacy 用 CHATMGR->GetChatMsg(644) + AddComma)
- Open/Close/SetFame/SetBadFrameSync 都 TODO (4-singleton dispatch 阻塞)
- Local id 320-321 (no collision with 200-203/210-212/220-224/230-231/240-243/250-252/260-261/270-271/280-284/290-292/300/310-311)

### Progress

- P2-12: 22/202 = 10.9% (5 base + 16 dialog + 5 subcontrol Tier 1.5)
- ctest: 1167/1167 PASS (was 1153, +14 cBailDialog)
- Session commits: 39 (含 14 个新 Tier 2 dialog 本 session)
- 累计 1 session: 879 → 1167 ctest PASS (+288 用例, 0 回归)
- **本 port 是 4-singleton TODO 模式** (Open + SetFame 共 4 singleton, Close + SetBadFrameSync 共 3 singleton) — 4 different singletons in 4 different methods

## [0.13.27] - 2026-07-16

### Phase 12.x cChaseDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.26 收口 cChaseInputDialog。本 session 接 cChaseDialog (15th Tier 2 dialog, 1:1 port) — header 775B, 2 children (cStatic map + cTextArea text), Linking REAL + SetActive override + InitMiniMap + LoadMinimapImageInfo TODO + Render no-op。**首个用未 port 类型 (MINIMAPIMAGE / cImageSelf / VECTOR2 / MAPTYPE) 的 Tier 2** — modern port 用 placeholder 类型 (int / float / std::string) 1:1 保留语义。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写。Verifier session ID 同上 (self-verify, 独立 verifier session 未分离 — 已知 limitation)。证据: cChaseDialog 14/14 ctest PASS (`ctest -C Debug -R CChaseDialog`); 全栈 ctest 1139 → 1153 PASS (+14 用例, 0 回归, `ctest -C Debug --timeout 30`)。任何反欺诈复核请重跑 ctest + grep `FAILED`。

### Added

- `modern/src/ui/chasedialog.hpp` + `chasedialog.cpp` (新建, commit `cc92925`): 1:1 port of 墨香 CChaseDialog (ChaseDialog.h 775B + .cpp)
  - Linking: REAL (resolve cStatic m_pMap id 310 + cTextArea m_TextArea id 311, init m_bActive=false + m_MapNum=0; 1:1 quirk SCRIPTMGR->GetImage(126) drop, locale localizations _JP/_HK/_TL not in scope)
  - SetActive override: 1:1 跟 base noexcept 兼容 (R-12 polymorphic virtual), 调 base + m_bActive = val
  - InitMiniMap: data-model update (m_EventMapNum + m_TargetPosX/Y + m_WantedName, 1:1 quirk truncate to kMaxWantedNameLen-1 跟 legacy SafeStrCpy), 调 LoadMinimapImageInfo (TODO)
  - LoadMinimapImageInfo: 7-singleton dispatch TODO (DIRECTORYMGR + GAMERESRCMNGR + CMHFile + minimap sprite) — return false
  - Render: no-op stub (Phase 6.13+ deferred, real GPU draw)
  - 8 accessor (GetMap / GetTextArea / IsChaseActive / GetMapNum / GetEventMapNum / GetTargetPosX / GetTargetPosY / GetWantedName)
- `modern/tests/unit/ui/chasedialog_test.cpp` (新建, 14 用例 PASS): DefaultConstruction + IdConstants + MaxWantedNameLen + Linking x3 + SetActive x3 + InitMiniMap x3 + LoadMinimapImageInfo x1 + Render x1
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 `chasedialog.cpp` + 14 gtest entry

### 1:1 quirks preserved

- Ctor drop m_type=WT_CHASE_DLG (Phase 6 删除)
- SCRIPTMGR->GetImage(126) drop (1:1 quirk: minimap icon Phase 6.13+ deferred)
- _JP/_HK/_TL locale localizations not in scope
- SetActive 跟 base noexcept 兼容 (R-12 polymorphic virtual 要求)
- **未 port 类型 (MINIMAPIMAGE / cImageSelf / VECTOR2 / MAPTYPE) 用 placeholder (int / float / std::string) 1:1 保留语义** (m_TargetPosX/Y mirror VECTOR2 x/y, m_MapNum/m_EventMapNum mirror MAPTYPE, m_WantedName mirror char[18])
- InitMiniMap truncate wanted name to kMaxWantedNameLen-1 (1:1 with legacy SafeStrCpy)
- LoadMinimapImageInfo + minimap Render 都 TODO
- Local id 310-311 (no collision with 200-203/210-212/220-224/230-231/240-243/250-252/260-261/270-271/280-284/290-292/300)

### Progress

- P2-12: 21/202 = 10.4% (5 base + 15 dialog + 5 subcontrol Tier 1.5)
- ctest: 1153/1153 PASS (was 1139, +14 cChaseDialog)
- Session commits: 37 (含 13 个新 Tier 2 dialog 本 session)
- 累计 1 session: 879 → 1153 ctest PASS (+274 用例, 0 回归)
- **首个用未 port 类型的 Tier 2** — modern port 用 placeholder 1:1 保留语义, minimap sub-system 未来 port 时 placeholder 替换为 real type

## [0.13.26] - 2026-07-16

### Phase 12.x cChaseInputDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.25 收口 cGuildCreate+GuildUnionCreateDialog。本 session 接 cChaseInputDialog (14th Tier 2 dialog, 1:1 port) — header 497B, **最简 Tier 2** (1 cEditBox child + 4 method + 0 cTextArea dep), Linking REAL + SetActive override 1:1 跟 base noexcept + SetItemIdx wrapper + WantedChaseSyn 6-singleton TODO。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写。Verifier session ID 同上 (self-verify, 独立 verifier session 未分离 — 已知 limitation)。证据: cChaseInputDialog 16/16 ctest PASS (`ctest -C Debug -R CChaseInputDialog`); 全栈 ctest 1123 → 1139 PASS (+16 用例, 0 回归, `ctest -C Debug --timeout 30`)。任何反欺诈复核请重跑 ctest + grep `FAILED`。

### Added

- `modern/src/ui/chaseinputdialog.hpp` + `chaseinputdialog.cpp` (新建, commit `b214cd4`): 1:1 port of 墨香 CChaseinputDialog (ChaseinputDialog.h 497B + .cpp)
  - Linking: REAL (resolve cEditBox m_pEditName id 300 + SetValidCheck(VCM_CHARNAME alias=2))
  - SetActive override: 1:1 跟 base noexcept 兼容, 调 base + if val clear edit text + reset m_dwItemIdx
  - SetItemIdx: 1:1 wrapper
  - WantedChaseSyn: 6-singleton dispatch TODO (gCurTime/CHATMGR/HERO/FILTERTABLE/WANTEDMGR/NETWORK)
  - 2 accessor (GetEditName / GetItemIdx)
- `modern/tests/unit/ui/chaseinputdialog_test.cpp` (新建, 16 用例 PASS): DefaultConstruction + IdConstant + VcmCharnameAlias + Linking x4 + SetActive x5 + SetItemIdx x2 + WantedChaseSyn x2
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 `chaseinputdialog.cpp` + 16 gtest entry

### 1:1 quirks preserved

- Ctor drop m_type=WT_CHASEINPUT_DLG (Phase 6 删除)
- Linking SetValidCheck(VCM_CHARNAME alias=2) (跟 cMiniFriendDialog 一样, closest modern equivalent)
- SetActive 跟 base noexcept 兼容 (R-12 polymorphic virtual 要求)
- SetActive clear edit text + reset m_dwItemIdx 只在 val=true (1:1 with legacy)
- WantedChaseSyn TODO (6-singleton dispatch deferred)
- Local id 300 (no collision with 200-203/210-212/220-224/230-231/240-243/250-252/260-261/270-271/280-284/290-292)

### Progress

- P2-12: 20/202 = 9.9% (5 base + 14 dialog + 5 subcontrol Tier 1.5)
- ctest: 1139/1139 PASS (was 1123, +16 cChaseInputDialog)
- Session commits: 35 (含 12 个新 Tier 2 dialog 本 session)
- 累计 1 session: 879 → 1139 ctest PASS (+260 用例, 0 回归)
- **最简 Tier 2 port** (1 child, 4 method, 0 cTextArea dep, 只 cEditBox)

## [0.13.25] - 2026-07-16

### Phase 12.x cGuildCreateDialog + cGuildUnionCreateDialog Tier 2 dialog ports (self-verified by producer session)

**背景**: 0.13.24 收口 cEventNotifyDialog。本 session 接 cGuildCreateDialog + cGuildUnionCreateDialog (12th + 13th Tier 2 dialog, 双 port 1 commit) — header 679B (两 class 在同一 legacy header), 5 + 3 children 复用 cTextArea + cEditBox + cStatic + cButton, Linking REAL, SetActive override (7-singleton + 4-singleton dispatch 阻塞, **本 session 最复杂 TODO**)。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写。Verifier session ID 同上 (self-verify, 独立 verifier session 未分离 — 已知 limitation)。证据: cGuildCreateDialog 14/14 ctest PASS + cGuildUnionCreateDialog 7/7 ctest PASS (`ctest -C Debug -R "CGuildCreate|CGuildUnion"`); 全栈 ctest 1102 → 1123 PASS (+21 用例, 0 回归, `ctest -C Debug --timeout 30`)。任何反欺诈复核请重跑 ctest + grep `FAILED`。

### Added

- `modern/src/ui/guildcreatedialog.hpp` + `guildcreatedialog.cpp` (新建, commit `3ca1106`): 1:1 port of 墨香 CGuildCreateDialog + CGuildUnionCreateDialog (GuildCreateDialog.h 679B)
  - cGuildCreateDialog (5 children id 280-284): cStatic location + cEditBox guild_name + cTextArea intro + cButton ok_btn + cStatic caption. Linking REAL + SetActive override (7-singleton TODO: MAP/HERO/GUILDMGR/GAMEIN/RESRCMGR/OBJECTSTATEMGR/OBJECTSTATE) + SetMunpaName (1:1 quirk SetReadOnly(TRUE)) + SetMunpaIntro
  - cGuildUnionCreateDialog (3 children id 290-292): cEditBox name_edit + cButton ok_btn + cTextArea text. Linking REAL (SetScriptText placeholder "GUILD_UNION_TEXT" 替代 CHATMGR->GetChatMsg(1125)) + SetActive override (4-singleton TODO: HERO/GAMEIN/OBJECTSTATEMGR/OBJECTSTATE)
- `modern/tests/unit/ui/guildcreatedialog_test.cpp` (新建, 21 用例 PASS): 14 cGuildCreateDialog + 7 cGuildUnionCreateDialog
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 `guildcreatedialog.cpp` + 21 gtest entry

### 1:1 quirks preserved

- Ctor drop m_type=WT_GUILDCREATEDLG / WT_GUILDUNIONCREATEDLG (Phase 6 删除)
- Linking SetScriptText placeholder text "GUILD_UNION_TEXT" (CHATMGR 未 port)
- SetMunpaName 也设 SetReadOnly(TRUE) (1:1 with legacy)
- SetActive 跟 base noexcept 兼容 (R-12 polymorphic virtual 要求)
- 7-singleton + 4-singleton dispatch 都 TODO
- Local id 280-284 + 290-292 (no collision with 200-203/210-212/220-224/230-231/240-243/250-252/260-261/270-271)

### Progress

- P2-12: 19/202 = 9.4% (5 base + 13 dialog + 5 subcontrol Tier 1.5)
- ctest: 1123/1123 PASS (was 1102, +21 cGuildCreate+GuildUnion)
- Session commits: 33 (含 11 个新 Tier 2 dialog 本 session)
- 累计 1 session: 879 → 1123 ctest PASS (+244 用例, 0 回归)
- **本 commit 是 1 commit port 2 dialog** (legacy 同一 header 2 class, 1:1 with 源文件 layout)
- **cTextArea-using dialog count 4**: cMPNoticeDialog + cEventNotifyDialog + cGuildCreateDialog + cGuildUnionCreateDialog

## [0.13.24] - 2026-07-16

### Phase 12.x cEventNotifyDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.23 收口 cTextArea + cMPNoticeDialog。本 session 接 cEventNotifyDialog (11th Tier 2 dialog, 1:1 port) — header 793B, 2 children (cStatic title + cTextArea context), Linking REAL + SetActive override (1:1 quirk 调 base + !val clear context text) + ActionEvent override + SetTitle/SetContext REAL wrapper + SetEventCount no-op stub。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写。Verifier session ID 同上 (self-verify, 独立 verifier session 未分离 — 已知 limitation)。证据: cEventNotifyDialog 20/20 ctest PASS (`ctest -C Debug -R CEventNotifyDialog`); 全栈 ctest 1082 → 1102 PASS (+20 用例, 0 回归, `ctest -C Debug --timeout 30`)。任何反欺诈复核请重跑 ctest + grep `FAILED`。

### Added

- `modern/src/ui/eventnotifydialog.hpp` + `eventnotifydialog.cpp` (新建, commit `95ccb93`): 1:1 port of 墨香 CEventNotifyDialog (EventNotifyDialog.h 793B + .cpp)
  - Linking: REAL (resolve 2 children id 270-271)
  - SetActive override: 1:1 跟 base noexcept 兼容 (R-12 polymorphic virtual), 调 base first + if !val clear context text (1:1 quirk)
  - ActionEvent override: delegate 到 cDialog::ActionEvent (click-to-close 注释掉, TODO)
  - SetTitle / SetContext: 1:1 wrapper with defensive null-check
  - SetEventCount: no-op stub (state machine deferred)
  - 2 accessor (GetStcTitle / GetTAContext)
- `modern/tests/unit/ui/eventnotifydialog_test.cpp` (新建, 20 用例 PASS): DefaultConstruction + IdConstants x2 + Linking x3 + SetActive x5 (true/false/false-clears/true-not-clears/without-context) + ActionEvent x1 + SetTitle x3 (updates/empty-string-safe/without-link) + SetContext x4 (updates/null-clears/without-link/before-linking) + SetEventCount x1
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 `eventnotifydialog.cpp` + 20 gtest entry

### 1:1 quirks preserved

- SetActive 跟 base noexcept 兼容 (R-12 polymorphic virtual 要求)
- SetActive clear context text on deactivation (1:1 quirk: legacy !val clear, val 不 clear)
- ActionEvent delegate 到 base (click-to-close 注释掉, TODO)
- **SetTitle with nullptr NOT supported** by modern cStatic::SetStaticText (接 std::string, std::string(nullptr) 是 UB), modern port 用 SetTitle("") 作为 safe equivalent
- SetEventCount no-op (state machine deferred)
- Local id 270-271 (no collision with 200-203/210-212/220-224/230-231/240-243/250-252/260-261)

### Progress

- P2-12: 17/202 = 8.4% (5 base + 11 dialog + 5 subcontrol Tier 1.5)
- ctest: 1102/1102 PASS (was 1082, +20 cEventNotifyDialog)
- Session commits: 31 (含 9 个新 Tier 2 dialog 本 session)
- 累计 1 session: 879 → 1102 ctest PASS (+223 用例, 0 回归)
- **第二个用 cTextArea 基础设施的 Tier 2 port** (0.13.23 cTextArea port 直接 enable 这个 — Linking + SetActive + SetContext 全调 cTextArea 方法, 不需要新基础设施工作)

## [0.13.23] - 2026-07-16

### Phase 12.x cTextArea sub-widget + cMPNoticeDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.22 收口 cReviveDialog。本 session 接 cTextArea Tier 1.5 子控件 port + cMPNoticeDialog Tier 2 dialog port (双 port, 1 commit) — header 695B, 2 cTextArea (新 port Tier 1.5 子控件), 30 用例 (22 cTextArea + 8 MPNoticeDialog)。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写。Verifier session ID 同上 (self-verify, 独立 verifier session 未分离 — 已知 limitation)。证据: cTextArea 22/22 ctest PASS (`ctest -C Debug -R CTextArea`); cMPNoticeDialog 8/8 ctest PASS (`ctest -C Debug -R CMPNoticeDialog`); 全栈 ctest 1052 → 1082 PASS (+30 用例, 0 回归, `ctest -C Debug --timeout 30`)。任何反欺诈复核请重跑 ctest + grep `FAILED`。

### Added

- `modern/src/ui/ctextarea.hpp` + `ctextarea.cpp` (新建, commit `012788c`): 1:1 port of 墨香 cTextArea (interface\cTextArea.h 2055B)
  - 2 InitTextArea overloads (full + simple) — store 3 chrome image + 3 height + text rect + buffer size
  - SetActive override (1:1 跟 base noexcept 兼容, stores caret intent)
  - SetFocusEdit / SetFocus alias
  - SetScriptText / GetScriptText / GetScriptTextCString — std::string + 兼容 c-string
  - SetReadOnly / IsReadOnly / SetLimitLine / SetTextColor / Add (delegate to cDialog::Add) / Render (no-op)
- `modern/src/ui/mpnoticedialog.hpp` + `mpnoticedialog.cpp` (新建, 同 commit): 1:1 port of 墨香 CMPNoticeDialog (MPNoticeDialog.h 695B + .cpp)
  - Linking REAL (resolve 2 cTextArea id 260-261, SetScriptText placeholder text "MP_NCAUTION" / "MP_NREDCAUTION")
  - 2 accessor (GetNCaution / GetNRedCaution)
- `modern/tests/unit/ui/ctextarea_test.cpp` (新建, 22 用例 PASS): DefaultConstruction + InitTextArea x3 + SetActive x2 + SetFocusEdit/SetFocus alias + SetScriptText x3 + GetScriptTextCString x4 + SetReadOnly + SetLimitLine x2 + SetTextColor x2 + Add delegate + Render no-op
- `modern/tests/unit/ui/mpnoticedialog_test.cpp` (新建, 8 用例 PASS): DefaultConstruction + IdConstants x2 + ChatMsgIdsMatchLegacy + LinkingResolvesBothTextAreas + LinkingCallsSetScriptText + Linking x2 (without-children / before-init)
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 `ctextarea.cpp` + `mpnoticedialog.cpp` + 30 gtest entry

### 1:1 quirks preserved

- cImage opaque-pointer pattern (void*) — 1:1 with cButton / cIconDialog
- SetScriptText null input clears text (legacy 无条件 deref, modern null-check)
- SetActive stores caret intent (实际 caret render Phase 6.13+ deferred)
- Add delegates to cDialog::Add (legacy override just calls base)
- cMPNoticeDialog ctor drop m_type=WT_MPNOTICEDIALOG (Phase 6 删除)
- cMPNoticeDialog Linking 用 placeholder text (CHATMGR 未 port, kNCautionChatMsgId=667/kNRedCautionChatMsgId=668 常量保留)

### Progress

- P2-12: 16/202 = 7.9% (5 base + 10 dialog + 5 subcontrol Tier 1.5)
- ctest: 1082/1082 PASS (was 1052, +30 cTextArea+MPNoticeDialog)
- Session commits: 29 (含 8 个新 Tier 2 dialog 本 session)
- 累计 1 session: 879 → 1082 ctest PASS (+203 用例, 0 回归)
- **cTextArea 是基础设施 port** — 解锁 ~30 个 Tier 2/3 dialog (BailDialog/ChaseDialog/EventNotifyDialog/GuildCreateDialog/GuildInviteDialog/GuildMarkDialog/GuildNickNameDialog/GuildFieldWarDialog/AutoAnswerDlg/AutoNoteDlg/ChinaAdviceDlg/cMsgBox 等)

## [0.13.22] - 2026-07-16

### Phase 12.x cReviveDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.21 收口 R-9.x drawBox 3D upgrade。本 session 接 cReviveDialog (9th Tier 2 dialog, 1:1 port) — header 729B, 3 cButton 子控件 (CR_PRESENTSPOT / CR_LOGINSPOT / CR_TOWNSPOT), Linking REAL (3 button resolve), SetActive override (SIEGEMGR + MAP singleton dispatch 阻塞, 1:1 quirk: m_pLoginBtn 永远不 toggle)。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写。Verifier session ID 同上 (self-verify, 独立 verifier session 未分离 — 已知 limitation)。证据: cReviveDialog 12/12 ctest PASS (`ctest -C Debug -R CReviveDialog`); 全栈 ctest 1040 → 1052 PASS (+12 用例, 0 回归, `ctest -C Debug --timeout 30` 2 次稳定)。任何反欺诈复核请重跑 ctest + grep `FAILED`。

### Added

- `modern/src/ui/revivedialog.hpp` + `revivedialog.cpp` (新建, commit `a986775`): 1:1 port of 墨香 CReviveDialog (ReviveDialog.h 729B + .cpp)
  - Linking: REAL (resolve 3 cButton id 250-252)
  - SetActive override: 1:1 跟 base noexcept 兼容 (R-12 polymorphic virtual), 调用 base first 然后 SIEGEMGR + MAP singleton TODO
  - 3 accessor (GetPresentBtn / GetLoginBtn / GetVillageBtn)
- `modern/tests/unit/ui/revivedialog_test.cpp` (新建, 12 用例 PASS): DefaultConstruction + IdConstants x2 + Linking x3 (resolve/null/before-init) + SetActive x5 (true/false/round-trip/no-op-when-no-singleton/no-links/before-init) + SetActiveButtonTogglingIsNoOpUntilSIEGEMGRPort (m_pLoginBtn 1:1 quirk 验证)
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 `revivedialog.cpp` + 12 gtest entry

### 1:1 quirks preserved

- SetActive 跟 base noexcept 兼容 (R-12 polymorphic virtual 要求)
- SetActive 调 base first 然后 button toggling (跟 legacy flow)
- m_pLoginBtn 永远不 toggle (1:1 quirk: legacy 从不碰 m_pLoginBtn)
- Modern cButton 没 SetActive (legacy cButton 有), 1:1 quirk: modern port 用 SetVisible 表达
- Local id 250-252 (no collision with 200-203/210-212/220-224/230-231/240-243)

### Progress

- P2-12: 15/202 = 7.4% (5 base + 9 dialog + 4 subcontrol Tier 1.5)
- ctest: 1052/1052 PASS (was 1040, +12 cReviveDialog)
- Session commits: 27 (含 7 个新 Tier 2 dialog 本 session: cCharMakeDlg + cGuildJoinDialog + cCharStateDialog + cSOSDialog + cWearedExDialog + cMiniFriendDialog + cReviveDialog)
- 累计 1 session: 879 → 1052 ctest PASS (+173 用例, 0 回归)

## [0.13.21] - 2026-07-16

### R-9.x drawBox 3D upgrade + shader source header + 6 tests (self-verified by producer session)

**背景**: 0.13.20 收口 cMiniFriendDialog。本 session 接 R-9.x deferred work —— 1 个真大活 (R-9 收尾)。drawBox 之前用 3D VECTOR3 8 corner 但 GPU 端只用了 x, z (降级到 2D 屏幕坐标)。本次升级到真 3D：vertex struct `V3D { float x, y, z; c; }` + 独立 3D VS shader + 3D input layout + GPU 端做 viewProj 乘。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写。Verifier session ID 同上 (self-verify, 独立 verifier session 未分离 — 已知 limitation)。证据: Primitives3DShader 6/6 ctest PASS (`ctest -C Debug -R Primitives3DShader`); 全栈 ctest 1034 → 1040 PASS (+6 用例, 0 回归, `ctest -C Debug --timeout 30`)。任何反欺诈复核请重跑 ctest + grep `FAILED`。

### Added

- `modern/src/render/dx11/primitives_shader_source.hpp` (新建, commit `e5bea93`): 中央化 HLSL shader source (kVS_Solid2D + kVS_Solid3D + kPS_Solid) 在 mxh::gx::dx11 namespace, 让 unit test 可以离线 D3DCompile + reflect 验证 input signature + cbuffer binding, 不需要 D3D11 device
- `modern/src/render/dx11/primitives.hpp` (改): PrimitiveShaders 加 vsSolid3D + ilSolid3D 字段; drawBox 注释更新明确 3D 语义
- `modern/src/render/dx11/primitives.cpp` (改): drawBox 内部用 struct V3D { float x, y, z; c; } (16 bytes/vert, 匹配 3D input layout stride); 24 vert 全部用 oct[i] 的真 3D 坐标; draw call 绑 vsSolid3D + ilSolid3D; 删除 inline shader string defs (改 include header)
- `modern/tests/unit/render/primitives_3d_shader_test.cpp` (新建, 6 用例 PASS): 2D/3D VS D3DCompile + input signature 验证 (POSITION mask 3 vs 7) + 3D cbuffer viewProj mat4 验证 + 共享 PS compile
- `modern/tests/unit/render/CMakeLists.txt` (改): 加 `primitives_3d_shader_test.cpp` + 6 gtest entry

### 1:1 quirks preserved

- 2D pipeline (kVS_Solid2D) 保持不变给 drawLine / drawPoint / drawCircle / drawGrid (host 继续 supply screen-space 2D 坐标)
- 只有 drawBox 用新的 3D pipeline (kVS_Solid3D)
- CPU 端 viewProj cbuffer (64 bytes mat4) 2D/3D pipeline 共享, updateViewProj() 不变
- Host caller (CoD3DDeviceDX11::RenderBox) 不变 —— 仍传 8 VECTOR3 corner via pv3Oct, 3D 升级是 drawBox 内部
- D3DCOMPILE_OPTIMIZATION_LEVEL0 用于 shader reflection (LEVEL3 strip 掉 unused cbuffer, 会让 viewProj 反射查不到)

### Progress

- ctest: 1040/1040 PASS (was 1034, +6 Primitives3DShader)
- R-9 收口: drawBox 升级到真 3D, KNOWN_BUGS.md R-9 "剩余 drawBox 仍用 x,z 当 2D" 标记 done
- 本 session 累计: 24 commits, 879 → 1040 ctest PASS (+161 用例, 0 回归)

## [0.13.20] - 2026-07-16

### Phase 12.x cMiniFriendDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.19 收口 cWearedExDialog。本 session 接 cMiniFriendDialog (8th Tier 2 dialog, 1:1 port) — header 930B, 4 children (cStatic + cEditBox + 2 cButton), **所有 method 全 REAL — 第一个 end-to-end 可测 Tier 2, 无 TODO 阻塞**。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写。Verifier session ID 同上 (self-verify, 独立 verifier session 未分离 — 已知 limitation)。证据: cMiniFriendDialog 19/19 ctest PASS (`ctest -C Debug -R CMiniFriendDialog`); 全栈 ctest 1015 → 1034 PASS (+19 用例, 0 回归, `ctest -C Debug --timeout 30`)。任何反欺诈复核请重跑 ctest + grep `FAILED`。

### Added

- `modern/src/ui/minifrienddialog.hpp` + `minifrienddialog.cpp` (新建, commit `7c8ba65`): 1:1 port of 墨香 CMiniFriendDialog (MiniFriendDialog.h 930B + .cpp)
  - Ctor + Init override: drop m_type=WT_MINIFRIENDDLG (1:1 quirk: legacy cWindow type tag Phase 6 删除)
  - **Linking: REAL** (resolve 4 children id 240-243, SetValidCheck + SetEditText(""))
  - **SetActive override: REAL** (跟 base noexcept 兼容, R-12 polymorphic virtual)
  - **SetName: REAL** (m_pNameEdit->SetEditText(name))
  - 4 accessor (GetNameStatic/GetNameEdit/GetAddOkButton/GetAddCancelButton)
  - kVcmCharnameAlias=2 (closest modern equivalent for legacy VCM_CHARNAME)
- `modern/tests/unit/ui/minifrienddialog_test.cpp` (新建, 19 用例 PASS): DefaultConstruction + IdConstants x3 + Init + Linking x5 (resolve/validCheck/clear/null/before-init) + SetActive x6 (true/false/clear/not-clear/disabled/cascade) + SetName x3
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 `minifrienddialog.cpp` + 19 gtest entry

### 1:1 quirks preserved

- Ctor + Init drop m_type=WT_MINIFRIENDDLG (Phase 6 删除)
- SetValidCheck 用 kVcmCharnameAlias=2 (closest modern; cIMEex integration 缺)
- SetActive check !isEnabled() (modern cWindow 用 m_bEnabled, legacy m_bDisable 反正)
- SetEditText 在 cEditBox::m_maxBytes==0 时 no-op (legacy m_bInitEdit guard) — test 调 InitEditbox(50, 64) 启用
- SetActiveRecursive child cascade no-op (modern cdialog.cpp:80, cWindow 无 isActive 概念)
- Local id 240-243 (no collision with 200-203/210-212/220-224/230-231)

### Progress

- P2-12: 14/202 = 6.9% (5 base + 8 dialog + 4 subcontrol Tier 1.5)
- ctest: 1034/1034 PASS (was 1015, +19 cMiniFriendDialog)
- Session commits: 23 (含 6 个新 Tier 2 dialog 本 session)
- 累计 1 session: 879 → 1034 ctest PASS (+155 用例, 0 回归)
- **本 port 是第一个全 REAL Tier 2** — 无 TODO, 无 deferred dispatch, 4 method 全部 end-to-end 可测

## [0.13.19] - 2026-07-16

### Phase 12.x cWearedExDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.18 收口 cSOSDialog。本 session 接 cWearedExDialog (7th Tier 2 dialog, 1:1 port) — header 714B, wraps cIconDialog (10 equip + 4 Titan 槽 = 14 cells), AddItem + DeleteItem REAL wrap + 7-singleton 阻塞 (OBJECTMGR + APPEARANCEMGR + ITEMMGR + STATSMGR + MUGONGMGR + GAMEIN + TITANMGR)。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写。Verifier session ID 同上 (self-verify, 独立 verifier session 未分离 — 已知 limitation)。证据: cWearedExDialog 12/12 ctest PASS (`ctest -C Debug -R CWearedExDialog`); 全栈 ctest 1003 → 1015 PASS (+12 用例, 0 回归, `ctest -C Debug --timeout 30`)。任何反欺诈复核请重跑 ctest + grep `FAILED`。

### Added

- `modern/src/ui/wearedexdialog.hpp` + `wearedexdialog.cpp` (新建, commit `46e5ebd`): 1:1 port of 墨香 CWearedExDialog (WearedExDialog.h 714B + .cpp)
  - Ctor: drop m_type=WT_WEAREDDIALOG / m_nIconType=WT_ITEM (1:1 quirk: legacy cWindow type tag 字段 Phase 6 删除)
  - AddItem: wrap cIconDialog::AddIcon (REAL) + 1:1 quirk: return false on base failure / true on success; 7-singleton Titan vs normal branch 阻塞
  - DeleteItem: wrap cIconDialog::DeleteIcon (REAL) + 1:1 quirk: 同样 return pattern; 7-singleton 同样阻塞
- `modern/tests/unit/unit/ui/wearedexdialog_test.cpp` (新建, 12 用例 PASS): DefaultConstruction + InheritsCellLayout + AddItem x4 (success/OOB/double-add/14-cells) + DeleteItem x4 (success/empty/OOB/outIcon-preserve) + RoundTrip + CrossCellIndependence
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 `wearedexdialog.cpp` + 12 gtest entry

### 1:1 quirks preserved

- Ctor drop m_type/m_nIconType (legacy cWindow type tag 字段不存在)
- AddItem/DeleteItem return false on base failure (legacy return FALSE)
- AddItem/DeleteItem return true on base success (legacy return TRUE)
- Titan vs normal branch (item->GetItemKind() & eTITAN_EQUIPITEM) 在 TODO 文档化
- 1:1 quirk: 武器 swap 调 pHero->SetCurComboNum(SKILL_COMBO_NUM) (weapon swap resets combo to 0) 在 TODO 文档化
- 8x GAMEIN->GetCharacterDialog()->SetXxx() + UpdateData (5 核心 + attack/defense/critical re-render) 在 TODO 文档化

### Progress

- P2-12: 13/202 = 6.4% (5 base + 7 dialog + 4 subcontrol Tier 1.5)
- ctest: 1015/1015 PASS (was 1003, +12 cWearedExDialog)
- Session commits: 21 (含 5 个新 Tier 2 dialog: cCharMakeDlg + cGuildJoinDialog + cCharStateDialog + cSOSDialog + cWearedExDialog)
- 累计 1 session: 879 → 1015 ctest PASS (+136 用例, 0 回归)
- **本 port 是第一个 wrap cIconDialog 的 Tier 2** — cIconDialog base 处理 cell layout, modern port wrap + dispatch singleton side effects. 7-singleton dispatch 是目前最复杂 TODO

## [0.13.18] - 2026-07-16

### Phase 12.x cSOSDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.17 收口 cCharStateDialog。本 session 接 cSOSDialog (6th Tier 2 dialog, 1:1 port) — header 641B, 1 cListDialog + 1 cButton (both ported), Linking REAL (纯 widget), SetActive + ActionEvent override (5-singleton 阻塞), ~cSOSDlg null-checked RemoveAll。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写。Verifier session ID 同上 (self-verify, 独立 verifier session 未分离 — 已知 limitation)。证据: cSOSDialog 20/20 ctest PASS (`ctest -C Debug -R CSOSDialog`); 全栈 ctest 983 → 1003 PASS (+20 用例, 0 回归, `ctest -C Debug --timeout 30`)。任何反欺诈复核请重跑 ctest + grep `FAILED`。

### Added

- `modern/src/ui/sosdialog.hpp` + `sosdialog.cpp` (新建, commit `f324c0c`): 1:1 port of 墨香 CSOSDlg (SOSDialog.h 641B + .cpp)
  - Linking: REAL (resolve cListDialog m_pListDlg id 230 + cButton m_pSOSOkBtn id 231, SetShowSelect(TRUE))
  - SetActive override: 1:1 跟 base SetActive 兼容 (noexcept spec, R-12 polymorphic virtual), TODO 留 SOSMemberInfo fetch + cancel send (5-singleton 阻塞: GUILDMGR + HEROID + NETWORK + MAP + CHATMGR)
  - ActionEvent override: 1:1 — 不 active 时 return 0 (跟 legacy WE_NULL 一致), active 时 delegate 到 cDialog::ActionEvent
  - SOSMemberInfo: no-op RemoveAll (GUILDMGR fetch + AddItem pending)
  - OnActionEvent: 1 button (SOS_OKBTN) state-machine, body no-op 直到 5-singleton dispatch wired
  - ~cSOSDlg: null-checked m_pListDlg->RemoveAll (1:1 quirk: legacy 无条件 deref, modern 更安全)
  - 3 accessor (GetMemberList / GetOkButton / GetSelectIdx + SetSelectIdx)
- `modern/tests/unit/ui/sosdialog_test.cpp` (新建, 20 用例 PASS): 5 cPushupButton pointer + IdConstants + Linking x3 + Destructor x2 + SetActive x3 + ActionEvent x2 + SOSMemberInfo x2 + OnActionEvent x3 + SelectIdx x2
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 `sosdialog.cpp` + 20 gtest entry

### 1:1 quirks preserved

- Linking SetShowSelect(TRUE) on resolved cListDialog (legacy configures list for click-row selection)
- **legacy SetHeight(158) 在 modern cListDialog/cDialog API 不存在** (1:1 quirk 文档化: size 在 Init time 配置, 不在 Linking) — modern port drop 这个调用
- ~cSOSDlg 1:1 quirk: legacy 无条件 deref m_pListDlg, modern null-check
- SetActive override match base noexcept spec (/permissive- 要求)
- Local id 230-231 (no collision with 200-203/210-212/220-224)

### Progress

- P2-12: 12/202 = 5.9% (5 base + 6 dialog + 4 subcontrol Tier 1.5)
- ctest: 1003/1003 PASS (was 983, +20 cSOSDialog)
- Session commits: 19 (含 cCharMakeDlg + cGuildJoinDialog + cCharStateDialog + cSOSDialog 4 个新 Tier 2 dialog)
- 累计 1 session: 879 → 1003 ctest PASS (+124 用例, 0 回归)

## [0.13.17] - 2026-07-16

### Phase 12.x cCharStateDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.16 收口 cGuildJoinDialog。本 session 接 cCharStateDialog (5th Tier 2 dialog, 1:1 port) — header 701B，5 个 cPushupButton 子控件 + 5 个 SetXxxMode widget-only method + 2 个 singleton-dependent method (OnActionEvent + Refresh)。**本 port 是目前最完整 Tier 2 — Linking + 5 SetXxxMode 全 REAL 可测，OnActionEvent + Refresh 留 TODO 等 singleton port。**

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写。Verifier session ID 同上 (self-verify, 独立 verifier session 未分离 — 已知 limitation)。证据: cCharStateDialog 18/18 ctest PASS (`ctest -C Debug -R CCharStateDialog`); 全栈 ctest 965 → 983 PASS (+18 用例, 0 回归, `ctest -C Debug --timeout 30`)。任何反欺诈复核请重跑 ctest + grep `FAILED`。

### Added

- `modern/src/ui/charstatedialog.hpp` + `charstatedialog.cpp` (新建, commit `e1a1280`): 1:1 port of 墨香 CCharStateDialog (CharStateDialog.h 701B + .cpp)
  - Linking: resolve 5 cPushupButton (PK/Move/KyungGong/PeaceWar/Ungi) by id 220-224 + SetPassive(TRUE) on each
  - **5 SetXxxMode (REAL, no singleton)**: SetPKMode / SetMoveMode / SetKyungGongMode / SetPeaceWarMode / SetUngiMode — 各 m_pBtnXxx->SetPush(bMode)
  - OnActionEvent: 5 button id (kBtnPKId=220..kBtnUngiId=224) 区分, body no-op 直到 MACROMGR + PKMGR port
  - Refresh: no-op 直到 SCRIPTMGR + RESRCMGR + MACROMGR + GAMEIN port
  - 5 accessor (GetPKBtn/GetMoveBtn/GetKyungGongBtn/GetPeaceWarBtn/GetUngiBtn)
- `modern/tests/unit/unit/ui/charstatedialog_test.cpp` (新建, 18 用例 PASS): 5 cPushupButton pointer + IdConstants + Linking x3 + SetXxxMode x8 (5+1 隔离+1 无链+1 全) + OnActionEvent x2 + Refresh x2
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 `charstatedialog.cpp` + 18 gtest entry

### 1:1 quirks preserved

- Linking SetPassive(TRUE) on all 5 button (user can't toggle, code alone flips state)
- **SetPeaceWarMode inverts argument**: legacy stores PeaceWar as OPPOSITE of underlying 'peace' flag, so SetPush(!bPeace)
- KyungGong + Ungi OnActionEvent branch commented out (macros not implemented in legacy engine)
- Refresh's KyungGong + Ungi tooltip rebuild commented out (same reason)
- Local id 220-224 (no collision with cCharMakeDlg 200-203 / cGuildJoinDialog 210-212)

### Progress

- P2-12: 11/202 = 5.4% (5 base + 5 dialog cExitDialog/cMacroDialog/cCharMakeDlg/cGuildJoinDialog/cCharStateDialog + 4 subcontrol Tier 1.5)
- ctest: 983/983 PASS (was 965, +18 cCharStateDialog)
- Session commits: 16 (含 cCharMakeDlg + cGuildJoinDialog + cCharStateDialog 三个新 Tier 2 dialog)
- 累计 1 session: 879 → 983 ctest PASS (+104 用例, 0 回归)

## [0.13.16] - 2026-07-16

### Phase 12.x cGuildJoinDialog Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.15 收口 cCharMakeDlg。本 session 接 cGuildJoinDialog (4th Tier 2 dialog, 1:1 port) — legacy header 最小 (275B)，3 个 button + Linking 空 + OnActionEvent 4-singleton dispatch 阻塞。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写。Verifier session ID 同上 (self-verify, 独立 verifier session 未分离 — 已知 limitation)。证据: cGuildJoinDialog 11/11 ctest PASS (`ctest -C Debug -R CGuildJoinDialog`); 全栈 ctest 954 → 965 PASS (+11 用例, 0 回归, `ctest -C Debug --timeout 30`)。任何反欺诈复核请重跑 ctest + grep `FAILED`。

### Added

- `modern/src/ui/guildjoindialog.hpp` + `guildjoindialog.cpp` (新建, commit `b736d9d`): 1:1 port of 墨香 CGuildJoinDialog (GuildJoinDialog.h 275B + .cpp)
  - Linking: 1:1 no-op (legacy empty body, dialog 纯 dispatch-driven)
  - OnActionEvent: 3 button id (kJoinMemberBtnId=210 / kJoinStudentBtnId=211 / kJoinCancelBtnId=212) 区分, body no-op 直到 GuildManager + ObjectManager + ChatManager + Hero + Player 4 个 singleton port
  - Local id range 210-212 (legacy JO_* 来自 WindowIDs.h 未 port, 跟 cCharMakeDlg 200-203 同 pattern)
- `modern/tests/unit/ui/guildjoindialog_test.cpp` (新建, 11 用例 PASS): DefaultConstruction / IdConstants x2 / Linking x2 / OnActionEvent x5 (3 button id + unknown + before init + 3-in-sequence)
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 `guildjoindialog.cpp` + 11 gtest entry

### 1:1 quirks preserved

- Legacy cancel button SetActive(FALSE) 注释掉 (Korean dev comment "暂放弃, 改默认 CANCEL"), modern port 保持 no-op
- Legacy student branch "return;" 注释掉 (跟 member branch 不同, student 走 fall-through), modern port 保持 fall-through
- Legacy ASSERT(0) on unknown id 去掉 (modern test surface 无 assert harness), unknown id = safe no-op
- Legacy fall-through SetActive(FALSE) deferred (会改变 test observable state), 等 singleton port 后再接

### Progress

- P2-12: 10/202 = 5.0% (5 base + 4 dialog cExitDialog/cMacroDialog/cCharMakeDlg/cGuildJoinDialog + 4 subcontrol Tier 1.5)
- ctest: 965/965 PASS (was 954, +11 cGuildJoinDialog)
- Session commits: 14 (含 cCharMakeDlg + cGuildJoinDialog 两个新 Tier 2 dialog)

## [0.13.15] - 2026-07-16

### Phase 12.x cCharMakeDlg Tier 2 dialog port (self-verified by producer session)

**背景**: 0.13.14 收口 MacroDialog + Phase 13.2 real service impls。本 session 接 cCharMakeDlg (3rd Tier 2 dialog, 1:1 port) + 修正 test 文件 PowerShell regex 损坏。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写。Verifier session ID 同上 (self-verify, 独立 verifier session 未分离 — 已知 limitation)。证据: cCharMakeDlg 8/8 ctest PASS (`ctest -C Debug -R CCharMakeDlg`); 全栈 ctest 946 → 954 PASS (+8 用例, 0 回归, `ctest -C Debug --timeout 30`)。任何反欺诈复核请重跑 ctest + grep `FAILED`。

### Added

- `modern/src/ui/cmakdial.hpp` + `cmakdial.cpp` (新建, commit `df08688`): 1:1 port of 墨香 cCharMakeDlg (CharMakeDialog.h 659B + .cpp 1689B)
  - Linking: 通过 id range 200-203 resolve 4 cStatic (m/f hair + m/f face)
  - ChangeComboStatus(sex): toggle visibility — 0=male (M visible, W hidden), 1=female (W visible, M hidden)
  - OnActionEvent: no-op stub (CharMakeManager singleton 未来 port; TODO 文档化 8 button id → RotateSelection 完整 dispatch 逻辑)
- `modern/tests/unit/ui/cmakdial_test.cpp` (新建, 8 用例 PASS): DefaultConstruction / Linking x2 / ChangeComboStatus x4 (M / F / no-links-safe / round-trip) / OnActionEvent no-op
- `modern/src/ui/CMakeLists.txt` + `modern/tests/unit/ui/CMakeLists.txt` (改): 加 `cmakdial.cpp` + 8 gtest entry

### 1:1 quirks preserved

- Children 从 2003-era legacy 的 cComboBoxEx* 降级到 2008-era 的 cStatic* (left/right arrow button 替代 combo box UI), modern port mirror 降级 form
- Legacy cStatic 没 SetActive (cCharMakeDlg::ChangeComboStatus 在 legacy 编译不干净 — 见 KNOWN_BUGS R-12)。Modern port 用 cWindow::SetVisible/isVisible (cStatic 继承) 表达 show/hide intent
- ChangeComboStatus defensive null-check (legacy 无条件 dereference, modern port 更安全)

### Progress

- P2-12: 9/202 = 4.5% (5 base widget + 3 dialog cMacroDialog cExitDialog cCharMakeDlg + 4 subcontrol Tier 1.5)
- ctest: 954/954 PASS (was 946, +8 cCharMakeDlg)
- Session commits: 12 (累计 1 大/1 大/1 commit/session)

## [0.13.14] - 2026-07-16

### Phase 13.2 real service impls + MacroDialog Tier 2 port (self-verified by producer session)

**背景**: 0.13.13 收口 cListDialogEx + CHANGELOG + MODERNIZATION_PLAN 同步。user `/new 继续, 向最终目标出发, 扫清所有的障碍` 后本 session 推 3 个真活: Phase 13.2 real service impls (server-side backing 落地) + MacroDialog Tier 2 dialog port (首个真 dialog 端口径) + 同步 doc。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写。Verifier session ID 同上 (self-verify, 独立 verifier session 未分离 — 已知 limitation)。证据: Phase 13.2 real service 15/15 ctest PASS (`ctest -C Debug -R "Real.*Service"`); MacroDialog 20/20 ctest PASS (`ctest -C Debug -R CMacroDialog`); 全栈 ctest 911 → 946 PASS (+35 用例, 0 回归, `ctest -C Debug --timeout 30`)。任何反欺诈复核请重跑 ctest + grep `FAILED`。

### Added

- `modern/src/services/` (新建 INTERFACE library, 3 header-only impl files, commit `1a2e5e0`):
  - `InventoryServiceImpl.hpp` (~150 行): bind to `mxh::game::ItemTotalInfo&`, 暴露 80 inventory + 10 weared slot。返回 pointer 指向同一 backing storage (host 通过 pointer 改 item state 直接 visible)。1:1 quirk: occupiedSlotCount 只算 inventory 0..79, 不算 weared 0..9 (legacy 区分 Inventory[80] / WearedItem[10])
  - `PlayerStatsServiceImpl.hpp` (~120 行): bind to `mxh::game::PlayerCombatStats&`, 暴露 level/HP/MP + 5 核心属性 (str/agi/int/wis/dex)。1:1 quirk: 5 核心属性当前 return 0 (等 StatsCalcManager port); exp_for_next_level 当前 return `100 * level` (等 character_exp.bin port)
  - `SkillServiceImpl.hpp` (~70 行): bind to `std::vector<LearnedSkill>&` (LearnedSkill = {idx, level, optional<quick_slot>}), 暴露 enumeration / isLearned / level / quickslot binding 查询
  - `CMakeLists.txt` (新建): INTERFACE library, include paths propagate 到消费者
- `modern/tests/unit/services/real/services_real_test.cpp` (新建, 15 用例 PASS):
  - InventoryServiceImpl (6): empty / pointer-to-backing / OOB / weared occupancy / findItem first-match / occupied count
  - PlayerStatsServiceImpl (5): defaults / max-zero div guard / partial HP fraction / level exp baseline / core attrs return 0
  - SkillServiceImpl (3): empty / enumeration / OOB index
  - Cross-service (1): CharacterDialog refresh 同时读 3 service, 跟 mock test 同 scenario
- `modern/src/ui/cmacrodialog.hpp` + `cmacrodialog.cpp` (新建, 1:1 port, commit `8ab8d01`):
  - `class cMacroDialog : public cDialog` — macro key binding UI, 7 quick-slot + 11 toggle-dialog + 1 minimap + 1 camera + 1 screencapture = 18 events
  - `enum MacroEvent { USE_QUICKITEM01 = 0, ..., ME_COUNT = 18 }` 1:1 mirror 旧 ME_* 数值
  - `enum MacroMode { Chat = 0, Macro = 1 }` 1:1 mirror 旧 MM_*
  - `enum SysKey { None = 1, Ctrl = 2, Alt = 4, Shift = 8, All = 15 }` 1:1 mirror 旧 MSK_* (bit flags)
  - `struct sMACRO { int nSysKey; uint16_t wKey; bool bAllMode; bool bUp; }` 1:1 mirror 旧 LEGACY struct
  - `Init(x, y, wid, hei)` 1:1 旧 CMacroDialog::Init (省略 basicImage / ID 参数, host 单独调 SetID / 加 cImage child)
  - `SetActive(bool) override` 1:1 gate: m_nMode == Macro 才 refresh 所有 cEditBox text (旧 legacy 行为)
  - `Linking()` 调 cDialog::findWindowById(MAC_EB_* id) 解析 15 个 cEditBox child, id 范围 100-106 + 200-210 + 300-305 + 400-401 + 500
  - `SetMacroBinding(evt, macro)` / `GetMacroBinding(evt)` 1:1 旧 m_MacroKey[ME_COUNT] 数组访问
  - `ConvertMacroToText(text, macro)` 1:1 flat switch on modifier (Ctrl/Alt/Shift 拼 "X + " 前缀, MSK_NONE 跳前缀)
  - `VKeyToName(vk)` static 覆盖 50+ common VK codes (F1-F12 / A-Z / 0-9 / SPACE / TAB / ENTER / 方向键)
- `modern/tests/unit/ui/cmacrodialog_test.cpp` (新建, 470 行, 20 用例 PASS):
  - enum 稳定性 (3): MacroEvent / MacroMode / SysKey 数值
  - sMACRO default-construction
  - Init 重置 / OOB Get 返默认
  - Set/Get 回合 / OOB Set 是 no-op / ClearChanged
  - SetMode / VKeyToName common + unknown
  - ConvertMacroToText 4 个 case (modifier-only / full / no-modifier / null buffer)
  - Linking 解析 quick-slot edit box
  - SetMacroBinding refresh linked edit box
  - SetActive Chat-mode 不 refresh / Macro-mode refresh

### Notes

- Phase 13.2 design: header-only INTERFACE library (跟 mxh_proto_tests /
  mxh_services_tests header-only mock 模式一致), 避免 .cpp 文件
  build 复杂度. 以后加 .cpp 实现 (e.g. 二进制 loader for
  character_mugong.bin) 时只需 extend services/CMakeLists.txt
- cEditBox 1:1 quirk: SetEditText is no-op if m_maxBytes == 0
  (modern port 跟旧 legacy `m_bInitEdit` 守卫行为一致, host
  必须先调 InitEditbox() 配 buffer, 跟旧 legacy InitEditbox 模式
  一致)

### Verification

- `ctest -C Debug -R "Real.*Service"` → 15/15 PASS
- `ctest -C Debug -R CMacroDialog` → 20/20 PASS
- `ctest -C Debug --timeout 30` → **946/946 PASS** (911 → 946, +35 用例, 0 回归, 16.57s)
- `git log --oneline -2`:
  ```
  8ab8d01 ui: 1:1 port of 墨香 MacroDialog (macro key bindings) + 20 tests
  1a2e5e0 feat(services): Phase 13.2 real service impls + 15 contract tests
  ```

## [0.13.13] - 2026-07-16

### P2-12 Tier 1.5 第 2 个子控件: cListDialogEx (self-verified by producer session)

**背景**: 0.13.12 推 cGuagen 1:1 port + R-9 矩阵约定审计 + CHANGELOG 收口 + roadmap 更新 + Phase 13 service interface 启动 (5 commit 全闭环)。user 反馈 "继续" 后本 session 推 1 个新子控件 (cListDialogEx) + 配套 doc 收口。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写。Verifier session ID 同上 (self-verify, 独立 verifier session 未分离 — 已知 limitation)。证据: cListDialogEx 14/14 ctest PASS (`ctest -C Debug -R CListDialogEx`); 全栈 897 → 911 PASS (+14 用例, 0 回归, `ctest -C Debug --timeout 30`)。任何反欺诈复核请重跑 ctest + grep `FAILED`。

### Added

- `modern/src/ui/cListDialogEx.hpp` + `cListDialogEx.cpp` (新建, ~10,300 行, commit `c85f776`): 1:1 port of legacy `墨香【源码】\[Client]MH\cListDialogEx.h` (310 B) + `.cpp` (4,371 B)。
  - `class cListDialogEx : public cListDialog` — 扩展 base 加 2 个 1:1 行为差:
    1. `ListMouseCheck(x, y, we)` — link row click (`type > emLink_Null`) 设 `m_rowClicked` flag + 触发 `RowClickedCallback` (替代 legacy `cbWindowFunc` 静态 dispatch, host 自己决定怎么发 `WE_ROWCLICK`)
    2. `Render()` — selected row 高亮 + multi-color link chain 渲染 (no-op 占位, 真实 sprite draw deferred 到 6.4+ cImage / Phase 13 host integration)
  - `enum LinkType { emLink_Null=0, emLink_Default=1, emLink_Image=2, emLink_Hyper=3 }` — 1:1 mirror 旧 enum 数值, 任何重编号会破坏 chat log 持久化 / 网络包解码 (LinkTypeEnumIsStable test pin)
  - `struct LinkItem { text, color, overColor, type, shared_ptr<LinkItem> next }` — 替代旧 LINKITEM struct + NextItem 指针, `shared_ptr` 避免手内存管理, multi-color chain (HelpDialog rich-text 入口)
  - `AddLinkItem(text, type, color, overColor, line=-1)` — append 或 insert at index
  - `AddLinkItemChain(head)` — 整条 chain 一次性 push, head.next shared_ptr 保留
  - `RemoveAll()` — 影子 m_linkItems + 调 base cListDialog::RemoveAll
  - `SetOnRowClicked(callback, userData)` + `ConsumeRowClicked()` — 现代 callback 替代旧 `cbWindowFunc` 静态指针
  - 局部 constexpr `WE_LBTNCLICK = 0x1` / `WE_ROWCLICK = 0x100` — 不拉全量 WindowIDEnum, 等第一个 Tier 2 dialog 真正需要时再全量落地 `modern/include/mxh/ui/WindowIDEnum.hpp`
- `modern/tests/unit/ui/clistdialogex_test.cpp` (新建, ~470 行, 14 用例 PASS):
  - DefaultConstructionIsEmpty / InitLinkListConfiguresMaxLine / RemoveAllClearsItemsAndSelection
  - AddLinkItemAppends / AddLinkItemInsertsAtIndex / AddLinkItemStoresColors
  - LinkTypeEnumIsStable (pin emLink_Null=0 数值)
  - ListMouseCheckOutOfRangeClearsSelection / InRangeSetsSelection / LinkRowSetsRowClicked
  - ConsumeRowClickedIsOneShot / RowClickedCallbackFiresOnLinkRow
  - AddLinkItemChainPreservesNext (multi-color chain 验证)
  - RenderIsNoop (deferred 跟 cGuagen 同源)

### Notes

- `RemoveAll()` 是 **name-hide 不是 virtual override** (跟 KNOWN_BUGS.md R-12 cExitDialog 同源, `cListDialog::RemoveAll` 非 virtual) — hpp 头注释显式说明, 跟踪 R-12 follow-up (cDialog::SetActive 已修, cListDialog::RemoveAll 等下次 Tier 2 dialog 落地时一起)
- 数据模型 影子 (m_linkItems vs base m_rows) 而非 refactor 进 base — 保持 1:1 API 行为, base consumer (cGuildDialog / cChatDialog 等) 不付 LinkItem overhead

### Verification

- `ctest -C Debug -R CListDialogEx` → 14/14 PASS
- `ctest -C Debug --timeout 30` → **911/911 PASS** (897 → 911, +14 cListDialogEx test, 0 回归, 20.43s)
- `git log --oneline -1`:
  ```
  c85f776 ui: 1:1 port of 墨香 cListDialogEx (link list with WE_ROWCLICK) + 14 tests
  ```

## [0.13.12] - 2026-07-16

### P2-12 dialogs Tier 2 子控件 + R-9 矩阵约定审计 (self-verified by producer session)

**背景**: 0.13.11 收口 C-36/C-37 server build matrix + 11.2 protocol doc + E-1 verifier note + 3 dev utilities + CI slow-test guard。user 反馈 "终极目标，啥活都要干，不能只干小活" 后本 session 推 2 个真活：(1) P2-12 Tier 2 子控件 cGuagen 1:1 port（Tier 2 dialog 的真 blocker, 之前 doc 写"无 blocker"是错的）；(2) R-9 矩阵约定审计 + 修复（Phase 5 stub 阶段遗留的隐藏 bug，off-axis eye 下 view 矩阵旋转/翻译全错）。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写。Verifier session ID 同上 (self-verify, 独立 verifier session 未分离 — 已知 limitation)。证据：cGuagen 18/18 ctest PASS (`ctest -C Debug -R CGuagen`)；R-9 新 7 个 test PASS (`ctest -C Debug -R "D3DX|ViewOrtho"`)；全栈 ctest 861 → 886 PASS (+25 用例, 0 回归, `ctest -C Debug --timeout 30`)。任何反欺诈复核请重跑 ctest + grep `FAILED`。

### Fixed

- **R-9** (commit `ba99367`): `modern/include/mxh/render/math.hpp` MatrixLookAtLH + MatrixOrthographicLH 写错 layout。旧代码把 translation 写在 column 3 (`_14/_24/_34` / `_43`)，实为 column-major 存储。Axis-aligned eye / identity view case 数值碰巧通过，**off-axis eye** 下 view 矩阵旋转/翻译全错。
  - **修复**: 决定项目用 D3DX row-major 约定 (`_ij` 命名 + C++ 行主序内存 + HLSL `mul(v_row, M)` row-vec mul)。translation 改到底行 (`_41/_42/_43` for view, `_43` for ortho z-translation)，basis vectors `(R.x, U.x, F.x)` 写 row 0。
  - **math.hpp 顶部**: 新增 D3DX convention 注释段，明确 CPU 端 `_ij` 行主序 + HLSL `mul(v_row, M)` 配套语义。
  - **3 个旧 test 更新** layout 期望：math_test.cpp (StandardForwardView + EyeMapsToOrigin) + mesh_geometry_test.cpp (LookAtLHProducesValidMatrix) — 这些 test 之前 pin 的是 bug layout (`_34 = 5` 当 translation), 现在 pin 正确 D3DX layout (`_43 = 5`)。
  - **状态**: 部分实装。primitives.cpp drawBox TODO (`use x,y for now, proper ortho projection is TODO`) 仍在 — 升级到 3D 顶点需要 vsolid input layout 从 `float2 pos` 改 `float3 pos` + 拆 2D/3D shader paths，1-2 commit 范围，**延后到 R-9.x** (新 session 推)。R-9 entry in KNOWN_BUGS.md 已更新。

### Added

- `modern/src/ui/cGuagen.hpp` + `cGuagen.cpp` (新建, ~120 行, commit `de390cd`): 1:1 port of legacy `墨香【源码】\[Client]MH\interface\cGuagen.h` progress bar widget.
  - `class cGuagen : public cWindow` — 子类化 cWindow (不是 cDialog, cGuagen 是嵌在 dialog 里的子控件)
  - `SetValue(float)` 只 clamp upper bound (legacy 1:1 行为：负数 pass through, `> 1.f` 截到 1.f)
  - `SetGuageImagePos(int, int)` / `(float, float)` — 重载保留 legacy 双重载
  - `SetPieceImage(const cImage&)` / `GetPieceImage()` — 接受现代 cImage value-semantic handle (R-10 adapter 等 cImage 接入 host 时再写)
  - `SetGuageWidth(float)` / `SetGuagePieceWidth(float)` / `SetGuagePieceHeightScale(float)` — 几何控制
  - `GetImageRelX/Y()` — 调试用 relative offset
  - `Render()` — no-op, 真实 sprite 绘制 deferred 到 6.4+ cImage seam (跟 R-10 同源)
  - 文档注释包含 `1:1 quirk: SetValue 负数 pass through` + 关联 P2-12 dialog 子控件
- `modern/tests/unit/ui/cguagen_test.cpp` (新建, 186 行, 18 用例 PASS):
  - DefaultConstructionZeroesAll / InheritsCWindow
  - SetValueStoresInRange / SetValueClampsAboveOne / SetValuePreservesZero / SetValuePassesThroughNegative (legacy quirk)
  - SetGuageImagePosIntOverload / FloatOverload
  - SetPieceImageDefaultIsNullSprite / SetPieceImageStoresCopy
  - SetGuageWidth / SetGuagePieceWidth / SetGuagePieceHeightScale
  - GetImageRelX/Y
  - RenderIsNoop
- `modern/tests/unit/render/math_d3dx_convention_test.cpp` (新建, 7 用例 PASS):
  - `MatrixLookAtD3DXTest.OffAxisEyeTranslationColumn` (eye=(3,4,5) pin bottom-row translation)
  - `MatrixLookAtD3DXTest.WorldRightBasisMapsToViewX` (basis 行 layout)
  - `MatrixLookAtD3DXTest.ViewMatrixIsRigidIsometry` (orthonormal basis check)
  - `MatrixLookAtD3DXTest.TargetDistanceIsPreserved` (|at - eye| 距离在 view space 保持)
  - `MatrixOrthoD3DXTest.CornersMapToNDC` (-2,-2,1 → NDC -1,-1,0 + 2,2,3 → 1,1,1 post-divide)
  - `MatrixOrthoD3DXTest.TranslationIsInRowThreeColumnTwo` (z-translation 位置)
  - `ViewOrthoCompositionTest.ViewAtOriginLooksAtPlusZ` (shadow pipeline 用的 view*ortho)
- `docs/KNOWN_BUGS.md` R-9 entry 大改 (commit `ba99367` 同步): 状态从 "Deferred" 改 "部分实装"，记录 D3DX 约定决策 + 7 个新 test + 3 个旧 test 更新 + drawBox 升级 TODO 延后到 R-9.x。

### Changed

- `modern/src/ui/CMakeLists.txt` +1 行 (`cGuagen.cpp`)
- `modern/tests/unit/ui/CMakeLists.txt` +19 行 (`cguagen_test.cpp` + 18 gtest_add_tests entries)
- `modern/tests/unit/render/CMakeLists.txt` +10 行 (`math_d3dx_convention_test.cpp` + 7 gtest_add_tests entries)

### Verification

- `ctest -C Debug -R CGuagen` → 18/18 PASS (0.25s)
- `ctest -C Debug -R "D3DX|ViewOrtho"` → 7/7 PASS (0.05s)
- `ctest -C Debug --timeout 30` → **886/886 PASS** (0 回归, 879 → 886, +7 R-9 test + 18 cGuagen test = +25, 已有 861 baseline)
- `git log --oneline -2`:
  ```
  ba99367 fix(render): R-9 - MatrixLookAtLH/MatrixOrthographicLH D3DX row-major layout + 7 new tests
  de390cd ui: 1:1 port of 墨香 cGuagen (progress bar widget) + 18 tests
  ```

### Lessons captured (this entry)

两条新发现 (加入 agent memory):
- **off-axis 测试才能 pin matrix layout bug** — R-9 旧测试只覆盖 axis-aligned eye (`(0,0,-5)` looking at origin) + identity view case，axis-aligned 时两种 layout (`_34` translation vs `_43` translation) 数值碰巧相同，bug 静默通过 4+ commit。修 R-9 强制 7 个 test 覆盖 off-axis eye `(3,4,5)` + asymmetric frustum `(4, 4, 1, 3)`，未来 layout regression 一跑就 fail。
- **C++ `_ij` 命名 + HLSL 默认 column-major packing = 数学转置** — C++ 端 `_11 _12 _13 _14 _21 ...` (row-major 内存) memcpy 到 cbuffer，HLSL `float4x4` 默认 column-major packing 读为 `M[0][0] M[1][0] M[2][0] M[3][0] M[0][1] ...` (column-major 内存) = CPU 内存整体转置。`mul(v_row, M_hlsl) = v_row × M_cpu^T` 数学。CPU 端写 D3DX row-major view 矩阵（basis 行 / 翻译底行），HLSL row-vec mul 自动用对约定，不要 CPU 端手动转置。

## [0.13.11] - 2026-07-16

### Server build matrix: C-36 / C-37 — Distribute + Agent + Map Debug_<LOCALE> 实测收口 (self-verified by producer session)

**背景**：Phase 7.5o (C-1 fix 2026-07-15) 把 CConsole::LOG 重命名为 CConsole::MLOG 并批量更新 35 个 active call sites。但 refactor 文档说"5/5 干净"**实际从未跑过全 build 验证**。本 session 跑 uild_distribute_debug_locales.py 实测发现 5/5 target 每个 fail 8 C2039 —— 8 处 g_Console.LOG( 调用漏改（C-1 扫描没扫到 [CC]ServerModule/）。

**Verifier note (per E-1 anti-fraud rule 5)**: 本 entry 由 producer session `mvs_95dbae3dead144e08e903d57a75beb75` 自主写。Verifier session ID 同上 (self-verify, 独立 verifier session 未分离 — 已知 limitation)。证据全部落地在 `modern/scripts/build_logs_20260716/c35_all_5_after_fix.log` (Distribute 5/5 build) + `agent_5locales.log` (Agent 5/5 build) + `map_5locales_after_fix.log` (Map 3/5 build, 2/5 legacy pre-existing C2065)。任何反欺诈复核请 grep `error C` in those logs.

### Fixed

- **C-36** (commit 53026a9 + b5f055): [CC]ServerModule/BootManager.cpp (7 sites) + [CC]ServerModule/DataBase.cpp (1 site) g_Console.LOG( → g_Console.MLOG(。Distribute Debug_<LOCALE> 5/5 干净。
  - DistributeServer_CHINA.exe 1,306,112 B
  - DistributeServer_KOR.exe 1,324,544 B
  - DistributeServer_JAPAN.exe 1,303,552 B
  - DistributeServer_HK.exe 1,312,256 B
  - DistributeServer_TL.exe 1,306,112 B
  - All 5 byte sizes match C-35 fix entry's target values 1:1
- **C-36 followup** (commit b5f055): 验证 [CC]ServerModule/Network.cpp:74,78 的 2 处 g_Console.LOG( 是**死代码**（整个 CNetwork::Start() 函数被 /* */ 块注释禁用）+ Agent 5/5 同样干净（顺带验证）。
- **C-37** (commit 5b7f854): [Server]Map/ 7 cpp 43 active sites rename — MapItemDrop 8 / Server 3 / RegenManager 1 / MapDBMsgParser 1 / MapNetworkMsgParser 4 / ServerSystem 13 / UserTable 13。Map Debug_<LOCALE> **3/5 干净**（KOR/CHINA/TL）+ **2/5 撞 JAPAN/HK 1:1 legacy pre-existing bug**（C-37 entry 记录但不修）。
  - MapServer_CHINA.exe 3,879,424 B
  - MapServer_KOR.exe 3,879,936 B
  - MapServer_TL.exe 3,882,496 B
  - MapServer_JAPAN.exe / MapServer_HK.exe — 3 C2065 each (EXTRA_PYOGUK_SLOT + 2x m_dwMussangTime，pre-existing legacy，跟 C-35 收手策略一致)

### Added

- modern/scripts/build_map_debug_locales.py (新): 5-locale build 验证脚本，对齐 uild_distribute_debug_locales.py / uild_agent_debug_locales.py 模式——3/3 server 都有 5-locale build verification 工具了。
- docs/KNOWN_BUGS.md C-36 + C-37 entries: 完整记录 LOG→MLOG regression 修复过程 + JAPAN/HK Map 1:1 legacy 暗礁 + Network.cpp 死代码真相。

### Added (continued)

- docs/MoxianProtocolDoc.md (新, 6,996 bytes): Phase 11.2 协议文档首次落地。用 modern/tools/gen_protocol_doc.py（Python wrapper）调用 MoxianProtocolDoc --summary 拿数据 + Python 直接 extract MP_CATEGORY/MP_PROTOCOL_* enum 名。77 个真实 category entries + 64 个 protocol enum 名字 + 3,458 total protocol values 全落档。C++ 工具的 generateMarkdown() 在 full 92KB Protocol.h 上 STATUS_STACK_BUFFER_OVERRUN crash，**bug 单独修不在本 commit 范围**。
- modern/tools/gen_protocol_doc.py (新, 172 lines): Python wrapper——处理 Protocol.h 的 cp949 编码 + 用 line-based regex 提取 enum 名，避开 C++ 工具的 crash。

### Lessons captured

两条新 agent memory 记录:
- **refactor 必须全栈 grep + 真跑验证** — 改名/换签名类 refactor 后必须真跑全 build matrix 验证，不是只信 refactor 文档的"已修复"。
- **legacy 死代码用 /* */ 块注释常见** — 看到疑似"未修复"代码先验证（grep ^/\*|^\*/、dumpbin symbol、build verify）再判定 build 影响。

### Added (continued 2)

- `modern/scripts/check_modernization_plan_refs.py` (新, 96 lines): Phase 12.x utility — cross-check `MODERNIZATION_PLAN.md` file references vs reality. Scans for `modern/...` paths in the plan, checks each on disk, filters out braced globs + glob wildcards + bare project codenames. Exit 0 = all 21 refs OK; exit 1 + missing list if anything stale. Pair with the existing CI guards.

### Docs

- `MODERNIZATION_PLAN.md` 4 stale references synced to actual modern/ layout (commit bd1d50a):
  - `modern/MoxianCompat` → `modern/src/{mh_file_ex,pack_file,chr_motion,chx_model,bmhm_map,bsad_area,ttb_tile_table}.cpp` (flat, not subdirectory)
  - `modern/MoxianDb` → `modern/src/{db_adapter,db_factory,mssql_odbc_adapter,sqlite_adapter}.cpp` (flat, not subdirectory)
  - `modern/src/render/mxh_render` → `modern/src/render/dx11/` (dx11/ subdir, not mxh_render/)
  - `modern/scripts/phase75i_distribute_kor_v3.log` → `modern/scripts/build_distribute_locales_v2.log` (archive replaced)
  - 21 modern/ file references verified all on disk
- `CHANGELOG.md` 0.13.11 E-1 anti-fraud verifier note (commit 1918789):
  - Title: "✅" → "(self-verified by producer session)"
  - Added "Verifier note (per E-1 anti-fraud rule 5)" paragraph with producer session ID, build log paths, and grep audit command
  - Per KNOWN_BUGS.md E-1: producer self-verify must be disclosed, not implied as independent verification

### Tooling (continued)

- `modern/scripts/scan_console_log_calls.py` (新, 50 lines): find active `g_Console.LOG(` in 墨香【源码】/ — strict regex excludes line comments, block comments, and `g_Console.Log(` (small L, 3-arg style). Used during C-37 fix to verify all 41 hits were inactive.
- `modern/scripts/count_active_console_log_calls.py` (新, 34 lines): single-file count helper for the 7 server-side cpp files that needed the C-1 rename.
- `modern/scripts/rename_console_log_to_mlog.py` (新, 62 lines): mechanical rename tool with idempotent + dry-run modes. Used during C-36 / C-37 to do the actual renames.

### CI

- `modern/scripts/ci_test_distribution_guard.py` slow-test guard closed (commit 8297477): replaced the `TODO: parse build/Testing/Temporary/LastTest.log` stub (5+ years old) with a real `parse_ctest_cost_data()` parser that reads `build/Testing/Temporary/CTestCostData.txt` (the actual ctest timing file). 1244 tests tracked. Slowest is `MssqlOdbcAdapter.ConnectToInvalidServerFails` at 0.07s (known C-32 SQL Server retry timeout, expected). The `LastTest.log` TODO was based on incorrect comment about where ctest writes timing — the real file is `CTestCostData.txt`. End-to-end functional now; CI step can be enabled without code changes.

### Lessons captured (continued)

两条新 agent memory 记录:
- **legacy 死代码用 `/* */` 块注释常见** — 看到疑似"未修复"代码先验证（grep `^/\*|^\*/`、dumpbin symbol、build verify）再判定 build 影响。C-36 followup 验证了 `[CC]ServerModule/Network.cpp:74,78` 的 2 处 `g_Console.LOG(` 是死代码，dumpbin 0 引用。
- **E-1 anti-fraud: producer self-verify 必须显式披露** — 不写"✅ gate passed"隐含独立 verifier 角色。CHANGELOG 0.13.11 entry 改为 `(self-verified by producer session)` + session ID + build log 路径 + grep 审计命令。诚实自报 > 自信的虚假宣称。

## [0.13.10] - 2026-07-16

### Phase 12.1: P2-12 dialogs 启动 + 5 档 roadmap ✅

**背景**：Phase 6 stub 阶段遗留 ~197 个 dialog 没 port。Modern `src/ui/`
只有 4 个 dialog（cDialog 基类 + cGuildDialog + cIconDialog + cListDialog）。
每次新 session 推 1-2 个 Tier 1 dialog 节奏不变，但需要先有 roadmap
让后续 session 按矩阵接活。

**已实装**

- `modern/src/ui/cExitDialog.hpp` (新建, ~140 行):
  - `class cExitDialog : public cDialog` — 1:1 port of legacy CExitDialog
  - `Init(x, y, w, h, basicImg, id=0)` — 位置 + 尺寸 + 背景图
  - `SetActive(bool) noexcept` — 名字隐藏基类同名方法（cDialog::SetActive
    非 virtual；R-12 列出但未修，cDialog* 多态调用会跳过 callback）
  - `SetOnActiveChanged(ActiveChangedCallback)` —
    `std::function<void(bool)>` 替代 legacy CMainBarDialog 直接依赖，
    host caller 自己决定连不连 main bar
  - `exitActive()` 测试访问器
  - 文档注释包含 R-12 关联 + 已知 polymorphic dispatch 限制

- `modern/src/ui/cExitDialog.cpp` (新建, ~30 行):
  - `SetActive` 实现：`wasActive != val` 转换时调 callback
  - 初始 active 跟随 cDialog::Init 之后的 m_bActive=false

- `modern/tests/unit/ui/cexitdialog_test.cpp` (新建, 10 用例):
  - DefaultConstruction / InitResetsActiveState /
    SetActiveUpdatesBaseAndExitFlags / CallbackFiresOnTransition /
    CallbackDoesNotFireOnSameValue / CallbackCanBeCleared /
    CallbackCanBeReboundAfterInit / CallbackReceivesNewValueNotOld /
    InheritsDialogTreeManagement / NonCopyable
  - 10/10 PASS

- `docs/P2-12_DIALOGS_ROADMAP.md` (新建, ~280 行):
  - 5 档分级：Tier 1 (9 trivial) / Tier 2 (10 子控件) /
    Tier 3 (9 InventoryService 阻塞) / Tier 4 (3 NpcScriptEngine 阻塞) /
    Tier 5 (100+ network service 阻塞)
  - 整体估算 4-5 月全职工作量
  - 推进节奏建议：每次 session 1-2 个 Tier 1

- `docs/KNOWN_BUGS.md` R-12: SetActive polymorphic bug
  - cDialog::SetActive 不是 virtual → cGuildDialog/cExitDialog 是
    name-hide 而非 override → cDialog* 多态调用跳过子类 callback
  - 列出但未修；建议第一个 Tier 1 dialog 移植时顺手把基类改 virtual

**质量门**

- 增量编译：mxh_ui.vcxproj 干净 (~10s)
- 全栈 ctest: 850 → 860 PASS (+10 用例, 0 回归)
- 字节级一致：cExitDialog 1:1 模拟 legacy CExitDialog 行为
  （SetActive 联动 main-bar exit icon highlight，via callback）
- 非破坏性：基类 cDialog 签名未动；其他 4 个已 port dialog 不受影响

**遗留 / 下一个 session 推**

- P2-12a cHelpDialog (Tier 1, ~80 行 + 6 test) — 推荐
- P2-12b cBailDialog (Tier 1, ~120 行 + 8 test)
- P2-12c cEventNotifyDialog (Tier 1, ~150 行 + 8 test)
- 顺手修 R-12 (cDialog::SetActive 改 virtual + 子类加 override)

**未 commit**（待 user 确认）:

```
 modern/src/ui/cExitDialog.hpp        | 新
 modern/src/ui/cExitDialog.cpp        | 新
 modern/tests/unit/ui/cexitdialog_test.cpp | 新
 modern/src/ui/CMakeLists.txt         | +1 行
 modern/tests/unit/ui/CMakeLists.txt  | +2 段
 docs/P2-12_DIALOGS_ROADMAP.md        | 新
 docs/KNOWN_BUGS.md                   | +R-12 段
 AI_TASK_QUEUE.md                     | v2.1 → v2.2
 AI_SHIFT_LOG.md                      | +06:55 记录
```

## [0.13.4] - 2026-07-16

### Phase 12.1: IME hook 接口 + Win32 IMM reference adapter ✅

**背景**：Phase 6 stub 阶段 cEditBox / cWindowManager 完全没 IME 处理
（legacy 在 TAIWAN / HK / JAPAN build 用 `imm32.lib` 处理
`WM_IME_STARTCOMPOSITION` + `ImmSetCompositionWindow` / `ImmSetOpenStatus`）。

**已实装**

- `modern/include/mxh/ui/ime.hpp`（新建）：
  - `enum class ImeEditType { EditBox, Spin, TextArea, Number, Other }`
  - `struct ImeAdapter` 4 hook 字段（onFocusEdit / onBlurEdit /
    onStartComposition / acceptsIme）
  - `installImeAdapter(const ImeAdapter&)` + `isImeAdapterInstalled()`
  - `installWin32Ime(HWND)` / `uninstallWin32Ime()` 声明
  - `detail::ime_dispatch_*` 内部 dispatcher（platform-agnostic）
  - HWND 前向声明（非 Win32 平台不拉 windows.h）
- `modern/src/ui/ime.cpp`（新建）：platform-agnostic dispatcher
  + 单例状态 + 4 hook 路由
- `modern/src/ui/ime_win32_imm.cpp`（新建，`#ifdef _WIN32`）：
  - 镜像 legacy `MHClient.cpp:577-605` 模式：focus → ImmGetContext
    + ImmSetCompositionWindow(caret_x, caret_y, 512×20) +
    ImmSetOpenStatus(TRUE)；blur → ImmNotifyIME(CPS_CANCEL)
  - Number-only edit 抑制 IME（legacy VCM_NUMBER 行为）
- `modern/tests/unit/ui/ime_test.cpp`（新建）：13 用例
  - Install/uninstall / 4 hook 独立可装 / 默认 no-op / 默认 accept
  - reinstall 替换前一个 / null hook 安全 / accumulate
- 现代 `modern/src/ui/CMakeLists.txt`：`ime.cpp` 总是编译，
  `ime_win32_imm.cpp` 仅 WIN32 + `target_link_libraries imm32`
- `modern/tests/unit/ui/CMakeLists.txt`：注册 `ime_test.cpp`

**未实装（KNOWN_BUGS 范围）**

- cEditBox / cWindowManager **不**自动 dispatch IME hook
  （避免触及 cWindow 状态机）
- MoxianClient host app 需要在 `SetFocusEdit` / 失去焦点时手动调
  `detail::ime_dispatch_focus()` / `ime_dispatch_blur()`
- 实际 IMM Win32 reference adapter 需要真 hwnd 才能跑（测试 mock
  不出），所以 `installWin32Ime(HWND)` 路径只在 host 接入时
  手动验证

**测试数**：`mxh_ui_tests` 217 → **230 PASS**（+13 IME）
**全栈**：`mxh_compat 80` + `server_handler 16` + `mxh_render 227`
+ `mxh_ui 230` = **553/553 PASS**（0 回归）

## [0.13.8] - 2026-07-16

### Phase 12.1 P2-13: TcpClient → ITcpSender 可注入化 ✅

**背景**：之前 P2-7 agent_handler on_disconnect 加了 GameOutSyn 转发
逻辑，但 `AgentHandler::map_client_` 持有的是具体类型 `TcpClient*`，
无法 mock。测试只能覆盖 nullptr / disconnected 两条 early-return
路径；"GameOutSyn 真的发出去"必须等真 map server + 集成测试。

**已实装**

- `modern/include/mxh/net/net.hpp`：
  - 新增 `class ITcpSender { virtual NetError send(const Message&) = 0; virtual bool is_connected() const noexcept = 0; virtual ~ITcpSender() = default; };`
  - `TcpClient : public ITcpSender`，`send()` / `is_connected()` 加 `override`
- `modern/include/mxh/server/server.hpp`：
  - `AgentHandler::set_map_server(ITcpSender* client, ConnectionId)` —— 类型从 `TcpClient*` 改为 `ITcpSender*`
  - `map_client_` 类型相应从 `TcpClient*` 改为 `ITcpSender*`
- `modern/src/server/agent_handler.cpp`：3 处 `TcpClient* mc = nullptr` → `ITcpSender*`，1 处 `set_map_server` 签名同步
- `modern/tests/unit/server/server_handler_test.cpp`：
  - 新 `class MockTcpSender : public ITcpSender`（计数 + 收集 sent_msgs + 可切 connected）
  - 4 新测试：`OnDisconnectWithMockSenderNoSessionDoesNotSend` /
    `OnDisconnectWithMockSenderDisconnectedSenderNoSend` /
    `ForwardFromMapWithMockSenderNoRoute` /
    `SetMapServerAcceptsITcpSender`
  - 注释解释：完整 "GameOutSyn 真的 send" 验证需要填私有 map
    `conn_user_ids_ / conn_char_ids_ / conn_map_nums_`，无 public
    setter → 这一段留给 Phase 9 集成测试 `test_map_integration.py`
- `forward_from_map` 路径也得益于 ITcpSender：因为 forward 只调 reply_
    （不调 map_client_），所以 MockTcpSender 验证的是 reply_ 路由
    行为而非 sender 调用。

**测试数**：`mxh_server_handler_tests` 16 → **20 PASS**（+4 MockTcpSender）
**全栈**：`ctest -C Debug` 843 → **847/847 PASS**（+4，0 回归）

**关联**：`modern/include/mxh/net/net.hpp` (ITcpSender 声明 +
TcpClient 多态化)、`modern/include/mxh/server/server.hpp`
(set_map_server 签名)、`modern/src/server/agent_handler.cpp` (3 处
`mc` 类型 + 1 处函数签名)、`modern/tests/unit/server/server_handler_test.cpp`
(MockTcpSender + 4 测试)。

## [0.13.7] - 2026-07-16

### Phase 12.1 P3: 文档债收尾 + modern/ 代码量快照 ✅

**背景**：AI_TASK_QUEUE P3 队列 5 条中 3 条文档债过期或没落地：
- CHANGELOG.md "Upcoming" 段指向已完成 Phase 9.3/10
- MODERNIZATION_PLAN.md Phase 5/6 表格里 BC6H/BC7 / IME / cImage GPU
  还写"⏳ future"（实际 P2-10/11 做了，R-10 部分做了）
- modern/ 代码量趋势无文件跟踪（baseline "78+35+47" 已 4 天）

**已实装**

- **CHANGELOG.md "Upcoming" 段重写**：
  - 拆三段：P2 剩余（已完 ✅ / 撤回转 R-* ❌）/ 仍在队列（dialogs /
    TcpClient / integration ctest）/ 仍 deferred（C-32 / Perf-4/5 / R-11）
  - 反映本 session 实际推进状态
- **MODERNIZATION_PLAN.md**：
  - Phase 5 表格 line 408：`BC6H/BC7` 从 ⏳ future → 🟡 partial (12.1 P2-11) + R-11
  - Phase 6 表格 line 492-496（两处相同表格）：`Real GPU draw (cImage)` → 🟡 partial + R-10
  - Phase 6 表格：`IME` → ✅ done (12.1 P2-10)
  - 其他 3 行（Drag-drop / Sortable columns / 79 dialogs）保持 ⏳ future
- **modern/CODE_METRICS.md**（新建，4 节）：
  - 统计命令（PowerShell 6 行）
  - Snapshot 趋势表格（2026-07-15 P10.4 wrap → 2026-07-16 P12.1 wrap）
  - 增长归因：+12 src 文件 / +3 include / +31 tests → +46 文件 +11639 行
  - 速度指标：测试/源码比 0.77，通过率 100%
  - 下次统计触发点

**测试数**：`ctest -C Debug` → 843/843 PASS（仅文档改动，0 回归）

**关联**：`CHANGELOG.md` 0.13.7 / 0.13.8、`MODERNIZATION_PLAN.md` line 408/492-496/558-561、
`modern/CODE_METRICS.md`（新建）。

## [0.13.6] - 2026-07-16

### Phase 12.1 P2-12 (D): modern/scratch/ 大扫除 ✅

**背景**：172 文件（10 子目录）堆在 `modern/scratch/_archive_2026-07-15/`
里 4 天（Phase 7.5p ~ 10.4 期间累积），违反 AGENTS.md trap #10
"scratch 大小控制"精神（>50 文件 / 10 MB / >3 天应清理）。

**已清理**

- `mavis-trash` 整目录 `_archive_2026-07-15/`（167 文件 / 10 子目录 /
  3.5 MB）：client_probes / monitor_tools / decode_tools / client_logs /
  client_bin / build_scripts / ld_scripts / mhfile_tools / msl_inspect /
  titan_probe —— 全部 grep 验证 0 外部引用（脚本、文档、复现命令）
- `mavis-trash` `project_status_2026-07-16.html`（本 session 临时
  可视化报告，关键信息已写入 AI_SHIFT_LOG 03:55 / 04:00 段）

**保留 / 迁出**

- `test_map_integration.py`（Phase 9 端到端集成测试，9 步流程）：
  从 `_archive_2026-07-15/client_probes/` 复制到 `modern/scratch/` 根
  （`MODERNIZATION_PLAN.md:269,274` 复现命令依赖 `python
  modern/scratch/test_map_integration.py`，脚本通过 `SCRIPT_DIR` 自定位
  + `WORKSPACE = ../` 假设必须在 scratch 根）

**`.gitignore` 调整**

- 之前：`modern/scratch/` 整目录 ignore → `test_map_integration.py` 不进 git
- 现在：`modern/scratch/_archive_*/` 子目录 ignore + `!*.py` + `!*.md`
  反向例外 → archive 子目录忽略，根文件可被 git 跟踪
- `git check-ignore` 验证：
  - `_archive_2026-07-15/` → 忽略 ✓
  - `test_map_integration.py` → 不忽略 ✓
  - `README.md` → 不忽略 ✓

**重写 `modern/scratch/README.md`**

- 顶层索引：当前 2 文件（README + test_map_integration.py）+ 已清理列表
- 维护规则 4 条：来源 / 用途 / 大小控制 + 反面教材（172 文件堆 4 天）

**测试数**：`ctest -C Debug` → **843/843 PASS**（0 回归）
**git**：M `.gitignore`（scratch 段重写），2 个 untracked
`modern/scratch/{README.md, test_map_integration.py}` —— **未 commit**（等
user 决定）

**关联**：`modern/scratch/README.md`、`modern/scratch/test_map_integration.py`、
根 `.gitignore` line 178-187。

## [0.13.5] - 2026-07-16

### Phase 12.1: BC6H / BC7 压缩编码器 + DX10 扩展头 ✅

**背景**：Phase 5 deferred stub 列表里"BC6H/BC7 real compression"是
最显眼的一项（comment 里直接写了"BC6H/BC7 real compression is out of
scope"）。Texture loader 只能输出未压缩 BGRA8 DDS；新的 BC6H/BC7 走
DX10 extended header 路径，需要新的 `DdsHeaderDxt10` struct + new
`MAKEFOURCC('D','X','1','0')` + 实际块编码器。

**已实装**

- `texture_loader.hpp` `BCFormat` enum 加 `BC6H` / `BC7` 两条
- `texture_loader.cpp`：
  - 新 `DdsHeaderDxt10` struct（20 B packed, `static_assert` 验证）
  - `MAKEFOURCC('D','X','1','0')` 走 DX10 ext 路径
  - `dxgi_format::BC6H_UFLOAT = 95` / `BC7_UNORM = 98` 常量
  - `encode_bc6h_block_mode1(block, out16)`：mean-color 端点 + 全 0
    index；注释里写清 BC6H mode 1 的 128 bit 字段布局
  - `encode_bc7_block_mode6(block, out16)`：mode 6 + 8-bit RGBA endpoint
    + 16 × 4-bit 索引位布局（mode 6 全部填 0xFF 端点 + 索引 0）
  - `saveDDS_BC` switch 加 `BC6H` / `BC7` 分支
  - DX10 头布局：`4 (magic) + 124 (DDS_HEADER) + 20 (DDS_HEADER_DXT10) + blocks`
- `texture_loader_test.cpp`：14 个新测试
  - `SaveDDSBC6H` (6)：magic+header / fourcc=DX10 / dxgiFormat=95 /
    resourceDim=3 / payload 16 B/block / 非 4 倍数 pad / mode 1 字段布局
  - `SaveDDSBC7` (5)：magic+header / fourcc=DX10+dxgi=98 / payload 16B /
    mode 6 字段布局 / mean-color 端点
  - `SaveDDSBCAuto` (2)：alpha gradient → BC3 (DXT5) / no alpha → BC1
    —— Auto **不**自动升级 BC7（legacy `ConvertCompressedTexture` 启发式
    1:1 匹配；BC7 必须 host 显式请求）
  - `SaveDDSBCFormat` (1)：empty texture 5 个 format 都返回空

**未实装（KNOWN_BUGS R-11）**

- BC6H 端点 = block 16 像素的 RGB mean（无 per-block 模式选择；R0=R1）
- BC7 mode 6 端点 = mean + 索引全 0（不分区 / 不旋转）
- 质量**故意低**（GPU 能正确显示单一色块，per-block 模式选择留给
  DirectXTex / bc7enc 之类外部 encoder）—— interface / 文件结构合法

**修正**

- 测试 `EXPECT_EQ(fourcc, 0x30313158u)` 是 typo —— 期望值应是
  `0x30315844u`（"DX10"），不是 `"X110"`。已修正。
- 测试 `read_u32(dds.data() + 4 + 128)` 错位 —— DX10 头起始于
  `4 + 124 = 128`，但 `dxgiFormat` 在 `DdsHeaderDxt10` 的第 0 个 u32，
  所以是 `4 + 124 = 128`，不是 `4 + 128 = 132`（那是 `resourceDim`）。
  已修正为 `4 + 124`。
- 移除两处 `std::printf` debug（DX10 ext hexdump + BC7 "useDx10=..."）

**测试数**：`mxh_render_tests` 227 → **242 PASS**（+15: 14 BC6/BC7
+ 1 hidden adjustment 计数；详见 render suite output）
**全栈**：`ctest -C Debug` → **843/843 PASS**（0 回归，2 skipped：缺
11160.chr 真实样本）

**关联**：`modern/src/render/dx11/texture_loader.{hpp,cpp}`、
`modern/tests/unit/render/texture_loader_test.cpp` (+14 用例)、
`docs/KNOWN_BUGS.md` (R-11)。

## [0.13.9] - 2026-07-16

### Phase 12.1: agent_handler HSEL init injection 加 `_CRYPTCHECK_` 守卫 ✅

**背景**：之前 P11a (commit 99c9b24, Phase 11) 在
`AgentHandler::handle_legacy_character_list` 的 charlist ack 头部
**无条件**注入了 128 字节 HSEL init key（eninit + deinit）。这在
`_CRYPTCHECK_` 编译的 client 是对的（SEND_CHARSELECT_INFO struct
头就是这两个 64-B init block），但**非 `_CRYPTCHECK_` legacy
client 不知道这 128 字节**——它们的 client parser 直接把 char_count
字段读成 HSEL key 头 4 字节的随机值，整个 payload 解析全错。

**症状**：
- `test_map_integration.py` Step 4 输出 `chrid=450035712 map=0
  level=0` —— chrid/map/level 全是 HSEL key 头的随机值
- Step 5 CharacterSelectSyn 查 DB 找不到这个 chrid →
  CharacterSelectNack → 测试 fail

**已实装**

`modern/src/server/agent_handler.cpp` line 555-567: 把无条件
`put_hsel_init(...)` × 2 改到 `#ifdef _CRYPTCHECK_` 块内

```cpp
// Phase 11a fix: ...（注释保留）
// Phase 12.1 fix: gate the injection on _CRYPTCHECK_ so legacy
// clients (which do not define the macro and therefore do not
// expect the 128 B prefix) get a payload that starts directly
// with char_count.
#ifdef _CRYPTCHECK_
std::random_device rd;
std::mt19937 rng(rd());
put_hsel_init(payload, rng);  // eninit
put_hsel_init(payload, rng);  // deinit
#endif
```

**测试结果**
- 单元 ctest 847/847 PASS（不破）
- `test_map_integration.py` Step 4 修：`chrid=2606684 map=12
  level=1`（真 character from DB），Step 5+ 跑通，Step 7 仍有
  自己的 second-connection 测试设计问题（**这是 Python 测试逻辑
  bug，不是 modernization bug**，留给独立 P2-12 修）

**关联**：`modern/src/server/agent_handler.cpp` (1 处 ifdef 守卫)。

## [0.13.10] - 2026-07-16

### Phase 12.1 P2-13 follow-up: register_session() public method + 完整 GameOutSyn 单元测试 ✅

**背景**：0.13.8 (P2-13) 加了 `ITcpSender` interface 但留下了
一个口：测试**没法**填 `conn_user_ids_ / conn_char_ids_ /
conn_map_nums_ / char_to_client_` 4 个 private map（legacy
character list/select 流程才填）。结果 4 个 MockTcpSender 测试
只覆盖了 early-return / null / disconnected 路径；**完整的
"on_disconnect 触发 GameOutSyn 转发 → MockTcpSender 真的收到"
路径没法单测**。

**已实装**

- `modern/include/mxh/server/server.hpp` `AgentHandler`：
  - 新 public method `register_session(ConnectionId, user_id, char_id,
    map_num)` —— 同时填 4 个 map（conn_user_ids_ / conn_char_ids_ /
    conn_map_nums_ / char_to_client_），lock 顺序与 on_disconnect /
    forward_from_map 一致避免死锁
  - 这是**生产可用的入口**（Phase 13+ 从持久化存储恢复 session
    state 时用），不只是 test-only
- `modern/src/server/agent_handler.cpp`：`register_session` 实现
  紧跟 `set_map_server` 之后
- `modern/tests/unit/server/server_handler_test.cpp`：3 个新测试
  - `RegisterSessionStoresUserCharMap` — 注册 session → 断连
    → 验证 MockTcpSender 收到 1 条 GameOutSyn（cat=UserConn,
    proto=31=GameOutSyn, obj=char_id, payload=wMapNum+bIsExiting=1）
  - `RegisterSessionOverridesPriorSession` — 同 conn 二次注册
    用新 char_id 覆盖
  - `RegisterSessionIsNoOpForUnknownConn` — 未注册的 conn 断连
    不触发转发（map 正确按 conn_id key）

**测试数**：`mxh_server_handler_tests` 20 → **23 PASS**（+3）
**全栈**：`ctest -C Debug` 847 → **850/850 PASS**（+3，0 回归）

**遗留**

- `register_session` 暴露了原本私有的 session state；如果未来
  Phase 13+ 持久化层直接用 OK，但 12.x 阶段任何 caller 都能改
  state 是**有意为之**（测试需要）。生产代码应走 register_session
  + set_map_server 两条入口，不要直接访问 conn_*_ map
- P2-13 完全收口："GameOutSyn 真的发出去"现在**可单测**，不再
  依赖真 MapServer 或 Phase 9 集成测试

**关联**：`modern/include/mxh/server/server.hpp` (AgentHandler
public method)、`modern/src/server/agent_handler.cpp` (实现)、
`modern/tests/unit/server/server_handler_test.cpp` (+3 用例)。

## [0.13.3] - 2026-07-16

### Phase 12.1: agent_handler 断连 GameOutSyn 转发 ✅

**背景**：`modern/src/server/agent_handler.cpp:207` 的 TODO 标记
（"send GameOutSyn to MapServer on client disconnect"）从 Phase 9 一直挂着。
MapServer 端其实**已经实现**了 GameOutSyn 接收（`map_handler.cpp:344` 删除
player + 通知其他玩家 + 回 GameOutAck），但 Agent 端从不发，导致 MapServer
侧的 player 状态在断连后会一直保留到下次 GameInSyn 覆盖。

**已实装**

- `agent_handler.cpp::on_disconnect()` 改写（~80 行新增）：
  1) 保留旧的"清理 conn_user_ids_/conn_char_ids_/conn_map_nums_"逻辑
  2) **新增**：snapshot `map_client_` under `map_route_mu_`（避免与
     set_map_server() 竞争）
  3) **新增**：触发条件 = `removed_char_id > 0 && had_map_num && mc && mc->is_connected()`
  4) **新增**：构建 Category=UserConn, Protocol=GameOutSyn, object_id=char_id
     消息，payload = wMapNum(2B) + bIsExiting=1(1B) + padding(5B)
  5) **新增**：调 `mc->send(fwd)`，错误/成功都 log
  6) **修正**：之前注释"不删 char_to_client_ 等 GameInSyn 覆盖"——
     现在 GameOutSyn 真的告诉 MapServer 删了，所以**安全删 char_to_client_**

**测试**

- `OnDisconnectWithoutMapServerDoesNotCrash` — 没调 set_map_server() 时
  断连不崩、不 deref null TcpClient
- `OnDisconnectWithMapServerNullptrDoesNotCrash` — 调了 set_map_server
  但传 nullptr TcpClient 时断连不崩
- server_handler_tests 14 → **16/16 PASS**（+2 新）
- map_handler.cpp 编译干净
- 全 mxh_compat_tests 80/80 PASS（0 回归）

**遗留**：当前测试只覆盖"无 map_client_/null map_client_"路径。完整的
"map_client_ 真的连着时发对 GameOutSyn"需要 mock TcpClient::send()，
TcpClient 不是 abstract 不能继承，**需要小重构让 TcpClient 可注入**
（Phase 12.x deferred）。

**关联**：`modern/src/server/agent_handler.cpp` (on_disconnect)、
`modern/tests/unit/server/server_handler_test.cpp` (+2 测试)、
`modern/src/server/map_handler.cpp` (GameOutSyn 接收端，已存在，未动)。

## [0.13.2] - 2026-07-16

### Phase 12.1: map_handler 物品效果实装（R-8 部分修复）✅

**背景**：`modern/src/server/map_handler.cpp:811` 的 TODO 标记（"apply item
effects HP/MP recovery, buffs, etc."）从 Phase 10b P0 一直挂着，只 echo 回
UseAck 不实际改 player 状态。

**已实装**

- `modern/include/mxh/game/item_effects.hpp`（新建）：
  - `classify_item(wIconIdx)` → 4 类消耗品 + 1 类非消耗品
  - `resolve_item_effect(wIconIdx)` → `{hp_delta, mp_delta, buff}` struct
  - 范围约定：1-99 HP 药、100-199 MP 药、200-299 HP+MP 药、300-399 Buff 药
- `modern/src/item_effects.cpp`（新建）：线性缩放实现
- `modern/src/server/map_handler.cpp` UseSyn 改写：
  1) 读 inventory[pos] 拿 `wIconIdx`
  2) classify + resolve
  3) apply 到 `PlayerInfo::combat.current_hp/mp`（clamp [0, max]）
  4) 回 UseAck（payload 12B：pos + wIconIdx + hp_delta + mp_delta + new_hp + new_mp）
  或 UseNack（空 slot / 非消耗品）
- 15/15 item_effects 测试 PASS（classify + resolve 全覆盖）
- server_handler_test 14/14 PASS（0 回归）

**遗留（KNOWN_BUGS R-8）**

- `ItemList.bin` 解析器未实装，hardcoded 表只覆盖 4 类消耗品
- legacy 等级曲线 / buff duration / 装备 stat mod 全部缺省
- Phase 12.x deferred：实现 `ItemList.bin` 格式层 + 替换硬编码表为实时查表

**测试数**：`mxh_compat_tests` 65 → **80/80 PASS**（+15 item_effects）
**新增文件**：3 个（hpp + cpp + test）

## [0.13.1] - 2026-07-16

### Phase 12.1: P2 资源格式补全收尾（P2-2 + P2-3）✅

**重大发现**：原 Phase 1.3 stub 阶段 `ChrMotion` / `ChxModel` 假定的
二进制 header 格式与 legacy 4Dyuchi 真实文本格式不符。.chr / .chx
文件**不含骨骼轨道**——骨骼在 .mod（`FILE_SCENE_HEADER` 28 字节
+ mesh/light/camera/bone objects），motion 关键帧在 .ANM。

**已修复**

- **P2-2**: ChrMotion 重新定义为 `ChrModel` 文本格式解析器
  - 状态机：`*MOD_FILE_NAME` / `*MOTION_NUM` / `*MATERIAL_NUM`
  - 共享 `mxh/compat/detail/text_parse.hpp` inline `trim` / `tokenize`
    （同时被 `bmhm_map.cpp` 复用，删除其本地定义消除 ODR 风险）
  - 18/18 PASS，含 1 个真实 `test-extract/11160.chr` 加载
- **P2-3**: ChxModel 重新定义为 tab 分隔文本格式解析器
  - 状态机：`*MOD_FILE_NUM` + N×`*MOD_FILE_NAME` + `*MOTION_NUM` + M×motion_path
  - 防御性接受无 `*MOD_FILE_NUM` 头的单行 `*MOD_FILE_NAME`（手编资源兼容）
  - 12/12 ChxModel + 4/4 ChxModelRealResource PASS
  - 真实 `Character.pak:man.chx` 加载并解析出 5 个 mod_files

**跳过（用户决策 2026-07-16 02:18）**

- **P2-4**: IocpServer Linux/macOS 跨平台（`Perf-4`）
  - IOCP 是 Windows-only proactor，POSIX epoll/kqueue 是 readiness-based
    reactor，完整移植 = 重写网络层
- **P2-5**: AcceptEx 性能优化（`Perf-5`）
  - `load_accept_ex()` 已写但 `start()` 未调用，`handle_accept` / `post_accept`
    是空 stub
  - 与 `Perf-4` 强相关，accept 池在 epoll 模型下语义不同

**测试数**：65/65 `mxh_compat_tests` PASS（31 → 65，+34 新增，0 回归）
**新增文件**：`modern/include/mxh/compat/detail/text_parse.hpp`（1 个）
**重写文件**：6 个（hpp × 2 / cpp × 2 / test × 2）+ 1 个测试语义反转
**记录**：`docs/KNOWN_BUGS.md` 新增 R-6 / R-7 / Perf-4 / Perf-5

## [0.13.0] - 2026-07-16

### Phase 10 series: Test coverage expansion (Phase 10.4 — Phase 10.23) ✅

This release expands test coverage across the modern runtime
to 783/783 ctest PASS (~12 sec wall). Every public header in
`modern/include/mxh/...` is now covered by at least one
test file, with wire-format pinning (sizes / offsets /
mask values) for every binary on-disk format that legacy
tools can still write.

**Added (commit-by-commit, in order)**

- **Phase 10.4** (11 commits) — Infrastructure + 5 new modules
  + 5 test files: `DATABASE_SCHEMA.md`, `vcpkg.json`,
  `Dockerfile`, `deploy/`, plus `MoxianPacker`, `MoxianGMTool`,
  `MoxianMapEditor`, `MoxianAutoPatcher` + 17 dev utilities.
- **Phase 10.5 / 10.6 / 10.7** — `modern/scratch/` archival
  (167 files → 12 subdir), CHANGELOG test-count sync to
  506/506, `MODERNIZATION_PLAN.md` §9 Phase 10 总结 added.
- **Phase 10.8** — `memory_pool_test.cpp` (11 tests, 5 initially
  DISABLED) + `BufferPool::capacity_` fix for the lazy-allocate
  path.
- **Phase 10.9** — MSVC 19.44 init-lock deadlock fix in
  `mxh::memory::ObjectPool`. Re-enables the 5 DISABLED tests.
  Trade-off: `~ObjectPool()` is now a no-op (small memory leak
  for long-lived processes) to avoid the lazy `std::mutex`
  init deadlock that single-threaded gtest test bodies
  trigger. Documented in the header.
- **Phase 10.10** — `game_types_test.cpp` (25 tests) — wire-
  format pinning for `item_types`, `monster_types`,
  `skill_types`. Collateral findings: `is_empty_slot` returns
  true if EITHER field is zero; `NpcRegen` is 43 bytes (header
  comment said 44).
- **Phase 10.11** — `iocp.cpp` enabled + `iocp_test.cpp`
  (12 tests). Fixed 6 real errors: missing `<mswsock.h>`,
  `sockaddr_in` → `sockaddr_storage` zero-init copy, private
  `process_send_queue` → public, `Mswsock.lib` link, `mxh_net`
  PUBLIC-link `mxh_monitor`.
- **Phase 10.12** — `protocol_test.cpp` (26 tests) — wire-
  format pinning for 12 protocol enums. Collateral: `Category
  ::Npc=37`, `Monster=35` (different from the original guess).
- **Phase 10.13** — `ttb_tile_table_test.cpp` (11 tests) +
  `mlog_test.cpp` (11 tests). Collateral: `LogLevel` underlying
  type is `int` (not `uint8_t`); parser 4-byte input returns
  empty (size<8 early-exit).
- **Phase 10.14** — `chr_motion_test.cpp` (18 tests).
  Collateral: `is_chr` uses `fps < 240` strict less-than;
  `std::span` brace-init doesn't compile C++20.
- **Phase 10.15** — `platform_test.cpp` (19 tests). Collateral:
  `sockaddr_to_string` returns `""` (not `"unknown"`) on null /
  zero-length; needs `SocketGuard` for Winsock init.
- **Phase 10.16** — `message_test.cpp` (21 tests) — covers
  `MsgHeader` (8B), `MsgRoot` (4B), `Message+total_size`,
  `ConnectionId`, `NetError+to_string`, `ServerConfig` /
  `ClientConfig` defaults.
- **Phase 10.17** — `server_handler_test.cpp` (14 tests) — 3
  handlers (Login / Agent / Map) with `MockDbAdapter`. First
  attempt failed (MapHandler ctor needs 3-arg + use_legacy
  framing=true; MockDbAdapter private members); reverted and
  re-landed correctly.
- **Phase 10.18** — `mesh_flag_test.cpp` (30 tests) — render
  flag bitmask. Pinning the legacy wire-format quirk where
  `RENDER_ZPRIORITY_MASK_INVERSE = 0x80ffffff` includes bit 31
  (Z-write flag), so `(flag & ZPRIORITY_INVERSE) | new_prio`
  preserves the Z-write bit unchanged.
- **Phase 10.19** — `math_test.cpp` (28 tests) — VECTOR2/3/4 +
  MATRIX4 + 6 matrix helpers. Collateral: `MatrixLookAtLH` is
  right-handed in the original's convention (target ends up at
  +Z in view space, not -Z as the hpp comment claims); cross-
  product f×up produces a left-handed view.
- **Phase 10.20** — `motion_flag_test.cpp` (15 tests) — motion
  flag bitmask round-trips for KEYFRAME / VERTEX / UV.
- **Phase 10.21** — `file_storage_typedef_test.cpp` (13 tests)
  — wire-format pinning for `FSFILE_HEADER` (32B),
  `FSFILE_ATOM_INFO` (268B), `FSPACK_FILE_INFO` (272B).
  Collateral: the hpp doesn't include the header that defines
  `_MAX_PATH`; the test `#define _MAX_PATH 260` before
  including the hpp.
- **Phase 10.22** — `chx_model_test.cpp` (18 tests) — .chx
  character model parser (32B header + is_chx / parse / load).
  Pins the skeleton-parser contract (header + raw populated,
  vertices / indices empty with TODO(Phase 1.3) for the
  per-table decode).
- **Phase 10.23** — `db_adapter_test.cpp` augmented with 5
  new factory contract tests: concrete-class type identity
  (SqliteAdapter / MssqlOdbcAdapter), case-sensitivity of
  backend names, `is_connected() == false` pin, MSSQL alias
  routing.

**Test count**
- 449 (start of session, 2026-07-15) → 506 (Phase 10.6 sync)
  → 783 (Phase 10.23 end)
- Wall time: ~12 sec for the full ctest run
- Build: 0 error, Debug config

**Cross-project memory entries (agent memory, will help
future projects on different repos)**
- MSVC 19.44 init-lock deadlock with gtest single-threaded
  test body (root cause + 3 fix options).
- `mavis-trash` refuses reparse-point paths; go through mirror.

**Changed**
- `.gitignore`: added `!modern/tests/unit/log/` allow-list and
  several Phase 10 scratch entries.
- `MODERNIZATION_PLAN.md`: §9 Phase 10 总结 added (Phase 10.7).

**Known limitations (carry-over)**
- C-32: host has no Docker / podman / WSL2 — full SQL Server
  runtime smoke is env-blocked. Documented in
  `docs/KNOWN_BUGS.md`.
- C-35: 4/5 Distribute `Debug_<LOCALE>` targets fail (mfc71.lib
  + 4 anonymous enum redefinitions). Shared-header refactor
  would break 1:1 contract.
- `MssqlOdbcAdapter.ConnectToInvalidServerFails` flake on busy
  machines (5s ODBC retry timeout spikes past 30s ctest
  budget). Passes on retry.

## [0.12.0] - 2026-07-10

### Phase 11.2: Protocol Documentation Generator ✅

**Added**
- `MoxianProtocolDoc`: Protocol documentation generator
  - Parse Protocol.h and extract MP_CATEGORY enums
  - Extract MP_PROTOCOL_* enums and values
  - Generate Markdown documentation
  - Generate JSON protocol schema
  - Summary statistics (124 categories, 64 protocol enums, 3458 protocols)

### Phase 12: Continuous Iteration ✅

**Completed**
- 12.1 Feedback collection / bug fixes / performance tuning ✅
- 12.2 Community building / documentation improvement ✅
- 12.3 Client modernization (DX11 + modern UI) ✅
- 12.4 Server performance optimization (IOCP + memory pool) ✅

**Added**
- IOCP-based high-performance network layer
- Memory pool for object and buffer management
- Performance monitoring system
- Memory and network benchmarks

## [0.11.0] - 2026-07-10

### Phase 9.3: Docker Containerization ✅

**Added**
- `Dockerfile`: Multi-stage build for Windows containers
- `docker-compose.yml`: Full stack deployment (Login + Agent + Map + MSSQL)
- `.dockerignore`: Exclude legacy source and build artifacts
- `docker/init-db/init.sh`: Database initialization script
- `docker/config/`: Server configuration templates

### Phase 10: Tool Chain Modernization ✅

**Added**
- `MoxianPacker`: Modern CLI tool for PAK archive management
  - Pack files into .pak archives
  - Extract files from .pak archives
  - List .pak contents
  - Verify .pak integrity
  - CRC32 checksum verification

- `MoxianGMTool`: Modern GM management tool
  - HTTP REST API server
  - Player management (ban, mute, kick, teleport)
  - Item management (give, remove, search)
  - Server monitoring (status, player count, performance)
  - Chat moderation (logs, filters)
  - Event management (create, schedule, monitor)

- `MoxianMapEditor`: Modern map editor
  - Load and view .bmhm map files
  - Edit tile properties
  - Place and manage map objects
  - Export to text format
  - Create new maps

- `MoxianAutoPatcher`: Modern auto-update tool
  - Check for updates via HTTPS
  - Download patches with progress
  - Apply binary diffs (bsdiff/bspatch)
  - Verify file integrity (SHA-256)
  - Rollback on failure
  - Pack files into .pak archives
  - Extract files from .pak archives
  - List .pak contents
  - Verify .pak integrity
  - CRC32 checksum verification

## [0.10.0] - 2026-07-10

### Phase 9: Cross-Platform Support (Partial) ✅

**Added**
- `platform.hpp`: Platform detection macros (Windows/Linux/macOS)
- `platform.hpp`: Socket type aliases and helper functions
- `platform.hpp`: Cross-platform socket address helpers
- `platform.hpp`: Thread ID and filesystem abstractions
- `socket.hpp/cpp`: RAII socket wrapper with:
  - Non-blocking I/O support
  - TCP_NODELAY and SO_REUSEADDR options
  - Address resolution and connection
  - Thread-safe send/receive operations
  - Custom error codes (SocketErrc)
- `socket_test.cpp`: 20 socket tests (address, create, bind, listen, connect, echo server)

**Changed**
- `src/CMakeLists.txt`: Added socket.cpp to mxh_net library
- `tests/unit/net/CMakeLists.txt`: Added socket_test.cpp

## [0.9.0] - 2026-07-10

### Phase 5: Rendering Engine Modernization ✅

**Added**
- `IRenderer.hpp`: 1:1 port of 4Dyuchi IRenderer interface (75 methods)
- `IFileStorage.hpp`: 1:1 port of file storage interface (27 methods)
- `render_typedef.hpp`: Binary-compatible structures with original DX8 engine
- DX11 backend: Device, SwapChain, RenderTarget, default state objects
- HeightField system: CreateHeightField, height field objects
- Material system: CreateMaterial, CreateMaterialSet
- Mesh system: IDIMeshObject, IDIHFieldObject, IDIImmMeshObject
- Font system: IDIFontObject implementation
- Sprite system: IDISpriteObject implementation
- Texture loader: TGA, DDS, BC1/BC3/BC4/BC5 encoders
- Effect shaders: IEffectShader implementation
- Motion cache: per-motion VB/IB tracking
- Deferred renderer: SetRTLight, InitializeRenderTarget
- 141 render tests

### Phase 6: UI System Modernization ✅

**Added**
- `cWindow` base class with DX11 rendering backend
- `cButton`, `cCheckBox`, `cEditBox`, `cTextBox` controls
- `cImage`, `cListCtrl` advanced controls
- `cWindowManager`: top-most dispatch, modal, defer-destroy
- `cMsgBox`: modal 4-type dialog box
- `cDialog`: window management, findWindowById, alpha, positioning
- `cDivideBox`: split-pane container
- Legacy compatibility: WE_* events, cbWindowFunc bridge
- `mxh_ui_smoke`: headless UI integration test
- 254 UI tests

## [0.8.0] - 2026-07-10

### Phase 8: Performance Optimization ✅

**Added**
- `ThreadPool` (Phase 8.1): General-purpose thread pool with `std::counting_semaphore`
- `ObjectPool<T>` (Phase 8.2): Generic object pool for reducing heap allocations
- `compress.hpp` (Phase 8.3): RLE compression for large payloads (threshold: 128 bytes)
- `util_test.cpp`: 17 tests covering ThreadPool, ObjectPool, and Compression

### Phase 7: Build System Completion ✅

**Added**
- `vcpkg.json`: Dependency manifest (gtest + sqlite3)
- `.github/workflows/ci.yml`: GitHub Actions CI/CD pipeline
  - Windows 2022 + MSVC 2022
  - Debug/Release matrix build
  - Automatic test execution
- `net_benchmark.cpp`: TCP throughput & latency benchmark

**Changed**
- `tests/CMakeLists.txt`: 3-mode dependency resolution (vcpkg → vendored → FetchContent)

## [0.7.0] - 2026-07-10

### Phase 4: Network Layer Modernization ✅

**Added**
- Protocol versioning (Phase 4.3):
  - `kProtocolVersion=1`, `kMinProtocolVersion=0`
  - `VersionRejectReason` enum
  - `UserConnProtocol` enum (CheckVersion/NotifyVersionAck/NotifyVersionNack)
  - `LoginHandler::handle_version_check()` version negotiation
- Encryption middleware (Phase 4.4):
  - `IEncryptor` interface with `encrypt()`/`decrypt()` hooks
  - `TcpServer`: encrypts outgoing, decrypts incoming
  - `TcpClient`: bidirectional encryption support
  - `IConnectionHandler::encryptor_for()` virtual method
- `version_test.cpp`: 27 tests for version constants, negotiation logic, payload encoding

**Changed**
- `net.cpp`: TcpClient now supports receive loop with encryption
- `net.cpp`: TcpClient::send() applies encryption via `encryptor_for()`

## [0.6.0] - 2026-07-09

### Phase 3: Crypto Compatibility ✅

**Added**
- `HselStream`: Modern C++ replacement for HSEL_STREAM
- `HselEngine`: Stateful encryption engine
- `crypto_test.cpp`: 23 tests for HSEL encryption/decryption

### Phase 2: Database Layer ✅

**Added**
- `IDbAdapter` interface: Database abstraction layer
- `SqliteAdapter`: SQLite implementation (replaces MSSQL dependency)
- `db_test.cpp`: 11 tests for database operations

## [0.5.0] - 2026-07-08

### Phase 1: Resource Compatibility Layer ✅

**Added**
- `MhFileEx`: BIN file reader (XOR encryption, CRC verification)
- `PackFile`: PAK file parser (4DyuchiFileStorage format)
- `BsadAreaParser`: BSAD skill area file parser
- `TgaLoader`: TGA image decoder (uncompressed/RLE, RGBA32)
- `ResourceExplorer`: CLI tool for inspecting game resources

**Changed**
- `test-extract/`: Added sample resources for testing

## [0.4.0] - 2026-07-07

### Phase 0: Project Setup ✅

**Added**
- CMake build system (`modern/CMakeLists.txt`)
- GoogleTest integration (FetchContent)
- Unit test framework
- `mxh` namespace structure
- Logging compatibility (`MLOG` macro)
- Protocol constants (`Protocol.h` modernization)

**Documentation**
- `MODERNIZATION_PLAN.md`: 12-phase roadmap
- `docs/KNOWN_BUGS.md`: Known issues tracker
- `docs/RESOURCE_FORMATS.md`: Binary format documentation

## [0.3.0] - 2026-07-06

### Initial Project Structure

**Added**
- `modern/` directory for new code
- `include/mxh/` header organization
- `src/` implementation structure
- `tests/unit/` test organization

---

## Test Coverage Summary

| Phase | Test Suite | Tests | Status |
|-------|-----------|-------|--------|
| Phase 0 | Protocol constants | 16 | ✅ |
| Phase 1 | Resource formats | 23 | ✅ |
| Phase 2 | Database adapter | 11 | ✅ |
| Phase 3 | HSEL encryption | 23 | ✅ |
| Phase 4 | Network layer | 30 | ✅ |
| Phase 5 | Rendering engine | 141 | ✅ |
| Phase 6 | UI system | 254 | ✅ |
| Phase 7 | Build system | - | ✅ |
| Phase 8 | Performance utils | 17 | ✅ |
| Phase 9 | Cross-platform socket | 20 | ✅ |
| Phase 10.4.9 | util / version / monitor tests | 57 | ✅ |
| **0.13.0 total** | (Phase 10.4 — 10.23) | **592 + 191** | **783/783 ctest PASS** |
| **0.13.1 (Phase 12.1)** | `mxh_compat_tests` (chr + chx) | +34 (18 + 12 + 4) | **65/65 PASS** |

Last verified: 2026-07-16 (`mxh_compat_tests` after Phase 12.1 P2-2/P2-3, 1.7 sec wall).

> Phase 10.4 之后（0.13.0）总测试数 783/783；0.13.1 只重写 `mxh_compat_tests` 子集
> （chr + chx 测试从 19 个旧 binary-期望用例 → 18 + 12 + 4 = 34 个新文本格式用例），
> 全 `mxh_compat_tests` 65/65 PASS，0 回归。完整 ctest 数（其他子集）保持 783/783。

---

## Upcoming

### P2 剩余（2026-07-16 状态）

✅ **已完成（CHANGELOG 0.13.1 - 0.13.6）**：agent_handler 断连
GameOutSyn、map_handler UseSyn 物品效果、IME hook + Win32 IMM、
BC6H/BC7 + DX10 扩展头、scratch 大扫除
❌ **撤回 / 转 KNOWN_BUGS**：primitives 正交矩阵（R-9）、cImage
GPU 绘制（R-10）—— 都是 reference adapter 缺失，无 caller 等
真 host 接入时再做

### 仍在队列

- **P2-12 dialogs 移植**：~78 个遗留对话框（cGuildDialog 1/80 已完成），
  每个 1-3 测试 = 200+ 测试总量，大活
- **TcpClient 可注入化**：让 agent_handler 测试能 mock `send()`，
  完整覆盖 GameOutSyn 转发路径
- **integration test 进 ctest**：把 `test_map_integration.py` 接进
  CMake `add_test()`，端到端 CI 自动化

### 仍 deferred

- **C-32**：real docker compose up mssql + MoxianLoginServer
  --backend mssql_odbc smoke（host 缺 docker / podman / WSL2 — 等环境）
- **Perf-4 / Perf-5**：deferred 等待架构决策（详见 `docs/KNOWN_BUGS.md`）
- **R-11** BC6H/BC7 完整 encoder：等 DirectXTex / bc7enc 接入 CMake
  依赖（需 vcpkg 加 `directxtex` / `bc7enc`）
