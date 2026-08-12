#include <gtest/gtest.h>

#include "../../../tools/MoxianGMTool/gm_security.hpp"

TEST(GmSecurity, ConstantTimeEqualityHandlesLengthAndContent) {
    EXPECT_TRUE(mxh::gm::constant_time_equal("same-token", "same-token"));
    EXPECT_FALSE(mxh::gm::constant_time_equal("same-token", "same-tokem"));
    EXPECT_FALSE(mxh::gm::constant_time_equal("short", "shorter"));
}

TEST(GmSecurity, RejectsMissingOrMalformedAuthorization) {
    const std::string token(32, 'a');
    EXPECT_FALSE(mxh::gm::authorize_bearer({}, token));
    EXPECT_FALSE(mxh::gm::authorize_bearer({{"Authorization", token}}, token));
    EXPECT_FALSE(mxh::gm::authorize_bearer({{"Authorization", "Basic " + token}}, token));
    EXPECT_FALSE(mxh::gm::authorize_bearer({{"Authorization", "Bearer wrong"}}, token));
}

TEST(GmSecurity, AcceptsBearerWithCaseInsensitiveHeaderName) {
    const std::string token = "0123456789abcdef0123456789abcdef";
    EXPECT_TRUE(mxh::gm::authorize_bearer({{"authorization", "Bearer " + token}}, token));
    EXPECT_TRUE(mxh::gm::authorize_bearer({{"AUTHORIZATION", "Bearer " + token}}, token));
}

TEST(GmSecurity, EmptyConfiguredTokenNeverAuthorizes) {
    EXPECT_FALSE(mxh::gm::authorize_bearer({{"Authorization", "Bearer "}}, ""));
}
