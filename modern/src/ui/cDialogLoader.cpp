// mxh/ui/cDialogLoader.cpp
// M-R3: 装载 132 dialog .bin 链 — 实现见 header.

#include "mxh/ui/cDialogLoader.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <string>
#include <system_error>
#include <unordered_map>

#include "mxh/compat/mh_file_ex.hpp"
#include "mxh/log/mlog.hpp"
#include "mxh/ui/cAni.hpp"               // M-R4.8: cAni stub (full def, no legacy)
#include "mxh/ui/cButton.hpp"
#include "mxh/ui/bigmapdlg.hpp"
#include "mxh/ui/ccharacterdialog.hpp"
#include "mxh/ui/cchatdialog.hpp"
#include "mxh/ui/cinventoryexdialog.hpp"
#include "mxh/ui/cmainbardialog.hpp"
#include "mxh/ui/cmakdial.hpp"
#include "mxh/ui/guagedialog.hpp"
#include "mxh/ui/mugongsuryundialog.hpp"
#include "mxh/ui/cCheckBox.hpp"
#include "mxh/ui/cComboBox.hpp"
#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cEditBox.hpp"
#include "mxh/ui/cGuageBar.hpp"
#include "mxh/ui/cGuagen.hpp"
#include "mxh/ui/cIconDialog.hpp"
#include "mxh/ui/cIconGridDialog.hpp"
#include "mxh/ui/cImage.hpp"
#include "mxh/ui/cItemShopGridDialog.hpp"
#include "mxh/ui/cItemShopInven.hpp"     // M-R4.8: cItemShopInven stub (full def, no legacy)
#include "mxh/ui/cJournalDialog.hpp"
#include "mxh/ui/cListCtrl.hpp"
#include "mxh/ui/cListDialog.hpp"
#include "mxh/ui/cListDialogEx.hpp"
#include "mxh/ui/cMugongDialog.hpp"
#include "mxh/ui/cObjectGuagen.hpp"
#include "mxh/ui/cPushupButton.hpp"
#include "mxh/ui/cQuestDialog.hpp"
#include "mxh/ui/cResourceManager.hpp"
#include "mxh/ui/cSpin.hpp"
#include "mxh/ui/cSpriteAtlas.hpp"
#include "mxh/ui/cStatic.hpp"
#include "mxh/ui/cTabDialog.hpp"
#include "mxh/ui/cTextArea.hpp"
#include "mxh/ui/cWantedDialog.hpp"
#include "mxh/ui/cWindowManager.hpp"
#include "mxh/ui/interface_script.hpp"
#include "legacy_window_ids.hpp"
// M-R4.8: 4 stub class (cWearedExDialog/cMunpaMarkDialog/cPrivateWarehouseDialog/
// cSuryunDialog) 走 legacy 1:1 port lowercase 头 (M-R4 era 完整版, 含
// ctor/dtor/AddItem/DeleteItem/SetMunpaMark 等). 实体定义由对应 legacy
// .cpp 提供, cDialogLoader.cpp 只拿到类声明 (ctor 声明, 不嵌入 ctor
// 实体), 跟 legacy .cpp 单一 ctor 实体 — 避免 LNK2005. 文件名 lowercase
// 跟老 1:1 port 风格一致 (M-R4 era 1:1 命名).
#include "mxh/ui/wearedexdialog.hpp"
#include "mxh/ui/munpamarkdialog.hpp"
#include "mxh/ui/privatewarehousedialog.hpp"
#include "mxh/ui/suryundialog.hpp"

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

// G2 follow-up: cache cImage by (image_idx, rect). Without this, 165 dialogs
// each load 4-20 children from the same 1.tif fallback sprite, allocating
// 4MB DX11 GPU textures per call (165 * ~10 = 1650+ textures, ~6.6GB GPU,
// which crashes the driver on a 4GB Intel Arc B580). Cache returns the
// same cImage for the same image_idx+rect.
struct ImageKey {
    std::int32_t idx = 0;
    std::int32_t l = 0, t = 0, r = 0, b = 0;
    bool operator==(const ImageKey& o) const noexcept {
        return idx == o.idx && l == o.l && t == o.t && r == o.r && b == o.b;
    }
};
struct ImageKeyHash {
    std::size_t operator()(const ImageKey& k) const noexcept {
        std::size_t h = static_cast<std::size_t>(k.idx);
        h ^= static_cast<std::size_t>(k.l) + 0x9e3779b9u + (h << 6) + (h >> 2);
        h ^= static_cast<std::size_t>(k.t) + 0x9e3779b9u + (h << 6) + (h >> 2);
        h ^= static_cast<std::size_t>(k.r) + 0x9e3779b9u + (h << 6) + (h >> 2);
        h ^= static_cast<std::size_t>(k.b) + 0x9e3779b9u + (h << 6) + (h >> 2);
        return h;
    }
};
std::unordered_map<ImageKey, cImage*, ImageKeyHash> g_cimage_cache;
std::unordered_map<std::string, void*> g_sprite_by_path;

void applyLegacyIdentity(cWindow& window, const InterfaceNode& node) {
    if (node.id.has_value()) {
        window.setLegacyId(*node.id);
        if (const auto numeric = resolve_legacy_window_id(*node.id)) {
            window.setId(*numeric);
        }
    }
    if (node.func.has_value()) {
        window.setLegacyFunc(*node.func);
    }
    if (node.font_idx_set) {
        const auto font = static_cast<std::uint16_t>(
            std::clamp(node.font_idx, 0, 0xFFFF));
        if (auto* label = dynamic_cast<cStatic*>(&window)) {
            label->SetFontIdx(font);
        } else if (auto* edit = dynamic_cast<cEditBox*>(&window)) {
            edit->SetFontIdx(font);
        } else if (auto* button = dynamic_cast<cButton*>(&window)) {
            button->SetFontIdx(font);
        }
    }
}

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
    const auto& ir = *rect;
    const ImageKey key{image_idx, ir.left, ir.top, ir.right, ir.bottom};
    if (auto it = g_cimage_cache.find(key); it != g_cimage_cache.end()) {
        return it->second;
    }
    const auto hp = cResourceManager::getInstance().getHardPath(
        image_idx, PathFileType::HardPath);
    if (!hp.has_value()) return nullptr;
    const auto info = cSpriteAtlas::getInstance().getInfo(hp->atlas_idx);
    if (!info.has_value()) return nullptr;
    const auto tif_abs = cSpriteAtlas::getInstance().resolvePath(*info);
    if (!std::filesystem::exists(tif_abs)) return nullptr;
    const auto tif_key = tif_abs.string();
    void* sprite = nullptr;
    if (auto sit = g_sprite_by_path.find(tif_key); sit != g_sprite_by_path.end()) {
        sprite = sit->second;
    } else {
        sprite = g_loadSprite(g_loadSpriteCtx, tif_key);
        if (sprite) g_sprite_by_path.emplace(tif_key, sprite);
    }
    if (!sprite) return nullptr;
    auto owner = std::make_unique<cImage>();
    owner->SetSource(ir.left, ir.top, ir.right, ir.bottom,
                     info->width, info->height);
    owner->SetSpriteObject(sprite);
    cImage* out = owner.get();
    g_cimage_owners.push_back(std::move(owner));
    g_cimage_cache.emplace(key, out);
    return out;
}

} // namespace

cImage* cDialogLoader::LoadLegacyImage(std::int32_t hard_idx) {
    const auto hard_path = cResourceManager::getInstance().getHardPath(
        hard_idx, PathFileType::HardPath);
    if (!hard_path) return nullptr;
    return loadImageForImageIdx(hard_idx, ImageRect{
        hard_path->left, hard_path->top, hard_path->right, hard_path->bottom});
}

namespace {

bool addInterfaceNode(cWindow& parent, const InterfaceNode& node,
                      DialogLoadReport& report, ResolutionMode mode,
                      std::size_t depth = 0) {
            // InterfaceScript is untrusted profile data.  A malformed
            // nested tree must fail fast instead of exhausting the stack or
            // hanging the client during GameLoading/UI startup.
            constexpr std::size_t kMaxInterfaceDepth = 128;
            if (depth > kMaxInterfaceDepth) {
                report.error = "InterfaceScript nesting exceeds safety limit";
                return false;
            }
            if (!node.point.has_value()) return false;
            const auto& p = *node.point;
            const auto before_count = parent.childCount();
            // 路由 widget class by type
            if (node.type == "BTN") {
                cImage* basic  = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                cImage* over   = loadImageForImageIdx(node.over_image_idx,
                                                     node.over_image_rect);
                cImage* press  = loadImageForImageIdx(node.press_image_idx,
                                                     node.press_image_rect);
                auto btn = std::make_unique<cButton>();
                btn->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, over, press, /*onClick=*/{}, /*userdata=*/nullptr,
                          /*id=*/0);
                if (basic)  ++report.cimg_count;
                if (over)   ++report.cimg_count;
                if (press)  ++report.cimg_count;
                parent.Add(std::move(btn));
            } else if (node.type == "STATIC") {
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto st = std::make_unique<cStatic>();
                st->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++report.cimg_count;
                parent.Add(std::move(st));
            } else if (node.type == "EDITBOX") {
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                cImage* focus = loadImageForImageIdx(node.focus_image_idx,
                                                     node.focus_image_rect);
                auto eb = std::make_unique<cEditBox>();
                eb->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, focus, /*id=*/0);
                if (basic) ++report.cimg_count;
                if (focus) ++report.cimg_count;
                parent.Add(std::move(eb));
            } else if (node.type == "LISTDLG") {
                // M-R4.5: cListDialog (text list) — basicImage 跨表查装
                // list_over / select image 留给 M-R4.5+ (基本 layout 1:1 优先)
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto ld = std::make_unique<cListDialog>();
                ld->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                // InitList 配置行数 + clip — 用默认 10 行 (无 clip 信息从 .bin)
                ld->InitList(/*maxLines=*/10,
                             /*clipX=*/p.x, /*clipY=*/p.y,
                             /*clipW=*/p.w, /*clipH=*/p.h);
                if (basic) ++report.cimg_count;
                parent.Add(std::move(ld));
            } else if (node.type == "ICONDLG") {
                // M-R4.5: cIconDialog (icon grid container) — basicImage 跨表查装
                // M-R4.9 (2026-08-22): when no explicit #INFO cell data is in
                // the .bin, default to one full-area cell matching the dialog
                // rect. This matches legacy cScriptManager::GetInfoFromFile
                // eICONDLG behaviour: it always creates at least one cell so
                // GetCellNum() / PtInCell() / AddIcon() work before explicit
                // layout data is loaded at runtime.
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto ic = std::make_unique<cIconDialog>();
                ic->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                // SetCellNum(1) populates m_cells with one empty cell;
                // AddIconCell then writes the dialog rect into that cell.
                // This matches legacy cScriptManager eICONDLG behaviour.
                ic->SetCellNum(1);
                ic->AddIconCell(/*x=*/0, /*y=*/0,
                                static_cast<std::int32_t>(p.w),
                                static_cast<std::int32_t>(p.h));
                if (basic) ++report.cimg_count;
                parent.Add(std::move(ic));
            } else if (node.type == "GUAGEBAR") {
                // M-R4.5: cGuageBar (draggable progress bar) — basicImage 跨表查装
                // InitGuageBar 默认 horizontal + 100 interval, 跟老版 cGuageBar contract 一致
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto gb = std::make_unique<cGuageBar>();
                gb->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                gb->InitGuageBar(/*interval=*/100, /*vertical=*/false);
                if (basic) ++report.cimg_count;
                parent.Add(std::move(gb));
            } else if (node.type == "TABDIALOG" || node.type == "TABDLG") {
                // M-R4.5: cTabDialog (tab container) — 1:1 化但 .bin 中 0 命中
                // (老版实际是 eCHARGUAGEDLG 路由, 但接口 1:1 保留)
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto tb = std::make_unique<cTabDialog>();
                tb->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                tb->InitTab(/*tabNum=*/1);
                if (basic) ++report.cimg_count;
                parent.Add(std::move(tb));
            } else if (node.type == "CHECKBOX") {
                // M-R4.6: cCheckBox — 3 类图 (basic + checkBox + check).
                // 1:1 with 老版 cCheckBox::Init(x,y,w,h,basic,checkBox,check,cb,id)
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                cImage* checkBoxImg = loadImageForImageIdx(node.select_image_idx,
                                                         node.select_image_rect);
                cImage* checkImg = loadImageForImageIdx(node.over_image_idx,
                                                       node.over_image_rect);
                auto cb = std::make_unique<cCheckBox>();
                cb->Init(p.x, p.y, static_cast<std::int16_t>(p.w),
                          static_cast<std::int32_t>(p.h),
                          basic, checkBoxImg, checkImg,
                          /*Func=*/{}, /*ID=*/0);
                if (basic) ++report.cimg_count;
                if (checkBoxImg) ++report.cimg_count;
                if (checkImg) ++report.cimg_count;
                parent.Add(std::move(cb));
            } else if (node.type == "PUSHUPBTN") {
                // M-R4.6: cPushupButton (toggle button) — 3 类图.
                // 1:1 with 老版 cPushupButton, 继承 cButton 走 3 类图
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                cImage* over = loadImageForImageIdx(node.over_image_idx,
                                                     node.over_image_rect);
                cImage* press = loadImageForImageIdx(node.press_image_idx,
                                                     node.press_image_rect);
                auto pb = std::make_unique<cPushupButton>();
                pb->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, over, press, /*onClick=*/{}, /*userdata=*/nullptr,
                          /*id=*/0);
                if (basic) ++report.cimg_count;
                if (over) ++report.cimg_count;
                if (press) ++report.cimg_count;
                parent.Add(std::move(pb));
            } else if (node.type == "ICONGRIDDLG") {
                // M-R4.6: cIconGridDialog (drag-drop icon grid) — 1 类图.
                // Init(x,y,w,h,basic, col, row, id) 老版 cIconGridDialog contract
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto ig = std::make_unique<cIconGridDialog>();
                ig->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, node.grid_cols, node.grid_rows, /*id=*/0);
                if (node.init_grid) {
                    const auto& g = *node.init_grid;
                    ig->InitGrid(g.x, g.y, static_cast<std::uint16_t>(std::clamp(g.w, 0, 65535)),
                                 static_cast<std::uint16_t>(std::clamp(g.h, 0, 65535)),
                                 node.grid_border_x, node.grid_border_y);
                }
                if (basic) ++report.cimg_count;
                parent.Add(std::move(ig));
            } else if (node.type == "LISTCTRL") {
                // M-R4.6: cListCtrl (multi-column list) — 1 类图 + InitListCtrl
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto lc = std::make_unique<cListCtrl>();
                lc->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                // InitListCtrl 默认 1 column + 10 lines per page
                lc->InitListCtrl(/*wMaxColumns=*/1, /*wLinePerPage=*/10);
                if (basic) ++report.cimg_count;
                parent.Add(std::move(lc));
            } else if (node.type == "COMBOBOX") {
                // M-R4.6: cComboBox (dropdown list) — 1 类图 + InitComboList
                // 4 image slots (top/middle/down/over) 留给 M-R4.6+ (无 #INFO)
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto co = std::make_unique<cComboBox>();
                co->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++report.cimg_count;
                parent.Add(std::move(co));
            } else if (node.type == "TEXTAREA") {
                // M-R4.6: cTextArea (scrollable text) — 继承 cDialog.
                // InitTextArea 留给 M-R4.6+ (无 textRelRect / bufSize 信息)
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto ta = std::make_unique<cTextArea>();
                ta->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++report.cimg_count;
                parent.Add(std::move(ta));
            } else if (node.type == "GUAGEN") {
                // M-R4.6: cGuagen (progress bar base) — 1 类图.
                // SetValue / SetPieceImage 留给 M-R4.6+ (无 piece image 信息)
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto gn = std::make_unique<cGuagen>();
                gn->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++report.cimg_count;
                parent.Add(std::move(gn));
            } else if (node.type == "GUAGENE") {
                // M-R4.6: cObjectGuagen (CObjectGuagen effect/interp gauge) — 1 类图
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto ogn = std::make_unique<cObjectGuagen>();
                ogn->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                           static_cast<std::uint16_t>(p.h),
                           basic, /*id=*/0);
                if (basic) ++report.cimg_count;
                parent.Add(std::move(ogn));
            } else if (node.type == "LISTDLGEX") {
                // M-R4.7: cListDialogEx (link-list text) — 1 类图 + InitLinkList
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto ld = std::make_unique<cListDialogEx>();
                ld->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                ld->InitLinkList(/*maxLines=*/10);
                if (basic) ++report.cimg_count;
                parent.Add(std::move(ld));
            } else if (node.type == "MUGONGDLG") {
                // M-R4.7: cMugongDialog (skill slots) — 1 类图.
                // 继承 cDialog 没自己 Init, 用 cDialog::Init
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto md = std::make_unique<cMugongDialog>();
                md->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++report.cimg_count;
                parent.Add(std::move(md));
            } else if (node.type == "QUESTDLG") {
                // M-R4.7: cQuestDialog (quest log) — 1 类图.
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto qd = std::make_unique<cQuestDialog>();
                qd->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++report.cimg_count;
                parent.Add(std::move(qd));
            } else if (node.type == "WANTEDDLG") {
                // M-R4.7: cWantedDialog (wanted list) — 1 类图.
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto wd = std::make_unique<cWantedDialog>();
                wd->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++report.cimg_count;
                parent.Add(std::move(wd));
            } else if (node.type == "JOURNALDLG") {
                // M-R4.7: cJournalDialog (quest journal) — 1 类图.
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto jd = std::make_unique<cJournalDialog>();
                jd->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++report.cimg_count;
                parent.Add(std::move(jd));
            } else if (node.type == "ITEMSHOPGRIDDLG") {
                // M-R4.7: cItemShopGridDialog (item shop drag-drop grid) — 1 类图.
                // 继承 cIconGridDialog, 用其 Init(x, y, w, h, basic, id)
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto isg = std::make_unique<cItemShopGridDialog>();
                isg->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                           static_cast<std::uint16_t>(p.h),
                           basic, /*id=*/0);
                if (basic) ++report.cimg_count;
                parent.Add(std::move(isg));
            } else if (node.type == "SPIN") {
                // M-R4.7: cSpin (number spinner) — 1 类图 (basic, 继承 cEditBox
                // 但 cSpin::Init 签名只接 basic + callback, focus 走 cEditBox 默认)
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto sp = std::make_unique<cSpin>();
                sp->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*callback=*/{}, /*id=*/0);
                sp->InitSpin(/*spinStrSize=*/10, /*strSize=*/10);
                if (basic) ++report.cimg_count;
                parent.Add(std::move(sp));
            } else if (node.type == "DLG") {
                // M-R4.7: 嵌套 cDialog as child (老版 eDLG 路由).
                // 跟 root 一样装 basicImage + apply_legacy_layout
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto nested = std::make_unique<cDialog>();
                const bool applied = apply_legacy_layout(*nested, node, basic, mode);
                if (!applied) {
                    // nested 没 #POINT — skip, 不挂
                    return false;
                }
                if (basic) ++report.cimg_count;
                parent.Add(std::move(nested));
            } else if (node.type == "WEAREDDLG") {
                // M-R4.8: cWearedExDialog 1 类图 (继承 cIconDialog, 完整
                // 1:1 port 在 src/ui/wearedexdialog.{hpp,cpp}).
                // M-R4.9 (2026-08-22): same default cell as ICONDLG so
                // PtInCell / AddIcon work pre-runtime layout.
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto we = std::make_unique<cWearedExDialog>();
                we->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                we->SetCellNum(1);
                we->AddIconCell(/*x=*/0, /*y=*/0,
                                static_cast<std::int32_t>(p.w),
                                static_cast<std::int32_t>(p.h));
                if (basic) ++report.cimg_count;
                parent.Add(std::move(we));
            } else if (node.type == "PRIVATEWAREHOUSEDLG") {
                // M-R4.8: cPrivateWarehouseDialog 1 类图 (继承 cDialog,
                // 完整 1:1 port 在 src/ui/privatewarehousedialog.{hpp,cpp}).
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto pwd = std::make_unique<cPrivateWarehouseDialog>();
                pwd->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                           static_cast<std::uint16_t>(p.h),
                           basic, /*id=*/0);
                if (basic) ++report.cimg_count;
                parent.Add(std::move(pwd));
            } else if (node.type == "MUNPAMARKDLG") {
                // M-R4.8: cMunpaMarkDialog 1 类图 (继承 cDialog, 完整
                // 1:1 port 在 src/ui/munpamarkdialog.{hpp,cpp}).
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto mm = std::make_unique<cMunpaMarkDialog>();
                mm->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++report.cimg_count;
                parent.Add(std::move(mm));
            } else if (node.type == "SHOPITEMINVENGRID") {
                // M-R4.8: cItemShopInven 1 类图 (继承 cIconGridDialog, 完整
                // 1:1 port 在 modern cItemShopInven.hpp + 1:1 老版逻辑 TODO).
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto isi = std::make_unique<cItemShopInven>();
                isi->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                           static_cast<std::uint16_t>(p.h),
                           basic, node.grid_cols, node.grid_rows, /*id=*/0);
                if (node.init_grid) {
                    const auto& g = *node.init_grid;
                    isi->InitGrid(g.x, g.y, static_cast<std::uint16_t>(std::clamp(g.w, 0, 65535)),
                                  static_cast<std::uint16_t>(std::clamp(g.h, 0, 65535)),
                                  node.grid_border_x, node.grid_border_y);
                }
                if (basic) ++report.cimg_count;
                parent.Add(std::move(isi));
            } else if (node.type == "ANI") {
                // M-R4.8: cAni 1 类图 (继承 cWindow, 完整 1:1 port 在
                // modern cAni.hpp + 老版 frame-loop TODO).
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto ani = std::make_unique<cAni>();
                ani->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                           static_cast<std::uint16_t>(p.h),
                           basic, /*id=*/0);
                if (basic) ++report.cimg_count;
                parent.Add(std::move(ani));
            } else if (node.type == "SURYUNDLG") {
                // M-R4.8: cSuryunDialog 1 类图 (继承 cDialog, 完整
                // 1:1 port 在 src/ui/suryundialog.{hpp,cpp}, 无 InitTab).
                cImage* basic = loadImageForImageIdx(node.basic_image_idx,
                                                     node.basic_image_rect);
                auto sy = std::make_unique<cSuryunDialog>();
                sy->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++report.cimg_count;
                parent.Add(std::move(sy));
            }
            if (parent.childCount() > before_count) {
                if (cWindow* added = parent.childAt(parent.childCount() - 1)) {
                    applyLegacyIdentity(*added, node);
                }
            }
            // data-only 类型 (PAGE / NPC / MOTION) 跳过, 不是 widget
            // 老版 cScriptManager 也没单独路由这些 (1:1 行为).
            // GUAGE / LIST 0 命中, 不实现.

    if (parent.childCount() <= before_count) return false;
    cWindow* added = parent.childAt(parent.childCount() - 1);
    if (!added) return false;

    for (const auto& nested : node.children) {
        const auto nested_before = added->childCount();
        if (!addInterfaceNode(*added, *nested, report, mode, depth + 1)) continue;
        if (auto* gauge = dynamic_cast<cGuageBar*>(added)) {
            if (cWindow* gauge_child = added->childAt(nested_before)) {
                if (dynamic_cast<cButton*>(gauge_child)) {
                    gauge->Add(gauge_child);
                }
            }
        }
    }
    return true;
}

std::unique_ptr<cDialog> makeDialogRoot(const InterfaceNode& node) {
    // Exact cases from recovered legacy cScriptManager::GetDlgInfoFromFile.
    // Keep the generic fallback for root types whose dedicated modern class
    // has not yet reached runtime parity; never invent a substitute class.
    if (node.type == "CHARGUAGEDLG") {
        return std::make_unique<cGuageDialog>();
    }
    if (node.type == "LISTDLG") {
        return std::make_unique<cListDialog>();
    }
    if (node.type == "LISTDLGEX") {
        return std::make_unique<cListDialogEx>();
    }
    if (node.type == "CHARINFODLG") {
        return std::make_unique<cCharacterDialog>();
    }
    if (node.type == "MUGONGSURYUNDLG") {
        return std::make_unique<cMugongSuryunDialog>();
    }
    if (node.type == "MAINDLG") {
        return std::make_unique<cMainBarDialog>();
    }
    if (node.type == "INVENTORYDLG") {
        return std::make_unique<cInventoryExDialog>();
    }
    if (node.type == "CHATDLG") {
        return std::make_unique<cChatDialog>();
    }
    if (node.type == "BIGMAPDLG") {
        return std::make_unique<cBigMapDlg>();
    }
    if (node.type == "CHARMAKEDLG") {
        return std::make_unique<cCharMakeDlg>();
    }
    return std::make_unique<cDialog>();
}
}  // namespace

// M-R7 (G3): header declares single overload with default arg
// `ResolutionMode mode = kDefaultResolutionMode`, 所以 callers 传 2/3 arg 都行.
// 只实现一次, 不写 2-arg wrapper 避免 LNK/overload 冲突.
DialogLoadReport cDialogLoader::LoadOne(const std::filesystem::path& bin_path,
                                        cWindowManager& wm,
                                        ResolutionMode mode) {
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

        auto dlg = makeDialogRoot(*root);
        const bool applied = apply_legacy_layout(*dlg, *root,
                                                  /*basicImage=*/cimg,
                                                  mode);
        if (!applied) {
            r.error = "apply_legacy_layout failed for root[" +
                      std::to_string(i) + "]";
            return r;
        }
        applyLegacyIdentity(*dlg, *root);
        // BigMap.bin identifies its root by type rather than an explicit
        // #ID token.  Preserve the legacy root name so runtime activation
        // can address the shipped map dialog just like other InterfaceScript
        // roots.
        if (root->type == "BIGMAPDLG" && !root->id.has_value()) {
            dlg->setLegacyId("BIGMAPDLG");
        }
        dlg->setName(r.bin_name);
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
            if (addInterfaceNode(*dlg, *child, r, mode)) {
                ++child_count;
            }
        }
        if (!r.error.empty()) return r;
        if (child_count > 0) {
            r.dialog_type += "+" + std::to_string(child_count) + "child";
        }

        wm.AddDialog(std::move(dlg));
        r.dlg_count += 1;  // 用于 stats 统计 AddDialog 成功次数
    }

    r.ok = true;
    return r;
}

// M-R7 (G3): 单一实现 (default arg 让 2/3 arg call 都走这里)
std::vector<DialogLoadReport> cDialogLoader::LoadAll(
    const std::filesystem::path& playdh_root, cWindowManager& wm,
    ResolutionMode mode) {
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
        auto rep = LoadOne(bin, wm, mode);
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
