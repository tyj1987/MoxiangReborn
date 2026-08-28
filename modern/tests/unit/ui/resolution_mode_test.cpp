// M-R7 (G3) resolution_mode 头less 单测 — 独立可跑.
//
// 验证 (1)-(7) 7 个 sub-test (42 个 EXPECT 断言):
//   (1) apply_legacy_layout 3 参数 (default mode = High) 用 #POINT 1:1
//   (2) apply_legacy_layout 4 参数 (mode = Low) + node.point_low 存在时用 #POINT_ 1:1
//   (3) apply_legacy_layout 4 参数 (mode = Low) + node 无 point_low 时仍用 #POINT 1:1
//   (4) apply_legacy_layout 4 参数 (mode = High) + node 有 point_low 时仍用 #POINT 1:1
//   (5) LoadAll/LoadOne 接受 mode 参数, 1:1 行为
//   (6) cWindowManager::OnResolutionChange + SetCurrentResolutionMode 1:1 切换
//   (7) detect_from_screen_size + mode_size + mode_name 1:1 头less 切换
//
// 跟 cDialogLoader_test 拆开, 是为了隔离: 之前 R-36 是 src/ui/cWindow.hpp 缺
// m_name 字段 ABI 错位 (现修), R-37 PUSHUPBTN 还没修但 15.bin 不用 PUSHUPBTN.

#include "mxh/compat/mh_file_ex.hpp"
#include "mxh/log/mlog.hpp"
#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cDialogLoader.hpp"
#include "mxh/ui/cWindowManager.hpp"
#include "mxh/ui/interface_script.hpp"
#include "mxh/ui/resolution_mode.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

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

fs::path resolvePlayDHRoot() {
    if (const char* env = std::getenv("MXH_PLAYDH_ROOT")) {
        return fs::path(env);
    }
    std::error_code ec;
    auto current = fs::absolute(fs::current_path(ec), ec);
    for (int depth = 0; !ec && depth < 10 && !current.empty(); ++depth) {
        const auto candidate = current / "modern" / "data" / "PlayDH";
        std::error_code candidate_ec;
        if (fs::is_directory(candidate, candidate_ec)) return candidate;
        const auto parent = current.parent_path();
        if (parent == current) break;
        current = parent;
    }
    return {};
}

int main() {
    const auto playdh = resolvePlayDHRoot();
    const auto is_dir = playdh / "Image" / "InterfaceScript";

    if (!fs::exists(is_dir)) {
        std::cerr << "SKIP: PlayDH not found at " << is_dir << "\n";
        return 0;
    }

    // (1) default 3 参数 apply_legacy_layout 走 #POINT 1:1
    {
        mxh::ui::InterfaceNode node;
        mxh::ui::WindowRect p{100, 200, 300, 400};
        mxh::ui::WindowRect p_low{50, 100, 150, 200};
        node.point = p;
        node.point_low = p_low;
        mxh::ui::cDialog dlg;
        EXPECT(mxh::ui::apply_legacy_layout(dlg, node, nullptr),
               "default 3-param apply_legacy_layout ok");
        EXPECT_EQ(dlg.absX(), 100, "default mode uses #POINT.x (not #POINT_)");
        EXPECT_EQ(dlg.absY(), 200, "default mode uses #POINT.y (not #POINT_)");
        EXPECT_EQ(dlg.width(), static_cast<std::uint16_t>(300), "default mode uses #POINT.w");
        EXPECT_EQ(dlg.height(), static_cast<std::uint16_t>(400), "default mode uses #POINT.h");
    }

    // (2) Low mode + point_low 存在 → 用 #POINT_
    {
        mxh::ui::InterfaceNode node;
        mxh::ui::WindowRect p{100, 200, 300, 400};
        mxh::ui::WindowRect p_low{50, 100, 150, 200};
        node.point = p;
        node.point_low = p_low;
        mxh::ui::cDialog dlg;
        EXPECT(mxh::ui::apply_legacy_layout(dlg, node, nullptr,
                                            mxh::ui::ResolutionMode::Low800x600),
               "Low + point_low apply ok");
        EXPECT_EQ(dlg.absX(), 50, "Low mode + point_low uses #POINT_.x");
        EXPECT_EQ(dlg.absY(), 100, "Low mode + point_low uses #POINT_.y");
        EXPECT_EQ(dlg.width(), static_cast<std::uint16_t>(150), "Low mode + point_low uses #POINT_.w");
        EXPECT_EQ(dlg.height(), static_cast<std::uint16_t>(200), "Low mode + point_low uses #POINT_.h");
    }

    // (3) Low mode + 无 point_low → 用 #POINT
    {
        mxh::ui::InterfaceNode node;
        mxh::ui::WindowRect p{100, 200, 300, 400};
        node.point = p;
        mxh::ui::cDialog dlg;
        EXPECT(mxh::ui::apply_legacy_layout(dlg, node, nullptr,
                                            mxh::ui::ResolutionMode::Low800x600),
               "Low + no point_low apply ok");
        EXPECT_EQ(dlg.absX(), 100, "Low + no point_low falls back to #POINT.x");
    }

    // (4) High mode + point_low 存在 → 用 #POINT (不切)
    {
        mxh::ui::InterfaceNode node;
        mxh::ui::WindowRect p{100, 200, 300, 400};
        mxh::ui::WindowRect p_low{50, 100, 150, 200};
        node.point = p;
        node.point_low = p_low;
        mxh::ui::cDialog dlg;
        EXPECT(mxh::ui::apply_legacy_layout(dlg, node, nullptr,
                                            mxh::ui::ResolutionMode::High1920x1080),
               "High + point_low apply ok");
        EXPECT_EQ(dlg.absX(), 100, "High mode always uses #POINT (point_low 忽略)");
    }

    // (5) LoadAll 接受 mode 参数 (用 LoadOne on 15.bin 替身 — 15.bin has no
    // PUSHUPBTN child, 不会触发 R-37). High vs Low 1:1 行为验证.
    {
        // (5a) LoadOne(15.bin) High vs Low — 同一文件不同 mode 行为 1:1
        // (15.bin 没 #POINT_, 所以 2 档 mode 走 #POINT 一样)
        mxh::ui::cWindowManager wm_h, wm_l;
        auto r_h = mxh::ui::cDialogLoader::LoadOne(
            is_dir / "15.bin", wm_h, mxh::ui::ResolutionMode::High1920x1080);
        auto r_l = mxh::ui::cDialogLoader::LoadOne(
            is_dir / "15.bin", wm_l, mxh::ui::ResolutionMode::Low800x600);
        EXPECT(r_h.ok, "LoadOne 15.bin (High) ok");
        EXPECT(r_l.ok, "LoadOne 15.bin (Low) ok");
        EXPECT_EQ(r_h.point_x, r_l.point_x, "LoadOne mode High vs Low: same point_x");
        EXPECT_EQ(r_h.point_y, r_l.point_y, "LoadOne mode High vs Low: same point_y");
        EXPECT_EQ(r_h.point_w, r_l.point_w, "LoadOne mode High vs Low: same point_w");
        EXPECT_EQ(r_h.point_h, r_l.point_h, "LoadOne mode High vs Low: same point_h");
        EXPECT_EQ(r_h.dialog_type, r_l.dialog_type, "LoadOne mode High vs Low: same dialog_type");
        // (5b) LoadAll(missing) — 验证 mode param 不破坏 error path
        mxh::ui::cWindowManager wm;
        auto reports = mxh::ui::cDialogLoader::LoadAll(
            fs::path("Z:/this/does/not/exist"), wm, mxh::ui::ResolutionMode::Low800x600);
        EXPECT_EQ(reports.size(), std::size_t(1), "1 report for missing root (mode param)");
        EXPECT(!reports[0].ok, "missing root: not ok (mode param)");
    }

    // (6) cWindowManager::OnResolutionChange + SetCurrentResolutionMode 切换
    {
        mxh::ui::cWindowManager wm;
        EXPECT(wm.currentResolutionMode() == mxh::ui::kDefaultResolutionMode,
               "default mode = High1920x1080");
        wm.SetCurrentResolutionMode(mxh::ui::ResolutionMode::Low800x600);
        EXPECT(wm.currentResolutionMode() == mxh::ui::ResolutionMode::Low800x600,
               "SetCurrentResolutionMode(Low) 1:1");
        wm.OnResolutionChange(mxh::ui::ResolutionMode::Ultra2560x1440);
        EXPECT(wm.currentResolutionMode() == mxh::ui::ResolutionMode::Ultra2560x1440,
               "OnResolutionChange(Ultra) 1:1");
    }

    // (7) detect_from_screen_size + mode_size 1:1
    {
        EXPECT(mxh::ui::detect_from_screen_size(800, 600) == mxh::ui::ResolutionMode::Low800x600,
               "detect 800x600 → Low800x600");
        EXPECT(mxh::ui::detect_from_screen_size(1024, 768) == mxh::ui::ResolutionMode::Mid1024x768,
               "detect 1024x768 → Mid1024x768");
        EXPECT(mxh::ui::detect_from_screen_size(1920, 1080) == mxh::ui::ResolutionMode::High1920x1080,
               "detect 1920x1080 → High1920x1080");
        EXPECT(mxh::ui::detect_from_screen_size(2560, 1440) == mxh::ui::ResolutionMode::Ultra2560x1440,
               "detect 2560x1440 → Ultra2560x1440");

        auto sz_low = mxh::ui::mode_size(mxh::ui::ResolutionMode::Low800x600);
        EXPECT_EQ(sz_low.first, 800, "Low mode_size w=800");
        EXPECT_EQ(sz_low.second, 600, "Low mode_size h=600");
        auto sz_mid = mxh::ui::mode_size(mxh::ui::ResolutionMode::Mid1024x768);
        EXPECT_EQ(sz_mid.first, 1024, "Mid mode_size w=1024");
        EXPECT_EQ(sz_mid.second, 768, "Mid mode_size h=768");
        auto sz_high = mxh::ui::mode_size(mxh::ui::ResolutionMode::High1920x1080);
        EXPECT_EQ(sz_high.first, 1920, "High mode_size w=1920");
        EXPECT_EQ(sz_high.second, 1080, "High mode_size h=1080");
        auto sz_ultra = mxh::ui::mode_size(mxh::ui::ResolutionMode::Ultra2560x1440);
        EXPECT_EQ(sz_ultra.first, 2560, "Ultra mode_size w=2560");
        EXPECT_EQ(sz_ultra.second, 1440, "Ultra mode_size h=1440");

        EXPECT(std::string(mxh::ui::mode_name(mxh::ui::ResolutionMode::Low800x600))
               == "Low800x600", "mode_name Low");
        EXPECT(std::string(mxh::ui::mode_name(mxh::ui::ResolutionMode::Mid1024x768))
               == "Mid1024x768", "mode_name Mid");
        EXPECT(std::string(mxh::ui::mode_name(mxh::ui::ResolutionMode::High1920x1080))
               == "High1920x1080", "mode_name High");
        EXPECT(std::string(mxh::ui::mode_name(mxh::ui::ResolutionMode::Ultra2560x1440))
               == "Ultra2560x1440", "mode_name Ultra");
    }

    std::cout << "[resolution_mode_test] M-R7 (G3) resolution_mode: PASS "
              << g_passes << " / FAIL " << g_failures << "\n";
    return g_failures > 0 ? 1 : 0;
}
