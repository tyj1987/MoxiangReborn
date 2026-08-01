// Modern 1:1 port of legacy CMonsterGuageDlg.

#pragma once

#include "mxh/ui/cDialog.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::ui {

class cObjectGuagen;
class cStatic;

enum class MonsterGuageMode : std::int32_t {
    Monster = 0,
    Character = 1,
    Npc = 2,
    Pet = 3,
    Max = 4,
};

struct MonsterGaugeInfo {
    std::uint32_t life = 0;
    std::uint32_t maxLife = 0;
    std::uint32_t shield = 0;
    std::uint32_t maxShield = 0;
};

class cMonsterGuageDlg final : public cDialog {
public:
    static constexpr std::int32_t kDialogId = 985;
    static constexpr std::int32_t kNameId = 986;
    static constexpr std::int32_t kLifeTextId = 987;
    static constexpr std::int32_t kLifeGaugeId = 988;
    static constexpr std::int32_t kShieldGaugeId = 989;
    static constexpr std::int32_t kGuildNameId = 990;
    static constexpr std::int32_t kGuildUnionNameId = 991;
    static constexpr std::int32_t kNpcNameId = 995;
    static constexpr std::int32_t kLifeBaseId = 996;

    static constexpr std::uint32_t kMonsterObjectType = 32;
    static constexpr int kGuildNameChatMsg = 783;
    static constexpr int kGuildUnionNameChatMsg = 1137;

    using ChatMsgCallback = const char*(*)(int messageId, void* user);

    cMonsterGuageDlg();
    ~cMonsterGuageDlg() override;

    cMonsterGuageDlg(const cMonsterGuageDlg&) = delete;
    cMonsterGuageDlg& operator=(const cMonsterGuageDlg&) = delete;

    void Linking();
    void SetMonsterName(const char* name);
    void SetNpcName(const char* name);
    void SetMonsterLife(std::uint32_t current, std::uint32_t maximum,
                        std::int32_t type = -1);
    void SetMonsterShield(std::uint32_t current, std::uint32_t maximum,
                          std::int32_t type = -1);
    void SetMonsterLife(const MonsterGaugeInfo& info, std::int32_t type = -1);
    void SetMonsterShield(const MonsterGaugeInfo& info, std::int32_t type = -1);
    void SetGuildUnionName(const char* guildName, const char* unionName);
    void SetMonsterNameColor(std::uint32_t color);

    void SetActiveMonGuageMode(std::int32_t mode, bool active);
    void ShowMonsterGuageMode(std::int32_t mode);
    void Render() override;

    void SetObjectType(std::uint32_t type) noexcept { m_type = type; }
    std::uint32_t GetObjectType() const noexcept { return m_type; }
    void* GetCurMonster() const noexcept { return m_pCurMonster; }
    void SetCurrentMonsterHandle(void* handle) noexcept { m_pCurMonster = handle; }

    void SetChatMsgCallback(ChatMsgCallback callback, void* user) noexcept;
    void SetCheatEnabled(bool enabled) noexcept { m_cheatEnabled = enabled; }

    void SetControlsForTest(cStatic* name, cStatic* lifeText,
                            cObjectGuagen* lifeGauge, cStatic* shieldText,
                            cObjectGuagen* shieldGauge, cStatic* guildName,
                            cStatic* guildUnionName, cStatic* npcName,
                            cStatic* lifeBase) noexcept;

    cStatic* nameControl() const noexcept { return m_pName; }
    cStatic* lifeTextControl() const noexcept { return m_pLifeText; }
    cObjectGuagen* lifeGauge() const noexcept { return m_pLifeGuage; }
    cObjectGuagen* shieldGauge() const noexcept { return m_pShieldGuage; }
    cStatic* guildNameControl() const noexcept { return m_pGuildName; }
    cStatic* guildUnionNameControl() const noexcept { return m_pGuildUnionName; }
    cStatic* npcNameControl() const noexcept { return m_pNpcName; }
    std::int32_t currentMode() const noexcept { return m_CurMode; }
    std::size_t modeControlCount(std::int32_t mode) const noexcept;

private:
    bool IsValidMode(std::int32_t mode) const noexcept;
    void FormatName(std::string& output, int messageId,
                    const char* fallback, const char* value) const;

    cStatic* m_pName = nullptr;
    cStatic* m_pLifeText = nullptr;
    cObjectGuagen* m_pLifeGuage = nullptr;
    cStatic* m_pShieldText = nullptr;
    cObjectGuagen* m_pShieldGuage = nullptr;
    cStatic* m_pGuildName = nullptr;
    cStatic* m_pGuildUnionName = nullptr;
    cStatic* m_pNpcName = nullptr;

    std::array<std::vector<cWindow*>, static_cast<std::size_t>(MonsterGuageMode::Max)> m_modeControls;
    std::int32_t m_CurMode = -1;
    void* m_pCurMonster = nullptr;
    std::uint32_t m_type = kMonsterObjectType;

    ChatMsgCallback m_chatMsgCallback = nullptr;
    void* m_chatMsgUser = nullptr;
    bool m_cheatEnabled = false;
};

}  // namespace mxh::ui
