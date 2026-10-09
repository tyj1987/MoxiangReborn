# Unity 重制状态

更新：2026-10-09。本页只记录 Unity 分线的当前状态与持续有效约束。当前 Unity 候选仍未通过首个可玩验收；旧阶段记录和测试结果已归档，见 [历史证据索引](HISTORICAL_EVIDENCE_INDEX.md)。总体验收等级见 [矩阵](VERIFICATION_MATRIX.md)，本轮执行事实见 [重整状态](REORGANIZATION_STATUS.md)。

## 当前候选与验证

候选工作树：`unity-integration`，基于 `b038`。本轮证据：

- 资源隔离阶段的原生 x64 宿主 CTest：110/110 通过。后续网络修改尚不继承该全套结果；当前定向测试和间歇异常见下文。
- 空 Library 编译及项目资产导入：通过。日志中两条资源错误来自故意损坏的负面测试，不是项目输入缺失。
- Unity EditMode：191 PASS、0 FAIL、13 SKIP。跳过项包括无图形设备的预览测试，不计通过。
- 历史资源隔离后的 Windows x64 Development Player 构建通过：354 文件、493,897,281 字节，EXE SHA256 为 `718444f6a718fa668da96eb0a58445b009673310e6129152a63dbf7a92b8de00`；仅包含 ConnectionValidation 和 Map44_Wuguan 两场景；该旧构建未运行，不与当前GUI证据混用。
- 冷导入、canonical、独立 overlays、完整 Effekseer、rootless CI 与 PVE 入图修复已分组提交。源码验证节点 `ad0e096b` 已在私有 GitLab 读回；241份派生文件迁出原资源，4份准确HFL采用独立recovered-map来源。当前资源基线19496逻辑项、19504物理来源、0冲突、8同字节副本、0未决选择；来源验证5135/0通过，仍有45个发布缺口（27 HFL、18 TTB）。
- `ad0e096b` 的 GitLab CI 748 已通过治理/Python、Effekseer精确279项、canonical精确5131项及复合来源5135/0；此前网络/工具节点866的CI747也通过。CI不包含Windows C++/Unity Editor构建；本机证据须单独绑定。
- PVE网络v5是历史基线：源码`0a75fa4b`/`904b3b44`，包SHA `33f61c72333e10e2dbdb6513ad6b58a0cc207c9930fd358eb6bd6a03208b8473`、manifest `85c1931a6f11c1a5bd5cc48377ecd7d86515e72dade92955a9b8353a6810a268`；v5测得Syn至末怪302/188ms。当前状态以v6b为准。
- 当前v6b两轮协议回归在严格2秒门槛内均收到LoginAck、选角、GameInAck及228/228怪物；GameInSyn至末条怪物分别336/190ms，GameInAck至末条分别96/67ms，正常发出GameOutSyn并完成重登。已有fixture导致建角跳过，不证明新建、战斗或拾取。
- `84d3a0c5`在904生命周期修复上追加连接初始化/断开通知排序，保留Unity两项cipher测试；独立审查和受影响目标重建通过，纯网络52/52、新增4/4及wire mock20/20。发送无进展超时10秒，同步handler仍可能延长stop，POSIX未验收。旧间歇SEH的历史根因仍未得到证明。
- v6b runtime包封装源码`69f98655`、服务器源码`84d3a0c5`，ZIP SHA `adb2d5167c78db703f7b26f67dfcdadd3d4cf30731d52d5d61daed78b68ae14f`，stage manifest SHA `1d7e63779e577f69eeecc094781079118317daa9dddebd00bf61f6a0c35d0380`；已在PVE部署，安装后11文件精确哈希、PlanOnly、三服进程/端口及原版连续性检查通过。CT930只读回查到既有角色`100000/UnityTrail01`。DBTool只读查询确认schemaVersion 1及writer contract，无本轮迁移。c0c7客户端入口要求显式远端主机和分线端口、默认计划模式；该历史Player已由当前独立构建替代，见下方本轮记录。
- 本轮PC误跑含SQLite的处理器194项，记录在`unity-shared-net-v6-host-test-deviation.md`，不计PVE数据库验收。客户端首次338通过、5个TEMP权限失败、1跳过；仅五个无DB配置用例在独立TEMP重跑5/5。`53e1ccad`将28处测试fixture引用统一为`MXH_TEST_FIXTURE_ROOT`，独立PVE测试包已构建。handler v3 runner因PowerShell `ExitCode`为空而失败，正常WinRM管理恢复后取回stdout：195项运行，193通过、2失败，另3项DISABLED未运行；失败涉及位置持久化和跨地图入口。PID3092历史查询仍无可信终态，QGA现为Stop Pending，已停止进一步操作。此段是v3历史失败；当前v6复跑结果见下方。

两项失败已定位为测试资源路径/MapChange缺包并修复；当前继续验证Managed实体呈现、移动、战斗、自然掉落、点击拾取及持久化。v6b部署、协议日志及只读数据库证据见本轮`unity-candidate-review/unity-v6b-runtime-20261009.md`及其引用的回执。旧冷导入/Player报告不覆盖后续源码。

- `a5bc6947` 的完整x64 clean-first构建1733/1733通过；dump读取器按目标架构解析、DIA选择匹配库。`86644985` 后续修复x86目录计数边界，重建受影响工具，合成样本9/9通过；没有重复宣称完整clean构建。
- Foundation在 `ad0e096b` 独立吸收：13个工具/测试、8个可编辑输入及原manifest快照，当时22/22 Git blobs与来源相同，37项通过/2项Windows能力跳过；生成器可复现7组35产物。后续 `7eda291d` 将独立单测接入现有GitLab作业，更新工具说明并从活动目录删除旧ci_check；其原字节保留在Git历史，不再以旧数量描述最新目录。源位于独立ProceduralStudies路径，原武馆/刀剑枪未覆盖；许可/权利来源仍待核，未进入Player。

## 当前 Player 与 PVE 复跑（05:40 UTC 快照）

- 当前代码节点d9e3d73f；Player构建入口42fe79dd、native源码39836319，DLL SHA `41c48df7e58fa0028ee8a80b1ea5a4c99728d6407ec6e19c3214b3846ab2218d`。354文件、493901073字节；5837项固定输入构建前后字节不变，canonical5131项精确复核。证据`unity-candidate-review/native-client-refresh/player-39836319-build-receipt.json`。
- v6 handler在PVE独立目录实际195/195通过、exit0，另3项DISABLED未运行；前后六服进程身份和两部署manifest一致。旧193/2失败由缺失MapChange及编译机绝对路径导致，原始失败仍保留。此结果不证明业务DB全链或GUI战斗。
- 当前Player真实界面完成登录、既有角色选择、Map10地形显示与正常原地复活；界面生命从0变为37/125。视角仍是全图概览，本地角色和怪物不可辨认，不能以协议228实体计数替代显示验收。开始添加不含认证数据的开发呈现计数，以区分事件、资源映射、位置与相机问题。
- URP未用后处理变体被构建剥离，导致Gaussian/Bokeh DepthOfField、Panini启动警告；资源GUID存在且效果配置关闭，目前无证据表明其阻止入图，不为消警告扩大渲染修改。
- GUI由root独占操作，05:38 UTC按PID/启动时间/文件哈希正常关闭Player，退出码0。首次输入焦点错误截图已私有隔离并排除共享导出，后续输入加入焦点等待；证据只引用无秘密截图及排除metadata。
- Unity服务仍为v6b，客户端新构建不代表服务端已升级。VM112由original owner通过正常WinRM独占管理；QGA仍Stop Pending，不继续终止或重启。接下来的呈现诊断源码尚不计验收。

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
