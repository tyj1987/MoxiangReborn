# modern/ 代码量统计

> 维护规则：每次 Phase 收官时跑一次 snapshot，记到下方表格。
> 不要 commit 大文件 diff（git status -s 应该永远 0 行变化），本文件
> 是 metrics 报告，不是 source of truth。

## 统计命令

```powershell
$modern = "<workspace>\modern"
$src = (Get-ChildItem -LiteralPath "$modern\src" -Recurse -File -Include "*.cpp","*.c" -EA SilentlyContinue | Measure-Object).Count
$hdr = (Get-ChildItem -LiteralPath "$modern\include" -Recurse -File -Include "*.hpp","*.h" -EA SilentlyContinue | Measure-Object).Count
$tests = (Get-ChildItem -LiteralPath "$modern\tests" -Recurse -File -Include "*.cpp","*.c" -EA SilentlyContinue | Measure-Object).Count
$srcLines = (Get-ChildItem -LiteralPath "$modern\src" -Recurse -File -Include "*.cpp","*.c" -EA SilentlyContinue | Get-Content -EA SilentlyContinue | Measure-Object -Line).Lines
$hdrLines = (Get-ChildItem -LiteralPath "$modern\include" -Recurse -File -Include "*.hpp","*.h" -EA SilentlyContinue | Get-Content -EA SilentlyContinue | Measure-Object -Line).Lines
$testLines = (Get-ChildItem -LiteralPath "$modern\tests" -Recurse -File -Include "*.cpp","*.c" -EA SilentlyContinue | Get-Content -EA SilentlyContinue | Measure-Object -Line).Lines
[PSCustomObject]@{
  Src = $src; Hdr = $hdr; Tests = $tests;
  SrcLines = $srcLines; HdrLines = $hdrLines; TestLines = $testLines;
  Total = $src+$hdr+$tests; TotalLines = $srcLines+$hdrLines+$testLines
} | Format-List
```

## Snapshot 趋势

| 日期 | Phase | src | hdr | tests | src 行 | hdr 行 | test 行 | 总文件 | 总行数 | ctest |
|------|-------|-----|-----|-------|--------|--------|---------|--------|--------|-------|
| 2026-07-15 (P10.4 wrap) | 10.4 | 78 | 35 | 47 | ~13500 | ~4500 | ~8500 | 160 | ~26500 | 506/506 |
| **2026-07-16 (P12.1 wrap)** | **12.1** | **90** | **38** | **78** | **18018** | **6164** | **13957** | **206** | **38139** | **843/843** |

## 增长归因（2026-07-15 → 2026-07-16，+46 文件 / +11639 行）

### src/ (+12 文件 / +4518 行)

| 文件 | 用途 | 行数 | 来源 phase |
|------|------|------|------------|
| `bmhm_map.cpp` | BMHM 高度场文本格式解析 | ~80 | 12.1 (P2-2/3 共享) |
| `chr_motion.cpp` | ChrModel 文本格式解析 | ~225 | 12.1 P2-2 |
| `chx_model.cpp` | ChxModel 文本格式解析 | ~280 | 12.1 P2-3 |
| `item_effects.cpp` | 物品效果分类 + resolve | ~150 | 12.1 P2-6 |
| `ui/ime.cpp` | IME dispatcher | ~80 | 12.1 P2-10 |
| `ui/ime_win32_imm.cpp` | Win32 IMM adapter | ~120 | 12.1 P2-10 |
| `server/agent_handler.cpp` | on_disconnect GameOutSyn | +80 | 12.1 P2-7 |
| `server/map_handler.cpp` | UseSyn 物品效果 | ~+100 | 12.1 P2-6 |
| `render/dx11/texture_loader.cpp` | BC6H/BC7 + DX10 ext | +250 | 12.1 P2-11 |
| `ui/ime_*.cpp` 测试驱动 stub | (deleted → 净 +0) | - | - |
| (其他 5 个) | 编译单元重组 | ~+200 | - |

### include/ (+3 文件 / +1664 行)

| 文件 | 用途 |
|------|------|
| `mxh/compat/detail/text_parse.hpp` | chr_motion/chx_model/bmhm 共享 trim/tokenize |
| `mxh/game/item_effects.hpp` | classify_item / resolve_item_effect 公开 API |
| `mxh/ui/ime.hpp` | ImeAdapter + dispatcher API |

### tests/ (+31 文件 / +5457 行)

| 来源 | 新测试文件数 | 累计新测试数 |
|------|--------------|--------------|
| P2-2 chr_motion | 1 (+real sample) | 18+1 |
| P2-3 chx_model | 1 (+real sample) | 12+4 |
| P2-6 item_effects | 1 | 15 |
| P2-7 agent_handler on_disconnect | 1 (+2 用例) | 14+2 |
| P2-10 ime | 1 | 13 |
| P2-11 BC6H/BC7 texture_loader | 1 (+14 用例) | 228+14 |
| (其他保留测试 + gtest_main) | ~24 | - |

## 速度指标

- **代码增长**：+46 文件 / +11639 行（+43.9%）
- **测试增长**：+31 文件 / +5457 行（+64.2%）
- **ctest 增长**：506 → 843（+337 测试，+66.6%）
- **测试/源码比**：13957 / 18018 = **0.77**（1 行 src 对应 0.77 行 test）
- **测试通过率**：100% / 0 回归

## 下次统计触发点

- 每次 P2 / P3 phase 收官时
- 任何主模块（render / ui / server / compat）完成 1+ 类完整实装时
- 1.0 release tag 前 24h
