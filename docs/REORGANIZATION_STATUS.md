# 重整执行状态

更新：2026-10-09。仅记录本轮事实，不继承历史 PASS。完整计划尚未交付。

## 已执行

- 原树初始为 182 修改、241 删除、1499 未跟踪，暂存区为空；没有整体提交或覆盖。
- 29 个 Git refs 已保存至 audit/pre-reorg-20261009.bundle；WIP、选定原资源和原创资产按文件哈希保全。私有恢复位置为 PVE /vmstore/ci-backups/moxiang-reorg/20261009/，逐文件读回校验、独立恢复 bundle 和 git fsck 通过。
- 保全范围和排除项见 E:\Workspaces\moxiang\audit\2026-10-09-reorg\baseline\baseline.json；不能视作所有忽略文件及数据库已备份。
- 独立原版工作树基于 89a11b3653549bab7471f53f8f0aafb72e98ce2a，分支 codex/reorg-original-20261009。
- 五对 UI 头文件差异为注释/格式；校准后 duplicate-header 检查通过。
- 候选启动器及验收脚本改为明确 PVE 客户端入口；启动器、客户端、Login、Agent CHINA、Map CHINA、DBTool 的 x86 Debug 构建通过。尚非运行验收。
- CT105 已快照 pre-reorg-20261009 并完整克隆为不自启的 CT930。仅在副本修正三个 hive 文件属主，原 mhcmember/mhgame/mhlog 全部 CHECKDB 通过，CT105 原件保留。
- CT930 现为 192.168.2.238，仅开放 VM112 到 SQL、PVE 到 SSH 等必要访问；SQL TLS 在 VM112 验证证书，不影响 PC 证书库。原版新库 MoxiangOriginalTest_20261009 已迁移 schema v1、20 表，两个现代 MSSQL fixture 均通过。Unity 新库单独创建但尚未迁移和验收。
- 本机默认实例六库及 SQLEXPRESS 的另一 Moxiang 库，共七份 COPY_ONLY/CHECKSUM 备份已私有上传、哈希核对、恢复到独立新库、VERIFYONLY 与 CHECKDB 通过。尚未验证这七个旧库的业务恢复，因此本机原库和共享 SQL 实例仍保留。
- VM112 192.168.2.244 已运行原版 Login/Agent/Map，端口 16001/17001/18001；启动前验证程序、资源与 writer contract，失败不创建服务进程。VM100 仍保留旧环境。
- 原版 24/24 无 DB C++ 测试、6/6 脚本测试、2/2 PVE 现代 MSSQL 测试通过；并非完整 CTest 或可玩验收。
- 原资源 playdh-current 在工作树与 VM112 各为 5131 文件、1,681,042,155 字节，SHA256 全匹配。78 个差异恢复 canonical 内容；241 个额外派生文件匹配 Git blob、归档后移除。新增 -text 属性防止 Git 换行转换，四个 PAK 保持 LFS。
- 真实 GUI 已完成启动器、登录、建角、选角、进图。地形 winding 修复后 Map10 地表恢复，38 项相关渲染测试通过。r6 新角色 MoxR6Suit 的武器、衣服和鞋已在 PVE 同事务持久化且进图穿着正确；r7 按原始源码修复 UI atlas 与自然源矩形绘制，3581 项 UI 测试通过。r7 GUI 复验因 Windows 锁屏未运行，阶段 D 仍未通过。
- 19 个登记项目已盘点；有独有 WIP 或恢复证据不足的项目保留。
- 已永久删除 pip 缓存 44,364,739 字节、npm _cacache 240,910,735 字节、四个已私有归档旧日志 2,268,465 字节、两份完全重复 reference 各 1,172,566,391 字节、两份旧执行报告及 25 份 XML 共 247,307 字节，以及 recovery 4,723,694,616 字节，合计 7,356,618,644 字节。每项均有删除记录。这是逻辑字节数，未扣除新增备份和构建占用；原资源归整另列，不重复计数。
- recovery 删除前，PVE 已逐字节恢复 9020 源文件，验证全部 41284 个 Git 对象及 fsck，并逐项验证 46 份 Git 管理/残留文件。删除执行器经独立审查和 23 项测试，通过本轮复检后用持续句柄锁防止路径替换，9076 文件全部删除；账本为 isolated-review/recovery-retirement-results.jsonl。Git pack 以对象级恢复，原索引副本仅作取证保留，不可直接覆盖重建 pack。
- 三份旧状态长文已在私有归档逐文件恢复验证后替换为当前状态，保留 E0–E5、G0–G11 定义和必要技术约束；恢复指针见 HISTORICAL_EVIDENCE_INDEX.md。
- 原版五个独立候选提交已普通推送 GitLab 的 codex/reorg-original-20261009，远端读回 689713647f459a2fd81b3511d6f9ec313690fe1b。尚未合并长期分支或删除历史分支。
- Unity 独立候选基于 b038ce40，已分组提交并普通推送到 d35743ec。x64 原生核心 110/110 宿主 CTest 通过，Effekseer 1.80.7 全部 279 文件按 SHA 锁定；空 Library 冷导入 EditMode 为 191 PASS、0 FAIL、13 SKIP，Development Player 构建通过。后续将 241 份派生文件迁到独立 overlay，补齐 4 份精确原始 HFL；canonical exact 5131 及复合源审计 5135/0 通过。45 项地图发布缺口继续阻塞，隔离后的 Editor/Player 回归进行中。
- Unity 私有输入包 5521 文件、1,119,770,980 原字节已在 PVE 逐项读回校验；不包含全量 PlayDH 和 Library/Player，新增 overlay 在独立 Git 提交中保全。完整包范围见 unity-candidate-review/unity-private-inputs-pve-proof.json。
- GitLab 两线 CI 配置已 lint 通过。Unity 流水线 738 因普通用户 Runner 不能运行 apk 安装而失败，测试未运行；已准备固定摘要的 Python 镜像与 helper LFS 检出配置，不放宽 Runner 权限。Windows Runner 仅允许受保护 refs 且缺少标准 C++ 工具链，不能声明 Windows CI 已可用。

## 尚未完成

所有忽略文件和隔离副本的独有内容去向；Unity 干净构建与独立 PVE 实例；共享候选修复吸收；两线 CI；原版画面修复和完整 GUI 可玩链；全地图与功能覆盖；旧数据库业务恢复；双客户端、故障恢复和 24 小时稳定性；最终分支合并删除和远端统一；清理清单闭合。

GitLab 项目 12 已验证。用户授权后，已通过 Broker 部署服务器恢复项目专用 mTLS 身份及正常 Git 通道。GitLab 四个 LFS 对象共 1,228,054,889 字节已独立下载并核对 SHA256/大小。GitHub 仅一个完整 LFS 对象通过，其余中断，不能称双端资源完整同步。

推送入口仅允许两个重整候选分支的普通推送，禁止 force/delete，凭据不离开服务器。证书于 2026-11-07 18:39:20 UTC 到期。客户端发送 PoP 签名，但部署版服务端校验尚未证实。不继承旧 401 结论，也不推断其他项目已获权限。

先前整体改写治理配置的动作被自动审批拒绝；随后批准并执行的范围仅为技术文档、AGENTS 顶部环境校准和精确远端身份元数据，未批量放宽授权字段。当前未正式发布、删除远端分支或整体合并 PR。

证据根：E:\Workspaces\moxiang\audit\2026-10-09-reorg；私有恢复根：PVE /vmstore/ci-backups/moxiang-reorg/20261009/。关键记录包括 restoration-proof.json、pc-sql-restore-proof.json、original-integration/README.md、gui-original/map-stable-recheck.png、broker-access/handoff.json、broker-access/gitlab-lfs-remote-proof.json、pc-inventory/、isolated-review/ 和 obsolete-logs-cleanup.json。

当前 VM112 r6 包 SHA256 为 0cb37ad5db941b908f1c9eee1b65b5696c40e085cc66db141669047e8a5da14c，对应源码补丁 a4401342c0cbcc9afd5e44911410b82e40648e4dd09e29de747cb08d99343162，manifest 为 92845837a991ce25c27328cb66c9ccc13c4e2574658c22a217498690ae00678b。原资源清单为 27ce90226af0b6a97ab2ebf8ec7272392f74bcbb3e75efb7d8f3e605023215a5。r7 是本机客户端候选，SHA256 为 5befafbb24d16a2b2eb32f34be46252c3fc1be660ce886749e2b8fed16965895，不能沿用 r6 的画面证明。
