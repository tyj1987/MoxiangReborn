// cGuildDialog.cpp — modern implementation of 墨香 CGuildDialog.

#include "cGuildDialog.hpp"

#include "cButton.hpp"
#include "cListDialog.hpp"
#include "cPushupButton.hpp"
#include "cStatic.hpp"

#include <algorithm>
#include <cstring>
#include <cstdio>

namespace mxh::ui {

cGuildDialog::cGuildDialog() = default;
cGuildDialog::~cGuildDialog() = default;

void cGuildDialog::Linking() {
    // Resolve the actual controls from Guild.bin. Numeric IDs are not stable
    // across locale/resource profiles, so the symbolic legacy IDs are the
    // canonical runtime lookup key.
    m_memberList = dynamic_cast<cListDialog*>(
        findWindowByLegacyId("GD_MEMBERLIST"));
    if (!m_memberList) {
        // Keep the numeric legacy ID path for hand-built callers and older
        // profile manifests; shipped Guild.bin is resolved by symbolic ID.
        m_memberList = dynamic_cast<cListDialog*>(findWindowById(7001));
    }
    m_guildNameControl = dynamic_cast<cStatic*>(
        findWindowByLegacyId("GD_NAME"));
    m_guildLevelControl = dynamic_cast<cStatic*>(
        findWindowByLegacyId("GD_LEVEL"));
    m_masterNameControl = dynamic_cast<cStatic*>(
        findWindowByLegacyId("GD_MASTER"));
    m_memberNumControl = dynamic_cast<cStatic*>(
        findWindowByLegacyId("GD_MEMBERNUM"));
    m_locationControl = dynamic_cast<cStatic*>(
        findWindowByLegacyId("GD_LOCATION"));
    m_unionNameControl = dynamic_cast<cStatic*>(
        findWindowByLegacyId("GD_UNIONNAME"));
    RefreshMemberList();
    SetInfo(m_guildName.c_str(), m_guildLevel, m_masterName.c_str(),
            m_memberNum, m_location.c_str());
    if (m_unionNameControl) m_unionNameControl->SetStaticText(m_unionName);
}

void cGuildDialog::SetInfo(const char* guildName, std::uint8_t guildLevel,
                            const char* masterName, std::uint8_t memberNum,
                            const char* location) {
    if (guildName)  m_guildName  = guildName;
    if (masterName) m_masterName = masterName;
    if (location)   m_location   = location;
    m_guildLevel = guildLevel;
    m_memberNum  = memberNum;
    if (m_guildNameControl) m_guildNameControl->SetStaticText(m_guildName);
    if (m_guildLevelControl) m_guildLevelControl->SetStaticValue(m_guildLevel);
    if (m_masterNameControl) m_masterNameControl->SetStaticText(m_masterName);
    if (m_memberNumControl) m_memberNumControl->SetStaticValue(m_memberNum);
    if (m_locationControl) m_locationControl->SetStaticText(m_location);
}

void cGuildDialog::SetGuildInfo(const char* guildName, const char* masterName,
                                 const char* mapName, std::uint8_t guildLevel,
                                 std::uint8_t memberNum, const char* unionName) {
    if (guildName)  m_guildName  = guildName;
    if (masterName) m_masterName = masterName;
    if (mapName)    m_location   = mapName;
    m_guildLevel = guildLevel;
    m_memberNum  = memberNum;
    if (unionName) m_unionName  = unionName;
    SetInfo(m_guildName.c_str(), m_guildLevel, m_masterName.c_str(),
            m_memberNum, m_location.c_str());
    if (m_unionNameControl) m_unionNameControl->SetStaticText(m_unionName);
}

void cGuildDialog::ResetMemberInfo(const MemberInfo& info) {
    m_members.push_back(info);
    RefreshMemberList();
}

void cGuildDialog::DeleteMemberAll() noexcept {
    m_members.clear();
    m_selectedMember = -1;
    if (m_memberList) m_memberList->RemoveAll();
}

void cGuildDialog::RefreshMemberList() {
    if (!m_memberList) {
        m_memberList = dynamic_cast<cListDialog*>(
            findWindowByLegacyId("GD_MEMBERLIST"));
        if (!m_memberList) {
            m_memberList = dynamic_cast<cListDialog*>(findWindowById(7001));
        }
    }
    if (!m_memberList) return;
    m_memberList->RemoveAll();
    for (const auto& m : m_members) {
        char buf[256];
        std::snprintf(buf, sizeof(buf), "%-16s L%u %s",
                      m.name.c_str(),
                      static_cast<unsigned>(m.level),
                      m.online ? "online" : "offline");
        m_memberList->AddItem(buf, 0xFF000000);
    }
    if (m_selectedMember >= 0 && m_selectedMember < static_cast<int>(m_members.size())) {
        m_memberList->SetCurSelectedRowIdx(m_selectedMember);
    }
}

void cGuildDialog::SortMemberListByPosition() {
    // The legacy sorts by Rank (the "position" field) ascending. We
    // implement the equivalent with stable_sort to preserve relative
    // order for equal-position members.
    m_positionFlag = (m_positionFlag == 0) ? 1 : 0;
    std::stable_sort(m_members.begin(), m_members.end(),
                     [](const MemberInfo& a, const MemberInfo& b) {
                         return a.rank < b.rank;
                     });
    RefreshMemberList();
}

void cGuildDialog::SortMemberListByLevel() {
    m_levelFlag = (m_levelFlag == 0) ? 1 : 0;
    std::stable_sort(m_members.begin(), m_members.end(),
                     [](const MemberInfo& a, const MemberInfo& b) {
                         return a.level < b.level;
                     });
    RefreshMemberList();
}

void cGuildDialog::SetActive(bool val) noexcept {
    cDialog::SetActive(val);
    if (m_memberList) m_memberList->SetActive(val);
}

std::uint32_t cGuildDialog::ActionEvent(std::int32_t mx, std::int32_t my,
                                          std::uint32_t flags) {
    const auto event = cDialog::ActionEvent(mx, my, flags);
    if (m_memberList) {
        const int selected = m_memberList->GetCurSelectedRowIdx();
        if (selected >= 0 && selected < static_cast<int>(m_members.size())) {
            m_selectedMember = selected;
        }
    }
    return event;
}

void cGuildDialog::SetDisableFuncBtn(Rank viewerRank) {
    // Per-button access policy. Master can do everything; vice-master
    // can't dissolve or change guild name; senior can only kick; member
    // can only see. The legacy uses 14 buttons; we abstract with a
    // single `for each child cButton: enable iff access(viewerRank, btn)`.
    const std::uint8_t rank = static_cast<std::uint8_t>(viewerRank);
    for (std::size_t i = 0; i < childCount(); ++i) {
        cButton* b = dynamic_cast<cButton*>(childAt(i));
        if (!b) continue;
        // Cheap policy: the higher the rank, the more access. Buttons
        // with id < 8000 are "general" (require Senior+); >= 8000 are
        // "admin" (require Master). This is a simplified mapping of
        // the legacy 14-button policy — a Phase 7 follow-up can wire
        // the exact id-to-permission matrix.
        const bool needsAdmin = b->id() >= 8000;
        const bool allow = needsAdmin ? (rank >= static_cast<std::uint8_t>(Rank::Master))
                                      : (rank >= static_cast<std::uint8_t>(Rank::Senior));
        b->SetDisable(!allow);
    }
}

void cGuildDialog::ClearDisableBtn() noexcept {
    for (std::size_t i = 0; i < childCount(); ++i) {
        cButton* b = dynamic_cast<cButton*>(childAt(i));
        if (!b) continue;
        b->SetDisable(false);
    }
}

void cGuildDialog::SetGuildPushupBtn(std::uint8_t showMode) noexcept {
    m_showMode = showMode;
    // The legacy flips two pushup buttons: one in pushed state, the
    // other released. The exact id mapping is dialog-layout-specific;
    // here we walk all cPushupButton children and push the one whose
    // id encodes `showMode` (id == 9000 + showMode).
    for (std::size_t i = 0; i < childCount(); ++i) {
        cPushupButton* pb = dynamic_cast<cPushupButton*>(childAt(i));
        if (!pb) continue;
        const bool shouldPush =
            (pb->id() == static_cast<std::int32_t>(9000u + showMode));
        pb->SetPush(shouldPush);
    }
}

void cGuildDialog::SetGuildPosition(const char* mapName) {
    if (mapName) m_location = mapName;
    if (m_locationControl) m_locationControl->SetStaticText(m_location);
}

} // namespace mxh::ui
