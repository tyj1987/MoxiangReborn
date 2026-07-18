// debugdlg.cpp — 1:1 port of 墨香 CDebugDlg
// (debug flag display dialog). See debugdlg.hpp
// for the data-model rationale + 1:1 quirks.

#include "debugdlg.hpp"
#include "clistdialog.hpp"

#include <cstdarg>

namespace mxh::ui {

cDebugDlg::cDebugDlg() {
    // 1:1 with legacy CDebugDlg ctor:
    //   cListDialog base init (handled by base ctor);
    //   6 flags default uninitialized.
    //
    // 1:1 quirk: modern bool uses default member
    // init (= false in header). ctor body is empty.
}

cDebugDlg::~cDebugDlg() = default;

void cDebugDlg::DebugMsgParser(std::uint8_t type, const char* msg, ...) {
    // 1:1 with legacy CDebugDlg::DebugMsgParser.
    // The legacy is:
    //   void CDebugDlg::DebugMsgParser(BYTE type, char* msg, ...)
    //   {
    //     va_list args;
    //     va_start(args, msg);
    //     char buf[256];
    //     vsprintf(buf, msg, args);
    //     va_end(args);
    //     switch (type) {
    //     case DBG_ATTACK: if (m_bAttackFlag) AddItem(buf); break;
    //     case DBG_ITEM: if (m_bItemFlag) AddItem(buf); break;
    //     // ... (4 more branches)
    //     }
    //   }
    //
    // The modern port: the variadic AddItem + 6-branch
    // dispatch is TODO (R-12.x deferred). Modern port
    // is no-op for now.
    (void)type;
    (void)msg;
    // TODO: 1:1 with legacy variadic + 6-branch dispatch
    //       (R-12.x deferred). When ported, the body
    //       becomes the legacy code with cListDialog::
    //       AddItem calls gated by m_bXxxFlag.
}

}  // namespace mxh::ui
