// cGuildDialog.cpp — modern implementation of 墨香 CGuildDialog.

#include "cGuildDialog.hpp"

#include "cButton.hpp"
#include "cListDialog.hpp"
#include "cPushupButton.hpp"
#include "cStatic.hpp"

#include <algorithm>
#include <cstring>

namespace mxh::ui {

cGuildDialog::cGuildDialog() = default;
cGuildDialog::~cGuildDialog() = default;

void cGuildDialog::Linking() {
    // The legacy Linking() looks up child widgets by id (e.g.
    // m_pGuildName = (cStatic*)GetWindowForID(GD_GUILDNAME)). The modern
    // port accepts the children as already-added by the caller; this
    // stub is a hook for tests that wire the dialog manually.
    // No-op in the modern port because findWindowById() handles lookups.
}

void cGuildDialog::SetInfo(const char* guildName, std::uint8_t guildLevel,
                            const char* masterName, std::uint8_t memberNum,
                            const char* location) {
    if (guildName)  m_guildName  = guildName;
    if (masterName) m_masterName = masterName;
    if (location)   m_location   = location;
    m_guildLevel = guildLevel;
    m_memberNum  = memberNum;
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
}

void cGuildDialog::ResetMemberInfo(const MemberInfo& info) {
    m_members.push_back(info);
}

void cGuildDialog::DeleteMemberAll() noexcept {
    m_members.clear();
    m_selectedMember = -1;
}

void cGuildDialog::RefreshMemberList() {
    // The legacy calls this after resetting/adding members. It populates
    // the embedded cListDialog with formatted strings like
    //   "  name   | rank | level | online | contribution"
    // We re-use the format string but apply it to whatever cListDialog
    // child is registered. If none, no-op (test path).
    cListDialog* list = static_cast<cListDialog*>(findWindowById(/*GD_MEMBERLIST*/ 7001));
    if (!list) return;
    list->RemoveAll();
    for (const auto& m : m_members) {
        char buf[256];
        std::snprintf(buf, sizeof(buf), "%-16s L%u %s",
                      m.name.c_str(),
                      static_cast<unsigned>(m.level),
                      m.online ? "online" : "offline");
        list->AddItem(buf, 0xFF000000);
    }
    if (m_selectedMember >= 0 && m_selectedMember < static_cast<int>(m_members.size())) {
        list->SetCurSelectedRowIdx(m_selectedMember);
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
    // The legacy SetActive cascades to all icons + the embedded list
    // dialog. We rely on cDialog's SetAbsXY cascade for the visual
    // mirroring, so no further work is needed here for Phase 6.12.
}

std::uint32_t cGuildDialog::ActionEvent(std::int32_t mx, std::int32_t my,
                                          std::uint32_t flags) {
    return cDialog::ActionEvent(mx, my, flags);
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
        const bool shouldPush = (pb->id() == 9000u + showMode);
        pb->SetPush(shouldPush);
    }
}

void cGuildDialog::SetGuildPosition(const char* mapName) {
    if (mapName) m_location = mapName;
}

} // namespace mxh::ui
