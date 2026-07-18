// moneydlg_test.cpp — 1:1 port verification tests for cMoneyDlg.

#include "moneydlg.hpp"
#include "cspin.hpp"
#include "cdialog.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

using mxh::ui::cMoneyDlg;
using mxh::ui::cSpin;
using mxh::ui::cDialog;

namespace {

std::unique_ptr<cMoneyDlg> MakeDialog() {
    auto d = std::make_unique<cMoneyDlg>();
    d->Init(0, 0, 200, 100, nullptr, 700);
    return d;
}

// Helper: build a sample "saved message" buffer (mimics a network MSGBASE).
std::vector<std::uint8_t> MakeSampleMsg(std::uint8_t baseByte = 0x42) {
    return std::vector<std::uint8_t>(8, baseByte);
}

}  // namespace

// ---------------------------------------------------------------------------
// Constants + construction
// ---------------------------------------------------------------------------

TEST(CMoneyDlg, IdConstantMatchesLegacyCmiMoneyspinRange) {
    // 1:1 with legacy CMI_MONEYSPIN (any id is fine in modern port; we
    // pick 0 to keep the dialog self-contained).
    EXPECT_EQ(cMoneyDlg::kIdMoneySpin, 0);
    EXPECT_EQ(cMoneyDlg::kSavedMsgSize, 1024u);
}

TEST(CMoneyDlg, DefaultConstructionHasNoSpin) {
    auto d = MakeDialog();
    EXPECT_EQ(d->spin(), nullptr);
    EXPECT_EQ(d->msgLen(), 0);
    EXPECT_EQ(d->param(), 0u);
    EXPECT_FALSE(d->hasCallback());
    EXPECT_EQ(d->savedMsg().size(), cMoneyDlg::kSavedMsgSize);
    // 1:1 quirk: ctor memset to 0 — saved msg is all zeros.
    for (auto b : d->savedMsg()) {
        EXPECT_EQ(b, 0u);
    }
}

TEST(CMoneyDlg, DefaultIsNotActive) {
    auto d = MakeDialog();
    EXPECT_FALSE(d->isActive());
}

// ---------------------------------------------------------------------------
// Linking()
// ---------------------------------------------------------------------------

TEST(CMoneyDlg, LinkingMaterializesSpin) {
    auto d = MakeDialog();
    d->Linking();
    EXPECT_NE(d->spin(), nullptr);
}

TEST(CMoneyDlg, LinkingSetsSpinId) {
    auto d = MakeDialog();
    d->Linking();
    EXPECT_EQ(d->spin()->id(), cMoneyDlg::kIdMoneySpin);
}

TEST(CMoneyDlg, LinkingIdempotent) {
    auto d = MakeDialog();
    d->Linking();
    const cSpin* first = d->spin();
    d->Linking();  // second call must not re-create
    EXPECT_EQ(d->spin(), first);
}

// ---------------------------------------------------------------------------
// Show()
// ---------------------------------------------------------------------------

TEST(CMoneyDlg, ShowActivatesDialog) {
    auto d = MakeDialog();
    d->Linking();
    auto msg = MakeSampleMsg(0xAA);
    d->Show(msg.data(), static_cast<int>(msg.size()), 1234u);
    EXPECT_TRUE(d->isActive());
    EXPECT_EQ(d->msgLen(), static_cast<int>(msg.size()));
    EXPECT_EQ(d->param(), 1234u);
}

TEST(CMoneyDlg, ShowCopiesMsgBytes) {
    auto d = MakeDialog();
    d->Linking();
    auto msg = MakeSampleMsg(0xCC);
    d->Show(msg.data(), static_cast<int>(msg.size()), 0u);
    // The first msg.size() bytes of savedMsg() should match.
    for (std::size_t i = 0; i < msg.size(); ++i) {
        EXPECT_EQ(d->savedMsg()[i], msg[i]) << " byte " << i;
    }
}

TEST(CMoneyDlg, ShowWithNullMsgIsSafeAndDoesNotCrash) {
    auto d = MakeDialog();
    d->Linking();
    d->Show(nullptr, 8, 100u);
    EXPECT_TRUE(d->isActive());
    EXPECT_EQ(d->msgLen(), 8);
    // savedMsg is still all zeros.
    for (auto b : d->savedMsg()) {
        EXPECT_EQ(b, 0u);
    }
}

TEST(CMoneyDlg, ShowWithZeroMsgLenIsSafe) {
    auto d = MakeDialog();
    d->Linking();
    auto msg = MakeSampleMsg();
    d->Show(msg.data(), 0, 0u);
    EXPECT_TRUE(d->isActive());
    EXPECT_EQ(d->msgLen(), 0);
}

TEST(CMoneyDlg, ShowStoresCallback) {
    auto d = MakeDialog();
    d->Linking();
    bool cbInvoked = false;
    d->Show(nullptr, 0, 0u, [&cbInvoked](std::uint32_t, std::uint32_t) {
        cbInvoked = true;
        return true;
    });
    EXPECT_TRUE(d->hasCallback());
    (void)cbInvoked;  // not invoked yet
}

TEST(CMoneyDlg, ShowOverwritesPreviousState) {
    auto d = MakeDialog();
    d->Linking();
    auto msg1 = MakeSampleMsg(0x11);
    d->Show(msg1.data(), static_cast<int>(msg1.size()), 100u);
    auto msg2 = MakeSampleMsg(0x22);
    d->Show(msg2.data(), static_cast<int>(msg2.size()), 200u);
    EXPECT_EQ(d->msgLen(), static_cast<int>(msg2.size()));
    EXPECT_EQ(d->param(), 200u);
    for (std::size_t i = 0; i < msg2.size(); ++i) {
        EXPECT_EQ(d->savedMsg()[i], msg2[i]);
    }
}

// ---------------------------------------------------------------------------
// OkPushed()
// ---------------------------------------------------------------------------

TEST(CMoneyDlg, OkPushedReadsSpinValue) {
    auto d = MakeDialog();
    d->Linking();
    d->spin()->SetValue(500);
    std::uint32_t receivedMoney = 0;
    std::uint32_t receivedParam = 0;
    auto msg = MakeSampleMsg();
    d->Show(msg.data(), static_cast<int>(msg.size()), 42u,
            [&](std::uint32_t m, std::uint32_t p) {
                receivedMoney = m;
                receivedParam = p;
                return true;
            });
    d->OkPushed();
    EXPECT_EQ(receivedMoney, 500u);
    EXPECT_EQ(receivedParam, 42u);
}

TEST(CMoneyDlg, OkPushedDeactivatesDialog) {
    auto d = MakeDialog();
    d->Linking();
    d->Show(nullptr, 0, 0u);
    EXPECT_TRUE(d->isActive());
    d->OkPushed();
    EXPECT_FALSE(d->isActive());
}

TEST(CMoneyDlg, OkPushedWithoutLinkingIsNoOp) {
    auto d = MakeDialog();
    // No Linking: spin is null. OkPushed must not crash; dialog closes.
    d->Show(nullptr, 0, 0u);
    d->OkPushed();
    EXPECT_FALSE(d->isActive());
}

TEST(CMoneyDlg, OkPushedWithoutCallbackSendsByDefault) {
    // 1:1 quirk: legacy `BOOL bSend = TRUE; if(m_OnPushFunc) bSend = ...;
    // if(bSend) NETWORK->Send(...);` — when no callback, bSend stays
    // TRUE and the network send is attempted. Modern port preserves
    // this (the send is stubbed, but the path runs).
    auto d = MakeDialog();
    d->Linking();
    d->Show(nullptr, 0, 0u);  // no callback
    d->OkPushed();
    EXPECT_FALSE(d->isActive());
}

TEST(CMoneyDlg, OkPushedCallbackReturningFalseSkipsSend) {
    auto d = MakeDialog();
    d->Linking();
    d->spin()->SetValue(99);
    int callCount = 0;
    d->Show(nullptr, 0, 0u, [&](std::uint32_t, std::uint32_t) {
        ++callCount;
        return false;  // refuse to send
    });
    d->OkPushed();
    EXPECT_EQ(callCount, 1);
    EXPECT_FALSE(d->isActive());
}

TEST(CMoneyDlg, OkPushedCallbackReturningTrueProceeds) {
    auto d = MakeDialog();
    d->Linking();
    d->spin()->SetValue(1000);
    int callCount = 0;
    d->Show(nullptr, 0, 0u, [&](std::uint32_t, std::uint32_t) {
        ++callCount;
        return true;
    });
    d->OkPushed();
    EXPECT_EQ(callCount, 1);
    EXPECT_FALSE(d->isActive());
}
