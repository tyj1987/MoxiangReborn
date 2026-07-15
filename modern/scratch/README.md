# modern/scratch/

Agent session 中间产物归档目录。**不在 git 跟踪范围**（见根 `.gitignore`）。

子目录命名约定：`<来源>_<日期>`，例如 `_archive_2026-07-15`。

## 当前内容（2026-07-16 清后）

- `test_map_integration.py` — Phase 9 AgentServer ↔ MapServer 端到端集成
  测试，9 步流程（连接 → 角色创建/选择 → GameIn → 第二连接一致性 →
  Move 双向同步 → Chat 双向广播）。`MODERNIZATION_PLAN.md:269,274` 复现
  命令的依赖项。脚本通过 `SCRIPT_DIR` 自定位，**必须**放在 `modern/scratch/`
  根（`WORKSPACE = ../` 路径假设）。

## 已清理（详见 AI_SHIFT_LOG）

> 清理原则：删除前先 `grep` 验证无任何脚本 / 文档 / 复现命令引用，再走
> `mavis-trash` 整目录删除（已在回收站）。两个例外会留：复现命令
> 依赖项（`test_map_integration.py`）+ README 顶层索引。

- ~~`_archive_2026-07-15/`~~（167 文件 / 10 子目录 / 3.5 MB）— Phase 7.5p
  ~ 10.4 期间 client 探测产物（client_probes / monitor_tools / decode_tools
  / client_logs / client_bin / build_scripts / ld_scripts / mhfile_tools /
  msl_inspect / titan_probe），4 天后 grep 验证 0 外部引用，整目录
  mavis-trash。`test_map_integration.py` 单独保留至根（见上）。
- ~~`_desktop_2026-07-10/`~~（18 文件 / 1.7 MB）— Phase 7.5d ~ 7.5n
  期间从桌面归档的探测日志，6 天后清理。
- ~~`_session_2026-07-10/`~~（3 文件 / 2.4 KB）— 同日旧 session 临时归档。
- ~~`project_status_2026-07-16.html`~~ — 本 session 01:53 的临时
  可视化报告，session 关键信息已写入 AI_SHIFT_LOG，HTML 单文件
  无外部引用，mavis-trash。

## 维护规则（AGENTS.md trap #10）

1. **不再往桌面写中间产物**。Agent 必须写到 `modern/scripts/`（build
   工具 / 一次性脚本）、`modern/scratch/<source>_<日期>/`（批量临时
   归档）、`test-extract/dbs/`（sqlite runtime db）、`modern/scripts/
   probe_artifacts/`（one-off probe exe/obj）、或 `$MAVIS_SCRATCHPAD`
   （跨 session 笔记 / session 状态报告）。
2. **新归档建子目录**：`<source>_<YYYY-MM-DD>/`，子目录根放一个
   `README.md` 索引（来源 + 用途 + 维护规则）。
3. **老归档**：`mavis-trash` 删整个子目录。删前先 `grep` 确认没被
   任何脚本 / 文档 / 复现命令引用。复现命令依赖项要**迁出**
   到 `modern/scratch/` 根或对应工具目录，不随 archive 一起丢。
4. **大小控制**：单一归档目录超过 50 文件 / 10 MB 或堆积超过 3 天
   即应触发清理。172 文件堆 4 天是 2026-07-16 这次清理的反面教材。
