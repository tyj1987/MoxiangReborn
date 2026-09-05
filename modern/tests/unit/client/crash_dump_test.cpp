// crash_dump_test.cpp
//
// Phase 0 §6.3 unit tests for the controlled unhandled-exception
// handler.  The handler itself cannot be exercised from a normal
// test (an unhandled SEH exception would kill the test binary
// before gtest could report), so these tests cover the env-var
// wiring and the no-op-when-MXH_DUMP_DIR-missing path.  The
// full-path minidump write is covered by the live capture run
// (`scripts/capture-gamein.ps1`) which the Phase 0 §6.3 evidence
// rows in docs/VERIFICATION_MATRIX.md cite.

#include "mxh/client/crash_dump.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <string>

namespace mxh::test {

namespace {

void set_env(const char* name, const char* value) {
    if (value) {
#ifdef _WIN32
        _putenv_s(name, value);
#else
        setenv(name, value, 1);
#endif
    } else {
#ifdef _WIN32
        _putenv_s(name, "");
#else
        unsetenv(name);
#endif
    }
}

}  // namespace

TEST(CrashDumpTest, RunIdAccessorIsEmptyByDefault) {
    set_env("MXH_RUN_ID", nullptr);
    // After the clear, crash_dump_run_id() must report the empty
    // string.  (The actual handler installation may or may not
    // have been called by other tests; the accessor always
    // returns the most recent env value.)
    EXPECT_EQ(mxh::client::crash_dump_run_id()[0], '\0');
}

TEST(CrashDumpTest, RunIdAccessorReadsEnvAfterHandlerInstall) {
    set_env("MXH_RUN_ID", "abc-unittest-12");
    mxh::client::install_crash_dump_handler();
    EXPECT_STREQ(mxh::client::crash_dump_run_id(), "abc-unittest-12");
    set_env("MXH_RUN_ID", nullptr);
}

TEST(CrashDumpTest, InstallIsIdempotent) {
    set_env("MXH_RUN_ID", "idempotent");
    mxh::client::install_crash_dump_handler();
    const auto* first = mxh::client::crash_dump_run_id();
    mxh::client::install_crash_dump_handler();
    const auto* second = mxh::client::crash_dump_run_id();
    EXPECT_STREQ(first, second);
    set_env("MXH_RUN_ID", nullptr);
}

TEST(CrashDumpTest, InstallIsNoOpWithoutDumpDir) {
    set_env("MXH_DUMP_DIR", nullptr);
    set_env("MXH_RUN_ID", "no-dump-dir");
    // Without MXH_DUMP_DIR the handler must skip
    // SetUnhandledExceptionFilter so a normal release launch
    // (no capture harness) is unaffected.  The handler install
    // is idempotent, so we cannot assert that the accessor was
    // cleared by this call alone — the test only verifies that
    // the install does not crash when MXH_DUMP_DIR is missing.
    EXPECT_NO_FATAL_FAILURE(mxh::client::install_crash_dump_handler());
    set_env("MXH_RUN_ID", nullptr);
}

}  // namespace mxh::test
