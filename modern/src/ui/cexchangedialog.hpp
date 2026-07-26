#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <vector>
namespace mxh::ui {
struct ExchangeSlot { std::uint16_t item_id{}; std::uint16_t quantity{}; };
class cExchangeDialog final : public cDialog {
public:
 static constexpr std::size_t kSlots=12;
 bool SetOwn(std::size_t slot,ExchangeSlot item); bool SetOther(std::size_t slot,ExchangeSlot item);
 bool ClearOwn(std::size_t slot); bool ClearOther(std::size_t slot);
 bool SetOwnConfirmed(bool value) noexcept; bool SetOtherConfirmed(bool value) noexcept;
 bool CanComplete() const noexcept; bool Complete(); void Cancel() noexcept;
 bool IsCompleted()const noexcept{return m_completed;} bool IsCancelled()const noexcept{return m_cancelled;}
 const std::vector<ExchangeSlot>& Own()const noexcept{return m_own;} const std::vector<ExchangeSlot>& Other()const noexcept{return m_other;}
private: std::vector<ExchangeSlot> m_own=std::vector<ExchangeSlot>(kSlots),m_other=std::vector<ExchangeSlot>(kSlots); bool m_own_confirmed{},m_other_confirmed{},m_completed{},m_cancelled{};
};
}
