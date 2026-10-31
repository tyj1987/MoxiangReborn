// Handler-level Item protocol gate. Drives LoginHandler and AgentHandler
// directly. The gate applies to legacy framing and modern framing.
// Synthetic hashes are wrong on purpose. A match uses kItemProtocolSha256
// from the CMake header and does not embed that digest in this file.

#include "mxh/db/db_adapter.hpp"
#include "mxh/proto/login_protocol_gate.hpp"
#include "mxh/proto/protocol.hpp"
#include "mxh/proto/protocol_hash.hpp"
#include "mxh/server/server.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>

namespace {

class CountingDb final : public mxh::db::IDbAdapter {
public:
    mxh::db::DbResult connect(const mxh::db::ConnectionConfig&) override { return {}; }
    void disconnect() override {}
    bool is_connected() const noexcept override { return true; }
    mxh::db::DbResult execute(std::string_view, std::span<const mxh::db::Bind>) override {
        ++execute_count;
        return {};
    }
    mxh::db::DbResult query(std::string_view sql, std::span<const mxh::db::Bind>,
                            mxh::db::ResultSet& out) override {
        ++query_count;
        out = {};
        if (sql.find("FROM chr_log_info") != std::string_view::npos) {
            out.columns = {"id", "pw", "userlevel"};
            out.rows.push_back({std::string("abc"), std::string("password1"),
                                std::int64_t{2}});
        } else if (sql.find("COALESCE(MAX(user_idx)") != std::string_view::npos) {
            out.columns = {"user_idx"};
            out.rows.push_back({std::int64_t{7}});
        }
        return {};
    }
    mxh::db::DbResult begin_transaction() override { return {}; }
    mxh::db::DbResult commit() override { return {}; }
    mxh::db::DbResult rollback() override { return {}; }
    std::string backend_name() const noexcept override { return "mock"; }

    int query_count = 0;
    int execute_count = 0;
};

struct ReplySpy {
    int call_count = 0;
    mxh::net::Message last{};
    void reset() { call_count = 0; last = {}; }
};

mxh::server::ReplyFn spy_reply(ReplySpy& spy) {
    return [&spy](mxh::net::ConnectionId, const mxh::net::Message& message) {
        ++spy.call_count;
        spy.last = message;
    };
}

class CoutCapture {
public:
    CoutCapture() : old_(std::cout.rdbuf(buffer_.rdbuf())) {}
    ~CoutCapture() { std::cout.rdbuf(old_); }
    std::string str() const { return buffer_.str(); }
private:
    std::stringstream buffer_;
    std::streambuf* old_;
};

mxh::net::Message check_version_message(std::string_view hash) {
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    msg.header.protocol = mxh::proto::kModernCheckVersion;
    msg.payload = mxh::proto::make_check_version_payload(
        mxh::proto::kProtocolVersion, "test", hash);
    return msg;
}

mxh::net::Message check_version_without_hash() {
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    msg.header.protocol = mxh::proto::kModernCheckVersion;
    msg.payload = {0x01, 0x00, 0x00};
    return msg;
}

mxh::net::Message legacy_login_message() {
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::RequestLogin);
    msg.payload.assign(38, 0);
    const char id[] = "abc";
    const char pw[] = "password1";
    std::memcpy(msg.payload.data() + 4, id, sizeof(id) - 1);
    std::memcpy(msg.payload.data() + 21, pw, sizeof(pw) - 1);
    return msg;
}

mxh::net::Message modern_login_message() {
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::RequestLogin);
    const std::string id = "abc";
    const std::string pw = "password1";
    msg.payload.push_back(static_cast<std::uint8_t>(id.size()));
    msg.payload.push_back(0);
    msg.payload.insert(msg.payload.end(), id.begin(), id.end());
    msg.payload.push_back(static_cast<std::uint8_t>(pw.size()));
    msg.payload.push_back(0);
    msg.payload.insert(msg.payload.end(), pw.begin(), pw.end());
    return msg;
}

mxh::net::Message character_list_message() {
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    msg.header.protocol = static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterListSyn);
    msg.payload.assign(8, 0);
    return msg;
}

mxh::net::Message item_message() {
    mxh::net::Message msg;
    msg.header.category = static_cast<std::uint8_t>(mxh::proto::Category::Item);
    msg.header.protocol = 1;
    msg.payload = {1, 2, 3};
    return msg;
}

void expect_protocol_mismatch(const mxh::net::Message& message) {
    EXPECT_EQ(message.header.protocol, mxh::proto::kModernNotifyVersionNack);
    ASSERT_GE(message.payload.size(), 3u);
    EXPECT_EQ(message.payload[2],
              static_cast<std::uint8_t>(mxh::proto::VersionRejectReason::ProtocolMismatch));
    EXPECT_EQ(message.payload[2], 3u);
}

class LoginGate : public ::testing::TestWithParam<bool> {
protected:
    bool legacy() const { return GetParam(); }
    CountingDb db;
    ReplySpy reply;
    mxh::server::LoginHandler handler{
        db, "127.0.0.1", 7000, spy_reply(reply), legacy()};
    mxh::net::ConnectionId id = mxh::net::make_connection_id(41);
};

TEST_P(LoginGate, MismatchRejectedWithReason3AndLog) {
    CoutCapture capture;
    handler.on_message(id, check_version_message(std::string(64, 'a')));
    ASSERT_EQ(reply.call_count, 1);
    expect_protocol_mismatch(reply.last);
    EXPECT_NE(capture.str().find("PROTOCOL_MISMATCH item="), std::string::npos);
    EXPECT_EQ(db.query_count, 0);
}

TEST_P(LoginGate, MissingHashRejectedWithReason3) {
    CoutCapture capture;
    handler.on_message(id, check_version_without_hash());
    ASSERT_EQ(reply.call_count, 1);
    expect_protocol_mismatch(reply.last);
    EXPECT_NE(capture.str().find("PROTOCOL_MISMATCH item=missing/"), std::string::npos);
    EXPECT_EQ(db.query_count, 0);
}

TEST_P(LoginGate, RequestLoginBeforeVerificationLeavesDbUntouched) {
    const auto login = legacy() ? legacy_login_message() : modern_login_message();
    handler.on_message(id, login);
    ASSERT_EQ(reply.call_count, 1);
    expect_protocol_mismatch(reply.last);
    EXPECT_EQ(db.query_count, 0);
    EXPECT_EQ(db.execute_count, 0);
}

TEST_P(LoginGate, MatchThenRequestLoginCompletes) {
    handler.on_message(id, check_version_message(mxh::proto::kItemProtocolSha256));
    ASSERT_EQ(reply.call_count, 1);
    EXPECT_EQ(reply.last.header.protocol, mxh::proto::kModernNotifyVersionAck);
    reply.reset();
    const int queries_before = db.query_count;
    handler.on_message(id, legacy() ? legacy_login_message() : modern_login_message());
    ASSERT_EQ(reply.call_count, 1);
    EXPECT_EQ(reply.last.header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::NotifyUserLoginAck));
    EXPECT_GT(db.query_count, queries_before);
}

TEST_P(LoginGate, DisconnectThenSameIdMustCheckAgain) {
    handler.on_message(id, check_version_message(mxh::proto::kItemProtocolSha256));
    ASSERT_EQ(reply.last.header.protocol, mxh::proto::kModernNotifyVersionAck);
    handler.on_disconnect(id, mxh::net::NetError::Disconnected);
    reply.reset();
    handler.on_message(id, legacy() ? legacy_login_message() : modern_login_message());
    ASSERT_EQ(reply.call_count, 1);
    expect_protocol_mismatch(reply.last);
    EXPECT_EQ(db.query_count, 0);
}

INSTANTIATE_TEST_SUITE_P(LegacyAndModern, LoginGate, ::testing::Bool(),
    [](const ::testing::TestParamInfo<bool>& info) {
        return info.param ? "Legacy" : "Modern";
    });

TEST(LoginHandlerProtocolGate, LegacyRequestLoginWithoutCheckVersionRejected) {
    CountingDb db;
    ReplySpy reply;
    mxh::server::LoginHandler handler(db, "127.0.0.1", 7000, spy_reply(reply), true);
    const auto id = mxh::net::make_connection_id(7);
    handler.on_message(id, legacy_login_message());
    ASSERT_EQ(reply.call_count, 1);
    expect_protocol_mismatch(reply.last);
    EXPECT_EQ(db.query_count, 0);
}

class AgentGate : public ::testing::TestWithParam<bool> {
protected:
    bool legacy() const { return GetParam(); }
    CountingDb db;
    ReplySpy reply;
    mxh::server::AgentHandler handler{
        db, spy_reply(reply), legacy(), false, {}, /*default_map_num=*/12};
    mxh::net::ConnectionId id = mxh::net::make_connection_id(88);
};

TEST_P(AgentGate, MismatchRejectedWithReason3AndLog) {
    CoutCapture capture;
    handler.on_message(id, check_version_message(std::string(64, 'b')));
    ASSERT_EQ(reply.call_count, 1);
    expect_protocol_mismatch(reply.last);
    EXPECT_NE(capture.str().find("PROTOCOL_MISMATCH item="), std::string::npos);
}

TEST_P(AgentGate, MissingHashRejectedWithReason3) {
    handler.on_message(id, check_version_without_hash());
    ASSERT_EQ(reply.call_count, 1);
    expect_protocol_mismatch(reply.last);
}

TEST_P(AgentGate, CharacterListBeforeVerificationLeavesDbUntouched) {
    const int queries_before = db.query_count;
    handler.on_message(id, character_list_message());
    ASSERT_EQ(reply.call_count, 1);
    expect_protocol_mismatch(reply.last);
    EXPECT_EQ(db.query_count, queries_before);
}

TEST_P(AgentGate, NonUserConnBeforeVerificationRejected) {
    handler.on_message(id, item_message());
    ASSERT_EQ(reply.call_count, 1);
    expect_protocol_mismatch(reply.last);
    EXPECT_EQ(db.query_count, 0);
}

TEST_P(AgentGate, MatchThenCharacterListCompletes) {
    handler.on_message(id, check_version_message(mxh::proto::kItemProtocolSha256));
    ASSERT_EQ(reply.call_count, 1);
    EXPECT_EQ(reply.last.header.protocol, mxh::proto::kModernNotifyVersionAck);
    reply.reset();
    handler.on_message(id, character_list_message());
    ASSERT_EQ(reply.call_count, 1);
    EXPECT_EQ(reply.last.header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterListAck));
    EXPECT_NE(reply.last.header.protocol, mxh::proto::kModernNotifyVersionNack);
}

TEST_P(AgentGate, DisconnectThenSameIdMustCheckAgain) {
    handler.on_message(id, check_version_message(mxh::proto::kItemProtocolSha256));
    handler.on_disconnect(id, mxh::net::NetError::Disconnected);
    reply.reset();
    handler.on_message(id, character_list_message());
    ASSERT_EQ(reply.call_count, 1);
    expect_protocol_mismatch(reply.last);
}

INSTANTIATE_TEST_SUITE_P(LegacyAndModern, AgentGate, ::testing::Bool(),
    [](const ::testing::TestParamInfo<bool>& info) {
        return info.param ? "Legacy" : "Modern";
    });

}  // namespace
