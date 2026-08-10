// mxh/src/services/TradeServiceImpl.hpp
// Phase 13.2: Real ITradeService implementation.
//
// The legacy trade commit path lives in CDealDialog::OnAction (client)
// -> MP_DEAL_SYN wire -> [Server]Map/DealDialog handler -> atomic
// inventory mutation + money settlement.  The modern service is the
// commit boundary: the dialog asks the service to commit, and the
// service delegates to a host-injected function (the MapHandler wires
// this up with its player_mu_-protected validation + DB persistence).
//
// Why a function pointer instead of holding the inventory + log directly?
//   * Trade is a per-player / per-counter-party operation; the inventory
//     mutation spans both players' ItemTotalInfo, so the service cannot
//     own either side.  The function pointer lets the orchestrator own
//     the cross-player lock.
//   * The MapHandler already has the critical section + the wire-format
//     MSG_TRADE packet layout; we reuse it.
//
// Threading: completeTrade() is called from the dialog's tick thread;
// the injected function runs inside the MapHandler's per-player mutex.
// The dialog holds the service pointer for the lifetime of the trade.

#pragma once

#include "mxh/services/ITradeService.hpp"

// Forward-declared to avoid pulling the full UI header chain; the
// ITradeService header forward-declares mxh::ui::DealItem already.
// The service implementation header pulls cdealdialog.hpp via the
// services CMakeLists INTERFACE include path (see
// modern/src/services/CMakeLists.txt: src/ui is on the include path
// for any consumer that needs DealItem by value).
#include "cdealdialog.hpp"

#include <cstdint>
#include <functional>
#include <vector>

namespace mxh::services {

class TradeServiceImpl final : public ITradeService {
public:
    // The commit function takes the same arguments as the interface
    // method (so the orchestrator can write a single lambda and let
    // the service forward).  Returning false from the function causes
    // completeTrade() to return false (the dialog will not mark the
    // deal as confirmed).
    using CommitFn = std::function<bool(const std::vector<mxh::ui::DealItem>&,
                                        const std::vector<mxh::ui::DealItem>&,
                                        std::uint32_t /*net_money*/)>;

    TradeServiceImpl() = default;
    explicit TradeServiceImpl(CommitFn fn) noexcept
        : m_commit(std::move(fn)) {}

    void setCommitter(CommitFn fn) noexcept {
        m_commit = std::move(fn);
    }

    // Fail-safe default: with no committer installed, completeTrade()
    // returns false (the dialog stays unconfirmed).  This matches the
    // legacy guard CDealDialog::OnAction when MP_DEAL_SYN dispatch is
    // not yet wired.
    bool completeTrade(const std::vector<mxh::ui::DealItem>& own_items,
                       const std::vector<mxh::ui::DealItem>& other_items,
                       std::uint32_t net_money) override {
        if (!m_commit) return false;
        return m_commit(own_items, other_items, net_money);
    }

    bool hasCommitter() const noexcept { return static_cast<bool>(m_commit); }

private:
    CommitFn m_commit;
};

}  // namespace mxh::services
