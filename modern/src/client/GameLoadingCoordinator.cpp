#include "GameLoadingCoordinator.hpp"

#include "CEngine.hpp"

namespace mxh::client {

bool GameLoadingCoordinator::consume_pending_transfer(CEngine& engine, std::string* error) {
    if (!engine.has_pending_transfer()) {
        if (error) *error = "waiting for GameEntryRequest";
        return false;
    }
    auto transfer = engine.TakePendingTransfer();
    const auto* request = std::get_if<GameEntryRequest>(&transfer);
    if (!request || request->character_id == 0 || request->map_num == 0) {
        if (error) *error = "invalid GameEntryRequest";
        mark_failed(error ? *error : "invalid GameEntryRequest");
        return false;
    }
    m_request = *request;
    m_context.character_id = request->character_id;
    m_context.map_num = request->map_num;
    m_context.completed_steps = 0;
    m_context.total_steps = 10;
    m_context.cancelled = false;
    m_context.failed = false;
    m_context.error = nullptr;
    return true;
}

void GameLoadingCoordinator::mark_completed(std::uint32_t completed_steps) noexcept {
    m_context.completed_steps = completed_steps > m_context.total_steps
        ? m_context.total_steps : completed_steps;
}

void GameLoadingCoordinator::mark_failed(std::string message) noexcept {
    m_context.failed = true;
    m_context.completed_steps = 0;
    m_error = std::move(message);
    m_context.error = m_error.c_str();
}

void GameLoadingCoordinator::cancel() noexcept {
    m_context.cancelled = true;
}

} // namespace mxh::client
