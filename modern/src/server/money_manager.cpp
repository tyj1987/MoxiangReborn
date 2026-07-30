// money_manager.cpp

#include "mxh/server/money_manager.hpp"

namespace mxh::server {



bool MoneyManager::set_money(std::uint32_t player_id, std::uint32_t amount,
                                const std::string& reason) noexcept {
    // Lua comment style: enforce ceiling; legacy clamps but does not fail.
    if (amount > MXH_PLAYER_MAX_MONEY) amount = MXH_PLAYER_MAX_MONEY;
    for (auto& r : ledger_) {
        if (r.player_id == player_id) {
            std::int64_t delta = static_cast<std::int64_t>(amount) - static_cast<std::int64_t>(r.money);
            if (log_sink_) {
                MoneyLogEntry e{}; e.player_id = player_id; e.delta = delta;
                e.before = r.money; e.after = amount; e.reason = reason;
                log_sink_(e);
            }
            r.money = amount;
            return true;
        }
    }
    Row r; r.player_id = player_id; r.money = amount;
    ledger_.push_back(r);
    if (log_sink_) {
        MoneyLogEntry e{}; e.player_id = player_id; e.delta = amount;
        e.before = 0; e.after = amount; e.reason = reason;
        log_sink_(e);
    }
    return true;
}

bool MoneyManager::add(std::uint32_t player_id, std::uint32_t amount,
                         const std::string& reason) noexcept {
    if (amount == 0) return false;
    for (auto& r : ledger_) {
        if (r.player_id == player_id) {
            std::uint64_t total = static_cast<std::uint64_t>(r.money) + amount;
            std::uint32_t after = total > MXH_PLAYER_MAX_MONEY ? MXH_PLAYER_MAX_MONEY : static_cast<std::uint32_t>(total);
            std::int64_t delta = static_cast<std::int64_t>(after) - static_cast<std::int64_t>(r.money);
            if (log_sink_) {
                MoneyLogEntry e{}; e.player_id = player_id; e.delta = delta;
                e.before = r.money; e.after = after; e.reason = reason;
                log_sink_(e);
            }
            r.money = after;
            return delta > 0;
        }
    }
    std::uint32_t after = amount > MXH_PLAYER_MAX_MONEY ? MXH_PLAYER_MAX_MONEY : amount;
    Row r; r.player_id = player_id; r.money = after;
    ledger_.push_back(r);
    if (log_sink_) {
        MoneyLogEntry e{}; e.player_id = player_id; e.delta = static_cast<std::int64_t>(after);
        e.before = 0; e.after = after; e.reason = reason;
        log_sink_(e);
    }
    return true;
}

bool MoneyManager::spend(std::uint32_t player_id, std::uint32_t amount,
                            const std::string& reason) noexcept {
    if (amount == 0) return false;
    for (auto& r : ledger_) {
        if (r.player_id == player_id) {
            if (r.money < amount) return false;     // insufficient
            std::uint32_t after = r.money - amount;
            std::int64_t delta = -static_cast<std::int64_t>(amount);
            if (log_sink_) {
                MoneyLogEntry e{}; e.player_id = player_id; e.delta = delta;
                e.before = r.money; e.after = after; e.reason = reason;
                log_sink_(e);
            }
            r.money = after;
            return true;
        }
    }
    return false;   // no record yet
}

std::uint32_t MoneyManager::balance(std::uint32_t player_id) const noexcept {
    for (const auto& r : ledger_) if (r.player_id == player_id) return r.money;
    return 0;
}

}  // namespace mxh::server



