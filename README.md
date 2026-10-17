# Moxian-Reborn（墨香重生）

优先完成 modern DX11 原版资源线的真实可玩闭环；Unity 重制线独立推进。服务端和数据库均在 PVE 测试，本机负责客户端、编译和无需数据库的单元测试。

**逆向工程项目，仅供学习研究。** 代码 MIT（见 [LICENSE](LICENSE)），原游戏资源版权属原权利方，**不可商用**（见 [NOTICE.md](NOTICE.md)）。Unity 新增公开素材逐包核查许可；这不会改变原资源的权利状态。

## 开发入口

[ROADMAP.md](ROADMAP.md) → [重整状态](docs/REORGANIZATION_STATUS.md) → [原版状态](docs/PLAYABLE_STATUS.md) / [Unity 状态](docs/UNITY_REMASTER_STATUS.md) / [验收矩阵](docs/VERIFICATION_MATRIX.md)。历史 PASS 只用于定位，不能代替当前版本验收。

GitLab 工程主源为 [52trz/moxiang-reborn，项目 12](https://gitlab.52trz.com/52trz/moxiang-reborn)；[GitHub](https://github.com/tyj1987/MoxiangReborn) 为代码镜像。当前 GitLab 分支已绑定并可恢复：`main` 与 `codex/reorg-original-20261009`=`647d43bb`，Unity 两条长期分支=`b873753d`。GitHub 原版候选已同步到 `647d43bb`；GitHub `main` 仍为 `4dddd9a6`，Unity 长期分支仍为 `b751df25`，因此 GitHub 仍是部分镜像，不能宣称双端完全一致。

当前原版候选位于 E:\Workspaces\moxiang\worktrees\original-integration，当前资源与代码验证基线为 `647d43bb`；旧 `main` 尚不是现代客户端基线。Unity 工作区位于 E:\Workspaces\moxiang\worktrees\unity-integration，当前提交为 `b873753d`，两线未整体合并。

每线使用自己的工作树、构建输出、日志和运行配置。工具链为 Visual Studio C++、CMake、Python，Unity 沿用项目声明的 Editor 版本。启动说明以候选工作树的 docs/HUMAN_ACCEPTANCE.md 为准，必须明确连接 PVE，不隐式启动本机三服或数据库。

原版使用 modern/data/PlayDH 的 playdh-current 原资源，按完整哈希清单核对；Map203 的 161 个准确地表纹理已从同源原始 `D:/MX/client/Package/3DData/Map.pak` 恢复并通过依赖审计。Map17 准确 `17.hfl` 已恢复，但 9 个使用纹理仍缺失，原版还剩 22 个 HFL、18 个 TTB 文件级缺口；Map203 客户端纹理虽已齐全，服务端 `Resource/Server/Monster_203.bin` 仍未找到，真实地图服启动因此被发布保护阻断。Unity 导入结果及重制美术独立存放，禁止回写。原资源缺失项不能用替代美术宣称 1:1。

## 技术参考

- [项目约束](AGENTS.md)
- [资源格式](docs/RESOURCE_FORMATS.md)
- [协议](docs/MoxianProtocolDoc.md)
- [数据库](docs/DATABASE_SCHEMA.md)
- [活动缺陷](docs/KNOWN_BUGS.md)
- [资源来源与许可](docs/ASSET_LICENSE_LEDGER.md)

原始源码和配套资源是只读基准。感谢原《墨香》研发团队、资源格式分析社区及所用开源项目的贡献。


