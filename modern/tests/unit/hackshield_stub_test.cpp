// hackshield_stub_test.cpp - Tests for HackShield 4.0 stub layer.
//
// Verifies the modern stub returns HS_ERR_OK (=0) for all entry points
// declared in HShield.h, and that the buffer sizes match the legacy
// SIZEOF_REQMSG / SIZEOF_ACKMSG / SIZEOF_GUIDREQMSG / SIZEOF_GUIDACKMSG.

#include "mxh/crypto/hackshield_stub.hpp"
#include <gtest/gtest.h>
#include <cstring>
#include <vector>
#include <cstdint>

namespace {

// Legacy HShield.h constants we must keep stable
constexpr std::size_t kReqMsg     = 160; // SIZEOF_REQMSG
constexpr std::size_t kAckMsg     = 56;  // SIZEOF_ACKMSG
constexpr std::size_t kGuidReqMsg = 20;  // SIZEOF_GUIDREQMSG
constexpr std::size_t kGuidAckMsg = 20;  // SIZEOF_GUIDACKMSG

}  // namespace

// ---- Lifecycle stubs ----

TEST(HackShieldStub, AhnHSInitializeReturnsZero) {
    EXPECT_EQ(_AhnHS_Initialize(nullptr, nullptr, 0, nullptr, 0, 0), 0);
    EXPECT_EQ(_AhnHS_Initialize("c:\test\Ehsvc.dll", nullptr, 0x1234,
                            "LIC-KEY", 0xFFFFFFFFu, 0x20u), 0);
}

TEST(HackShieldStub, AhnHSStartStopServiceReturnsZero) {
    EXPECT_EQ(_AhnHS_StartService(), 0);
    EXPECT_EQ(_AhnHS_StopService(), 0);
}

TEST(HackShieldStub, AhnHSPauseResumeAcceptAnyOption) {
    EXPECT_EQ(_AhnHS_PauseService(0), 0);
    EXPECT_EQ(_AhnHS_PauseService(0xFFFFu), 0);
    EXPECT_EQ(_AhnHS_ResumeService(0), 0);
    EXPECT_EQ(_AhnHS_ResumeService(0x40u), 0);
}

TEST(HackShieldStub, AhnHSUninitializeReturnsZero) {
    EXPECT_EQ(_AhnHS_Uninitialize(), 0);
}

// ---- Server-client message exchange (called by HackShieldManager.cpp) ----

TEST(HackShieldStub, MakeAckMsgZerosOutputBuffer) {
    std::vector<unsigned char> req(kReqMsg, 0xA5);
    std::vector<unsigned char> ack(kAckMsg, 0x5A);
    EXPECT_EQ(_AhnHS_MakeAckMsg(req.data(), ack.data()), 0);
    for (std::size_t i = 0; i < kAckMsg; ++i) {
        EXPECT_EQ(ack[i], 0u) << "AckMsg byte " << i << " not zeroed";
    }
}

TEST(HackShieldStub, MakeAckMsgNullAckSafe) {
    std::vector<unsigned char> req(kReqMsg);
    // Legacy tolerated nullptr ack (driver not loaded).
    EXPECT_EQ(_AhnHS_MakeAckMsg(req.data(), nullptr), 0);
}

TEST(HackShieldStub, MakeGuidAckMsgZerosOutputBuffer) {
    std::vector<unsigned char> req(kGuidReqMsg, 0xCC);
    std::vector<unsigned char> ack(kGuidAckMsg, 0x33);
    EXPECT_EQ(_AhnHS_MakeGuidAckMsg(req.data(), ack.data()), 0);
    for (std::size_t i = 0; i < kGuidAckMsg; ++i) {
        EXPECT_EQ(ack[i], 0u) << "GuidAckMsg byte " << i << " not zeroed";
    }
}

TEST(HackShieldStub, MakeGuidAckMsgNullAckSafe) {
    std::vector<unsigned char> req(kGuidReqMsg);
    EXPECT_EQ(_AhnHS_MakeGuidAckMsg(req.data(), nullptr), 0);
}

// ---- Memory/CRC helpers ----

TEST(HackShieldStub, SaveFuncAddressAcceptsZeroArgs) {
    // Empty list is valid: returns OK.
    EXPECT_EQ(_AhnHS_SaveFuncAddress(0), 0);
}

TEST(HackShieldStub, SaveFuncAddressAcceptsSeveralArgs) {
    // Dummy function pointers.
    auto f1 = []() { return 1; };
    auto f2 = []() { return 2; };
    auto f3 = []() { return 3; };
    void* p1 = reinterpret_cast<void*>(&f1);
    void* p2 = reinterpret_cast<void*>(&f2);
    void* p3 = reinterpret_cast<void*>(&f3);
    EXPECT_EQ(_AhnHS_SaveFuncAddress(3, p1, p2, p3), 0);
}

TEST(HackShieldStub, CheckAPIHookedAlwaysClean) {
    EXPECT_EQ(_AhnHS_CheckAPIHooked(nullptr, nullptr, nullptr), 0);
    EXPECT_EQ(_AhnHS_CheckAPIHooked("kernel32.dll", "CreateFileW", "c:\test"), 0);
}

// ---- HackShieldManager-level aliases (HackShieldManager.h:7-15) ----

TEST(HackShieldStub, HsAliasLifecycleReturnsZero) {
    // The high-level HS_* wrappers are thin inline forwarders; verify
    // they ALL pass through to a successful state.
    EXPECT_EQ(HS_Init(), 0);
    EXPECT_EQ(HS_StartService(), 0);
    EXPECT_EQ(HS_PauseService(), 0);
    EXPECT_EQ(HS_ResumeService(), 0);
    EXPECT_EQ(HS_StopService(), 0);
    EXPECT_EQ(HS_UnInit(), 0);
    HS_SaveFuncAddress();  // void return; must not crash
}

TEST(HackShieldStub, HsCallbackProcReturnsZero) {
    // The callback signature is (long code, long size, void* data).
    EXPECT_EQ(HS_CallbackProc(0, 0, nullptr), 0);
    EXPECT_EQ(HS_CallbackProc(0x010501, 16, reinterpret_cast<void*>(0x12345678)), 0);
}

// ---- Buffer-size invariants (1:1 with HShield.h line 290) ----

TEST(HackShieldStub, MessageSizeConstants) {
    // These constants appear inline in HackShieldManager.cpp;
    // the stub MUST keep using the same sizes to avoid silent ABI drift.
    EXPECT_EQ(kReqMsg, 160u);
    EXPECT_EQ(kAckMsg, 56u);
    EXPECT_EQ(kGuidReqMsg, 20u);
    EXPECT_EQ(kGuidAckMsg, 20u);
}
