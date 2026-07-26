// cnotedialog.cpp — modern port of 墨香 CNoteDialog (note inbox).
//
// 1:1 port of legacy `CNoteDialog` from
//   `墨香【源码】\[Client]MH\NoteDialog.cpp`.

#include "mxh/ui/cnotedialog.hpp"
#include "mxh/ui/ctextarea.hpp"
#include "mxh/ui/cstatic.hpp"

#include <cstring>
#include <utility>

namespace mxh::ui {

cNoteDialog::cNoteDialog() {
    m_SelectedNotePge = 1;
    m_CurNoteMode     = NoteMode_NormalNote;
    m_CurNoteID       = 0;
}

cNoteDialog::~cNoteDialog() = default;

void cNoteDialog::Init(std::int32_t x, std::int32_t y, std::uint16_t wid,
                       std::uint16_t hei, void* basicImage, std::int32_t id) {
    cDialog::Init(x, y, wid, hei, basicImage, id);
    // 1:1 with legacy: m_type = WT_NOTEDLG.  No modern equivalent.
}

void cNoteDialog::Linking() {
    // 1:1 with legacy Linking.  The modern port doesn't poke
    // cWindowManager::GetWindowForID; the host called
    // SetChildWindowsForTest beforehand.  We replicate the
    // legacy "SetMode(NormalNote)" tail call so the dialog
    // opens on the Normal tab.
    SetMode(NoteMode_NormalNote);
}

void cNoteDialog::SetActive(bool val) noexcept {
    if (!isEnabled()) return;
    // 1:1 with legacy: forward to cDialog::SetActive (which
    // sets m_bActive), then notify the host via the
    // ActiveChangedCallback.  Legacy reached into the
    // CMainBarDialog singleton to push the note icon +
    // clear the alarm; the modern port delegates both via
    // a std::function callback the host wires up.
    cDialog::SetActive(val);
    if (m_onActiveChanged) m_onActiveChanged(m_bActive);

    if (!val) {
        // 1:1 with legacy: SetActive(FALSE) resets the mode
        // to NormalNote.
        SetMode(NoteMode_NormalNote);
    } else {
        // 1:1 with legacy: SetActive(TRUE) clears the "select
        // all" checkbox.
        if (m_w.noteChkAll) m_w.noteChkAll->SetChecked(false);
    }
}

void cNoteDialog::SetMode(std::uint16_t mode) noexcept {
    m_CurNoteMode = mode;
    // 1:1 with legacy: the pushup button toggle is gated by
    // #ifndef TAIWAN_LOCAL.  The modern port keeps the same
    // conditional; Taiwan builds leave both buttons alone.
#ifndef TAIWAN_LOCAL
    if (mode == NoteMode_NormalNote) {
        if (m_w.noteBtn)   m_w.noteBtn->SetPush(true);
        if (m_w.psNoteBtn) m_w.psNoteBtn->SetPush(false);
    } else {
        if (m_w.noteBtn)   m_w.noteBtn->SetPush(false);
        if (m_w.psNoteBtn) m_w.psNoteBtn->SetPush(true);
    }
#endif
}

void cNoteDialog::SetNoteListFromFields(const std::vector<NoteListRow>& rows,
                                          std::uint8_t totalPage) {
    // 1:1 with legacy SetNoteList(MSG_FRIEND_NOTE_LIST*):
    //   * ShowNotePageBtn(TotalPage) -- 1st call
    //   * m_pNoteListLCtrl->DeleteAllItems()
    //   * for i in [0, NOTENUM_PER_PAGE):
    //       m_pNoteChk[i]->SetChecked(FALSE) + SetActive(FALSE)
    //       if rows[i].NoteID != 0:
    //         build cRITEMEx (truncate FromName to 12 chars,
    //                          format SendDate into pString[1],
    //                          color = TTTC_DEFAULT or RGBA(175,178,192,255) if !bIsRead)
    //         m_pNoteListLCtrl->InsertItem(i, ritem)
    //         m_pNoteChk[i]->SetActive(TRUE)
    //   * SetActive(TRUE)
    ShowNotePageBtn(totalPage);
    if (m_w.noteListLCtrl) {
        // 1:1 cListCtrl: clear all rows.  Modern cListCtrl
        // exposes row-by-row data via AddRow / RemoveAll;
        // the legacy cRITEMEx insertion is replaced with a
        // simple internal cache that the host can mirror via
        // AddRow + paint.
        m_w.noteListLCtrl->RemoveAll();
    }
    m_lastRows.clear();
    m_lastRows.reserve(rows.size());

    for (std::int32_t i = 0; i < kNoteNumPerPage; ++i) {
        if (static_cast<std::size_t>(i) < rows.size()) {
            const auto& r = rows[static_cast<std::size_t>(i)];
            if (i < kNoteNumPerPage && m_w.noteChk[i]) {
                m_w.noteChk[i]->SetChecked(false);
                m_w.noteChk[i]->SetActive(false);
            }
            if (r.NoteID != 0) {
                StoredRItem s{};
                s.noteID   = r.NoteID;
                s.bIsRead  = r.bIsRead;
                // 1:1 with legacy: truncate FromName to 12 chars.
                const std::size_t nlen = std::strlen(r.FromName);
                if (nlen > 12) {
                    std::memcpy(s.p0, r.FromName, 12);
                    s.p0[12] = '\0';
                } else {
                    std::memcpy(s.p0, r.FromName, nlen);
                    s.p0[nlen] = '\0';
                }
                // 1:1 with legacy: sprintf(ritem->pString[1], pmsg->NoteList[i].SendDate)
                std::memcpy(s.p1, r.SendDate, sizeof(s.p1) - 1);
                s.p1[sizeof(s.p1) - 1] = '\0';
                m_lastRows.push_back(s);
                if (i < kNoteNumPerPage && m_w.noteChk[i]) {
                    m_w.noteChk[i]->SetActive(true);
                }
            }
        } else if (i < kNoteNumPerPage && m_w.noteChk[i]) {
            m_w.noteChk[i]->SetChecked(false);
            m_w.noteChk[i]->SetActive(false);
        }
    }
    // 1:1 with legacy tail call: SetActive(TRUE) once the
    // list has been populated.
    cDialog::SetActive(true);
}

void cNoteDialog::ShowNotePageBtn(std::uint8_t totalPage) {
    if (totalPage == 0) {
        // 1:1 with legacy: hide every page button.
        for (std::int32_t i = 0; i < kMaxNotePage; ++i) {
            if (m_w.notePageBtn[i]) m_w.notePageBtn[i]->SetActive(false);
        }
        return;
    }
    for (std::int32_t i = 0; i < kMaxNotePage; ++i) {
        if (m_w.notePageBtn[i]) {
            // 1:1 with legacy: SetTextValue(i+1) + SetActive(i+1 <= TotalPage)
            // Modern cButton uses SetText(string) for the page label.
            m_w.notePageBtn[i]->SetText(std::to_string(static_cast<std::int32_t>(i) + 1));
            m_w.notePageBtn[i]->SetActive(
                static_cast<std::int32_t>(i) + 1 <=
                static_cast<std::int32_t>(totalPage));
        }
    }
}

void cNoteDialog::SetChkAll() {
    if (m_w.noteChkAll == nullptr) return;
    const bool chk = m_w.noteChkAll->IsChecked();
    // 1:1 with legacy: only flip the checkboxes that are
    // currently active (legacy: m_pNoteChk[i]->IsActive()).
    for (std::int32_t i = 0; i < kNoteNumPerPage; ++i) {
        if (m_w.noteChk[i] && m_w.noteChk[i]->isActive()) {
            m_w.noteChk[i]->SetChecked(chk);
        }
    }
}

std::size_t cNoteDialog::NoteListSize() const noexcept {
    return m_lastRows.size();
}

} // namespace mxh::ui
