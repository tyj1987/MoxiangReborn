# Moxiang 开发路线图

## 当前状态校准（2026-10-17）

当前原版整合线提交为 `0eb971250bf8b71d7be040d1333d3fb30be8fede`，工作树干净，GitLab 分支已同步。最新精确资源门禁为 HFL **22**、TTB **18**、Map17 使用纹理 **9**、Map203 纹理 **161**；新增的未解析 `D:/MX/client/Package/3DData/Map.pak` 通过一次性字节扫描仍未命中 170 个目标文件名，没有任何候选被复制或准入。原版出生地图路由修复已在 `mxh_server_handler_tests` 162/162 和商业 smoke 35/35（MSSQL/GUI 显式跳过）中通过，证据见 `docs/evidence/original-canonical-birth-map-route-20261017.json`。当前 Release/PVE 代码闭环证据不能替代缺失资源、真人 GUI、数据库故障恢复、回滚和 24 小时稳定性验收。

更新：2026-10-15。当前原版候选绑定 `codex/reorg-original-20261009`；工程代码基线为
`179aa2e650f6f754744cfe4cd5e84eb58b8ac4e0`。Release 包内二进制与资源绑定提交为 `3f355242bb337c467f15692598514fa5f4ada591`。GitLab 为工程主源，GitHub 为镜像。
Unity 客户端保持独立工作树和独立构建/运行数据，不从本页的原版资源结论借用完成声明。

## 当前已闭合

- 独立 Visual Studio Debug 构建通过。
- 完整 CTest：12,577/12,577 通过，14 项因显式 PVE/MSSQL/历史资源前置条件跳过；见
  `docs/evidence/original-clean-build-ctest-20261015.json`。
- Release 配置也已完成独立构建并通过同一完整 CTest（12,577/12,577，14 项显式跳过）；见
  `docs/evidence/original-release-build-ctest-20261015.json`。
- 已按当前提交生成不可覆盖的 VM112 Release 包，清单绑定源码、四个资源包、12 个部署文件（含 Map17 增量）和写入契约；包已部署到 PVE 独立 stage，见
  `docs/evidence/original-release-pve-package-20261015.json`。
- 当前 Release 包已在 VM112 独立 stage 完成 5136 文件资源校验，并完成 Map10 真实远端 E2E（LoginAck、建角、重列、选角、GameIn、228 怪物，退出码 0）；见
  `docs/evidence/original-pve-current-release-map10-20261015.json`。
- 当前 Release 包在 VM112 独立 stage 完成 Map17 真实远端战斗闭环：登录、选角、GameIn、216 怪物、15 NPC、权威技能伤害、目标死亡、自然掉落、点击拾取、背包移动和重登回读；临时账号及相关数据库行已清理，见
  `docs/evidence/original-pve-current-release-map17-combat-20261015.json`。测试后已恢复默认 Map10 五端口服务状态。

## 仍未闭合

- 原版准确资源仍缺 22 个 HFL、18 个 TTB、Map203 的 161 个纹理依赖，以及 Map17 使用中的 9 个地表纹理。已检查的 Map.pak 候选没有准确条目；不得用替代资源宣称 1:1。
- 真人 GUI 视觉/音频、全地图语义对照、断线重连、服务重启、数据库失败恢复、回滚和 24 小时稳定性尚未完成。
- 发布门禁保持关闭；历史通过记录只用于定位，不作为当前提交的验收。

## 下一条最快路径

1. 在真实 GUI 中完成移动、NPC/任务、战斗、掉落、拾取、装备、切图、重登和数据库读回。
2. 对每个缺失资源继续寻找准确原包；没有可验证来源时维持阻塞，不引入替代美术。
3. 通过断线重连、服务重启、数据库失败恢复、更新回滚和 24 小时稳定性验收后，才评估 `main` 合并和最终冗余清理。

## 证据入口

- 资源准入：`docs/evidence/original-resource-admission-20261015.json`
- 资源来源复核：`docs/evidence/original-map17-texture-source-search-20261015.json`
- PVE 事件序列：`docs/evidence/original-pve-map17-combat-current-20261015.json`
- 干净构建与测试：`docs/evidence/original-clean-build-ctest-20261015.json`
- 当前 PVE Release 包：`docs/evidence/original-release-pve-package-20261015.json`
- 当前 Release PVE Map10：`docs/evidence/original-pve-current-release-map10-20261015.json`
- 当前 Release PVE Map17 战斗：`docs/evidence/original-pve-current-release-map17-combat-20261015.json`
- 协议、数据库和资源格式说明仍以 `MoxianProtocolDoc.md`、`DATABASE_SCHEMA.md`、`RESOURCE_FORMATS.md` 为准。


