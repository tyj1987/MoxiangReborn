// cGuildDialog.hpp — modern port of 墨香 CGuildDialog (guild info + member list).
//
// 1:1 port of legacy `CGuildDialog` from
//   `墨香【源码】\[Client]MH\GuildDialog.{h,cpp}`.
//
// The guild dialog is the central UI for managing a guild: header
// (name, level, master, member count, location, union name), a member
// list (sortable by position/level), and a stack of function buttons
// (kick, give-nick, give-rank, invite, secede, declare war, etc.) that
// get enabled/disabled based on the player's rank (member / senior /
// vice-master / master).
//
// The legacy uses many helper widgets — cStatic (labels), cListDialog
// (member list), cPushupButton (tab buttons), cButton (function buttons).
// This port wires them through the modern equivalents. The data model
// (member list, ranks, sort, enable/disable matrix) is 1:1.

#pragma once

#include "cDialog.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mxh::ui {

class cStatic;
class cListDialog;
class cButton;
class cPushupButton;

class cGuildDialog : public cDialog {
public:
    cGuildDialog();
    ~cGuildDialog() override;

    // Member data model (1:1 with legacy GUILDMEMBERINFO).
    struct MemberInfo {
        std::string  name;
        std::uint8_t rank     = 0;        // 0=member, 1=senior, 2=vice, 3=master
        std::uint8_t job      = 0;
        std::uint8_t level    = 0;
        bool         online   = false;
        std::int32_t contribution = 0;
    };

    // Linking: looks up child widgets by id from the dialog tree. The
    // legacy calls this from the dialog's own Init path; the modern
    // port exposes it for tests to wire deterministic ids.
    void Linking();

    // Header info setters (legacy SetInfo / SetGuildInfo).
    void SetInfo(const char* guildName, std::uint8_t guildLevel,
                 const char* masterName, std::uint8_t memberNum,
                 const char* location);
    void SetGuildInfo(const char* guildName, const char* masterName,
                      const char* mapName, std::uint8_t guildLevel,
                      std::uint8_t memberNum, const char* unionName);

    // Member list management.
    void ResetMemberInfo(const MemberInfo& info);
    void DeleteMemberAll() noexcept;
    void RefreshMemberList();
    void SortMemberListByPosition();
    void SortMemberListByLevel();

    // UI state.
    void SetActive(bool val) noexcept;
    std::uint32_t ActionEvent(std::int32_t mx, std::int32_t my,
                              std::uint32_t flags) override;

    // Rank-based button enable / disable. The legacy uses 4 ranks; the
    // matrix here is the per-button access policy.
    enum class Rank : std::uint8_t { Member = 0, Senior = 1, Vice = 2, Master = 3 };
    void SetDisableFuncBtn(Rank viewerRank);
    void ClearDisableBtn() noexcept;

    // Tabs (pushup buttons): ShowMode 0 = member view, 1 = info view.
    void SetGuildPushupBtn(std::uint8_t showMode) noexcept;
    std::uint8_t GetShowMode() const noexcept         { return m_showMode; }

    // Position (location) — legacy separate setter for "where the guild is".
    void SetGuildPosition(const char* mapName);

    // Member list access (read-only).
    const std::vector<MemberInfo>& Members() const noexcept { return m_members; }
    std::size_t MemberCount() const noexcept                { return m_members.size(); }
    int  GetSelectedMember() const noexcept                 { return m_selectedMember; }
    void SetSelectedMember(int idx) noexcept                { m_selectedMember = idx; }

    // Header getters (used by the legacy RefreshGuildInfo test paths).
    const std::string& GuildName()  const noexcept           { return m_guildName; }
    const std::string& MasterName() const noexcept           { return m_masterName; }
    const std::string& Location()   const noexcept           { return m_location; }
    const std::string& UnionName()  const noexcept           { return m_unionName; }
    std::uint8_t       GuildLevel() const noexcept           { return m_guildLevel; }
    std::uint8_t       MemberNum()  const noexcept           { return m_memberNum; }

    // Constants
    static constexpr std::uint8_t kShowModeMember = 0;
    static constexpr std::uint8_t kShowModeInfo   = 1;

private:
    // Header.
    std::string   m_guildName;
    std::string   m_masterName;
    std::string   m_location;
    std::string   m_unionName;
    std::uint8_t  m_guildLevel = 0;
    std::uint8_t  m_memberNum  = 0;

    // Member list.
    std::vector<MemberInfo> m_members;
    int                     m_selectedMember = -1;

    // Tab mode.
    std::uint8_t  m_showMode = kShowModeMember;

    // Sort mode flags.
    int           m_positionFlag = 0;     // -1=desc, 0=none, 1=asc
    int           m_levelFlag    = 0;
};

} // namespace mxh::ui
