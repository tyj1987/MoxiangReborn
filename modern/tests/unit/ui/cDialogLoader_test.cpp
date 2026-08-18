// tests/unit/ui/cDialogLoader_test.cpp
// M-R3: 装载 132 dialog .bin 链 — 单测验证
//
// 与 cResourceManager_test 一样,本 target mxh_ui_tests 已有 hand-rolled main
// (cResourceManager_test.cpp),所以本测试也用 hand-rolled main-style
// (EXPECT/EXPECT_EQ macros),不用 gtest_main,避免双 main 冲突。
//
// 完成判据 (与 cDialogLoader.hpp 头注释对应):
//   - LoadAll: 132/132 装完 0 exception, 全部 ok=true
//   - 至少 130 个有 #POINT (有 #POINT 才有 1:1 位置意义)
//   - 抽 5 个 sample 验证 #POINT byte-equal 老版 1:1 已知值
//   - 装完后 cWindowManager::dialogCount() == 解析 root 总数

#include "mxh/compat/mh_file_ex.hpp"
#include "mxh/log/mlog.hpp"
#include "mxh/ui/cCheckBox.hpp"
#include "mxh/ui/cComboBox.hpp"
#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cDialogLoader.hpp"
#include "mxh/ui/cGuageBar.hpp"
#include "mxh/ui/cGuagen.hpp"
#include "mxh/ui/cIconDialog.hpp"
#include "mxh/ui/cIconGridDialog.hpp"
#include "mxh/ui/cListCtrl.hpp"
#include "mxh/ui/cListDialog.hpp"
#include "mxh/ui/cObjectGuagen.hpp"
#include "mxh/ui/cPushupButton.hpp"
#include "mxh/ui/cResourceManager.hpp"
#include "mxh/ui/cSpriteAtlas.hpp"
#include "mxh/ui/cTabDialog.hpp"
#include "mxh/ui/cTextArea.hpp"
#include "mxh/ui/cWindowManager.hpp"
#include "mxh/ui/interface_script.hpp"

#include <functional>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

fs::path resolvePlayDHRoot() {
    if (const char* env = std::getenv("MXH_PLAYDH_ROOT")) {
        return fs::path(env);
    }
    #ifdef MXH_PLAYDH_ROOT_TS
    return fs::path(MXH_PLAYDH_ROOT_TS);
    #endif
    return fs::path("C:/moxiang/墨香【源码配套资源】/PlayDH");
}

int g_failures = 0;
int g_passes = 0;

#define EXPECT(cond, msg) do { \
    if (cond) { ++g_passes; } \
    else { ++g_failures; std::cerr << "FAIL: " << msg << " @ " << __FILE__ << ":" << __LINE__ << "\n"; } \
} while (0)

#define EXPECT_EQ(a, b, msg) do { \
    auto _a = (a); auto _b = (b); \
    if (_a == _b) { ++g_passes; } \
    else { ++g_failures; std::cerr << "FAIL: " << msg << " (got=" << _a << " want=" << _b << ") @ " << __FILE__ << ":" << __LINE__ << "\n"; } \
} while (0)

}  // namespace

int main() {
    const auto playdh = resolvePlayDHRoot();
    const auto is_dir = playdh / "Image" / "InterfaceScript";
    std::cout << "[cDialogLoader_test] playdh=" << playdh << "\n";
    std::cout << "[cDialogLoader_test] is_dir=" << is_dir << "\n";

    if (!fs::exists(is_dir)) {
        std::cerr << "SKIP: InterfaceScript dir not found: " << is_dir << "\n";
        std::cout << "\n[cDialogLoader_test] PASS " << g_passes
                  << " / SKIP\n";
        return 0;
    }

    // ---- Test 1: LoadOneParsesRealMainDlg ----
    {
        mxh::ui::cWindowManager wm;
        auto rep = mxh::ui::cDialogLoader::LoadOne(is_dir / "15.bin", wm);
        EXPECT(rep.ok, "LoadOne 15.bin ok");
        EXPECT_EQ(rep.bin_name, std::string("15.bin"), "bin_name == 15.bin");
        // dialog_type 包含 "+Nchild" 后缀 (M-R4.3 装 children cButton)
        EXPECT(rep.dialog_type.find("MAINDLG") == 0,
               "dialog_type starts with MAINDLG");
        EXPECT(rep.root_count >= 1, "root_count >= 1");
        EXPECT(rep.has_point, "has_point true");
        EXPECT_EQ(rep.point_x, 422, "MAINDLG #POINT.x == 422");
        EXPECT_EQ(rep.point_y, 726, "MAINDLG #POINT.y == 726");
        EXPECT_EQ(rep.point_w, 602, "MAINDLG #POINT.w == 602");
        EXPECT_EQ(rep.point_h, 42,  "MAINDLG #POINT.h == 42");
        EXPECT_EQ(wm.dialogCount(), std::size_t(1), "WM holds 1 dialog");
    }

    // ---- Test 2: LoadOneMissingFileReportsError ----
    {
        mxh::ui::cWindowManager wm;
        auto rep = mxh::ui::cDialogLoader::LoadOne(
            is_dir / "DOES_NOT_EXIST.bin", wm);
        EXPECT(!rep.ok, "missing file: not ok");
        EXPECT(!rep.error.empty(), "missing file: error message present");
        EXPECT_EQ(wm.dialogCount(), std::size_t(0), "missing file: WM empty");
    }

    // ---- Test 3: LoadOneCorruptPayloadReportsError ----
    {
        auto bad = is_dir / "_tmp_empty.bin";
        {
            std::ofstream of(bad, std::ios::binary);
            of << "";
        }
        mxh::ui::cWindowManager wm;
        auto rep = mxh::ui::cDialogLoader::LoadOne(bad, wm);
        EXPECT(!rep.ok, "empty file: not ok");
        std::error_code ec;
        fs::remove(bad, ec);  // 显式清理,避免污染
        EXPECT_EQ(wm.dialogCount(), std::size_t(0), "empty file: WM empty");
    }

    // ---- Test 4: LoadAllLoadsEveryBinFile ----
    {
        mxh::ui::cWindowManager wm;
        auto reports = mxh::ui::cDialogLoader::LoadAll(playdh, wm);
        auto stats = mxh::ui::cDialogLoader::Aggregate(reports);

        std::cout << "[cDialogLoader_test] total_bins=" << stats.total_bins
                  << " ok=" << stats.ok
                  << " failed=" << stats.failed
                  << " with_point=" << stats.with_point
                  << " roots_total=" << stats.roots_total
                  << " dialogs_added=" << stats.dialogs_added
                  << " wm.dialogCount=" << wm.dialogCount() << "\n";

        EXPECT(stats.total_bins > 130,
               "expected ≥130 .bin files (legacy has 132+, modern 157)");
        EXPECT_EQ(stats.failed, std::size_t(0), "no failures allowed");
        EXPECT_EQ(stats.ok, stats.total_bins, "all .bin files loaded ok");
        EXPECT(stats.with_point >= 130,
               "expected ≥130 dialogs with #POINT (visual 1:1 position)");
        // WM 只装 root 有 #POINT 的 (无 #POINT 是辅助 dlg / 字符表, 不画)
        EXPECT_EQ(wm.dialogCount(), stats.dialogs_added,
                  "WM dialog count == stats.dialogs_added (有 #POINT 的 root)");
    }

    // ---- Test 5: LoadAllMissingPlayDHReportsError ----
    {
        mxh::ui::cWindowManager wm;
        auto reports = mxh::ui::cDialogLoader::LoadAll(
            fs::path("Z:/this/does/not/exist"), wm);
        auto stats = mxh::ui::cDialogLoader::Aggregate(reports);
        EXPECT_EQ(stats.total_bins, std::size_t(1), "1 report row for missing root");
        EXPECT_EQ(stats.ok, std::size_t(0), "no ok rows for missing root");
        EXPECT_EQ(stats.failed, std::size_t(1), "1 fail row for missing root");
        EXPECT_EQ(wm.dialogCount(), std::size_t(0), "WM empty for missing root");
    }

    // ---- Test 6: LoadAllSampleParseAndApplyByteEqualLegacy ----
    // 抽 5 个 sample, 验证 modern parse + apply 与老 1:1 字节一致
    //   (parser 跟老版共用同一 .bin 源 + 同一 XOR 算法, 所以 #POINT byte-equal
    //    是 byte-equal 老版 cScriptManager::GetInfoFromFile 解析结果)
    {
        std::vector<fs::path> bins;
        for (auto it = fs::directory_iterator(is_dir);
             it != fs::directory_iterator(); ++it) {
            if (!it->is_regular_file()) continue;
            if (it->path().extension() != ".bin") continue;
            bins.push_back(it->path());
        }
        std::sort(bins.begin(), bins.end());
        if (bins.size() < 5) {
            std::cerr << "FAIL: not enough .bin samples: " << bins.size() << "\n";
            ++g_failures;
        } else {
            int checked = 0;
            for (std::size_t i = 0; i < bins.size() && checked < 5; ++i) {
                auto raw = mxh::compat::read_mh_bin(bins[i]);
                if (!raw.ok()) continue;
                std::string_view payload(
                    reinterpret_cast<const char*>(raw.value.data.data()),
                    raw.value.data.size());
                auto parsed = mxh::ui::parse_interface_script(payload);
                if (parsed.roots.empty()) continue;

                const mxh::ui::InterfaceNode* with_point = nullptr;
                for (const auto& root : parsed.roots) {
                    if (root->point.has_value()) { with_point = root.get(); break; }
                }
                if (!with_point) continue;

                mxh::ui::cDialog dlg;
                if (!mxh::ui::apply_legacy_layout(dlg, *with_point, nullptr)) {
                    std::cerr << "FAIL: apply_legacy_layout returned false for "
                              << bins[i] << "\n";
                    ++g_failures;
                    continue;
                }
                const auto& p = *with_point->point;
                EXPECT_EQ(static_cast<std::int32_t>(dlg.absX()), p.x,
                          bins[i].string() + " absX byte-equal");
                EXPECT_EQ(static_cast<std::int32_t>(dlg.absY()), p.y,
                          bins[i].string() + " absY byte-equal");
                EXPECT_EQ(dlg.width(),  static_cast<std::uint16_t>(p.w),
                          bins[i].string() + " width byte-equal");
                EXPECT_EQ(dlg.height(), static_cast<std::uint16_t>(p.h),
                          bins[i].string() + " height byte-equal");
                ++checked;
            }
            EXPECT(checked >= 5, "expected to check at least 5 sample .bin files");
        }
    }

    // ---- Test 7: M-R4.1 LoadAll With Sprite Hook 跨表查装老 sprite ----
    // 模拟 hook 接受 .tif 路径, 返回非 nullptr 假装是 IDISpriteObject*
    // 验证 cimg_count > 0 (loader 调了 hook, 创了 cImage, 持到 g_cimage_owners)
    {
        static int g_mock_sprite_calls = 0;
        static std::string g_mock_last_path;
        g_mock_sprite_calls = 0;
        g_mock_last_path.clear();
        auto mockHook = [](void* /*ctx*/, const std::string& tif_path) -> void* {
            g_mock_sprite_calls += 1;
            g_mock_last_path = tif_path;
            // 假装是 IDISpriteObject* (void*) - 单测不真画, 只验证 hook 被调
            static char mock_sprite = 'S';
            return static_cast<void*>(&mock_sprite);
        };
        mxh::ui::cDialogLoader::SetSpriteLoader(
            static_cast<mxh::ui::LoadSpriteFn>(mockHook), nullptr);

        // M-R1 + M-R2 必须装好 (hook 跨表查需要)
        {
            const auto image_dir = playdh / "Image";
            if (!mxh::ui::cResourceManager::getInstance().allLoaded()) {
                mxh::ui::cResourceManager::getInstance().InitScriptManager(image_dir);
            }
            if (!mxh::ui::cSpriteAtlas::getInstance().loaded()) {
                mxh::ui::cSpriteAtlas::getInstance().Init(playdh);
            }
        }

        mxh::ui::cWindowManager wm;
        auto reports = mxh::ui::cDialogLoader::LoadAll(playdh, wm);
        auto stats = mxh::ui::cDialogLoader::Aggregate(reports);

        std::cout << "[cDialogLoader_test] M-R4 hook mock_sprite_calls=" << g_mock_sprite_calls
                  << " cimages_loaded=" << stats.cimages_loaded << "\n";

        EXPECT(stats.ok == stats.total_bins, "all bins still ok after M-R4 hook");
        EXPECT(g_mock_sprite_calls > 0,
               "M-R4 sprite hook should be called at least once (root with #BASICIMAGE)");
        EXPECT(stats.cimages_loaded > 0,
               "cimages_loaded should track hook hits");
        EXPECT_EQ(static_cast<int>(stats.cimages_loaded), g_mock_sprite_calls,
                  "cimages_loaded == mock_sprite_calls (1:1)");
        EXPECT(!g_mock_last_path.empty(),
               "mock hook should have received at least 1 .tif path");
        EXPECT(g_mock_last_path.find(".tif") != std::string::npos ||
               g_mock_last_path.find(".tga") != std::string::npos,
               "mock hook path should end with .tif/.tga");

        // 清理 hook (后续测试用回 nullptr)
        mxh::ui::cDialogLoader::SetSpriteLoader(nullptr, nullptr);
    }

    // ---- Test 8: M-R4.5 + M-R4.6 12 widget class children 路由命中 ----
    // 验证 12 个 widget class 在 165 dialog .bin 中作为 children 出现时, 能被
    // cDialogLoader 路由到对应 class instance (而不是默默丢弃).
    //
    // M-R4.5 (4): cListDialog / cIconDialog / cGuageBar / cTabDialog
    // M-R4.6 (8): cCheckBox / cPushupButton / cIconGridDialog / cListCtrl
    //            / cComboBox / cTextArea / cGuagen / cObjectGuagen
    //
    // 老版 cScriptManager 行为: children 没 #POINT 也调 Init(0,0,0,0) —
    // 但因为 m_absX=m_absY=0,m_w=m_h=0 不画图, 实际等价于"无 POINT 不挂".
    // 我们的 cDialogLoader 选择跳过无 POINT children (1:1 跟视觉一致).
    {
        mxh::ui::cWindowManager wm;
        auto reports = mxh::ui::cDialogLoader::LoadAll(playdh, wm);
        auto stats = mxh::ui::cDialogLoader::Aggregate(reports);

        std::size_t n_listdlg = 0, n_icondlg = 0, n_guagebar = 0, n_tabdlg = 0;
        std::size_t n_checkbox = 0, n_pushup = 0, n_icongrid = 0, n_listctrl = 0;
        std::size_t n_combo = 0, n_textarea = 0, n_guagen = 0, n_guagene = 0;
        for (const auto& d : wm.dialogs()) {
            if (!d) continue;
            std::function<void(mxh::ui::cWindow*)> walk = [&](mxh::ui::cWindow* w) {
                if (!w) return;
                if (dynamic_cast<mxh::ui::cListDialog*>(w)) ++n_listdlg;
                else if (dynamic_cast<mxh::ui::cIconDialog*>(w)) ++n_icondlg;
                else if (dynamic_cast<mxh::ui::cGuageBar*>(w)) ++n_guagebar;
                else if (dynamic_cast<mxh::ui::cTabDialog*>(w)) ++n_tabdlg;
                else if (dynamic_cast<mxh::ui::cCheckBox*>(w)) ++n_checkbox;
                else if (dynamic_cast<mxh::ui::cPushupButton*>(w)) ++n_pushup;
                else if (dynamic_cast<mxh::ui::cIconGridDialog*>(w)) ++n_icongrid;
                else if (dynamic_cast<mxh::ui::cListCtrl*>(w)) ++n_listctrl;
                else if (dynamic_cast<mxh::ui::cComboBox*>(w)) ++n_combo;
                else if (dynamic_cast<mxh::ui::cTextArea*>(w)) ++n_textarea;
                else if (dynamic_cast<mxh::ui::cObjectGuagen*>(w)) ++n_guagene;
                else if (dynamic_cast<mxh::ui::cGuagen*>(w)) ++n_guagen;
                if (auto* dlg = dynamic_cast<mxh::ui::cDialog*>(w)) {
                    for (std::size_t i = 0; i < dlg->componentCount(); ++i) {
                        walk(dlg->componentAt(i));
                    }
                }
            };
            walk(d.get());
        }

        std::cout << "[cDialogLoader_test] M-R4 widget class children: "
                  << "LISTDLG=" << n_listdlg
                  << " ICONDLG=" << n_icondlg
                  << " GUAGEBAR=" << n_guagebar
                  << " TABDLG=" << n_tabdlg
                  << " CHECKBOX=" << n_checkbox
                  << " PUSHUPBTN=" << n_pushup
                  << " ICONGRIDDLG=" << n_icongrid
                  << " LISTCTRL=" << n_listctrl
                  << " COMBOBOX=" << n_combo
                  << " TEXTAREA=" << n_textarea
                  << " GUAGEN=" << n_guagen
                  << " GUAGENE=" << n_guagene << "\n";

        EXPECT_EQ(stats.ok, stats.total_bins, "all .bin ok in M-R4 test");
        // M-R4.5: 4 widget class
        EXPECT(n_listdlg  >= 28, "LISTDLG children routed to cListDialog");
        EXPECT(n_icondlg  >= 20, "ICONDLG children routed to cIconDialog");
        EXPECT(n_guagebar >= 5,  "GUAGEBAR children routed to cGuageBar");
        EXPECT(n_tabdlg   >= 0,  "TABDLG children routed to cTabDialog");
        // M-R4.6: 8 widget class — 全部至少 1:1 路由命中 (跟老版
        // cScriptManager 行为 1:1 — 老版也无 POINT 不挂 widget)
        EXPECT(n_checkbox >= 30, "CHECKBOX children routed to cCheckBox (79 树, 42+ 有 #POINT)");
        EXPECT(n_pushup   >= 100, "PUSHUPBTN children routed to cPushupButton (171 树, 132+ 有 #POINT)");
        EXPECT(n_icongrid >= 30, "ICONGRIDDLG children routed to cIconGridDialog (52 树, 43+ 有 #POINT)");
        EXPECT(n_listctrl >= 5,  "LISTCTRL children routed to cListCtrl (8 树, 8 有 #POINT)");
        EXPECT(n_combo    >= 10, "COMBOBOX children routed to cComboBox (15 树, 15 有 #POINT)");
        EXPECT(n_textarea >= 40, "TEXTAREA children routed to cTextArea (65 树, 50+ 有 #POINT)");
        EXPECT(n_guagen   >= 10, "GUAGEN children routed to cGuagen (15 树, 15 有 #POINT)");
        EXPECT(n_guagene  >= 25, "GUAGENE children routed to cObjectGuagen (34 树, 31+ 有 #POINT)");

        // 总计 12 class 路由命中累计 ≥ 400 (有 #POINT 子集, 跟老版 1:1)
        const std::size_t total =
            n_listdlg + n_icondlg + n_guagebar + n_tabdlg +
            n_checkbox + n_pushup + n_icongrid + n_listctrl +
            n_combo + n_textarea + n_guagen + n_guagene;
        EXPECT(total >= 380, "12 widget class total children routed");
    }

    std::cout << "\n[cDialogLoader_test] PASS " << g_passes
              << " / FAIL " << g_failures << "\n";
    return g_failures == 0 ? 0 : 1;
}
