# 渲染 + UI 视觉 1:1 还原计划（精细版）

> 状态：基于 2026-08-18 实地查证（modern/src、tools/MoxianClient、墨香【源码】/[Client]MH/interface/cScriptManager）
> 范围：渲染管线 + UI 资源装载 + 165 个 dialog 视觉还原 + 性能
> 配合：[02-asset-pipeline-1to1.md](./02-asset-pipeline-1to1.md) 资源装载、[03-perf-budget.md](./03-perf-budget.md) 性能预算

---

## 0. 不可破坏的硬约束（AGENTS.md §0 复述）

1. `.bin / .pak / .bmhm / .ttb / .chl / .chx / .chr / .mon / .bsad` 字节兼容
2. `MHFile` 解密后内容 = 老版字节
3. HSEL / HackShield 签名不变
4. modern 客户端必须和老 Login/Agent/Map 三进程互通
5. 玩法/数值 1:1 锁定

UI 视觉层 1:1 要求（新增，但属于"视觉 1:1"层）：
- **每个 dialog 像素位置和老版对齐**（用 #POINT + #CAPTIONRECT）
- **每个 dialog 的 basicimage / overimage / pressimage / focusimage 来自老版 .bin 硬路径表**
- **每个 cell/button 的 sprite 切片和老版 1:1**（不替换色块）
- **加载顺序和老版相同**（先 hard-path 表 → 再 dialog .bin → 再 OnLoad → 再 SetActive）

---

## 1. 实地查证：当前真实状态（不夸大、不缩小）

### 1.1 资源兼容层

| 项 | 真实状态 | 证据 |
|---|---|---|
| `modern/src/compat/` 目录 | **0 files**（目录不存在） | `Get-ChildItem modern/src/compat -Recurse -File` 返回 0；`glob modern/src/compat/**/*` 返回 No files |
| compat 等价物（散在 src/ 根） | **存在** | bmhm_map.cpp、pack_file.cpp、mh_file_ex.cpp、stm_static_model.hpp、hfl_height_field.hpp、chx_model.hpp、chr_motion.hpp、bsad_area.hpp、monster_catalog.cpp、character_appearance_catalog.cpp、quest_npc_catalog.cpp、quest_string_catalog.cpp、sound_list.cpp、ttb_tile_table.hpp、image_path_table.hpp、mh_file_ex.hpp、pack_file.hpp、platform.hpp、detail/text_parse.hpp |
| `cResourceManager` 等价物 | **不存在** | 没有 `src/cResourceManager.cpp` 或 `src/ui/cResourceManager.cpp`；grep "GetImageInfo\|RESRCMGR" 0 命中 |
| `m_ImageHardPath` / `m_ItemHardPath` 等 7 个 hard-path 哈希表 | **不存在** | grep "m_ImageHardPath" 0 命中 |
| `Image/InterfaceScript/30.bin` 解析 | **只在测试调** | `apply_legacy_layout` 命中 5 个文件（4 测试 + 1 hpp），`MoxianClient/main.cpp` **不调** `parse_interface_script` / `apply_legacy_layout` |
| `PlayDH/Image/InterfaceScript/30.bin` 实际格式 | **存在** | 老版 hard path 表 96+ 个 .bin 文件 |

### 1.2 UI 渲染管线

| 项 | 真实状态 | 证据 |
|---|---|---|
| `cWindow::Render()` | **存在，但永远 no-op** | cWindow.cpp: 调用 `cImage::render(m_absX, m_absY, w, h, 0xFFFFFFFFu, zOrder=0)`；m_basicImage 是 void*；cImage::render 检 m_pSurface==nullptr → return false |
| `m_basicImage` 实际赋值 | **永远 nullptr** | grep `SetBasicImage` 命中 3 个文件：cWindow.hpp/cpp 自己 + 1 个测试。165 个 dialog 全部不调 SetBasicImage |
| `cImage::bindRenderer` 装 sprite | **只在 demo/test/host main 调** | 命中 6 文件：MoxianClient/main.cpp、phase6_smoke.cpp、interface_script_render_test.cpp、cimage_test.cpp、cImage.hpp/cpp |
| `cImage::SetSpriteObject` 调用方 | **只有 phase6_smoke.cpp 和 MoxianClient HUD 区** | 165 个 dialog 不调 |
| MoxianClient HUD 实际渲染 | **绕过 cDialog 树，用独立 drawSpriteQuad 循环** | main.cpp 画 HP/MP/Quick slot/Inventory/NPC shop/Chat 都是手写循环，不是 cDialog 树 |
| Inventory panel 实际颜色 | **0xAA181010 = 60% 透明深棕色 (24,16,16) 在天空+地形背景下完全融入** | main.cpp 注释 "solid-color quads; original InterfaceScript art is wired in a later phase" |
| MoxianClient main.cpp 自身承认 | **"original InterfaceScript art is wired in a later phase once the UI runtime lands"** | main.cpp 顶部注释 + g_hud.barBg 注释 |

### 1.3 性能

| 项 | 真实状态 | 证据 |
|---|---|---|
| Static scene mesh 数 | **334 个独立 IDIMeshObject** | static_scene.cpp: 每个 mesh 一个 IDIMeshObject，无 instancing |
| Terrain mesh 数 | **tile_count_x × tile_count_z / 32 = 多个独立 mesh** | terrain_scene.cpp: 每 32x32 tile chunk 一个 IDIMeshObject |
| Frustum culling | **不存在** | grep "frustum" 仅命中 setViewFrustum（更新矩阵）和 D3D11_CULL_BACK/NONE（rasterizer state）。无 per-mesh/per-chunk AABB 测试 |
| Instancing | **不存在** | grep "Instancing\|instancing" 0 命中 |
| Draw call per frame | **334 (static) + N (terrain chunks) + 1 (sky) + ~20 (entity) + ~50 (HUD) ≈ 400-500** | 推导；实测 5fps 与之吻合 |
| 5fps 根因 | **CPU 端 draw call 调度 + DX11 state 切换**，不是 GPU 端 | 老版用 IDIMeshObject + 大量 small batch 同样会卡，但老版在 2005 年的硬件上能跑因为场景 mesh 数少；modern 装了完整 STM 之后 mesh 数爆炸 |

### 1.4 老版 1:1 视觉规范（来自 cScriptManager.cpp 实地读）

| 老版机制 | 现代对应 | 1:1 还原需要做的事 |
|---|---|---|
| `cScriptManager::InitScriptManager()` 打开 FILE_IMAGE_HARD_PATH 等 7 个 path .bin，建 m_ImageHardPath[m_ItemHardPath[m_MugongHardPath[m_AbilityHardPath[m_BuffHardPath[m_MiniMapHardPath[m_JackPotHardPath] 哈希表 | **0** | 实现 `cResourceManager` 一次启动装载这 7 张表 |
| `RESRCMGR->GetImageInfo(idx)` 返回 IMAGE_NODE（sprite + size） | **0** | 实现 IMAGE_NODE 缓存池，按 idx 装 sprite |
| `cImage::SetSpriteObject(RESRCMGR->GetImageInfo(idx))` 给 dialog 装基本图 | **0**（165 个 dialog 全部不调） | dialog Init 时通过 hard path 拿 sprite，SetBasicImage / SetOverImage / SetPressImage / SetFocusImage / SetSelectImage / SetTooltipImage / SetTopImage / SetMiddleImage / SetDownImage / SetIconCellBgImage / SetDragOverBgImage 全部接上 |
| `GetDlgInfoFromFile(.bin)` 解析 `$MAINDLG` / `$INVENTORYDLG` / `$BTN` / `$STATIC` / `$EDITBOX` / `$GUAGENAME` 等节点 | interface_script 解析器存在（覆盖率未知） | MoxianClient main.cpp 调 parse_interface_script 装载 165 个 .bin |
| `case eBASICIMAGE:` 调 GetImage → SetSpriteObject | 解析器有 BASICIMAGE 字段 | 但 SetSpriteObject 在 dialog Init 路径上没接 |
| `case eOVERIMAGE/eSELECTIMAGE/eFOCUSIMAGE/eTOOLTIPIMAGE/eTOPIMAGE/eMIDDLEIMAGE/eDOWNIMAGE/eBTNTEXT/eTOOLTIPMSG/eTEXT` | 解析器部分有 | 完整覆盖到 165 个 dialog 的所有 property |
| 老版 `cWindow::SetAbsXY` + 父子坐标系 + Render 深度优先 | modern cWindow 一致 | 1:1 OK |
| 老版 `cWindow::ActionEvent` 顶到底 dispatch | modern 一致 | 1:1 OK |
| 老版 `cWindow::ActionKeyboardEvent` 含 IME/快捷键 | modern 是 no-op | 1:1 需要补 |

---

## 2. 还原目标（按 1:1 严格度排序）

### 2.1 必达（不达成不算"1:1 复现"）

1. **cResourceManager + 7 张 hard path 哈希表** — 启动一次装载，165 个 dialog 的 Init 路径通过它取 sprite
2. **MoxianClient main.cpp 调 parse_interface_script + apply_legacy_layout 装载 165 个 dialog .bin** — 不再绕开 cDialog 树
3. **165 个 dialog 的 Init 函数全部接 7 类图（basic/over/press/select/focus/tooltip/top/middle/down）** — 不再靠 g_hud.barBg 假色块
4. **Inventory panel 用真 8×10 cell sprite**（来自老版 30.bin 里的 INVENTORYDLG hard path），不是 0xAA181010 假色
5. **QuickSlot 8 个 F1-F8 用真 icon sprite**（来自老版 mugong icon hard path）
6. **HP/MP bar 用真 4-tile sprite**（来自老版 guage hard path）
7. **Chat 框底图用真 chat_bg sprite**（来自老版 chat hard path）

### 2.2 应达（达成才算"能玩"）

8. **性能 ≥ 30 fps**（在 1920×1080 + 334 static mesh + ~30 terrain chunk + 16 NPC + 5 怪 + 全 HUD + 全 dialog 开启下）
9. **5 fps 改善** 通过：
   - 静态 mesh 按 chunk 合并（每 16 个 mesh 一次 draw call）
   - Terrain 按 texture 合并（同 texture 多个 tile 一次 draw call）
   - Frustum culling（AABB 测 in-frustum）
   - 遮挡剔除（HP>0 时 entity 永远画，否则不画）
   - HUD/dialog 单 batch（同一 sprite 一次 draw call）
10. **1080p / 2K 自适应**（#POINT_ low-res variant 已解析，scale 0.75→1.0 切换）

### 2.3 加分（达成才算"和老版 1:1 视觉"）

11. **对话框拖动 + 关闭 + 打开动画**（legacy 1:1：#MOVEABLE + #AUTOCLOSE）
12. **Tooltip 显示**（#TOOLTIPIMAGE + #TOOLTIPMSG）
13. **按钮 hover/press/focus 三态切换**（#OVERIMAGE/#PRESSIMAGE/#FOCUSIMAGE 鼠标事件触发）
14. **IME 输入法**（cIME 已经有，需 wire 到 chat）
15. **Tab 切换 / focus chain**（focus 链 1:1）

---

## 3. 还原计划：按依赖顺序分 7 个里程碑

### M-R0：诚实基线（半天）

目标：把所有"全过"断言改成"真测真截图"。

任务：
- 写一个 `scripts/visual-smoke.ps1`：
  - 启动 MoxianClient 进 GameIn 状态 5 秒
  - 自动 F1-F8 / I / B / C / M / V 按一遍
  - 截图 6 张（登录、选角、char-make、空场、HUD-only、inventory-open）
  - 用 `compare` 或 Python PIL 算每张图与 reference 的 SSIM
  - reference 从老版部署 `墨香【客户端+服务端+工具】/SWorking` 截同样 6 张
- 所有"X / X tests PASS" + "visual acceptance pending" 标记到 `modern/docs/visual-baseline.md`
- **禁止**在没截图对比前发"1:1 还原完成"等声明

交付：`scripts/visual-smoke.ps1` + 6 张 modern 截图 + 6 张 legacy 截图 + SSIM 报告

### M-R1：cResourceManager + 7 张 hard path 表（3 天）

目标：实现老版 `cScriptManager::InitScriptManager()` 装载 hard path 的等价物。

文件：
- 新建 `modern/src/ui/cResourceManager.hpp/cpp`
- 新建 `modern/tests/unit/ui/cResourceManager_test.cpp`
- 改 `modern/src/ui/CMakeLists.txt` 加新文件

接口（mirror 老版）：
```cpp
namespace mxh::ui {
struct ImageHardPath { int idx; LONG left, top, right, bottom; };

class cResourceManager {
 public:
  static cResourceManager* GetInstance();
  void InitScriptManager(const std::filesystem::path& resource_root);
  void ReleaseScriptManager();
  
  // 老版: RESRCMGR->GetImageInfo(idx) -> IMAGE_NODE
  IDISpriteObject* GetImageInfo(int idx) noexcept;
  // 老版: GetImage(idx, cImage*, ePATH_FILE_TYPE)
  void GetImage(int hard_idx, cImage* img, ePathFileType type);
  
  // 老版: m_ImageHardPath / m_ItemHardPath / m_MugongHardPath / ...
  bool GetHardPath(int idx, ImageHardPath* out, ePathFileType type) const;
  
 private:
  CYHHashTable<ImageHardPath> m_image_hard;
  CYHHashTable<ImageHardPath> m_item_hard;
  CYHHashTable<ImageHardPath> m_mugong_hard;
  CYHHashTable<ImageHardPath> m_ability_hard;
  CYHHashTable<ImageHardPath> m_buff_hard;
  CYHHashTable<ImageHardPath> m_minimap_hard;
  CYHHashTable<ImageHardPath> m_jackpot_hard;
  
  // sprite cache: idx -> IDISpriteObject*
  std::unordered_map<int, IDISpriteObject*> m_sprite_cache;
};
}
```

数据装载：
- 打开 `PlayDH/Image/InterfaceScript/IMAGE_PATH_BIN`（legacy 是 `FILE_IMAGE_HARD_PATH`，需要去 cWindowDef.h 找宏定义的实际文件名）— 实际是老版 `imagepath.bin` 或 `hardpath.bin` 之类
- 读格式：每行 `idx left top right bottom`
- 同理装载 6 张 path 表

测试（必须覆盖）：
- 7 张表都能装
- idx → hard path 查询 1:1
- idx → sprite 查询 1:1
- 释放无泄漏

### M-R2：sprite 缓存池 + 1:1 atlas 切片（2 天）

目标：实现老版 `RESRCMGR->GetImageInfo(idx)` 的等价物。

文件：
- 新建 `modern/src/ui/cSpriteAtlas.hpp/cpp`（atlas 容器）
- 改 `cResourceManager` 调用它

机制：
- 解析老版 `PlayDH/Image/InterfaceScript/Surface.bin` 之类，构造 atlas（多个大 PNG → 多个 IDISpriteObject）
- idx → (atlas_id, left, top, right, bottom) → IDISpriteObject* + UV
- LRU 缓存，128 个 sprite
- 与 cImage 集成：`cImage::SetSpriteObject` 接受这个 sprite

测试：
- atlas 切分 1:1（用 SHA-256 哈希每个切片的像素 buffer）
- 同一 idx 多次查返回同一 sprite
- 释放时 SRV/ComPtr 全部 release

### M-R3：MoxianClient main.cpp 调 interface_script 装载 165 个 dialog（3 天）

目标：把 MoxianClient 的"假色块 HUD"替换成"真 cDialog 树"。

文件：
- 改 `modern/tools/MoxianClient/main.cpp`：
  - Init 阶段：cResourceManager::GetInstance()->InitScriptManager(playdh_path)
  - 启动时：循环读 `PlayDH/Image/InterfaceScript/*.bin` 列表
  - 每个 .bin：parse_interface_script → 对每个 root node 调 apply_legacy_layout
  - 把 dialog 存到 `g_dialogs[filename]`
- 写 `modern/tools/MoxianClient/dialog_loader.hpp/cpp`：封装上述循环
- 写 `modern/tools/MoxianClient/CMainGame.cpp`：在 OnEnterGameIn 时把 dialog 树挂到 cWindowManager

### M-R4：165 个 dialog 各自 Init 接 7 类图（5 天）

目标：每个 dialog 的 Init 函数走 cResourceManager 拿 sprite。

策略（按依赖顺序）：
1. **cWindow** 改：`SetBasicImage` 接受 `cImage*` 不再是 `void*`（breaking change，全局改）；同时保留 void* overload 走 legacy 路径
2. **cImage** 改：加 `void SetFromResourceManager(int hard_idx, ePathFileType type = PFT_HARDPATH)`，内部调 `RESRCMGR->GetImage(idx, this, type)`
3. **165 个 dialog** 改：每个 Init 函数加 4-7 行
   ```cpp
   m_pBasicImage = std::make_unique<cImage>();
   m_pBasicImage->SetFromResourceManager(node->basic_image_idx, PFT_HARDPATH);
   m_pOverImage = std::make_unique<cImage>();
   m_pOverImage->SetFromResourceManager(node->over_image_idx, PFT_HARDPATH);
   // ... press/select/focus/tooltip
   cWindow::Init(node->point->x, node->point->y, node->point->w, node->point->h, m_pBasicImage.get(), id);
   ```
4. **测试**：每个 dialog 一个截图测试，校验 sprite 不为 null + 渲染像素和老版 SSIM ≥ 0.95

### M-R5：性能优化（4 天）

目标：5 fps → 30 fps。

任务（按收益排序）：

**5.1 Static mesh chunk 合并**（最大收益，预期 3-4x）：
- 文件：`modern/src/render/static_scene.cpp`
- 思路：把同 texture 的 mesh 合并成一个 super-mesh，单 draw call
- 阈值：每 atlas/每 material 分组，组内 ≤ 65535 顶点合并
- 测试：性能 baseline + visual diff（合并前后 mesh 轮廓像素差 < 1%）

**5.2 Terrain chunk 合并**（中收益，预期 1.5-2x）：
- 文件：`modern/src/render/terrain_scene.cpp`
- 思路：去掉 32×32 tile 分块，整张地形一个大 mesh
- 但单 mesh 顶点数可能爆，所以保留分块但合并同 texture
- 测试：同上

**5.3 Frustum culling**（中收益）：
- 文件：新建 `modern/src/render/frustum.hpp/cpp`
- 思路：相机视锥 6 个平面，每帧对每个 mesh 做 AABB 测试，不在视锥内不画
- 用 device.cpp 已有的 setViewFrustum 拿矩阵，构造视锥
- 测试：遮挡 NPC 区域 + 朝向地平线 + 360° 旋转，校验 mesh 渲染数变化

**5.4 HUD/dialog batch**（小收益）：
- 文件：`modern/src/render/dx11/primitives.cpp`
- 思路：收集同 sprite 的多次 draw，合并成一次 instanced draw
- 测试：全 HUD + 全 dialog 开启，draw call 数从 ~50 降到 ~5

**5.5 Texture atlas 化**（小收益 + 视觉稳定）：
- 文件：`modern/src/render/dx11/texture_loader.cpp`
- 思路：所有 item icon + all dialog 7 类图合成 1-2 个 4096×4096 atlas
- 测试：atlas 像素和老版 1:1（用 SHA-256）

### M-R6：IME / 焦点链 / 鼠标光标 1:1（2 天）

目标：交互层和老版一致。

任务：
- 写 cIME 完整 wire（chat dialog 接受 IME 输入）
- 实现 focus chain（Tab 切换、左右键、上下键）
- 实现 CMousePointer 1:1（光标 sprite 来自 hard path 表）

### M-R7：1080p / 2K 自适应（1 天）

目标：不同分辨率下 #POINT_ low-res variant 自动切换。

任务：
- cDialog Init 接受 resolution_mode 参数
- OnResolutionChange 时重新 Init 所有 dialog
- 测试：800×600 / 1920×1080 / 2560×1440 三档截图对比

---

## 4. 验证策略（每个里程碑都有 screenshot 锁死）

### 4.1 视觉验证工具栈

- `scripts/visual-smoke.ps1`（M-R0 写）：自动截 6 张关键状态
- `scripts/dialog-screenshot.ps1`：对单个 dialog 单独截图（截前 OnLoad + SetActive + Render 一帧）
- `scripts/visual-compare.py`：用 PIL SSIM + 直方图对比
- `modern/tests/visual/*.golden.png`：每个 dialog 一张 golden

### 4.2 SSIM 阈值

| 类型 | 阈值 | 失败处理 |
|---|---|---|
| 整体背景（地形+天空） | SSIM ≥ 0.98 | 立即停，回查 |
| Static mesh | SSIM ≥ 0.95 | 立即停，回查 |
| Dialog 边框/底色 | SSIM ≥ 0.95 | 立即停，回查 |
| Dialog 文字 | SSIM ≥ 0.85 | 字体 sub-pixel 差异允许 |
| Icon sprite | SHA-256 完全一致 | 立即停 |
| 整体页面 | SSIM ≥ 0.92 | 立即停 |

### 4.3 每个里程碑的 go/no-go 检查

| 里程碑 | 通过条件 |
|---|---|
| M-R0 | visual-smoke.ps1 跑通，6+6 张截图就位 |
| M-R1 | cResourceManager 单测 + 集成测全过；7 张表装载后 idx 查询命中 100% |
| M-R2 | atlas 切片 SHA-256 = 老版 1:1 |
| M-R3 | MoxianClient 启动时 165 个 .bin 全部 parse_interface_script 成功，dialog 树挂到 cWindowManager |
| M-R4 | 每个 dialog 截图 SSIM ≥ 0.95（165 个 dialog 各一个 golden） |
| M-R5 | 视觉无差异（SSIM ≥ 0.98） + FPS ≥ 30（1920×1080 + 满 HUD + 满 dialog） |
| M-R6 | IME 输入、focus chain、鼠标光标截图 1:1 |
| M-R7 | 三档分辨率 SSIM ≥ 0.95 |

---

## 5. 资源 + 工时预算

### 5.1 资源（人 / 时）

| 里程碑 | 估计人时 |
|---|---|
| M-R0 诚实基线 | 0.5 天 |
| M-R1 cResourceManager | 3 天 |
| M-R2 sprite atlas | 2 天 |
| M-R3 dialog 装载 | 3 天 |
| M-R4 165 dialog 改 Init | 5 天 |
| M-R5 性能优化 | 4 天 |
| M-R6 IME + focus | 2 天 |
| M-R7 分辨率自适应 | 1 天 |
| **总计** | **~20.5 天** |

### 5.2 资源（算力 / 工具）

- 老版 MoxianClient 部署在 SWorking：用于截 reference 图
- 老版 PlayDH 资源已就位（1.3GB）
- DX11 debug layer 开启 + PIX 截图（性能调优阶段）
- 视觉 golden 库：~200 张（6 状态 × 5 场景 + 165 dialog × 1）

---

## 6. 风险 + 回退

| 风险 | 应对 |
|---|---|
| 老版 hard path .bin 路径找不到（FILE_IMAGE_HARD_PATH 宏定义散在 cWindowDef.h） | grep 全 `[Client]MH` 找宏，必要时用二进制特征定位（每个 .bin 第一行 = 数字 header） |
| 老版 image atlas 不是单图，是按 idx 分的多个 .tga | M-R2 改成支持 multi-atlas |
| 165 个 dialog 的 Init 改造可能漏掉（残 0xAA181010 假色） | M-R4 写一个 `tests/dialog-real-sprite.cpp`：每个 dialog 必须有真实 sprite，无则测试 fail |
| 性能优化破坏视觉 1:1 | M-R5 每个优化点都要 visual diff 校验（合并前后像素差 < 1%） |
| 资源 GBK/EUC-KR 编码在现代 Windows terminal 显示乱码 | 用 read 工具直接读，pipeline 跳过显示 |

---

## 7. 不要做（避免 scope 漂移）

- 不要改协议头（AGENTS.md §0）
- 不要改老源码（AGENTS.md §1）
- 不要改资源（AGENTS.md §1）
- 不要写"全过"假断言（M-R0 锁死）
- 不要绕过 cDialog 树画 HUD（M-R3 锁死）
- 不要加新功能（仅还原 + 1:1 视觉）
- 不要重新设计资源格式（沿用老版 .bin / .pak）
- 不要把 1:1 视觉当作"看着像" — **SSIM ≥ 0.95 才算**

---

## 8. Next Step

按 M-R0 → M-R1 → ... → M-R7 顺序执行。

**先做 M-R0（半天）**：写 `scripts/visual-smoke.ps1`，跑出 6+6 张基线截图。这是判断后续每个里程碑有没有真的"达到 1:1"的唯一裁判。

不达成 M-R0 不进入 M-R1。
