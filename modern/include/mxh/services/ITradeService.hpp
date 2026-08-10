#pragma once

#include <cstdint>
#include <vector>

namespace mxh::ui { struct DealItem; }

namespace mxh::services {

// Phase B trade commit boundary.  The service owns atomic validation,
// inventory mutation, money settlement, and network/database side effects.
class ITradeService {
public:
    virtual ~ITradeService() = default;
    virtual bool completeTrade(const std::vector<mxh::ui::DealItem>& own_items,
                               const std::vector<mxh::ui::DealItem>& other_items,
                               std::uint32_t net_money) = 0;
};

} // namespace mxh::services
