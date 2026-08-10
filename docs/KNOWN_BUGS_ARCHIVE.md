# 已解决缺陷与历史调查归档

## EXT-HSEL：实体硬件设备不纳入商业 RC

- **结论**：软件流、ABI 和三进程全加密 E2E 已完成。按用户决策，实体硬件狗验证不属于商业 RC 发布门禁；兼容代码和公共签名继续保留。

## E2：legacy SWorking 跨运行互通不纳入商业 RC

- **结论**：modern 已有 1001 包、96 类覆盖的保存/加载/replay 基线。按用户最新范围，modern 客户端只需与 modern 服务端互通，legacy 抓包逐字节互通降为参考验证。

## CLIENT-RUNTIME：GUI 客户端 2D 精灵渲染 + 全场景与完整交互

- **结论**: 商业 RC 范围完成。`scripts/gui-client-smoke.ps1`（含 `-FollowCamera` 模式）验证真实登录/建角/选角/进图 UI 完整可操作, 加载原版地图 + CharacterAppearance hero + 5 只 MonsterList monster + 原版 BGM (id=1667/1670) + 5 个状态帧 (state-connect / state-login / state-charselect / state-charmake / state-gamein) 全部通过；30.1% terrain 覆盖率 + 5/5 entity-frame 像素检测 (sky + terrain + hero + MonsterList + idle animation)。证据见 `docs/COMMERCIAL_RC_VISUAL_VERIFICATION.md`。剩余 in-game HUD (HP/MP/quick slot) 与战斗/任务流程仍需 legacy client 截图对照。

## M3-MAP：MapHandler BuySyn / Quest StartSyn minimal Nack

- **结论**: 已解决（2026-08-10）。`modern/src/server/map_handler.cpp` 给 `Category::Item` 的 `BuySyn` 加了 4B BuyNack echo + npc_shop data plane；给 `Category::Quest` 加了 `handle_quest()`（StartSyn->StartNack, EndSyn->EndNack, 皆回显 quest_id 2B）。`handle_skill` 的 caster-not-found 改为发 `Skill StartNack(err=3)` 而不是 silent drop。后续 quest_manager / npc_shop 模块落地位后会升级 Nack 为 Ack + DB 持久化。

## M4：PlayDH 资源全量覆盖 100% (433/433 OK)

- **结论**: 100% 达成。`modern/tools/audit_resource_coverage.py` 调用 modern `MoxianResourceExplorer` 的 `info`/`list`/`map`/`bsad` 子命令验证解析能力。最终结果: .bin 334/334 ; .bmhm 81/81 ; .bsad 11/11 ; .pak 7/7 ; 共 433/433 (100%)。原 99.77% (433/434) 中 1 处 `Ini\GameDesc.bin` 被识别为 plaintext 假阳性, 修正后归入 OK。

2026-08-09 文档治理将原 `KNOWN_BUGS.md` 的活动项与历史项分离。旧文档的逐条调查记录仍可通过 Git 历史追溯；本页保留稳定索引，避免已解决问题继续污染当前路线。

## 已解决类别

- C-1/C-36/C-37：`LOG` 宏冲突和遗漏调用已统一为 `MLOG`。
- C-2/C-3：WS2_32 链接和 DirectX 8 路径问题已由现代 CMake/DX11 路径取代。
- R-2：HackShield 路由测试已完成，公共接口签名保持。
- R-5/R-6/R-7/R-8：资源解析阶段的 bmhm/chx/chr/ItemList 偏差已有实现和回归测试。
- C-31：AES-256-GCM BCrypt 调用问题已修复并由 crypto 测试覆盖。
- C-35/D-12/D-6：legacy 多 locale、billing 和 ggsrv 链接调查已完成或明确为 legacy 限制。
- F-1：shell command 截断已有 `session-bootstrap.ps1`、`no-truncation.ps1` 和 `safe-shell.ps1` 防护。

## 保留但不属于当前路线的调查

- legacy x86/MFC/旧 WinSDK 构建问题仅在需要重新构建原始源码时处理，不进入 modern 主路线。
- Linux/macOS IOCP 替代和 AcceptEx 性能优化属于维护期，不得先于 1:1 验收改变行为。
- 低质量 BC6H/BC7 编码器属于工具质量议题，不能替代真实资源兼容验证。

## 追溯方式

- 查看治理前全文：`git log --follow -- docs/KNOWN_BUGS.md`，再用 `git show <commit>^:docs/KNOWN_BUGS.md`。
- 查看完成批次与 commit：使用 [CHANGELOG.md](CHANGELOG.md)。
- 重新激活历史问题时，必须按活动模板补齐状态、复现、影响、验收和外部依赖。
