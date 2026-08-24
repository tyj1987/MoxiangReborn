// skillpointnotify.hpp — modern port of 墨香 CSkillPointNotify
//
// 1:1 port of legacy `CSkillPointNotify` from
//   `墨香【源码】\[Client]MH\SkillPointNotify.{h,cpp}`.
//
// A modal 3-child dialog that shows 2 lines of skill-point-reset info
// text + 1 "start reset" button. InitTextArea() re-applies the localized
// chat messages (placeholder strings in modern port since CHATMGR is
// unported). No ActionEvent / OnActionEvent / Render — the dialog relies
// on its parent cDialog::ActionEvent for hit dispatch.

#pragma once

#include "cDialog.hpp"

#include <memory>

namespace mxh::ui {

class cButton;
class cTextArea;

class cSkillPointNotify : public cDialog {
public:
    // Local id range 1:1 with legacy SK_INFOTEXT1/2 + SK_STARTBTN
    // (rebased to 800..802 to avoid conflicts).
    static constexpr int kIdNotifyText1 = 800;
    static constexpr int kIdNotifyText2 = 801;
    static constexpr int kIdStartBtn    = 802;

    cSkillPointNotify();
    ~cSkillPointNotify() override;

    void Linking();
    void InitTextArea();

    // Test accessors.
    const cTextArea* GetNotifyText1() const noexcept { return m_Notifyta1.get(); }
    const cTextArea* GetNotifyText2() const noexcept { return m_Notifyta2.get(); }
    const cButton*   GetStartButton() const noexcept { return m_RedistBtn.get(); }

private:
    std::unique_ptr<cTextArea> m_Notifyta1;
    std::unique_ptr<cTextArea> m_Notifyta2;
    std::unique_ptr<cButton>   m_RedistBtn;
};

} // namespace mxh::ui
