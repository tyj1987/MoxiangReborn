# Moxiang 开发路线图

更新：2026-10-15。当前原版候选绑定 `codex/reorg-original-20261009`，提交
`b9a52b2784fb63784916972c02b43c54bccfb321`；GitLab 为工程主源，GitHub 为镜像。
Unity 客户端保持独立工作树和独立构建/运行数据，不从本页的原版资源结论借用完成声明。

## 当前已闭合

- 独立 Visual Studio Debug 构建通过。
- 完整 CTest：12,577/12,577 通过，14 项因显式 PVE/MSSQL/历史资源前置条件跳过；见
  `docs/evidence/original-clean-build-ctest-20261015.json`。
- VM112 隔离 MSSQL Map17 事件序列已完成登录、建角、选角、GameIn、216 怪物、15 NPC、技能命中、自然掉落、点击拾取、背包移动和重登回读；测试行已清理，见
  `docs/evidence/original-pve-map17-combat-current-20261015.json`。

## 仍未闭合

- 当前源码尚未重新部署为本轮 PVE 三服版本；现有 Map17 事件证据使用祖先 stage，不替代当前提交部署验收。
- 原版准确资源仍缺 23 个 HFL、18 个 TTB、Map203 的 161 个纹理依赖，以及 Map17 使用中的 9 个地表纹理。已检查的 Map.pak 候选没有准确条目；不得用替代资源宣称 1:1。
- 真人 GUI 视觉/音频、全地图语义对照、断线重连、服务重启、数据库失败恢复、回滚和 24 小时稳定性尚未完成。
- 发布门禁保持关闭；历史通过记录只用于定位，不作为当前提交的验收。

## 下一条最快路径

1. 以当前提交重新打包并绑定 VM112 原版三服和独立 MSSQL，复跑 Map17 全链并捕获可靠进程退出码。
2. 在真实 GUI 中完成移动、NPC/任务、战斗、掉落、拾取、装备、切图、重登和数据库读回。
3. 对每个缺失资源继续寻找准确原包；没有可验证来源时维持阻塞，不引入替代美术。
4. 通过稳定性、更新回滚和双客户端验收后，才评估 `main` 合并和最终冗余清理。

## 证据入口

- 资源准入：`docs/evidence/original-resource-admission-20261015.json`
- 资源来源复核：`docs/evidence/original-map17-texture-source-search-20261015.json`
- PVE 事件序列：`docs/evidence/original-pve-map17-combat-current-20261015.json`
- 干净构建与测试：`docs/evidence/original-clean-build-ctest-20261015.json`
- 协议、数据库和资源格式说明仍以 `MoxianProtocolDoc.md`、`DATABASE_SCHEMA.md`、`RESOURCE_FORMATS.md` 为准。
