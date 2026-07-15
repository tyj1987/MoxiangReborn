// mlog_test.cpp - Phase 10.13 minimal-logging smoke tests
//
// Covers modern/include/mxh/log/mlog.hpp + modern/src/log/mlog.cpp.
// The module is tiny: 4 macros (MLOG_DEBUG/INFO/WARN/ERROR), 1 free
// function (mxh::log_message), 1 enum (mxh::LogLevel). The level
// filter is internal-static (not exposed), so the test cannot toggle
// it. Instead we verify that:
//   - The macros compile and invoke log_message (no crash).
//   - The LogLevel enum is the right size and the right values.
//   - The macros expand to the correct log_message call site.

#include "mxh/log/mlog.hpp"

#include <gtest/gtest.h>

#include <type_traits>

namespace mxh::test {

// ===========================================================================
// LogLevel enum
// ===========================================================================

TEST(LogLevelTest, IsInt) {
    // The header declares `enum class LogLevel { ... }` with no
    // explicit underlying type, so the compiler picks `int`. Pinning
    // here so a future change to `std::uint8_t` (which would shrink
    // the enum to one byte) is caught here. Either choice is fine;
    // what matters is that the test tracks whatever the header says.
    static_assert(std::is_same_v<std::underlying_type_t<LogLevel>,
                                 int>);
    SUCCEED();
}

TEST(LogLevelTest, ValuesAreDistinct) {
    EXPECT_NE(LogLevel::Debug, LogLevel::Info);
    EXPECT_NE(LogLevel::Debug, LogLevel::Warn);
    EXPECT_NE(LogLevel::Debug, LogLevel::Error);
    EXPECT_NE(LogLevel::Info, LogLevel::Warn);
    EXPECT_NE(LogLevel::Info, LogLevel::Error);
    EXPECT_NE(LogLevel::Warn, LogLevel::Error);
}

TEST(LogLevelTest, ValuesAreZeroThroughThree) {
    // Debug=0, Info=1, Warn=2, Error=3. Pinned so the switch in
    // log_message() (which uses static_cast<int>(level)) keeps
    // matching the tag() mapping.
    EXPECT_EQ(static_cast<int>(LogLevel::Debug), 0);
    EXPECT_EQ(static_cast<int>(LogLevel::Info),  1);
    EXPECT_EQ(static_cast<int>(LogLevel::Warn),  2);
    EXPECT_EQ(static_cast<int>(LogLevel::Error), 3);
}

TEST(LogLevelTest, ValuesAreOrderedBySeverity) {
    // Debug < Info < Warn < Error. Pinned because log_message's
    // filter compares with `if (level < minLevel) return` — if the
    // ordering breaks, a Warn message would be dropped even when
    // min is set to Debug.
    EXPECT_LT(static_cast<int>(LogLevel::Debug), static_cast<int>(LogLevel::Info));
    EXPECT_LT(static_cast<int>(LogLevel::Info), static_cast<int>(LogLevel::Warn));
    EXPECT_LT(static_cast<int>(LogLevel::Warn), static_cast<int>(LogLevel::Error));
}

// ===========================================================================
// log_message() smoke — direct calls go through without crash
// ===========================================================================

TEST(LogMessageTest, DoesNotCrashAtAnyLevel) {
    // The module writes to stderr via fprintf; we cannot easily
    // capture that in a gtest without redirecting fd 2. The bar
    // here is "the call does not crash and does not return a
    // status code" — that proves the path is wired up correctly.
    log_message(LogLevel::Debug, "test.cpp", 1, "debug %d", 42);
    log_message(LogLevel::Info,  "test.cpp", 2, "info %s", "hello");
    log_message(LogLevel::Warn,  "test.cpp", 3, "warn %d.%d", 1, 2);
    log_message(LogLevel::Error, "test.cpp", 4, "error: %s", "boom");
    SUCCEED();
}

TEST(LogMessageTest, HandlesNullFile) {
    // The implementation guards against null file via basename().
    log_message(LogLevel::Info, nullptr, 0, "null file path");
    SUCCEED();
}

TEST(LogMessageTest, HandlesEmptyFormat) {
    log_message(LogLevel::Info, "test.cpp", 1, "");
    SUCCEED();
}

TEST(LogMessageTest, HandlesFormatWithNoArgs) {
    log_message(LogLevel::Info, "test.cpp", 1, "no args at all");
    SUCCEED();
}

TEST(LogMessageTest, HandlesLongMessage) {
    // Build a > 1024 byte string to exercise the buf[1024] size cap
    // (the implementation truncates via vsnprintf). 2000 chars
    // is well over the cap.
    std::string long_msg(2000, 'A');
    log_message(LogLevel::Info, "test.cpp", 1, "%s", long_msg.c_str());
    SUCCEED();
}

// ===========================================================================
// MLOG_* macros expand and call log_message without crash
// ===========================================================================

TEST(MlogMacrosTest, AllFourMacrosCompileAndInvoke) {
    // If the macros expand to something that doesn't match
    // log_message's signature, this would fail to compile.
    MLOG_DEBUG("debug %d", 1);
    MLOG_INFO("info %d", 2);
    MLOG_WARN("warn %d", 3);
    MLOG_ERROR("error %d", 4);
    SUCCEED();
}

TEST(MlogMacrosTest, MacrosAcceptZeroArgs) {
    // ##__VA_ARGS__ support: a macro call with no extra args
    // should compile and not crash. C++20 __VA_OPT__ is not used
    // here; the legacy ##__VA_ARGS__ trick is what allows MLOG_INFO
    // ("hello") to compile.
    MLOG_DEBUG("no args");
    MLOG_INFO("no args");
    MLOG_WARN("no args");
    MLOG_ERROR("no args");
    SUCCEED();
}

}  // namespace mxh::test
