#include "NetworkEventQueue.hpp"

#include <utility>

namespace mxh::client {

NetworkEventQueue::NetworkEventQueue(std::size_t capacity) noexcept
    : m_capacity(capacity == 0 ? 1 : capacity) {}

bool NetworkEventQueue::push(ClientRuntimeEvent event) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_events.size() >= m_capacity) {
        ++m_dropped;
        return false;
    }
    m_events.push_back(std::move(event));
    return true;
}

std::vector<ClientRuntimeEvent> NetworkEventQueue::drain() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ClientRuntimeEvent> result;
    result.reserve(m_events.size());
    while (!m_events.empty()) {
        result.push_back(std::move(m_events.front()));
        m_events.pop_front();
    }
    return result;
}

std::size_t NetworkEventQueue::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_events.size();
}

std::uint64_t NetworkEventQueue::dropped_count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_dropped;
}

void NetworkEventQueue::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_events.clear();
}

}  // namespace mxh::client
