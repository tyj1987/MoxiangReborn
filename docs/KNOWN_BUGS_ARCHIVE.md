# 已解决缺陷与历史调查归档

## EXT-HSEL：实体硬件设备不纳入商业 RC

- **结论**：软件流、ABI 和三进程全加密 E2E 已完成。按用户决策，实体硬件狗验证不属于商业 RC 发布门禁；兼容代码和公共签名继续保留。

## E2：legacy SWorking 跨运行互通不纳入商业 RC

- **结论**：modern 已有 1001 包、96 类覆盖的保存/加载/replay 基线。按用户最新范围，modern 客户端只需与 modern 服务端互通，legacy 抓包逐字节互通降为参考验证。

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
