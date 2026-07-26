#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <optional>
namespace mxh::ui {
struct UniqueItemCurseCancelState { std::uint32_t item_id{}; };
class cUniqueItemCurseCancellationDlg final : public cDialog {
public:
 using UniqueItemCurseCancelCallback = std::function<bool(const UniqueItemCurseCancelState&)>;
 bool Set(UniqueItemCurseCancelState s) {
   m_state = s; m_confirmed = false; return true;
 }
 void SetUniqueItemCurseCancelCallback(UniqueItemCurseCancelCallback cb) noexcept { m_uniqueitemcursecancel_cb = std::move(cb); }
 void Clear() noexcept { m_state.reset(); m_confirmed = false; }
 bool Confirm() {
   if (!m_state || m_confirmed) return false;
   if (m_uniqueitemcursecancel_cb && !m_uniqueitemcursecancel_cb(*m_state)) return false;
   m_confirmed = true; return true;
 }
 bool IsConfirmed() const noexcept { return m_confirmed; }
 const std::optional<UniqueItemCurseCancelState>& State() const noexcept { return m_state; }
private:
 std::optional<UniqueItemCurseCancelState> m_state{};
 bool m_confirmed{};
 UniqueItemCurseCancelCallback m_uniqueitemcursecancel_cb{};
};
}
