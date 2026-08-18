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
#include "mxh/ui/cAni.hpp"               // M-R4.8: cAni stub (full def, no legacy)
#include "mxh/ui/cButton.hpp"
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
            } else if (child->type == "STATIC") {
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto st = std::make_unique<cStatic>();
                st->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(st));
                ++child_count;
            } else if (child->type == "EDITBOX") {
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                cImage* focus = loadImageForImageIdx(child->focus_image_idx,
                                                     child->focus_image_rect);
                auto eb = std::make_unique<cEditBox>();
                eb->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, focus, /*id=*/0);
                if (basic) ++r.cimg_count;
                if (focus) ++r.cimg_count;
                dlg->Add(std::move(eb));
                ++child_count;
            } else if (child->type == "LISTDLG") {
                // M-R4.5: cListDialog (text list) — basicImage 跨表查装
                // list_over / select image 留给 M-R4.5+ (基本 layout 1:1 优先)
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto ld = std::make_unique<cListDialog>();
                ld->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                // InitList 配置行数 + clip — 用默认 10 行 (无 clip 信息从 .bin)
                ld->InitList(/*maxLines=*/10,
                             /*clipX=*/p.x, /*clipY=*/p.y,
                             /*clipW=*/p.w, /*clipH=*/p.h);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(ld));
                ++child_count;
            } else if (child->type == "ICONDLG") {
                // M-R4.5: cIconDialog (icon grid container) — basicImage 跨表查装
                // cell 数量 留默认 0,AddIconCell 留给 M-R4.5+ (基本 layout 1:1 优先)
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto ic = std::make_unique<cIconDialog>();
                ic->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(ic));
                ++child_count;
            } else if (child->type == "GUAGEBAR") {
                // M-R4.5: cGuageBar (draggable progress bar) — basicImage 跨表查装
                // InitGuageBar 默认 horizontal + 100 interval, 跟老版 cGuageBar contract 一致
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto gb = std::make_unique<cGuageBar>();
                gb->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                gb->InitGuageBar(/*interval=*/100, /*vertical=*/false);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(gb));
                ++child_count;
            } else if (child->type == "TABDIALOG" || child->type == "TABDLG") {
                // M-R4.5: cTabDialog (tab container) — 1:1 化但 .bin 中 0 命中
                // (老版实际是 eCHARGUAGEDLG 路由, 但接口 1:1 保留)
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto tb = std::make_unique<cTabDialog>();
                tb->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                tb->InitTab(/*tabNum=*/1);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(tb));
                ++child_count;
            } else if (child->type == "CHECKBOX") {
                // M-R4.6: cCheckBox — 3 类图 (basic + checkBox + check).
                // 1:1 with 老版 cCheckBox::Init(x,y,w,h,basic,checkBox,check,cb,id)
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                cImage* checkBoxImg = loadImageForImageIdx(child->select_image_idx,
                                                         child->select_image_rect);
                cImage* checkImg = loadImageForImageIdx(child->over_image_idx,
                                                       child->over_image_rect);
                auto cb = std::make_unique<cCheckBox>();
                cb->Init(p.x, p.y, static_cast<std::int16_t>(p.w),
                          static_cast<std::int32_t>(p.h),
                          basic, checkBoxImg, checkImg,
                          /*Func=*/{}, /*ID=*/0);
                if (basic) ++r.cimg_count;
                if (checkBoxImg) ++r.cimg_count;
                if (checkImg) ++r.cimg_count;
                dlg->Add(std::move(cb));
                ++child_count;
            } else if (child->type == "PUSHUPBTN") {
                // M-R4.6: cPushupButton (toggle button) — 3 类图.
                // 1:1 with 老版 cPushupButton, 继承 cButton 走 3 类图
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                cImage* over = loadImageForImageIdx(child->over_image_idx,
                                                     child->over_image_rect);
                cImage* press = loadImageForImageIdx(child->press_image_idx,
                                                     child->press_image_rect);
                auto pb = std::make_unique<cPushupButton>();
                pb->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, over, press, /*onClick=*/{}, /*userdata=*/nullptr,
                          /*id=*/0);
                if (basic) ++r.cimg_count;
                if (over) ++r.cimg_count;
                if (press) ++r.cimg_count;
                dlg->Add(std::move(pb));
                ++child_count;
            } else if (child->type == "ICONGRIDDLG") {
                // M-R4.6: cIconGridDialog (drag-drop icon grid) — 1 类图.
                // Init(x,y,w,h,basic, col, row, id) 老版 cIconGridDialog contract
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto ig = std::make_unique<cIconGridDialog>();
                ig->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*col=*/1, /*row=*/1, /*id=*/0);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(ig));
                ++child_count;
            } else if (child->type == "LISTCTRL") {
                // M-R4.6: cListCtrl (multi-column list) — 1 类图 + InitListCtrl
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto lc = std::make_unique<cListCtrl>();
                lc->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                // InitListCtrl 默认 1 column + 10 lines per page
                lc->InitListCtrl(/*wMaxColumns=*/1, /*wLinePerPage=*/10);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(lc));
                ++child_count;
            } else if (child->type == "COMBOBOX") {
                // M-R4.6: cComboBox (dropdown list) — 1 类图 + InitComboList
                // 4 image slots (top/middle/down/over) 留给 M-R4.6+ (无 #INFO)
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto co = std::make_unique<cComboBox>();
                co->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(co));
                ++child_count;
            } else if (child->type == "TEXTAREA") {
                // M-R4.6: cTextArea (scrollable text) — 继承 cDialog.
                // InitTextArea 留给 M-R4.6+ (无 textRelRect / bufSize 信息)
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto ta = std::make_unique<cTextArea>();
                ta->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(ta));
                ++child_count;
            } else if (child->type == "GUAGEN") {
                // M-R4.6: cGuagen (progress bar base) — 1 类图.
                // SetValue / SetPieceImage 留给 M-R4.6+ (无 piece image 信息)
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto gn = std::make_unique<cGuagen>();
                gn->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(gn));
                ++child_count;
            } else if (child->type == "GUAGENE") {
                // M-R4.6: cObjectGuagen (CObjectGuagen effect/interp gauge) — 1 类图
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto ogn = std::make_unique<cObjectGuagen>();
                ogn->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                           static_cast<std::uint16_t>(p.h),
                           basic, /*id=*/0);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(ogn));
                ++child_count;
            } else if (child->type == "LISTDLGEX") {
                // M-R4.7: cListDialogEx (link-list text) — 1 类图 + InitLinkList
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto ld = std::make_unique<cListDialogEx>();
                ld->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                ld->InitLinkList(/*maxLines=*/10);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(ld));
                ++child_count;
            } else if (child->type == "MUGONGDLG") {
                // M-R4.7: cMugongDialog (skill slots) — 1 类图.
                // 继承 cDialog 没自己 Init, 用 cDialog::Init
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto md = std::make_unique<cMugongDialog>();
                md->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(md));
                ++child_count;
            } else if (child->type == "QUESTDLG") {
                // M-R4.7: cQuestDialog (quest log) — 1 类图.
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto qd = std::make_unique<cQuestDialog>();
                qd->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(qd));
                ++child_count;
            } else if (child->type == "WANTEDDLG") {
                // M-R4.7: cWantedDialog (wanted list) — 1 类图.
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto wd = std::make_unique<cWantedDialog>();
                wd->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(wd));
                ++child_count;
            } else if (child->type == "JOURNALDLG") {
                // M-R4.7: cJournalDialog (quest journal) — 1 类图.
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto jd = std::make_unique<cJournalDialog>();
                jd->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(jd));
                ++child_count;
            } else if (child->type == "ITEMSHOPGRIDDLG") {
                // M-R4.7: cItemShopGridDialog (item shop drag-drop grid) — 1 类图.
                // 继承 cIconGridDialog, 用其 Init(x, y, w, h, basic, id)
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto isg = std::make_unique<cItemShopGridDialog>();
                isg->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                           static_cast<std::uint16_t>(p.h),
                           basic, /*id=*/0);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(isg));
                ++child_count;
            } else if (child->type == "SPIN") {
                // M-R4.7: cSpin (number spinner) — 1 类图 (basic, 继承 cEditBox
                // 但 cSpin::Init 签名只接 basic + callback, focus 走 cEditBox 默认)
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto sp = std::make_unique<cSpin>();
                sp->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*callback=*/{}, /*id=*/0);
                sp->InitSpin(/*spinStrSize=*/10, /*strSize=*/10);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(sp));
                ++child_count;
            } else if (child->type == "DLG") {
                // M-R4.7: 嵌套 cDialog as child (老版 eDLG 路由).
                // 跟 root 一样装 basicImage + apply_legacy_layout
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto nested = std::make_unique<cDialog>();
                const bool applied = apply_legacy_layout(*nested, *child, basic);
                if (!applied) {
                    // nested 没 #POINT — skip, 不挂
                    continue;
                }
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(nested));
                ++child_count;
            } else if (child->type == "WEAREDDLG") {
                // M-R4.8: cWearedExDialog 1 类图 (继承 cIconDialog, 完整
                // 1:1 port 在 src/ui/wearedexdialog.{hpp,cpp}).
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto we = std::make_unique<cWearedExDialog>();
                we->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(we));
                ++child_count;
            } else if (child->type == "PRIVATEWAREHOUSEDLG") {
                // M-R4.8: cPrivateWarehouseDialog 1 类图 (继承 cDialog,
                // 完整 1:1 port 在 src/ui/privatewarehousedialog.{hpp,cpp}).
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto pwd = std::make_unique<cPrivateWarehouseDialog>();
                pwd->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                           static_cast<std::uint16_t>(p.h),
                           basic, /*id=*/0);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(pwd));
                ++child_count;
            } else if (child->type == "MUNPAMARKDLG") {
                // M-R4.8: cMunpaMarkDialog 1 类图 (继承 cDialog, 完整
                // 1:1 port 在 src/ui/munpamarkdialog.{hpp,cpp}).
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto mm = std::make_unique<cMunpaMarkDialog>();
                mm->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(mm));
                ++child_count;
            } else if (child->type == "SHOPITEMINVENGRID") {
                // M-R4.8: cItemShopInven 1 类图 (继承 cIconGridDialog, 完整
                // 1:1 port 在 modern cItemShopInven.hpp + 1:1 老版逻辑 TODO).
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto isi = std::make_unique<cItemShopInven>();
                isi->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                           static_cast<std::uint16_t>(p.h),
                           basic, /*col=*/1, /*row=*/1, /*id=*/0);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(isi));
                ++child_count;
            } else if (child->type == "ANI") {
                // M-R4.8: cAni 1 类图 (继承 cWindow, 完整 1:1 port 在
                // modern cAni.hpp + 老版 frame-loop TODO).
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto ani = std::make_unique<cAni>();
                ani->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                           static_cast<std::uint16_t>(p.h),
                           basic, /*id=*/0);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(ani));
                ++child_count;
            } else if (child->type == "SURYUNDLG") {
                // M-R4.8: cSuryunDialog 1 类图 (继承 cDialog, 完整
                // 1:1 port 在 src/ui/suryundialog.{hpp,cpp}, 无 InitTab).
                cImage* basic = loadImageForImageIdx(child->basic_image_idx,
                                                     child->basic_image_rect);
                auto sy = std::make_unique<cSuryunDialog>();
                sy->Init(p.x, p.y, static_cast<std::uint16_t>(p.w),
                          static_cast<std::uint16_t>(p.h),
                          basic, /*id=*/0);
                if (basic) ++r.cimg_count;
                dlg->Add(std::move(sy));
                ++child_count;
            }
            // data-only 类型 (PAGE / NPC / MOTION) 跳过, 不是 widget
            // 老版 cScriptManager 也没单独路由这些 (1:1 行为).
            // GUAGE / LIST 0 命中, 不实现.
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
