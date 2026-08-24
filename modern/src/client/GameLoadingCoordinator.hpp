#pragma once

#include "GameStateStubs.hpp"
#include "StateTransfer.hpp"

#include <optional>
#include <string>

namespace mxh::client {

class CEngine;

class GameLoadingCoordinator {
public:
    bool consume_pending_transfer(CEngine& engine, std::string* error = nullptr);
    void mark_completed(std::uint32_t completed_steps) noexcept;
    void mark_failed(std::string message) noexcept;
    void cancel() noexcept;

    bool has_request() const noexcept { return m_request.has_value(); }
    const GameEntryRequest& request() const noexcept { return *m_request; }
    const LoadStateContext& context() const noexcept { return m_context; }
    bool terminal() const noexcept { return m_context.failed || m_context.cancelled; }

private:
    std::optional<GameEntryRequest> m_request;
    LoadStateContext m_context;
    std::string m_error;
};

} // namespace mxh::client
