// mh_error_test.cpp - Phase D6 MHError 1:1 port tests.

#include "mxh/server/mh_error.hpp"

#include <gtest/gtest.h>

#include <cstring>

namespace {

using mxh::server::mh_error_format;
using mxh::server::mh_error_format_length;

TEST(MHErrorFormat, BasicString) {
    char buf[64];
    int n = mh_error_format(buf, sizeof(buf), "hello %s", "world");
    EXPECT_GE(n, 0);
    EXPECT_STREQ(buf, "hello world");
}

TEST(MHErrorFormat, IntegerInts) {
    char buf[64];
    int n = mh_error_format(buf, sizeof(buf), "x=%d y=%d", 7, 42);
    EXPECT_GE(n, 0);
    EXPECT_STREQ(buf, "x=7 y=42");
}

TEST(MHErrorFormat, ReturnsExpectedLength) {
    char buf[64];
    int n = mh_error_format(buf, sizeof(buf), "abcdef");
    EXPECT_EQ(n, 6);
}

TEST(MHErrorFormat, NullBufferReturnsMinusOne) {
    int n = mh_error_format(nullptr, 0, "fmt");
    EXPECT_EQ(n, -1);
}

TEST(MHErrorFormat, NullFormatReturnsMinusOne) {
    char buf[8];
    int n = mh_error_format(buf, sizeof(buf), nullptr);
    EXPECT_EQ(n, -1);
}

TEST(MHErrorFormatLength, FormatOnlyString) {
    EXPECT_EQ(mh_error_format_length("%d", 123), 3u);
}

TEST(MHErrorFormatLength, Empty) {
    EXPECT_EQ(mh_error_format_length(""), 0u);
}

TEST(MHErrorFormatLength, MultiArg) {
    EXPECT_EQ(mh_error_format_length("[%s %d]", "x", 5), 5u);
}

TEST(MHErrorFormatLength, NullReturnsZero) {
    EXPECT_EQ(mh_error_format_length(nullptr), 0u);
}

}  // namespace
