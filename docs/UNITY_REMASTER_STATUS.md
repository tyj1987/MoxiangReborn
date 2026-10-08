# Unity 重制状态

更新：2026-10-09。本页只记录 Unity 分线的当前状态与持续有效约束。当前 Unity 候选仍未通过首个可玩验收；旧阶段记录和测试结果已归档，见 [历史证据索引](HISTORICAL_EVIDENCE_INDEX.md)。总体验收等级见 [矩阵](VERIFICATION_MATRIX.md)，本轮执行事实见 [重整状态](REORGANIZATION_STATUS.md)。

## 当前候选与验证

候选工作树：`unity-integration`，基于 `b038`。本轮证据：

- 原生 x64 宿主 CTest：110/110 通过。
- 空 Library 编译及项目资产导入：通过。日志中两条资源错误来自故意损坏的负面测试，不是项目输入缺失。
- Unity EditMode：191 PASS、0 FAIL、13 SKIP。跳过项包括无图形设备的预览测试，不计通过。
- 资源隔离后的 Windows x64 Development Player 构建通过：354 文件、493,897,281 字节，EXE SHA256 为 `718444f6a718fa668da96eb0a58445b009673310e6129152a63dbf7a92b8de00`；仅包含 ConnectionValidation 和 Map44_Wuguan 两场景，尚未运行。
- 冷导入、canonical 源目录、独立 overlays、完整 Effekseer 和 rootless CI 已分组提交，GitLab 已读回 `16c5fe87`。241 份派生文件已迁出原资源目录；4 份准确 HFL 使用独立 recovered-map 来源。最新基线为 19496 个逻辑资源、19504 个物理来源、0 个来源冲突、8 个同字节副本、0 个未决来源选择。源验证 5135/0 通过；45 个发布缺口仍在，包括 27 个 HFL、18 个 TTB。
- GitLab CI 739 的治理/Python 测试通过，Effekseer 精确字节通过。原资源阶段发现 7 个文件的 Git 历史换行归一化，虽本机 canonical 字节正确，远端检出仍不一致；逐文件修复后须重新通过 CI，不能沿用本机结果。
- PVE Unity 数据库尚未迁移；Unity 独立服务实例未部署。没有本轮真实服务器闭环或用户本人验收。

下一步建立独立 PVE 数据库与三服实例，验证登录、选角、Map10、移动、战斗、掉落拾取及持久化，再做视觉对照和人工审查。资源隔离后的冷导入和 Player 构建证据见本轮 audit/unity-candidate-review/postseal-regression-and-vm112-stage-20261009.md；更早的 integration-run 仅作历史定位。

## 持续有效的范围与保护

- 目标环境为 Windows x64 / Unity 6000.6.0f1 / URP 17.6.0 / D3D11。Login、Agent、Map 服务端保持权威；Unity 不得直连数据库。
- `playdh-current` 原始资源及其字节保持只读。`unity-remaster-v1` 仅允许可追溯的视觉派生，不改玩法、数值、碰撞语义、地图拓扑、输入语义、协议或安全签名。不得把未选来源、占位生成物或 loose-first 结果冒充正式资源。
- 原生 DLL 与 C# 固定宽 ABI 必须配套；ClientWire、HSEL、安全接口、三服、DLL 和客户端按同一版本核对与回退。旧文记录的 ABI 版本、结构尺寸及运行 ID 是历史值，必须以当前头文件和 managed assertions 重新核验。
- 资源审计基线和来源选择仍是发布门禁；旧来源计数不继承，每次使用前复核基线及来源哈希。

## `b038` 的独立性与缺项

`b038` 是 Unity 独立候选，不是原版 main 的整体合并单元。按文件复核协议、桥接和服务端差异后再分批吸收。

- `b038` 缺失的 Effekseer 1.80.7 正文已恢复并独立提交，279 个文件与供应清单 SHA 一致，MIT notice 已纳入 Player；CI 3019 已验证全部 279 项原始字节。
- `b038` 缺失的 x86_64 Unity core DLL 已从当前工作树源码构建并配对 ABI `0x00010011`，SHA256 为 `d3e77aa08de8493963e3ddc78bb09584486c72eb2b0906d0f4541acb54f6c1ae`；未复用旧隔离 DLL。
- 现有保全副本中有大量候选未跟踪的派生视觉资源。既有审查记录：`Assets/Moxiang/Derived` 4,346 项、约 831.8 MB 在主仓和已核验备份 manifest 中没有同 SHA；`modern/out/resource-repair` 另有 3,874 项、约 975 MB。需逐项核验源哈希、导出器、许可证和 GUID，并从可重建输入生成；不得当作可清理缓存。
- Unity 隔离树与 legacy 隔离树仍含分叉源码、工具、Prefab/场景、`.meta` 和资产差异。按本机 `E:\Workspaces\moxiang\audit\2026-10-09-reorg\isolated-review\README.md` 的清单逐项保全和评审，不能因 Git receipt 或 Prefab 存在就推断内容完整。

## 通过标准

Unity 编辑器/宿主测试通过只覆盖对应逻辑。整体验收还要求：空 Library 冷导入、无缺依赖的 Editor/Player 构建、受控 PVE 三服和独立数据库、真实登录和核心玩法状态闭环、资源来源可追溯、人物/HUD/地图/动画可辨认、原版对照和人工审查。特效、音频、性能、双客户端、全地图覆盖、签名更新与回滚、故障注入及 24 小时稳定性另行验收。发布门禁失败关闭，不以历史 PASS 替代本轮结果。

