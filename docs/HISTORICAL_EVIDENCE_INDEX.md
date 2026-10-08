# 历史状态与测试结果恢复索引

本索引取代活动状态文档中的长篇历史运行流水。历史正文、测试 XML 和版本记录继续保存在 Git 历史及私有归档中；本页只保留恢复指针与校验信息。活动结论以 [PLAYABLE_STATUS.md](PLAYABLE_STATUS.md)、[UNITY_REMASTER_STATUS.md](UNITY_REMASTER_STATUS.md)、[VERIFICATION_MATRIX.md](VERIFICATION_MATRIX.md) 和本轮审查记录为准。

## 私有归档

- 私有 PVE 根：`/vmstore/ci-backups/moxiang-reorg/20261009/`
- 归档文件：`superseded-status-and-results.zip`
- ZIP SHA-256：`02f3fb41b9a06cce5b99636d92e17c83a3c2b9d7f6b724afc4d038ab3c101161`
- 本地归档清单：`E:\Workspaces\moxiang\audit\2026-10-09-reorg\superseded-status-and-results.manifest.json`
- 本地读回证明：`E:\Workspaces\moxiang\audit\2026-10-09-reorg\superseded-status-and-results.proof.json`
- PVE 已逐项读取并核对 30 个文件的 SHA-256/大小；恢复前仍应重新验证 ZIP 哈希和目标条目。

## 归档内容

归档含旧版 `docs/PLAYABLE_STATUS.md`、`docs/UNITY_REMASTER_STATUS.md`、`docs/VERIFICATION_MATRIX.md`，`docs/EXECUTION_PLAN_STATUS_2026-09-05.md`、`docs/PHASE_0_GATE65_GATE64_REPORT.md`，以及仓库根目录 25 个 `unity_*.xml` 测试结果，共 30 项。逐文件原路径、字节数和 SHA-256 以 manifest 为准。Git 历史仍保留原文；此归档提供独立于当前工作树的恢复副本。

## 使用规则

恢复旧文或 XML 时，先核对 manifest 与 ZIP 的整体/条目哈希，再解到临时审查目录，不要覆盖活动状态文档。历史 PASS 只说明当时对应提交、构建、资源和环境的结果；不得迁移成当前 G0–G11 状态。旧 ABI 版本、结构尺寸、运行 ID、截图和测试汇总均是历史定位信息，复用前须按当前源码、资源和环境重验。
