// guildlevelupdialog.hpp — modern port of 墨香 CGuildLevelUpDialog
//
// 1:1 port of legacy `CGuildLevelUpDialog` from
//   `墨香【源码】\[Client]MH\GuildLevelUpDialog.{h,cpp}`.
//
// A non-modal dialog that shows the guild's current level + tier completion.
// 13 cStatic children: 4 "not complete" markers + 4 "complete" markers + 5
// level labels. Linking resolves all 13 by id range. SetLevel(0..5) toggles
// the visible state of the markers and recolors the active-level label.
//
// Render is a no-op (text labels render through the cImage seam in 6.4+).

#pragma once

#include "cDialog.hpp"

#include <array>
#include <cstdint>

namespace mxh::ui {

class cStatic;

class cGuildLevelUpDialog : public cDialog {
public:
    // Number of "not complete" / "complete" marker pairs (one per tier).
    static constexpr int kNumTiers = 4;
    // Number of level labels (1..5).
    static constexpr int kNumLevels = 5;

    cGuildLevelUpDialog();
    cGuildLevelUpDialog(const cGuildLevelUpDialog&) = delete;
    cGuildLevelUpDialog& operator=(const cGuildLevelUpDialog&) = delete;
    ~cGuildLevelUpDialog() override;

    void Linking();
    void SetLevel(std::uint8_t level);
    void SetActive(bool val) noexcept override;

    // Test accessors (read-only handles for verification).
    const cStatic* GetLevelupNotComplete(int idx) const noexcept {
        return (idx >= 0 && idx < kNumTiers) ? m_pLevelupNotComplete[idx].get() : nullptr;
    }
    const cStatic* GetLevelupComplete(int idx) const noexcept {
        return (idx >= 0 && idx < kNumTiers) ? m_pLevelupComplete[idx].get() : nullptr;
    }
    const cStatic* GetLevel(int idx) const noexcept {
        return (idx >= 0 && idx < kNumLevels) ? m_pLevel[idx].get() : nullptr;
    }
    std::uint8_t GetCurrentLevel() const noexcept { return m_currentLevel; }

private:
    std::array<std::unique_ptr<cStatic>, kNumTiers>  m_pLevelupNotComplete{};
    std::array<std::unique_ptr<cStatic>, kNumTiers>  m_pLevelupComplete{};
    std::array<std::unique_ptr<cStatic>, kNumLevels> m_pLevel{};

    // 1:1 with legacy: tracks the most recent SetLevel(level) value (1..5).
    // 0 means "never set" so callers can detect first SetActive(TRUE) boot.
    std::uint8_t m_currentLevel = 0;
};

} // namespace mxh::ui
