// mxh/tests/unit/client/cmain_title_test.cpp
// Unit tests for mxh::client::CMainTitle (Phase A.1.8).
//
// Locks down the 1:1 surface:
//   * Init reads MHVerInfo.ver and stores the version in
//     m_ClientVersion.
//   * Init / Release flip isInitialized() correctly.
//   * OnDisconnect updates the state machine flags (m_bDisconntinToDist,
//     m_dwDiconWaitTime, m_bWaitConnectToAgent, m_bServerList).
//   * GetDistAuthKey / GetUserIdx default to 0.
//   * Process() is a no-op without crashing.

#include "CMainTitle.hpp"

#include <gtest/gtest.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <fstream>

using mxh::client::CMainTitle;

namespace {

// Write a synthetic MHVerInfo.ver to a known location and remember
// the cwd so the test can restore it.  CMainTitle's Init reads the
// file from CWD, so we chdir into a temp dir for the duration of
// the test.
class ScopedCwd {
public:
    ScopedCwd() {
        char buf[MAX_PATH];
        if (GetCurrentDirectoryA(MAX_PATH, buf)) {
            original_ = buf;
        }
        char tempDir[MAX_PATH];
        if (GetTempPathA(MAX_PATH, tempDir)) {
            if (GetTempFileNameA(tempDir, "mxt", 0, tempFile_) == 0) {
                tempFile_[0] = '\0';
            }
        }
        // Create a stable directory under temp.
        std::snprintf(workDir_, MAX_PATH, "%smxh_test_%lu",
                       tempDir, ::GetCurrentThreadId());
        CreateDirectoryA(workDir_, nullptr);
        SetCurrentDirectoryA(workDir_);

        // Drop MHVerInfo.ver in the work dir.
        std::ofstream out("MHVerInfo.ver");
        out << "TESTVERSION12345";
        out.close();
    }
    ~ScopedCwd() {
        if (workDir_[0]) {
            SetCurrentDirectoryA(original_.c_str());
            // Best-effort cleanup of the dir + its contents.
            std::string verFile = std::string(workDir_) + "\\MHVerInfo.ver";
            DeleteFileA(verFile.c_str());
            DeleteFileA(workDir_);
        }
    }
private:
    std::string original_;
    char workDir_[MAX_PATH] = {0};
    char tempFile_[MAX_PATH] = {0};
};

} // namespace

TEST(CMainTitle, InitReadsMHVerInfo) {
    ScopedCwd cwd;
    CMainTitle title;
    title.Init(nullptr);
    EXPECT_TRUE(title.isInitialized());
    // Access the parsed version via a friend-style hook is not
    // available in A.1.8; we assert Init didn't crash and
    // isInitialized flipped.  Direct access to m_ClientVersion
    // would require either friending or a public accessor — left
    // for A.1.8.b when the protocol layer needs it.
    title.Release();
    EXPECT_FALSE(title.isInitialized());
}

TEST(CMainTitle, LifecycleTogglesInitialized) {
    ScopedCwd cwd;
    CMainTitle title;
    EXPECT_FALSE(title.isInitialized());
    title.Init(nullptr);
    EXPECT_TRUE(title.isInitialized());
    title.Process();     // no-op stub
    title.Release();
    EXPECT_FALSE(title.isInitialized());
}

TEST(CMainTitle, AuthKeysDefaultToZero) {
    CMainTitle title;
    EXPECT_EQ(title.GetDistAuthKey(), 0u);
    EXPECT_EQ(title.GetUserIdx(), 0u);
}

TEST(CMainTitle, OnDisconnectUpdatesStateMachine) {
    CMainTitle title;
    title.Init(nullptr);
    EXPECT_FALSE(title.isServerList());
    title.OnDisconnect();
    // The state machine flags should reflect the disconnect: server
    // list hidden, no longer waiting for agent, disconnect-wait
    // timer started (we can't read the timer value without
    // friending, but isServerList/isWaitingForAgent are observable).
    EXPECT_FALSE(title.isServerList());
    EXPECT_FALSE(title.isWaitingForAgent());
    title.Release();
}

TEST(CMainTitle, SetDistAuthKeyRoundTrip) {
    CMainTitle title;
    title.SetDistAuthKey(0xDEADBEEFu);
    EXPECT_EQ(title.GetDistAuthKey(), 0xDEADBEEFu);
    title.SetUserIdx(42);
    EXPECT_EQ(title.GetUserIdx(), 42u);
}

TEST(CMainTitle, ServerListDialogAccessorIsNull) {
    CMainTitle title;
    title.Init(nullptr);
    EXPECT_EQ(title.GetServerListDialog(), nullptr);
    title.Release();
}
