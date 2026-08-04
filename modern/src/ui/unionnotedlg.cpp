// unionnotedlg.cpp — 1:1 port of 墨香
// CUnionNoteDialog (guild union note sender
// dialog). See unionnotedlg.hpp for the data-model
// rationale + 1:1 quirks.

#include "unionnotedlg.hpp"
#include "ctextarea.hpp"
#include "ceditbox.hpp"

namespace mxh::ui {

cUnionNoteDlg::cUnionNoteDlg() {
    // 1:1 with legacy CUnionNoteDlg ctor:
    //   m_bUse = FALSE;
    //
    // 1:1 quirk: modern bool uses default member
    // init (m_bUse = false in header). ctor body
    // is empty.
}

cUnionNoteDlg::~cUnionNoteDlg() = default;

void cUnionNoteDlg::Linking() {
    // 1:1 with legacy CUnionNoteDlg::Linking. The
    // legacy is:
    //   m_pNoteText = (cTextArea*)GetWindowForID(AN_TEXTREA);
    //   m_pNoteText->SetEnterAllow(FALSE);
    //   m_pNoteText->SetScriptText("");
    m_pNoteText =
        static_cast<cTextArea*>(findWindowById(kIdNoteText));
    if (m_pNoteText) {
        // 1:1 with legacy SetEnterAllow(FALSE).
        m_pNoteText->SetEnterAllow(false);
        m_pNoteText->SetScriptText("");
    }
    // m_pTitleEdit is unused in legacy cpp; modern
    // port doesn't resolve it (preserves 1:1 with
    // legacy unused field).
}

void cUnionNoteDlg::SetCallbacks(
    AddSystemMessageFn addSystemMessage,
    GetHeroDwordFn getGuildIdx,
    GetHeroRankFn getGuildMemberRank,
    GetHeroDwordFn getGuildUnionIdx,
    GetHeroDwordFn getHeroObjectId,
    GetHeroNameFn getHeroName,
    GetItemWordFn getItemIdx,
    GetItemPositionFn getItemPosition,
    SendItemUseFn sendItemUse,
    SendUnionNoteFn sendUnionNote,
    IncrementItemUseCountFn incrementItemUseCount,
    void* userData) noexcept {
    m_addSystemMessage = addSystemMessage;
    m_getGuildIdx = getGuildIdx;
    m_getGuildMemberRank = getGuildMemberRank;
    m_getGuildUnionIdx = getGuildUnionIdx;
    m_getHeroObjectId = getHeroObjectId;
    m_getHeroName = getHeroName;
    m_getItemIdx = getItemIdx;
    m_getItemPosition = getItemPosition;
    m_sendItemUse = sendItemUse;
    m_sendUnionNote = sendUnionNote;
    m_incrementItemUseCount = incrementItemUseCount;
    m_callbackUserData = userData;
}

void cUnionNoteDlg::Show(void* pItem) {
    const auto addMessage = [this](std::int32_t messageId) {
        if (m_addSystemMessage) {
            m_addSystemMessage(messageId, m_callbackUserData);
        }
    };

    if (!m_getGuildIdx || m_getGuildIdx(m_callbackUserData) == 0u) {
        addMessage(kNoGuildMessageId);
        return;
    }
    const auto rank = m_getGuildMemberRank
        ? m_getGuildMemberRank(m_callbackUserData)
        : 0;
    if (rank != kGuildMaster && rank != kGuildViceMaster) {
        addMessage(kInvalidRankMessageId);
        return;
    }
    if (!m_getGuildUnionIdx || m_getGuildUnionIdx(m_callbackUserData) == 0u) {
        addMessage(kNoUnionMessageId);
        return;
    }
    if (!pItem) {
        addMessage(kInvalidItemMessageId);
        return;
    }
    if (m_bUse) {
        addMessage(kAlreadyUsingMessageId);
        return;
    }

    m_pItem = pItem;
    SetActive(true);
}

void cUnionNoteDlg::Use() {
    m_bUse = false;
    if (m_pNoteText) m_pNoteText->SetScriptText("");

    if (m_pItem && m_getHeroObjectId && m_getItemIdx &&
        m_getItemPosition && m_sendItemUse) {
        m_sendItemUse(m_getHeroObjectId(m_callbackUserData),
                      m_getItemIdx(m_pItem, m_callbackUserData),
                      m_getItemPosition(m_pItem, m_callbackUserData),
                      m_callbackUserData);
        if (m_incrementItemUseCount) {
            m_incrementItemUseCount(m_callbackUserData);
        }
    }
}

void cUnionNoteDlg::OnActionEvent(std::int32_t lId, void* p,
                                  std::uint32_t we) {
    (void)p;
    if ((we & kWeBtnClick) == 0u) return;

    switch (lId) {
    case kIdSendOkBtn:
        if (m_pNoteText && m_getGuildUnionIdx && m_getHeroObjectId &&
            m_getHeroName && m_sendUnionNote) {
            const char* heroName = m_getHeroName(m_callbackUserData);
            m_sendUnionNote(m_getHeroObjectId(m_callbackUserData),
                            m_getGuildUnionIdx(m_callbackUserData),
                            heroName ? heroName : "",
                            m_pNoteText->GetScriptText().c_str(),
                            m_callbackUserData);
        }
        SetActive(false);
        [[fallthrough]];
    case kIdCancelBtn:
        SetActive(false);
        break;
    default:
        break;
    }
}


}  // namespace mxh::ui
