# 清理清单（待用户确认）

> 生成日期：2026-08-09。当前仅盘点，尚未删除。删除前应重新检查引用并取得用户最终确认。

## 保留

| 路径 | 约大小 | Git | 理由 |
|---|---:|---|---|
| `modern/build/` | 7,593,721,046 B | ignored | 唯一受支持主构建；当前 11,741 tests 来源 |
| `modern/scratch/README.md` | tracked | tracked | scratch 保留规则和索引 |
| `modern/scratch/{compile_mxh_client,build_client_lib,build_client_tests,rebuild_mxh_ui}.bat` | tracked | tracked | 无完整 CMake 环境时的受控构建辅助 |
| `modern/scratch/stub_search.cpp` | tracked | tracked | 既有受控辅助样本 |
| `modern/scratch/test_map_integration.py` | tracked | tracked | Agent/Map E2E 复现依赖 |

## 建议删除：可重建构建产物

| 路径 | 文件数 | 大小 | 引用检查 | 处理 |
|---|---:|---:|---|---|
| `modern/build2/` | 13 | 174,060 B | `.gitignore` 明确标为 alternate build | 删除目录 |
| `modern/build_check/` | 4 | 22,581 B | `.gitignore` 明确标为 alternate build | 删除目录 |
| `modern/build_vs/` | 13 | 112,560 B | `.gitignore` 明确标为 alternate build | 删除目录 |
| `Testing/` | 生成物 | 可变 | 根 CTest 遗留；正式输出在 `modern/build/Testing` | 仅在为空或确认无证据后删除 |

## 建议删除或归档：scratch

- `modern/scratch/` 共 973 个文件、223,431,709 B、62 个任务目录；其中根层 347 个文件约 138 MB。
- 保留上表列出的受 Git 跟踪文件；保留 R-9 最终选定帧和 E2/E3 被正式复现命令引用的 fixture。
- 根层 `*_out.txt`、`*_err.txt`、重复 `.tga/.png`、临时 `append_*.py`/`fix_*.py`、旧 ctest/build log 在确认无引用后删除。
- 任务目录仅在含 README 且被测试/文档引用时保留；长期 fixture 应迁入对应 `modern/tests/fixtures/`，不能永久依赖 session scratch。

## 建议归档：过时阶段文档

| 路径 | Git | 处理 |
|---|---|---|
| `docs/PHASE_6_3_STATUS.md` | tracked | 内容核对进入 CHANGELOG 后删除 |
| `docs/PHASE_6_PROGRESS_2026-07-30.md` | tracked | 内容核对进入 CHANGELOG 后删除 |
| `docs/PLAN_2026Q3.md` | tracked | 已由当前 ROADMAP 取代，删除 |

这些是受版本控制文件，必须作为独立 docs 提交处理；不得由 bootstrap 自动删除。

## 建议迁移或删除：运行时数据库与根目录污染

| 路径 | Git | 处理 |
|---|---|---|
| `moxian.db`, `moxian.db-shm`, `moxian.db-wal` | ignored | 停止进程后删除；后续运行实例写入 `modern/build/runtime/` |
| `modern/moxian*.db*` | ignored | 停止进程后删除；需要的 schema/fixture 迁入正式测试目录 |
| `modern/test_quick.txt` | ignored | 删除 |

## 删除前命令

1. `git status --short`：确认没有新用户改动混入。
2. `rg -n "<候选文件名或目录名>" ROADMAP.md README.md AGENTS.md docs modern scripts deploy`：确认无引用。
3. 完整构建与 CTest 通过后记录基线。
4. 用户确认本清单后，按精确路径逐项删除；禁止使用仓库根目录递归通配删除。
5. 删除后重新配置/构建/测试并运行 `python scripts/check-project-governance.py`。
