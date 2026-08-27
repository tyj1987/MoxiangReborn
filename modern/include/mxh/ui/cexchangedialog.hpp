#pragma once
#include "mxh/ui/cDialog.hpp"
#include "mxh/services/IInventoryService.hpp"
#include "mxh/services/ITradeService.hpp"
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
 // IInventoryService + ITradeService are the modern economy + commit
 // boundaries for player-to-player exchange. When set, SetOwn() validates
 // each offered item against service->hasItem(item_id) (mirroring the
 // cDealDialog pattern) and Complete() delegates the atomic commit to
 // service->completeTrade() so the MapHandler owns the inventory mutation
 // + money settlement + wire dispatch. The local m_own / m_other +
 // confirmed-flags remain valid fallbacks for unit tests + legacy NPC
 // types not yet wired.
 void SetInventoryService(mxh::services::IInventoryService* service) noexcept {m_inventory_service=service;}
 mxh::services::IInventoryService* GetInventoryService() const noexcept {return m_inventory_service;}
 void SetTradeService(mxh::services::ITradeService* service) noexcept {m_trade_service=service;}
 mxh::services::ITradeService* GetTradeService() const noexcept {return m_trade_service;}
 const std::vector<ExchangeSlot>& Own()const noexcept{return m_own;} const std::vector<ExchangeSlot>& Other()const noexcept{return m_other;}
private: std::vector<ExchangeSlot> m_own=std::vector<ExchangeSlot>(kSlots),m_other=std::vector<ExchangeSlot>(kSlots); bool m_own_confirmed{},m_other_confirmed{},m_completed{},m_cancelled{}; mxh::services::IInventoryService* m_inventory_service{};
 mxh::services::ITradeService* m_trade_service{};
};
}
