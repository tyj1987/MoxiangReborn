// moneydlg.hpp — modern port of 墨香 CMoneyDlg (money-amount picker).
//
// 1:1 port of legacy `CMoneyDlg` from
//   `墨香【源码】\[Client]MH\MoneyDlg.{h,cpp}`.
//
// CMoneyDlg is the modal "how much money do you want to send / pay /
// charge" dialog. The legacy class wraps a cSpin (the amount input) and
// forwards the result via a C-style function pointer callback
// `m_OnPushFunc(Money, dwParam)`; if the callback returns TRUE, the
// saved message buffer is sent over the network via the `NETWORK`
// global singleton.
//
// 1:1 contract preserved:
//   - `Show(pmsg, msglen, dwParam, OnPushFunc)` — activates the dialog,
//     stores the saved message + params + callback. Defensive: empty
//     pmsg or msglen == 0 → no copy (matches legacy early return).
//   - `OkPushed()` — reads the cSpin value, invokes the callback, and
//     (if the callback returns TRUE) emits the saved message via the
//     NETWORK singleton. Closes the dialog.
//   - State: m_MsgLen (int), m_SavedMsg (1024-byte buffer), m_dwParam
//     (DWORD), m_OnPushFunc (function pointer).
//
// 1:1 quirks preserved:
//   - Ctor `m_type = WT_MONEYDIALOG` → DROPPED (modern cWindow no
//     m_type field per Phase 6 "removed fields" rule).
//   - Ctor `memset(m_SavedMsg, 0, sizeof(1024))` → preserved verbatim
//     (the legacy literal `sizeof(1024)` evaluates to 1024 bytes, which
//     happens to be the buffer size; modern port uses std::vector::assign
//     for the same effect, but the spirit of "zero-init 1024 bytes" is
//     maintained).
//   - OkPushed uses `ASSERT(pSpin)` in legacy → modern port uses
//     `m_pSpin` captured at Linking time; null guard is silent
//     (matches legacy: if Linking is never called, pSpin is null and
//     deref crashes — modern port returns early on null).
//   - The NETWORK singleton is stubbed (returns a no-op emitter). The
//     callback contract is preserved regardless.

#pragma once

#include "cDialog.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace mxh::ui {

class cSpin;

// Callback signature: 1:1 with legacy
// `BOOL (*OnPushFunc)(DWORD Money, DWORD dwParam)`. Returns true to
// send the saved message, false to cancel.
using MoneyCallback =
    std::function<bool(std::uint32_t money, std::uint32_t dwParam)>;

class cMoneyDlg : public cDialog {
public:
    // The cSpin child is resolved at Linking() by id (legacy uses
    // GetWindowForID(CMI_MONEYSPIN)). Modern port uses a non-owning
    // raw pointer, set in Linking, used in OkPushed.
    static constexpr int kIdMoneySpin = 0;  // 1:1 with legacy CMI_MONEYSPIN

    static constexpr std::size_t kSavedMsgSize = 1024;

    cMoneyDlg();
    ~cMoneyDlg() override;

    cMoneyDlg(const cMoneyDlg&) = delete;
    cMoneyDlg& operator=(const cMoneyDlg&) = delete;

    void Linking();

    void Show(const void* pmsg, int msglen, std::uint32_t dwParam = 0,
              MoneyCallback onPush = {});

    void OkPushed();

    // Test accessors.
    int  msgLen() const noexcept   { return m_MsgLen; }
    std::uint32_t param() const noexcept { return m_dwParam; }
    // 1:1 quirk: cSpin is mutable (cMoneyDlg needs to set its value in
    // OkPushed via m_pSpin->GetValue()). The accessor is non-const so
    // tests can also call SetValue / SetMin / SetMax. For const-safe
    // read-only access, use the const overload below.
    cSpin* spin() noexcept { return m_pSpin.get(); }
    const cSpin* spin() const noexcept { return m_pSpin.get(); }
    bool hasCallback() const noexcept { return static_cast<bool>(m_OnPushFunc); }
    const std::vector<std::uint8_t>& savedMsg() const noexcept { return m_SavedMsg; }

private:
    int            m_MsgLen = 0;
    std::vector<std::uint8_t> m_SavedMsg;  // size kSavedMsgSize
    std::uint32_t  m_dwParam = 0;
    MoneyCallback  m_OnPushFunc;

    // cSpin is owned as a unique_ptr so Linking can materialize it
    // without requiring the test (or resource loader) to wire it
    // manually. 1:1 with legacy: the cSpin is a child of this dialog,
    // resolved by id at runtime.
    std::unique_ptr<cSpin> m_pSpin;
};

} // namespace mxh::ui
