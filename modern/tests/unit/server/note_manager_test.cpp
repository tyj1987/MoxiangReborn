// note_manager_test.cpp

#include "mxh/server/note_manager.hpp"
#include <gtest/gtest.h>

namespace {
using mxh::server::NoteManager;
using mxh::server::AutoNoteManager;
using mxh::server::AutoNoteRoom;
}

TEST(NoteManager, SendAndRead) {
    NoteManager mgr;
    EXPECT_TRUE(mgr.send(/*sender*/7, /*recv*/100, "Hello from server", 1000));
    EXPECT_EQ(mgr.total(), 1u);
    auto inbox = mgr.read(100);
    ASSERT_EQ(inbox.size(), 1u);
    EXPECT_EQ(inbox[0].sender_id, 7u);
    EXPECT_STREQ(inbox[0].body, "Hello from server");
    EXPECT_GT(inbox[0].expire_ms, inbox[0].send_ms);
}

TEST(NoteManager, InboxCountFiltersByRecipient) {
    NoteManager mgr;
    mgr.send(1, 100, "alpha", 1000);
    mgr.send(2, 200, "beta",  1000);
    mgr.send(3, 100, "gamma", 1000);
    EXPECT_EQ(mgr.inbox_count(100), 2u);
    EXPECT_EQ(mgr.inbox_count(200), 1u);
    EXPECT_EQ(mgr.inbox_count(999), 0u);
}

TEST(NoteManager, AcknowledgeMarksExpired) {
    NoteManager mgr;
    mgr.send(1, 100, "msg", 1000);
    auto inbox = mgr.read(100);
    ASSERT_EQ(inbox.size(), 1u);
    EXPECT_TRUE(mgr.acknowledge(100, inbox[0].note_id));
    // After ack, inbox is empty (expire_ms != 0 is sentinel).
    EXPECT_EQ(mgr.read(100).size(), 0u);
}

TEST(NoteManager, BodyTruncatesAt100Chars) {
    NoteManager mgr;
    std::string big(150, 'x');
    mgr.send(1, 2, big, 1000);
    auto inbox = mgr.read(2);
    ASSERT_EQ(inbox.size(), 1u);
    EXPECT_EQ(std::strlen(inbox[0].body), 100u);
}

TEST(AutoNoteManager, EnqueueAndAcknowledge) {
    AutoNoteManager mgr;
    mxh::server::AutoNoteEntry e{};
    e.target_player = 100;
    e.kind = 1;
    EXPECT_TRUE(mgr.enqueue(e));
    EXPECT_EQ(mgr.size(), 1u);
    auto snap = mgr.snapshot();
    EXPECT_TRUE(mgr.acknowledge(snap[0].note_id));
    EXPECT_EQ(mgr.size(), 0u);
}

TEST(AutoNoteManager, EnforceLimit) {
    AutoNoteManager mgr;
    for (std::uint32_t i = 0; i <1000; ++i) {
        mxh::server::AutoNoteEntry e{};
        EXPECT_TRUE(mgr.enqueue(e));
    }
    EXPECT_EQ(mgr.size(), 1000u);
    mxh::server::AutoNoteEntry overflow{};
    EXPECT_FALSE(mgr.enqueue(overflow));
}

TEST(AutoNoteRoom, AddHasRemove) {
    AutoNoteRoom r;
    r.add_player(1); r.add_player(2); r.add_player(3);
    r.add_player(0);  // ignored
    r.add_player(2);  // duplicate -- ignored
    EXPECT_EQ(r.size(), 3u);
    EXPECT_TRUE(r.has(2));
    EXPECT_FALSE(r.has(99));
    EXPECT_TRUE(r.remove_player(2));
    EXPECT_EQ(r.size(), 2u);
    EXPECT_FALSE(r.remove_player(99));
}


