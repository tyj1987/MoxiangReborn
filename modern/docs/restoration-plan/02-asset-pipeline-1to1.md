# M-R3 设计 + 165 dialog .bin 索引

> 状态：设计阶段 (M-R1 ✅ 已 commit 02890ce7，M-R2/M-R3 准备)
> 配套：[01-rendering-ui-1to1.md](./01-rendering-ui-1to1.md) M-R3 详细计划

---

## 1. M-R3 范围（接口装载链）

**目标**：MoxianClient main.cpp 启动时调 `parse_interface_script` + `apply_legacy_layout` 装载 `InterfaceScript/*.bin` 列表，把每个 dialog 挂到 cWindowManager。

文件：
- 改 `modern/tools/MoxianClient/main.cpp`：
  - 启动时 enum `PlayDH/Image/InterfaceScript/*.bin` 列表
  - 调 cResourceManager::getInstance().InitScriptManager(playdh/Image) (M-R1 已做)
  - 每个 .bin 调 parse_interface_script + apply_legacy_layout
  - dialog 存到 g_dialogs[filename]
- 写 `modern/tools/MoxianClient/dialog_loader.hpp/cpp`：封装 165 .bin 装载链
- 改 `modern/src/ui/CMakeLists.txt` / `modern/tools/MoxianClient/CMakeLists.txt`

go/no-go：
- 启动时 165 .bin 全部 parse_interface_script 成功 (0 exception, 0 null dialog)
- 165 dialog 树挂到 cWindowManager 完成
- 截 5 状态截图能看到真 dialog (不再是黑屏)

---

## 2. 165 dialog .bin 索引（基线 = 132，老版 = 199）

### 2.1 modern/data/PlayDH/Image/InterfaceScript/ 基线（132 个）

modern 部署包含的 132 个 dialog .bin，**M-R3 至少要装这 132 个**。

```
10.bin 11.bin 14.bin 15.bin 17.bin 19.bin 21.bin 22.bin 23.bin
24.bin 25.bin 27.bin 28.bin 29.bin 30.bin 31.bin 32.bin 33.bin
34.bin 51.bin 55.bin 56.bin 57.bin 58.bin 60.bin

AllyNote AutoAnswerDlg AutoNoteDlg bail barInfo
BigMap Changejob ChangeName Channel CharChange CharMakeNewDlg
CharSelectDlg Chase ChNameChange CH_ALLWORLD CH_ALLWORLDCreate
copyright CostumeSkinSelectDlg DivideBox Engrave EventMapCount
FortWarEngraveDlg FortWarTimeDlg FortWarWareHouseDlg Friend
GDTCANCEL GDTENTRY GDTournament GDViewTournament GFWarDeclare
GFWarInfo GFWarResult gmopentalk Guild GuildCreate GuildInvite
GuildLevelUp GuildMark GuildNickName GuildNote GuildNotice
GuildPlusTime GuildRank GuildTrainee GuildTraineeInfo GuildUnionCreate
GuildWarehouse GuildWarInfo Helper Help_Script IDDlg InitDlg
IntroReplay ItemLock Itemmall ItemSearch ItemShop JoinOption
KeySetting looting MiniFriend MiniNote MPGuage MPMission MPNotice
MPRegist NewLoadDlg Note Npc_Script NumberPad OPChNameChange
PartyBtnDialog PartyCreate PartyInvite PartyMatchingDlg
partymember1dlg partymember2dlg partymember3dlg partymember4dlg
partymember5dlg partymember6dlg PartyWarDlg PetInven PetRevival
PetState PetStateMini PetUpgrade Pyoguk QuestTotal RareCreate
ReinforceDefault Revive RFDataGuide SaveMove ScreenShotDlg
SearchChr SEEChase ServerListDlg Skillpointagain Skillpointop
SkillPointReset SkillTrans SkinSelectDlg SOScall StallOption
SurvivalDlg SWCount SWInfo SWProfit SWProtectReg SWStartTime
SWTimeReg TipBrowser TitanBreak Titanmix TitanmixProgressBarDlg
TitanPartsChange TitanPartsChangePreview TitanPartsProgressBarDlg
TitanUpgrade Titan_Bongin Titan_guage Titan_inventory Titan_recall
Titan_Repair Titan_use TransDefault UniqueItemCurseCancellation
UniqueItemMixDlg UniqueItemProgressBarDlg WantNpc WantRegist
```

### 2.2 老版专属 dialog（modern 没有，67 个）

`墨香【客户端+服务端+工具】/Image/InterfaceScript/` 才有：

```
71.bin ~ 87.bin (17 个数字 .bin，老版独有)
AttRedist.bin
ch_allworldcreate2.bin
ExpressionDlg.bin
Farm.bin
FarmAnimalCageDlg.bin
FarmManage.bin
Farm_Get.bin
Farm_Upgrade.bin
fw1warinfo.bin fw2warinfo.bin fwcount1.bin fwcount2.bin
fwtimereg1.bin fwtimereg2.bin
HangariUseDlg.bin
influence_info.bin
InsDGEntranceInfoDlg.bin InsDGInfoDlg.bin InsDGMissionInfoDlg.bin
InsDGPartyMakeDlg.bin InsDGRankDlg.bin
itemsocketdlg.bin ItemSocket.bin
MallItem.bin
moraleinfodialog.bin
occupationengravedlg.bin occupationtimedlg.bin occupationwarfareenterdlg.bin
PriceSetting.bin
ProgressDialog.bin
Pyoguk_recall.bin
Rank.bin
surprisewarfareenterdlg.bin
warjobselect.bin
zx_richeng.bin
```

**M-R3+ 二期**：把老版专属 67 个 .bin 复制到 `modern/data/PlayDH/Image/InterfaceScript/`，让 modern 也能装载。

---

## 3. 装载链伪代码

```cpp
// modern/tools/MoxianClient/dialog_loader.hpp
namespace mxh::client {
class DialogLoader {
public:
  // 启动时调
  static std::size_t LoadAllBin(
      const std::filesystem::path& interface_dir,  // <PlayDH>/Image/InterfaceScript
      mxh::ui::cWindowManager& mgr);
  
private:
  // 每个 .bin 一个 dialog 实例
  static std::unique_ptr<mxh::ui::cDialog> BuildDialog(
      const mxh::ui::InterfaceNode& root,
      const std::filesystem::path& bin_path);
};
}
```

## 4. 资源 + 工具

- 老版 InterfaceScript/*.bin = 199 个（modern 132 + 老版专属 67）
- 现代 InterfaceScript parser 已存在（`modern/src/ui/interface_script.cpp`）
- M-R1 cResourceManager 已落地（commit 02890ce7）

## 5. 风险

| 风险 | 应对 |
|---|---|
| 老版 199 .bin 解析覆盖率 | parser 已能解 25/25 单元测试，但只覆盖 7 个基础 dialog。需扩测试 |
| 老版 hard path 加密变种（type 几亿） | 复制 132 .bin 时用 modern 的（type=154 正常） |
| dialog tree 165 个 vs 132 实际 | M-R3 一期只装 132，二期扩 67 |
| 165 dialog 各自 Init 走 cResourceManager 拿 sprite | M-R4 范围 |

## 6. Next Step

按 plan-style 拍板：M-R3 一期 132 dialog 装载（3 天）+ M-R4 165 dialog Init 接真 sprite（5 天）= 8 天。
