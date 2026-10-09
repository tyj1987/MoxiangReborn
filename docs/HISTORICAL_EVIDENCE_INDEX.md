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

旧 AGENTS 和本轮文档/头文件的增量原文另存于私有 PVE `branch-consolidation/main-wip-delta-20261009.zip`，SHA-256 为 `fd0ece85fc7aec0f90d93c2d5314a29514cb71b2633ea2a0068e7c1f21a8e1ef`。16 文件、135202 原字节已独立恢复并逐项核对；包内 `__delta_manifest__.json` 记录路径与哈希，本地证明为 `audit/2026-10-09-reorg/branch-consolidation/main-wip-delta-restore-proof.json`。旧 AGENTS 的过时路径、默认本机数据库和完成声明不再作为活动指令。

后续活动文档和 WIP 的 17 文件、130363 原字节快照已在私有 PVE `branch-consolidation/main-wip-delta-v2-20261009.zip` 独立恢复，SHA-256 为 `0c2868f06c53be3285b9d60e4c59293927fbeecb175755d373d5b4bc5d88fa2c`，证明为同目录 `main-wip-delta-v2-restore-proof.json`。这是绑定原版94d6/CI745及Unity b395/CI744的阶段快照，不覆盖其后运行证据。

2026-10-09 01:32 UTC 的 18 文件、140027 原字节快照已在私有 PVE `branch-consolidation/main-wip-delta-v3-20261009.zip` 独立恢复，ZIP 为68770字节，SHA-256 `06db783177d11a633761f3ea5935aa9574fc8e82ae41fa000f15f2758b155183`。本地证明为 `audit/2026-10-09-reorg/branch-consolidation/main-wip-delta-v3-restore-proof.json`；该快照记录原版r12维护中、Unity v6待部署、本机测试范围偏差及第三批bundle清理后的状态，不能当作后续部署已完成。

旧缺陷清单另保存在私有 PVE `branch-consolidation/KNOWN_BUGS-before-20261009.md`，4199 字节，SHA-256 `89638d1c2c351637a9f5c0959ab9144be88aeca6618e3e69c94a6ae84ac40b47`，已精确读回验证。活动缺陷页不再保留已解决的 SQL 项、已完成的头文件校准或旧的等待审批描述；格式与历史地图线索已提取为当前复验要求。

恢复旧文或 XML 时，先核对 manifest 与 ZIP 的整体/条目哈希，再解到临时审查目录，不要覆盖活动状态文档。历史 PASS 只说明当时对应提交、构建、资源和环境的结果；不得迁移成当前 G0–G11 状态。旧 ABI 版本、结构尺寸、运行 ID、截图和测试汇总均是历史定位信息，复用前须按当前源码、资源和环境重验。
