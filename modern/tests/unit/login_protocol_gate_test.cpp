// Login Item-protocol gate. The client must present the build-time Item
// protocol hash before login or agent entry. A mismatch, or a missing
// hash, is a terminal reject: reason byte 3 on the modern version nack,
// a user-visible sentence, and the server log
// PROTOCOL_MISMATCH item=<client-or-missing>/<server>.
//
// Hashes in this file are synthetic. The real value stays in the
// CMake-generated protocol_hash.hpp and is not committed here.
//
// Until login_protocol_gate.hpp exists, the stand-in below copies today's
// login path: a version in range is enough, RequestLogin proceeds, and
// there is no ProtocolMismatch reason or PROTOCOL_MISMATCH log.

#include "mxh/proto/protocol.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#if defined(__has_include)
#  if __has_include("mxh/proto/login_protocol_gate.hpp")
#    include "mxh/proto/login_protocol_gate.hpp"
#    define MXH_LOGIN_PROTOCOL_GATE_HEADER 1
#  endif
#endif

#ifndef MXH_LOGIN_PROTOCOL_GATE_HEADER
namespace mxh::proto {

struct LoginItemProtocolDecision {
    bool accepted = false;
    std::uint8_t reason = 0;
    std::string log_line;
    std::string user_message;
};

inline const char* login_protocol_user_message(std::uint8_t reason) {
    switch (reason) {
    case 1: return "Client version is too new.";
    case 2: return "Server is full.";
    default: return "Client version is too old.";
    }
}

inline LoginItemProtocolDecision evaluate_login_item_protocol(
    std::string_view, std::string_view) {
    LoginItemProtocolDecision decision;
    decision.accepted = true;
    decision.reason = 0;
    decision.user_message = login_protocol_user_message(0);
    return decision;
}

inline std::string item_hash_from_check_version(const std::uint8_t*, std::size_t) {
    return {};
}

inline void append_item_hash_to_check_version(std::vector<std::uint8_t>&,
                                              std::string_view) {}

inline void write_version_nack_payload(std::uint8_t* out,
                                       std::uint16_t server_version,
                                       std::uint8_t reason) {
    out[0] = static_cast<std::uint8_t>(server_version & 0xFF);
    out[1] = static_cast<std::uint8_t>((server_version >> 8) & 0xFF);
    out[2] = reason;
}

inline std::uint8_t version_nack_reason(const std::uint8_t* payload, std::size_t size) {
    if (payload == nullptr || size < 3) return 0;
    return payload[2];
}

}  // namespace mxh::proto
#endif

namespace {

constexpr char kMismatchUserText[] =
    "Item protocol mismatch. The client and server Item protocols differ.";

std::string hex64(char digit) {
    return std::string(64, digit);
}

}  // namespace

TEST(LoginProtocolGate, MismatchRejectedWithReason) {
    const std::string client = hex64('a');
    const std::string server = hex64('b');
    const auto decision = mxh::proto::evaluate_login_item_protocol(client, server);

    EXPECT_FALSE(decision.accepted);
    EXPECT_EQ(decision.reason, 3u);
    EXPECT_EQ(decision.log_line, "PROTOCOL_MISMATCH item=" + client + "/" + server);
    EXPECT_EQ(decision.user_message, kMismatchUserText);
    EXPECT_EQ(decision.user_message,
              mxh::proto::login_protocol_user_message(decision.reason));
    EXPECT_NE(decision.user_message, mxh::proto::login_protocol_user_message(0));
    EXPECT_NE(decision.user_message, mxh::proto::login_protocol_user_message(1));
    EXPECT_NE(decision.user_message, mxh::proto::login_protocol_user_message(2));

    std::string upper = client;
    upper[0] = 'A';
    const auto case_mismatch = mxh::proto::evaluate_login_item_protocol(upper, client);
    EXPECT_FALSE(case_mismatch.accepted);
    EXPECT_EQ(case_mismatch.reason, 3u);
    EXPECT_EQ(case_mismatch.log_line, "PROTOCOL_MISMATCH item=missing/" + client);
}

TEST(LoginProtocolGate, MissingHashRejected) {
    const std::string server = hex64('c');
    const auto absent = mxh::proto::evaluate_login_item_protocol("", server);
    EXPECT_FALSE(absent.accepted);
    EXPECT_EQ(absent.reason, 3u);
    EXPECT_EQ(absent.log_line, "PROTOCOL_MISMATCH item=missing/" + server);
    EXPECT_EQ(absent.user_message, kMismatchUserText);

    const auto blank = mxh::proto::evaluate_login_item_protocol("   ", server);
    EXPECT_EQ(blank.reason, absent.reason);
    EXPECT_EQ(blank.log_line, absent.log_line);
    EXPECT_FALSE(blank.accepted);

    const auto short_token = mxh::proto::evaluate_login_item_protocol("abc", server);
    EXPECT_EQ(short_token.log_line, "PROTOCOL_MISMATCH item=missing/" + server);
    EXPECT_EQ(short_token.reason, absent.reason);

    const std::uint8_t version_only[] = {0x01, 0x00, 0x00};
    const auto parsed = mxh::proto::item_hash_from_check_version(
        version_only, sizeof(version_only));
    const auto from_body = mxh::proto::evaluate_login_item_protocol(parsed, server);
    EXPECT_FALSE(from_body.accepted);
    EXPECT_EQ(from_body.reason, absent.reason);
    EXPECT_EQ(from_body.log_line, absent.log_line);
    EXPECT_FALSE(from_body.user_message.empty());
}

TEST(LoginProtocolGate, MatchAccepted) {
    const std::string hash = hex64('a');
    std::vector<std::uint8_t> payload{0x01, 0x00, 0x00};
    mxh::proto::append_item_hash_to_check_version(payload, hash);
    ASSERT_EQ(payload.size(), 3u + 1u + hash.size());
    EXPECT_EQ(payload[0], 0x01);
    EXPECT_EQ(payload[1], 0x00);
    EXPECT_EQ(payload[2], 0x00);

    const auto parsed = mxh::proto::item_hash_from_check_version(
        payload.data(), payload.size());
    const auto decision = mxh::proto::evaluate_login_item_protocol(parsed, hash);
    EXPECT_TRUE(decision.accepted);
    EXPECT_TRUE(decision.log_line.empty());
    EXPECT_TRUE(decision.user_message.empty());
    EXPECT_NE(decision.reason, 3u);
}

TEST(LoginProtocolGate, ReasonCodeReachesClient) {
    EXPECT_EQ(mxh::proto::kModernCheckVersion, 200u);
    EXPECT_EQ(mxh::proto::kModernNotifyVersionAck, 201u);
    EXPECT_EQ(mxh::proto::kModernNotifyVersionNack, 202u);
    EXPECT_EQ(static_cast<unsigned>(mxh::proto::UserConnProtocol::RequestLogin), 1u);
    EXPECT_EQ(static_cast<std::uint8_t>(mxh::proto::VersionRejectReason::TooOld), 0u);
    EXPECT_EQ(static_cast<std::uint8_t>(mxh::proto::VersionRejectReason::TooNew), 1u);
    EXPECT_EQ(static_cast<std::uint8_t>(mxh::proto::VersionRejectReason::ServerFull), 2u);

    const auto decision = mxh::proto::evaluate_login_item_protocol(
        hex64('d'), hex64('e'));
    std::uint8_t nack[3] = {};
    mxh::proto::write_version_nack_payload(nack, /*server_version=*/1, decision.reason);
    EXPECT_EQ(nack[0], 0x01);
    EXPECT_EQ(nack[1], 0x00);
    const auto shown_reason = mxh::proto::version_nack_reason(nack, sizeof(nack));
    EXPECT_EQ(shown_reason, 3u);
    EXPECT_NE(shown_reason, static_cast<std::uint8_t>(mxh::proto::VersionRejectReason::TooOld));
    EXPECT_NE(shown_reason, static_cast<std::uint8_t>(mxh::proto::VersionRejectReason::TooNew));
    EXPECT_NE(shown_reason, static_cast<std::uint8_t>(mxh::proto::VersionRejectReason::ServerFull));

    const char* shown = mxh::proto::login_protocol_user_message(shown_reason);
    EXPECT_STREQ(shown, decision.user_message.c_str());
    EXPECT_STREQ(shown, kMismatchUserText);
    EXPECT_STRNE(shown, mxh::proto::login_protocol_user_message(0));
    EXPECT_STRNE(shown, mxh::proto::login_protocol_user_message(1));
    EXPECT_STRNE(shown, mxh::proto::login_protocol_user_message(2));
    EXPECT_NE(std::string(shown).find("protocol"), std::string::npos);

    EXPECT_EQ(std::string(mxh::proto::login_protocol_user_message(0)),
              "Client version is too old.");
    EXPECT_EQ(std::string(mxh::proto::login_protocol_user_message(1)),
              "Client version is too new.");
    EXPECT_EQ(std::string(mxh::proto::login_protocol_user_message(2)),
              "Server is full.");
}
