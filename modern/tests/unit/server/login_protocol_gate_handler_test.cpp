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

struct LoginGate {
    CountingDb db;
    ReplySpy reply;
    mxh::server::LoginHandler handler;
    mxh::net::ConnectionId id = mxh::net::make_connection_id(41);

    explicit LoginGate(bool legacy)
        : handler(db, "127.0.0.1", 7000, spy_reply(reply), legacy) {}
};

void login_mismatch_rejected(bool legacy) {
    LoginGate gate(legacy);
    CoutCapture capture;
    gate.handler.on_message(gate.id, check_version_message(std::string(64, 'a')));
    ASSERT_EQ(gate.reply.call_count, 1);
    expect_protocol_mismatch(gate.reply.last);
    EXPECT_NE(capture.str().find("PROTOCOL_MISMATCH item="), std::string::npos);
    EXPECT_EQ(gate.db.query_count, 0);
}

void login_missing_hash_rejected(bool legacy) {
    LoginGate gate(legacy);
    CoutCapture capture;
    gate.handler.on_message(gate.id, check_version_without_hash());
    ASSERT_EQ(gate.reply.call_count, 1);
    expect_protocol_mismatch(gate.reply.last);
    EXPECT_NE(capture.str().find("PROTOCOL_MISMATCH item=missing/"), std::string::npos);
    EXPECT_EQ(gate.db.query_count, 0);
}

void login_before_verification_leaves_db_untouched(bool legacy) {
    LoginGate gate(legacy);
    gate.handler.on_message(gate.id, legacy ? legacy_login_message()
                                            : modern_login_message());
    ASSERT_EQ(gate.reply.call_count, 1);
    expect_protocol_mismatch(gate.reply.last);
    EXPECT_EQ(gate.db.query_count, 0);
    EXPECT_EQ(gate.db.execute_count, 0);
}

void login_match_then_request_completes(bool legacy) {
    LoginGate gate(legacy);
    gate.handler.on_message(
        gate.id, check_version_message(mxh::proto::kItemProtocolSha256));
    ASSERT_EQ(gate.reply.call_count, 1);
    EXPECT_EQ(gate.reply.last.header.protocol, mxh::proto::kModernNotifyVersionAck);
    gate.reply.reset();
    const int queries_before = gate.db.query_count;
    gate.handler.on_message(gate.id, legacy ? legacy_login_message()
                                            : modern_login_message());
    ASSERT_EQ(gate.reply.call_count, 1);
    EXPECT_EQ(gate.reply.last.header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::NotifyUserLoginAck));
    EXPECT_GT(gate.db.query_count, queries_before);
}

void login_disconnect_requires_check_again(bool legacy) {
    LoginGate gate(legacy);
    gate.handler.on_message(
        gate.id, check_version_message(mxh::proto::kItemProtocolSha256));
    ASSERT_EQ(gate.reply.last.header.protocol, mxh::proto::kModernNotifyVersionAck);
    gate.handler.on_disconnect(gate.id, mxh::net::NetError::Disconnected);
    gate.reply.reset();
    gate.handler.on_message(gate.id, legacy ? legacy_login_message()
                                            : modern_login_message());
    ASSERT_EQ(gate.reply.call_count, 1);
    expect_protocol_mismatch(gate.reply.last);
    EXPECT_EQ(gate.db.query_count, 0);
}

void login_without_check_version_rejected(bool legacy) {
    CountingDb db;
    ReplySpy reply;
    mxh::server::LoginHandler handler(db, "127.0.0.1", 7000, spy_reply(reply), legacy);
    const auto id = mxh::net::make_connection_id(7);
    handler.on_message(id, legacy ? legacy_login_message() : modern_login_message());
    ASSERT_EQ(reply.call_count, 1);
    expect_protocol_mismatch(reply.last);
    EXPECT_EQ(db.query_count, 0);
}

TEST(LoginHandlerProtocolGate, LegacyMismatchRejectedWithReason3AndLog) {
    login_mismatch_rejected(true);
}
TEST(LoginHandlerProtocolGate, ModernMismatchRejectedWithReason3AndLog) {
    login_mismatch_rejected(false);
}
TEST(LoginHandlerProtocolGate, LegacyMissingHashRejectedWithReason3) {
    login_missing_hash_rejected(true);
}
TEST(LoginHandlerProtocolGate, ModernMissingHashRejectedWithReason3) {
    login_missing_hash_rejected(false);
}
TEST(LoginHandlerProtocolGate, LegacyRequestLoginBeforeVerificationLeavesDbUntouched) {
    login_before_verification_leaves_db_untouched(true);
}
TEST(LoginHandlerProtocolGate, ModernRequestLoginBeforeVerificationLeavesDbUntouched) {
    login_before_verification_leaves_db_untouched(false);
}
TEST(LoginHandlerProtocolGate, LegacyMatchThenRequestLoginCompletes) {
    login_match_then_request_completes(true);
}
TEST(LoginHandlerProtocolGate, ModernMatchThenRequestLoginCompletes) {
    login_match_then_request_completes(false);
}
TEST(LoginHandlerProtocolGate, LegacyDisconnectThenSameIdMustCheckAgain) {
    login_disconnect_requires_check_again(true);
}
TEST(LoginHandlerProtocolGate, ModernDisconnectThenSameIdMustCheckAgain) {
    login_disconnect_requires_check_again(false);
}
TEST(LoginHandlerProtocolGate, LegacyRequestLoginWithoutCheckVersionRejected) {
    login_without_check_version_rejected(true);
}
TEST(LoginHandlerProtocolGate, ModernRequestLoginWithoutCheckVersionRejected) {
    login_without_check_version_rejected(false);
}

struct AgentGate {
    CountingDb db;
    ReplySpy reply;
    mxh::server::AgentHandler handler;
    mxh::net::ConnectionId id = mxh::net::make_connection_id(88);

    explicit AgentGate(bool legacy)
        : handler(db, spy_reply(reply), legacy, false, {}, /*default_map_num=*/12) {}
};

void agent_mismatch_rejected(bool legacy) {
    AgentGate gate(legacy);
    CoutCapture capture;
    gate.handler.on_message(gate.id, check_version_message(std::string(64, 'b')));
    ASSERT_EQ(gate.reply.call_count, 1);
    expect_protocol_mismatch(gate.reply.last);
    EXPECT_NE(capture.str().find("PROTOCOL_MISMATCH item="), std::string::npos);
}

void agent_missing_hash_rejected(bool legacy) {
    AgentGate gate(legacy);
    gate.handler.on_message(gate.id, check_version_without_hash());
    ASSERT_EQ(gate.reply.call_count, 1);
    expect_protocol_mismatch(gate.reply.last);
}

void agent_character_list_before_verification(bool legacy) {
    AgentGate gate(legacy);
    const int queries_before = gate.db.query_count;
    gate.handler.on_message(gate.id, character_list_message());
    ASSERT_EQ(gate.reply.call_count, 1);
    expect_protocol_mismatch(gate.reply.last);
    EXPECT_EQ(gate.db.query_count, queries_before);
}

void agent_non_userconn_before_verification(bool legacy) {
    AgentGate gate(legacy);
    gate.handler.on_message(gate.id, item_message());
    ASSERT_EQ(gate.reply.call_count, 1);
    expect_protocol_mismatch(gate.reply.last);
    EXPECT_EQ(gate.db.query_count, 0);
}

void agent_match_then_character_list(bool legacy) {
    AgentGate gate(legacy);
    gate.handler.on_message(
        gate.id, check_version_message(mxh::proto::kItemProtocolSha256));
    ASSERT_EQ(gate.reply.call_count, 1);
    EXPECT_EQ(gate.reply.last.header.protocol, mxh::proto::kModernNotifyVersionAck);
    gate.reply.reset();
    gate.handler.on_message(gate.id, character_list_message());
    ASSERT_EQ(gate.reply.call_count, 1);
    EXPECT_EQ(gate.reply.last.header.protocol, static_cast<std::uint8_t>(
        mxh::proto::UserConnProtocol::CharacterListAck));
    EXPECT_NE(gate.reply.last.header.protocol, mxh::proto::kModernNotifyVersionNack);
}

void agent_disconnect_requires_check_again(bool legacy) {
    AgentGate gate(legacy);
    gate.handler.on_message(
        gate.id, check_version_message(mxh::proto::kItemProtocolSha256));
    gate.handler.on_disconnect(gate.id, mxh::net::NetError::Disconnected);
    gate.reply.reset();
    gate.handler.on_message(gate.id, character_list_message());
    ASSERT_EQ(gate.reply.call_count, 1);
    expect_protocol_mismatch(gate.reply.last);
}

TEST(AgentHandlerProtocolGate, LegacyMismatchRejectedWithReason3AndLog) {
    agent_mismatch_rejected(true);
}
TEST(AgentHandlerProtocolGate, ModernMismatchRejectedWithReason3AndLog) {
    agent_mismatch_rejected(false);
}
TEST(AgentHandlerProtocolGate, LegacyMissingHashRejectedWithReason3) {
    agent_missing_hash_rejected(true);
}
TEST(AgentHandlerProtocolGate, ModernMissingHashRejectedWithReason3) {
    agent_missing_hash_rejected(false);
}
TEST(AgentHandlerProtocolGate, LegacyCharacterListBeforeVerificationLeavesDbUntouched) {
    agent_character_list_before_verification(true);
}
TEST(AgentHandlerProtocolGate, ModernCharacterListBeforeVerificationLeavesDbUntouched) {
    agent_character_list_before_verification(false);
}
TEST(AgentHandlerProtocolGate, LegacyNonUserConnBeforeVerificationRejected) {
    agent_non_userconn_before_verification(true);
}
TEST(AgentHandlerProtocolGate, ModernNonUserConnBeforeVerificationRejected) {
    agent_non_userconn_before_verification(false);
}
TEST(AgentHandlerProtocolGate, LegacyMatchThenCharacterListCompletes) {
    agent_match_then_character_list(true);
}
TEST(AgentHandlerProtocolGate, ModernMatchThenCharacterListCompletes) {
    agent_match_then_character_list(false);
}
TEST(AgentHandlerProtocolGate, LegacyDisconnectThenSameIdMustCheckAgain) {
    agent_disconnect_requires_check_again(true);
}
TEST(AgentHandlerProtocolGate, ModernDisconnectThenSameIdMustCheckAgain) {
    agent_disconnect_requires_check_again(false);
}

}  // namespace
