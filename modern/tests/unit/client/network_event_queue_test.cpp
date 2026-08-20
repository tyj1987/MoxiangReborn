#include "NetworkEventQueue.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

namespace {

TEST(NetworkEventQueue, PreservesSingleProducerOrder) {
    mxh::client::NetworkEventQueue queue(4);
    for (std::uint32_t id = 1; id <= 3; ++id) {
        mxh::client::ClientRuntimeEvent event;
        event.kind = mxh::client::ClientRuntimeEventKind::Message;
        event.message.header.object_id = id;
        ASSERT_TRUE(queue.push(std::move(event)));
    }
    const auto events = queue.drain();
    ASSERT_EQ(events.size(), 3u);
    EXPECT_EQ(events[0].message.header.object_id, 1u);
    EXPECT_EQ(events[1].message.header.object_id, 2u);
    EXPECT_EQ(events[2].message.header.object_id, 3u);
}

TEST(NetworkEventQueue, IsBoundedAndCountsDrops) {
    mxh::client::NetworkEventQueue queue(2);
    EXPECT_TRUE(queue.push({}));
    EXPECT_TRUE(queue.push({}));
    EXPECT_FALSE(queue.push({}));
    EXPECT_EQ(queue.size(), 2u);
    EXPECT_EQ(queue.dropped_count(), 1u);
}

TEST(NetworkEventQueue, AcceptsConcurrentProducersWithoutDataRace) {
    constexpr int kThreads = 4;
    constexpr int kPerThread = 1000;
    mxh::client::NetworkEventQueue queue(kThreads * kPerThread);
    std::atomic<int> accepted{0};
    std::vector<std::thread> producers;
    for (int thread = 0; thread < kThreads; ++thread) {
        producers.emplace_back([&queue, &accepted, thread]() {
            for (int index = 0; index < kPerThread; ++index) {
                mxh::client::ClientRuntimeEvent event;
                event.kind = mxh::client::ClientRuntimeEventKind::Message;
                event.message.header.object_id =
                    static_cast<std::uint32_t>(thread * kPerThread + index);
                if (queue.push(std::move(event))) ++accepted;
            }
        });
    }
    for (auto& producer : producers) producer.join();
    EXPECT_EQ(accepted.load(), kThreads * kPerThread);
    EXPECT_EQ(queue.drain().size(), static_cast<std::size_t>(kThreads * kPerThread));
    EXPECT_EQ(queue.dropped_count(), 0u);
}

}  // namespace
