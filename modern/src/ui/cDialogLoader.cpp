// mxh/ui/cDialogLoader.cpp
// M-R3: 装载 132 dialog .bin 链 — 实现见 header.

#include "mxh/ui/cDialogLoader.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <string>
#include <system_error>

#include "mxh/compat/mh_file_ex.hpp"
#include "mxh/log/mlog.hpp"
#include "mxh/ui/cButton.hpp"
#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cImage.hpp"
#include "mxh/ui/cResourceManager.hpp"
#include "mxh/ui/cSpriteAtlas.hpp"
#include "mxh/ui/cWindowManager.hpp"
#include "mxh/ui/interface_script.hpp"

namespace mxh::ui {

namespace {

// 排序用: 数字 basename < 字母 basename(老版命名混用,字典序不稳)
// 我们用 std::filesystem::path 比较,让 OS 决定
struct BinLess {
    bool operator()(const std::filesystem::path& a,
                    const std::filesystem::path& b) const {
        return a.filename().string() < b.filename().string();
    }
};

// M-R4.1 sprite 装填 hook + 持 cImage 句柄 (进程级 singleton 行为)
LoadSpriteFn g_loadSprite = nullptr;
void*        g_loadSpriteCtx = nullptr;
// 持所有 cImage 句柄, 程序退出前不会被释放
std::vector<std::unique_ptr<cImage>> g_cimage_owners;

}  // namespace

void cDialogLoader::SetSpriteLoader(LoadSpriteFn fn, void* ctx) noexcept {
    g_loadSprite = fn;
    g_loadSpriteCtx = ctx;
    MLOG_INFO("[cDialogLoader] SetSpriteLoader: %s (M-R4 sprite 装填%s)",
              fn ? "registered" : "cleared",
              fn ? "启用" : "禁用");
}

namespace {

// M-R4.3 helper: 跨表查 1 张老 .tif 装 cImage. 命中 cimages_owners vector.
// 失败 (hook 未注册 / image_idx < 0 / rect 缺失 / 跨表查 miss) 返 nullptr.
// M-R4.1 root basicImage + M-R4.3 children 9 类图都用这个.
cImage* loadImageForImageIdx(std::int32_t image_idx,
                              const std::optional<mxh::ui::ImageRect>& rect) {
    if (!g_loadSprite || image_idx < 0 || !rect.has_value()) {
        return nullptr;
    }
    const auto hp = cResourceManager::getInstance().getHardPath(
        image_idx, PathFileType::HardPath);
    if (!hp.has_value()) return nullptr;
    const auto info = cSpriteAtlas::getInstance().getInfo(hp->atlas_idx);
    if (!info.has_value()) return nullptr;
    const auto tif_abs = cSpriteAtlas::getInstance().resolvePath(*info);
    if (!std::filesystem::exists(tif_abs)) return nullptr;
    void* sprite = g_loadSprite(g_loadSpriteCtx, tif_abs.string());
    if (!sprite) return nullptr;
    auto owner = std::make_unique<cImage>();
    const auto& ir = *rect;
    owner->SetSource(ir.left, ir.top, ir.right, ir.bottom,
                     info->width, info->height);
    owner->SetSpriteObject(sprite);
    cImage* out = owner.get();
    g_cimage_owners.push_back(std::move(owner));
    return out;
}

}  // namespace

DialogLoadReport cDialogLoader::LoadOne(const std::filesystem::path& bin_path,
                                        cWindowManager& wm) {
    DialogLoadReport r;
    r.path = bin_path;
    r.bin_name = bin_path.filename().string();

    // 1) 读 + XOR 解密
    auto read = mxh::compat::read_mh_bin(bin_path);
    if (!read.ok()) {
        r.error = "read_mh_bin failed (err=" +
                  std::to_string(static_cast<int>(read.error)) + ")";
        return r;
    }
    if (read.value.data.empty()) {
        r.error = "decrypted payload is empty";
        return r;
    }

    // 2) parse_interface_script
    std::string_view payload(
        reinterpret_cast<const char*>(read.value.data.data()),
        read.value.data.size());
    InterfaceScript parsed;
    try {
        parsed = parse_interface_script(payload);
    } catch (const std::exception& e) {
        r.error = std::string("parse_interface_script exception: ") + e.what();
        return r;
    }
    r.root_count = parsed.roots.size();
    if (parsed.roots.empty()) {
        // 合法空: 辅助 .bin (e.g. barInfo.bin 是 progress bar 字符表, 不是 dialog
        // 描述). 老版 cScriptManager::GetDlgInfoFromFile 对这类文件也是 no-op.
        r.ok = true;
        r.dialog_type = "(auxiliary)";
        return r;
    }

    // 3) 每个 root 创建 cDialog + apply_legacy_layout + AddDialog
    bool first_point_set = false;
    for (std::size_t i = 0; i < parsed.roots.size(); ++i) {
        const auto& root = parsed.roots[i];
        if (i == 0) {
            r.dialog_type = root->type;
        } else {
            r.dialog_type += "+" + root->type;
        }

        // 没有 #POINT 的 root (辅助 .bin, 老版不实际渲染) — 跳过
        if (!root->point.has_value()) {
            continue;
        }

        // M-R4.1: root basicImage 跨表查
        cImage* cimg = loadImageForImageIdx(root->basic_image_idx,
                                            root->basic_image_rect);
        if (cimg) r.cimg_count += 1;

        auto dlg = std::make_unique<cDialog>();
        const bool applied = apply_legacy_layout(*dlg, *root,
                                                  /*basicImage=*/cimg);
        if (!applied) {
            r.error = "apply_legacy_layout failed for root[" +
                      std::to_string(i) + "]";
            return r;
        }
        if (!first_point_set) {
            const auto& p = *root->point;
            r.has_point = true;
            r.point_x = p.x;
            r.point_y = p.y;
            r.point_w = p.w;
            r.point_h = p.h;
            first_point_set = true;
        }

        // M-R4.3: 装 children (1 步跨表查装 3 类图 per cButton + per child type)
        // 1:1 with 老版 cScriptManager::GetInfoFromFile 递归 + GetImage
        // 9 类图 (basic/over/press/select/focus/tooltip) 跨表查装
        std::size_t child_count = 0;
        for (const auto& child : root->children) {
            if (!child->point.has_value()) continue;
            const auto& p = *child->point;
            // 路由 widget class by type
            if (child->type == "BTN") {
                cImage* basic  = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                cImage* over   = loadImageForImageIdx(child->over_image_idx,
                                                     child->over_image_rect);
                cImage* press  = loadImageForImageIdx(child->press_image_idx,
                                                     child->press_image_rect);
                auto btn = std::make_unique<cButton>();
                btn->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, over, press, /*onClick=*/{}, /*userdata=*/nullptr,
                          /*id=*/0);
                if (basic)  ++r.cimg_count;
                if (over)   ++r.cimg_count;
                if (press)  ++r.cimg_count;
                dlg->Add(std::move(btn));
                ++child_count;
            }
            // 其他 type (STATIC/EDITBOX/LISTDIALOG/...) 留给 M-R4.4+
        }
        if (child_count > 0) {
            r.dialog_type += "+" + std::to_string(child_count) + "child";
        }

        wm.AddDialog(std::move(dlg));
        r.dlg_count += 1;  // 用于 stats 统计 AddDialog 成功次数
    }

    r.ok = true;
    return r;
}

std::vector<DialogLoadReport> cDialogLoader::LoadAll(
    const std::filesystem::path& playdh_root, cWindowManager& wm) {
    std::vector<DialogLoadReport> out;

    // 提示 M-R1/M-R2 状态(不强制)
    if (!cResourceManager::getInstance().allLoaded()) {
        MLOG_WARN("[cDialogLoader] cResourceManager not loaded; "
                  "M-R1 missing — sprite hookup will be 0 (expected for M-R3)");
    } else {
        MLOG_INFO("[cDialogLoader] cResourceManager ready (M-R1)");
    }
    if (!cSpriteAtlas::getInstance().loaded()) {
        MLOG_WARN("[cDialogLoader] cSpriteAtlas not loaded; "
                  "M-R2 missing — basic image lookups will be 0 (expected for M-R3)");
    } else {
        MLOG_INFO("[cDialogLoader] cSpriteAtlas ready (M-R2, %zu entries)",
                  cSpriteAtlas::getInstance().size());
    }

    const auto dir = playdh_root / "Image" / "InterfaceScript";
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) {
        DialogLoadReport r;
        r.path = dir;
        r.bin_name = dir.string();
        r.error = "InterfaceScript directory not found: " + dir.string();
        out.push_back(std::move(r));
        MLOG_ERROR("[cDialogLoader] %s", r.error.c_str());
        return out;
    }

    // 枚举所有 *.bin
    std::vector<std::filesystem::path> bins;
    for (std::filesystem::directory_iterator it(dir, ec), end;
         !ec && it != end; it.increment(ec)) {
        if (!it->is_regular_file(ec)) continue;
        const auto& p = it->path();
        if (p.extension() != ".bin") continue;
        bins.push_back(p);
    }
    std::sort(bins.begin(), bins.end(), BinLess{});

    MLOG_INFO("[cDialogLoader] found %zu .bin files under %s",
              bins.size(), dir.string().c_str());

    out.reserve(bins.size());
    for (const auto& bin : bins) {
        auto rep = LoadOne(bin, wm);
        const char* level = rep.ok ? "OK" : "FAIL";
        MLOG_INFO("[cDialogLoader] %-4s %-32s type=%-20s roots=%zu point=(%d,%d,%d,%d)",
                  level, rep.bin_name.c_str(),
                  rep.dialog_type.c_str(), rep.root_count,
                  rep.point_x, rep.point_y, rep.point_w, rep.point_h);
        if (!rep.ok) {
            MLOG_ERROR("[cDialogLoader] %s error: %s",
                       rep.bin_name.c_str(), rep.error.c_str());
        }
        out.push_back(std::move(rep));
    }
    return out;
}

DialogLoadStats cDialogLoader::Aggregate(
    const std::vector<DialogLoadReport>& reports) {
    DialogLoadStats s;
    s.total_bins = reports.size();
    for (const auto& r : reports) {
        if (r.ok) ++s.ok; else ++s.failed;
        s.roots_total += r.root_count;
        s.dialogs_added += r.dlg_count;
        s.cimages_loaded += r.cimg_count;
        if (r.has_point) ++s.with_point;
    }
    return s;
}

}  // namespace mxh::ui
