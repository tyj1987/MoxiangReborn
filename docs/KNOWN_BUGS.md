# 活动缺陷与外部阻塞

> 仅记录仍影响当前里程碑的事项。已解决和历史调查见 [KNOWN_BUGS_ARCHIVE.md](KNOWN_BUGS_ARCHIVE.md)，完成记录见 [CHANGELOG.md](CHANGELOG.md)。

## R-9：DX11 完整场景尚未完成原版对照

- **状态**：modern 闭环完成、legacy 视觉待验收；headless 3 帧可自然退出，像素门禁已锁定 grid、cube、checker 纹理与深度遮挡。
- **复现**：运行 `mxh_render_demo --headless --save-frame <path> --frame-count 3`，检查 grid、cube、纹理/HUD 是否同时出帧。
- **影响**：阻塞 Phase A/B 的视觉验收和后续 UI 截图对照。
- **验收**：modern 自动化门禁已完成；剩余要求为与原版登录/空场景截图的差异有明确结论。
- **外部依赖**：需要可运行原版客户端的对照环境。

## CLIENT-RUNTIME：GUI 客户端仍未加载真实场景与完整交互

- **状态**：活动中；GUI 已能使用可配置端口登录 modern LoginServer、转入 AgentServer 并取得角色列表。已修复未安装状态回调和 WM_PAINT 持续占用消息泵导致状态机不运行的问题。
- **复现**：启动 `deploy/scripts/start_modern.ps1` 后运行 `mxh_client.exe --login-port 16001 --map-port 18001`；空账号会停在无可见交互的角色列表阶段，画面仍为程序化占位 sprite。
- **影响**：直接阻塞商业 RC；无头五步 E2E 不能替代玩家可操作客户端。
- **验收**：真实登录/建角/选角/进图 UI 可操作，加载原版地图、角色、怪物、界面、音乐和音效，完成至少一段战斗/任务流程。
- **外部依赖**：无。

## C-Tier-3：九个业务 dialog 尚未完成真实 service 集成

- **状态**：活动中；历史路线将 Quest/Deal 等依赖 Phase B 的业务 dialog 定义为 Tier-3。`cQuestDialog`、`cQuestTotalDialog` 已通过 `IQuestService` 接入领取路径，`cDealDialog` 已通过 `IInventoryService` 校验自有物品，均有行为测试；其余业务 dialog 与 9 项截图验收仍未完成。既有 inventory/skill/player wiring 属于可复用进展，不能替代完整业务验收。
- **复现**：启动真实 inventory/skill/player service 路径，逐项打开 Tier-3 dialog。
- **影响**：阻塞 UI 1:1 集成验收，不能仅用“hpp 已 port”判定完成。
- **验收**：9/9 接线完成，每项至少一个真实服务行为断言并有原版/modern 截图。
- **外部依赖**：无。

## E3：五段核心玩法行为尚未全部 diff=0

- **状态**：活动中；数据面和 side-effect runtime 单测覆盖广泛，但缺完整跨实现证据。
- **复现**：分别运行登录进图、战斗/任务、商城/物品、PK 五个固定场景。
- **影响**：阻塞 T3 和 1.0。
- **验收**：副作用顺序、网络包、数据库变化、数值和 UI 状态逐项一致。
- **外部依赖**：原版客户端/服务端对照环境。

## DEPLOY-MSSQL：生产部署环境尚未验收

- **状态**：本机 modern 路径完成；ODBC Driver 18 已安装，适配器默认优先 18 并仅在 `IM002` 时回退 17。LocalDB 初始化、真实 schema roundtrip，以及客户端/Login/Agent/Map 五步 MSSQL E2E 已通过。
- **复现**：在干净 Windows/SQL Server 环境安装并运行商业冒烟。
- **影响**：不阻塞本机开发，阻塞商业部署声明。
- **验收**：无人值守安装、建库、登录、建角、进图、数据库 roundtrip、升级和回滚全部通过且无 schema 漂移。
- **外部依赖**：一套干净机或等价隔离环境；legacy `.bak` 不属于强制门禁。
