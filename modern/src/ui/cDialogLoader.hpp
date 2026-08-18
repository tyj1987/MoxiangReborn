// mxh/ui/cDialogLoader.hpp
// M-R3: 装载 132 dialog .bin 链 — 老版 cScriptManager::GetDlgInfoFromFile +
// GetInfoFromFile + cResourceManager::InitScriptManager 的现代等价物。
//
// 老版机制 (实地查证 墨香【源码】/[Client]MH/interface/cScriptManager.cpp):
//   1. 启动时遍历 <PlayDH>/Image/InterfaceScript/*.bin
//   2. 每个 .bin 用 CMHFile (XOR 解密) 读到内存
//   3. CMHFile::GetString() 逐 token 解析:
//        - $TYPE { ... }   → 新建对应 dialog class (CMainBarDialog / CInventoryExDialog / ...)
//        - #POINT x y w h  → 调 cDialog::Init(x, y, w, h)
//        - #CAPTIONRECT    → 调 cDialog::SetCaptionRect
//        - #BASICIMAGE     → 调 cImage::SetSpriteObject (M-R1/M-R2 跨表查 hard path)
//        - { ... } 嵌套     → 递归到 child widget
//   4. 全部装完后 GetDlgInfoFromFile 返回 cWindow* 给 cWindowManager 注册
//
// M-R3 范围 (无 GPU sprite 装填):
//   - 枚举 132 个 *.bin → 解析 InterfaceScript 树 (老 1:1 字节保真,既有 parser)
//   - 每个 root node 创建 cDialog → apply_legacy_layout(已经实现,line 394)
//   - AddDialog 到 cWindowManager (不递归 children — child 留给 M-R4 sprite 装填)
//   - 报告: 成功/失败 + root type + #POINT 校验
//   - 与 M-R1 cResourceManager + M-R2 cSpriteAtlas 解耦(loader 不调,只 verify 已装)
//
// 完成判据 (M-R3 go/no-go):
//   - 132/132 *.bin 装完 0 exception
//   - 全部 ok=true
//   - 至少 130 个有 #POINT (有 #POINT 才有 1:1 位置意义)
//   - 抽 5 个 sample 验证 #POINT byte-equal 老版 1:1 已知值
//   - 装完后 cWindowManager::dialogCount() == 解析 root 总数

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace mxh::ui {

class cWindowManager;

// 1 个 .bin 的装载报告
struct DialogLoadReport {
    std::filesystem::path path;     // 绝对路径 *.bin
    std::string bin_name;           // basename e.g. "10.bin" or "ChatOption.bin"
    std::string dialog_type;        // root[0].type (e.g. "MAINDLG", "INVENTORYDLG"); 多 root 拼接
    std::size_t root_count = 0;     // 解析出的 root 节点数
    bool has_point = false;         // 至少 1 个 root 有 #POINT
    std::int32_t point_x = 0;
    std::int32_t point_y = 0;
    std::int32_t point_w = 0;
    std::int32_t point_h = 0;
    bool ok = false;
    std::string error;              // 失败原因 (XOR 失败 / parse 错误 / 装载到 WM 失败)
    std::size_t dlg_count = 0;      // 实际 AddDialog 成功次数 (root 有 #POINT 才有)
};

// 装载总体统计
struct DialogLoadStats {
    std::size_t total_bins = 0;     // 找到的 .bin 总数
    std::size_t ok = 0;             // 成功
    std::size_t failed = 0;         // 失败
    std::size_t with_point = 0;     // 至少 1 个 root 有 #POINT
    std::size_t dialogs_added = 0;  // AddDialog 成功次数
    std::size_t roots_total = 0;    // 所有 *.bin 解析出的 root 节点总数
};

class cDialogLoader {
public:
    cDialogLoader() = delete;

    // 主入口: 枚举 <playdh_root>/Image/InterfaceScript/*.bin,逐个解析
    // + 装 cDialog 树到 wm。返回报告 list(按 .bin 文件名字典序)。
    //
    // 不调 cResourceManager / cSpriteAtlas (M-R3 范围只到 layout 装载,
    // sprite 装填是 M-R4)。但若 M-R1/M-R2 未装,会记 error 提醒。
    //
    // 失败不抛异常 — 每行 ok=false + error 信息。
    static std::vector<DialogLoadReport> LoadAll(
        const std::filesystem::path& playdh_root,
        cWindowManager& wm);

    // 单文件装载(便于单测和 debug)
    static DialogLoadReport LoadOne(
        const std::filesystem::path& bin_path,
        cWindowManager& wm);

    // 报告统计
    static DialogLoadStats Aggregate(const std::vector<DialogLoadReport>& reports);
};

}  // namespace mxh::ui
