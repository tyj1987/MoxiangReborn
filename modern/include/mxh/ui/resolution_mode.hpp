// mxh/ui/resolution_mode.hpp
// M-R7 (G3) 分辨率自适应 (M-R7 §1.1 plan 头less 端) — 1:1 with legacy cScriptManager
// 装载时按 GetSystemMetrics(SM_CXSCREEN) 选 #POINT (default) 或 #POINT_ (low-res variant)。
//
// 老版 1:1 行为: cScriptManager::GetDlgInfoFromFile 在 eDLG case 读 #POINT 时, 如果有
// #POINT_ 同时存在, 按当前 resolution_mode 决定用哪个. 现代 1:1 完整化 — 装载端在
// apply_legacy_layout 选 point 或 point_low.
//
// 4 档 resolution_mode (G3 头less 完成判据 = 800x600 / 1024x768 / 1920x1080 / 2560x1440
// 3 档全覆盖, 2560x1440 由 Ultra 覆盖). 跟老版 cScriptManager 分档 1:1 (老版实际
// 800/1024/1280/1600 四档, modern 简化成 4 档但覆盖 user 当前 2560x1440 显示器).
//
// 不影响: 默认 mode = High (1920x1080+), 用 #POINT (跟 1:1 老版 1.0 黄金锁像同, 不破坏
// 现有 224 dialog 装载 + 11863 ctest).

#pragma once

#include <cstdint>
#include <utility>

namespace mxh::ui {

enum class ResolutionMode : std::int32_t {
    // 老版 800x600 (1:1 黄金锁像基准 — visual-smoke + G4 SSIM 1:1)
    Low800x600 = 0,
    // 老版 1024x768 (常见 4:3 fallback, 跟老 1024x768 黄金对照)
    Mid1024x768 = 1,
    // 1920x1080 (modern 主流 16:9, 默认 mode, 跟 M-R4.x 装载现状 1:1)
    High1920x1080 = 2,
    // 2560x1440 (本地 Intel Arc B580 4GB VRAM 物理分辨率, G3 头less 完成)
    Ultra2560x1440 = 3,
};

// Default resolution_mode for 1:1 backward compat: M-R4.x 装载现状都用 #POINT (default
// 老 800x600 黄金锁像 + modern 1920x1080+ 主流). 选 High1920x1080 让 现有 224 dialog +
// 11863 ctest 完全不破坏.
constexpr ResolutionMode kDefaultResolutionMode = ResolutionMode::High1920x1080;

// detect_from_screen_size: 根据当前屏幕尺寸返回最接近的 ResolutionMode. 跟老版
// cScriptManager 启动时 GetSystemMetrics 选档 1:1 行为. 头less 可单测 (传 int w, int h).
inline ResolutionMode detect_from_screen_size(int width, int /*height*/) noexcept {
    if (width <= 800) return ResolutionMode::Low800x600;
    if (width <= 1024) return ResolutionMode::Mid1024x768;
    if (width <= 1920) return ResolutionMode::High1920x1080;
    return ResolutionMode::Ultra2560x1440;
}

// mode_size: 头less helper, 返回 (width, height) tuple. 跟 detect_from_screen_size
// 反向 — 给单测断言 resolution_mode 实际尺寸 1:1.
inline std::pair<int, int> mode_size(ResolutionMode m) noexcept {
    switch (m) {
        case ResolutionMode::Low800x600:      return {800, 600};
        case ResolutionMode::Mid1024x768:     return {1024, 768};
        case ResolutionMode::High1920x1080:   return {1920, 1080};
        case ResolutionMode::Ultra2560x1440:  return {2560, 1440};
    }
    return {1920, 1080};  // fallback 不该到这
}

// mode_name: 头less helper, 返回 "Low800x600" / "Mid1024x768" / "High1920x1080" /
// "Ultra2560x1440". 单测断言用, 不需 i18n.
inline const char* mode_name(ResolutionMode m) noexcept {
    switch (m) {
        case ResolutionMode::Low800x600:      return "Low800x600";
        case ResolutionMode::Mid1024x768:     return "Mid1024x768";
        case ResolutionMode::High1920x1080:   return "High1920x1080";
        case ResolutionMode::Ultra2560x1440:  return "Ultra2560x1440";
    }
    return "Unknown";
}

}  // namespace mxh::ui
