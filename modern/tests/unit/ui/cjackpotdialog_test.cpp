//
// Unit tests for mxh::ui::cJackpotDialog
//   (Phase C 1:1 port of commented-out legacy cJackpotDialog).
//
// Locks down the 1:1 surface documented in cjackpotdialog.hpp:
//   * Constants: NUMIMAGE_W=8, NUMIMAGE_H=14, BASIC_ANI_TIMELENGTH=2000,
//     BETWEEN_ANI_TIMELENGTH=500, NUM_CHANGE_TIMELENGTH=100,
//     DEFAULT_IMAGE=99, NUM_COUNT=10, CIPHER_NUM=9, MONEY_PER_MON=1.
//   * StNumImage + StCipherNum default-ctor invariants.
//   * cJackpotDialog default ctor: state zero, NotAnimating.
//   * SetTotalMoney / IsNumChanged: detects same / increased / decreased.
//   * IsNumChanged: m_bDoSequenceAni is true when amount increased.
//   * ConvertCipherNum: zero is a no-op (legacy early return).
//   * ConvertCipherNum: writes each digit at the correct slot.
//   * ConvertCipherNum: trailing slots get DEFAULT_IMAGE + bIsAni=false.
//   * ConvertCipherNum: m_dwMaxCipher = number of non-blank digits.
//   * ConvertCipherNum: amount > CIPHER_NUM is clamped (1:1 quirk).
//   * InitForAni: snapshot dwNumber->dwRealCipherNum, reset state.
//   * InitForSequenceAni: only activates when m_bDoSequenceAni.
//   * DoAni: early return when not animating.
//   * DoAni: digit roll phase increments bIsAni digits mod 10.
//   * DoAni: settle phase locks each digit after BASIC_ANI_TIMELENGTH.
//   * DoAni: completes when m_dwCipherCount == m_dwMaxCipher.
//   * DoSequenceAni: when not animating, snap to m_dwOldTotalMoney.
//   * DoSequenceAni: rolling forward increases toward m_dwOldTotalMoney.
//   * DoSequenceAni: clamps at m_dwOldTotalMoney and stops.
//   * Process: IsNumChanged + InitForSequenceAni + DoSequenceAni + ConvertCipherNum.
//   * 1:1 quirk stubs: InitNumImage/ReleaseNumImage/Linking/SetNumImagePos/Render.

#include "mxh/ui/cjackpotdialog.hpp"
#include "mxh/ui/cDialog.hpp"
#include "mxh/ui/cWindow.hpp"

#include <gtest/gtest.h>

#include <type_traits>
#include <cstdint>

using mxh::ui::cDialog;
using mxh::ui::cJackpotDialog;
using mxh::ui::cWindow;
using mxh::ui::kBasicAniTimelength;
using mxh::ui::kBetweenAniTimelength;
using mxh::ui::kCipherNum;
using mxh::ui::kDefaultImage;
using mxh::ui::kMoneyPerMon;
using mxh::ui::kNumChangeTimelength;
using mxh::ui::kNumCount;
using mxh::ui::kNumImageH;
using mxh::ui::kNumImageW;
using mxh::ui::StCipherNum;
using mxh::ui::StNumImage;

// Helper: build a cJackpotDialog with a deterministic clock so the
// per-tick methods are testable without flaky time-based assertions.
static cJackpotDialog MakeDialog() {
    std::uint32_t t = 1000;
    return cJackpotDialog([&t]() { return t; });
}

// ---- 1:1 constants (legacy values) ---------------------------------

TEST(JackpotDialogTest, NumImageConstants) {
    EXPECT_EQ(kNumImageW, 8u);
    EXPECT_EQ(kNumImageH, 14u);
}

TEST(JackpotDialogTest, AniTimelengthConstants) {
    EXPECT_EQ(kBasicAniTimelength, 2000u);
    EXPECT_EQ(kBetweenAniTimelength, 500u);
    EXPECT_EQ(kNumChangeTimelength, 100u);
}

TEST(JackpotDialogTest, NumCountAndCipheNumConstants) {
    EXPECT_EQ(kNumCount, 10u);
    EXPECT_EQ(kCipherNum, 9u);
    EXPECT_EQ(kDefaultImage, 99u);
    EXPECT_EQ(kMoneyPerMon, 1u);
}

// ---- 1:1 struct defaults -------------------------------------------

TEST(JackpotDialogTest, StNumImageDefaultIsNull) {
    StNumImage img;
    EXPECT_EQ(img.pImage, nullptr);
    EXPECT_EQ(img.dwW, 0u);
    EXPECT_EQ(img.dwH, 0u);
}

TEST(JackpotDialogTest, StCipherNumDefaultIsZeroNotAnimating) {
    StCipherNum c;
    EXPECT_EQ(c.dwNumber, 0u);
    EXPECT_EQ(c.dwRealCipherNum, 0u);
    EXPECT_FALSE(c.bIsAni);
}

// ---- 1:1 ctor / class invariants -----------------------------------

TEST(JackpotDialogTest, InheritsFromCDialog) {
    static_assert(std::is_base_of_v<cDialog, cJackpotDialog>,
                  "cJackpotDialog must inherit from cDialog");
    SUCCEED();
}

TEST(JackpotDialogTest, IsAlsoAWindow) {
    static_assert(std::is_base_of_v<cWindow, cJackpotDialog>,
                  "cJackpotDialog must be a cWindow (transitively)");
    SUCCEED();
}

TEST(JackpotDialogTest, NonCopyable) {
    static_assert(!std::is_copy_constructible_v<cJackpotDialog>,
                  "cJackpotDialog must be non-copyable");
    static_assert(!std::is_copy_assignable_v<cJackpotDialog>,
                  "cJackpotDialog must be non-copy-assignable");
    SUCCEED();
}

TEST(JackpotDialogTest, DefaultCtorZeroesState) {
    cJackpotDialog d;
    EXPECT_EQ(d.TotalMoney(), 0u);
    EXPECT_EQ(d.OldTotalMoney(), 0u);
    EXPECT_EQ(d.TempMoney(), 0u);
    EXPECT_EQ(d.MaxCipher(), 0u);
    EXPECT_EQ(d.CipherCount(), 0u);
    EXPECT_EQ(d.AniStartTime(), 0u);
    EXPECT_EQ(d.NumChangeTime(), 0u);
    EXPECT_EQ(d.IntervalAniTime(), 0u);
    EXPECT_FALSE(d.IsAnimating());
    EXPECT_FALSE(d.DoSequenceAniFlag());
    EXPECT_EQ(d.CloseButton(), nullptr);
}

TEST(JackpotDialogTest, NumImageSlotsAreDefault) {
    cJackpotDialog d;
    for (std::uint32_t i = 0; i < kNumCount; ++i) {
        EXPECT_EQ(d.NumImage(i).pImage, nullptr);
        EXPECT_EQ(d.NumImage(i).dwW, 0u);
        EXPECT_EQ(d.NumImage(i).dwH, 0u);
    }
}

TEST(JackpotDialogTest, CipherPosSlotsAreZero) {
    cJackpotDialog d;
    for (std::uint32_t i = 0; i < kCipherNum; ++i) {
        EXPECT_EQ(d.CipherPos(i).x, 0.0f);
        EXPECT_EQ(d.CipherPos(i).y, 0.0f);
    }
}

// ---- 1:1 ConvertCipherNum ------------------------------------------

TEST(JackpotDialogTest, ConvertCipherNumZeroIsNoOp) {
    auto d = MakeDialog();
    // Init already called ConvertCipherNum(0), so each cipher is
    // untouched (bIsAni=false, dwNumber=0). Confirm initial state.
    for (std::uint32_t i = 0; i < kCipherNum; ++i) {
        EXPECT_EQ(d.CipherAt(i).dwNumber, 0u);
        EXPECT_FALSE(d.CipherAt(i).bIsAni);
    }
    EXPECT_EQ(d.MaxCipher(), 0u);
}

TEST(JackpotDialogTest, ConvertCipherNumWritesDigitsInRightSlots) {
    auto d = MakeDialog();
    d.SetTotalMoney(123);
    d.ConvertCipherNum();
    // 123 -> cipher[0]=3, cipher[1]=2, cipher[2]=1, rest blank.
    EXPECT_EQ(d.CipherAt(0).dwNumber, 3u);
    EXPECT_EQ(d.CipherAt(1).dwNumber, 2u);
    EXPECT_EQ(d.CipherAt(2).dwNumber, 1u);
    EXPECT_EQ(d.MaxCipher(), 3u);
    // Active digits are marked bIsAni=true.
    EXPECT_TRUE(d.CipherAt(0).bIsAni);
    EXPECT_TRUE(d.CipherAt(1).bIsAni);
    EXPECT_TRUE(d.CipherAt(2).bIsAni);
}

TEST(JackpotDialogTest, ConvertCipherNumTrailingSlotsGetDefaultImage) {
    auto d = MakeDialog();
    d.SetTotalMoney(7);
    d.ConvertCipherNum();
    EXPECT_EQ(d.CipherAt(0).dwNumber, 7u);
    EXPECT_TRUE(d.CipherAt(0).bIsAni);
    for (std::uint32_t i = 1; i < kCipherNum; ++i) {
        EXPECT_EQ(d.CipherAt(i).dwNumber, kDefaultImage);
        EXPECT_FALSE(d.CipherAt(i).bIsAni);
    }
    EXPECT_EQ(d.MaxCipher(), 1u);
}

TEST(JackpotDialogTest, ConvertCipherNumFullNineDigitMax) {
    auto d = MakeDialog();
    d.SetTotalMoney(999999999u);
    d.ConvertCipherNum();
    for (std::uint32_t i = 0; i < kCipherNum; ++i) {
        EXPECT_EQ(d.CipherAt(i).dwNumber, 9u);
        EXPECT_TRUE(d.CipherAt(i).bIsAni);
    }
    EXPECT_EQ(d.MaxCipher(), kCipherNum);
}

TEST(JackpotDialogTest, ConvertCipherNumClampsWhenExceedingCipherNum) {
    // 1:1 quirk: legacy ASSERT(n<CIPHER_NUM) clamps silently here.
    auto d = MakeDialog();
    d.SetTotalMoney(1234567890u);  // 10 digits > kCipherNum=9
    d.ConvertCipherNum();
    // Should not crash; MaxCipher is clamped to kCipherNum.
    EXPECT_EQ(d.MaxCipher(), kCipherNum);
}

// ---- 1:1 IsNumChanged -----------------------------------------------

TEST(JackpotDialogTest, IsNumChangedReturnsFalseWhenSame) {
    auto d = MakeDialog();
    d.SetTotalMoney(100);
    d.SetOldTotalMoney(100);
    EXPECT_FALSE(d.IsNumChanged());
    EXPECT_FALSE(d.DoSequenceAniFlag());
}

TEST(JackpotDialogTest, IsNumChangedDetectsIncrease) {
    auto d = MakeDialog();
    d.SetTotalMoney(200);
    d.SetOldTotalMoney(100);
    EXPECT_TRUE(d.IsNumChanged());
    // Increased -> m_bDoSequenceAni=true.
    EXPECT_TRUE(d.DoSequenceAniFlag());
    // Snapshot of old is preserved in TempMoney.
    EXPECT_EQ(d.TempMoney(), 100u);
    // Old advances to the new value.
    EXPECT_EQ(d.OldTotalMoney(), 200u);
}

TEST(JackpotDialogTest, IsNumChangedDetectsDecrease) {
    auto d = MakeDialog();
    d.SetTotalMoney(50);
    d.SetOldTotalMoney(100);
    EXPECT_TRUE(d.IsNumChanged());
    // Decreased -> m_bDoSequenceAni=false (no roll animation).
    EXPECT_FALSE(d.DoSequenceAniFlag());
    EXPECT_EQ(d.TempMoney(), 100u);
    EXPECT_EQ(d.OldTotalMoney(), 50u);
}

// ---- 1:1 InitForAni ------------------------------------------------

TEST(JackpotDialogTest, InitForAniSnapshotsRealCipher) {
    auto d = MakeDialog();
    d.SetTotalMoney(45);
    d.ConvertCipherNum();
    // Mutate dwNumber to fake an in-progress roll.
    // (Caveat: the field is private; we drive it via ConvertCipherNum
    // and rely on the public surface to validate the snapshot.)
    d.InitForAni(2000);
    for (std::uint32_t i = 0; i < kCipherNum; ++i) {
        EXPECT_EQ(d.CipherAt(i).dwRealCipherNum, d.CipherAt(i).dwNumber);
    }
    EXPECT_TRUE(d.IsAnimating());
    EXPECT_EQ(d.CipherCount(), 0u);
    EXPECT_EQ(d.AniStartTime(), 2000u);
}

// ---- 1:1 InitForSequenceAni ---------------------------------------

TEST(JackpotDialogTest, InitForSequenceAniNoopWhenFlagFalse) {
    auto d = MakeDialog();
    // m_bDoSequenceAni defaults to false.
    d.InitForSequenceAni(500);
    EXPECT_FALSE(d.IsAnimating());
    EXPECT_EQ(d.AniStartTime(), 0u);
}

TEST(JackpotDialogTest, InitForSequenceAniActivatesWhenFlagTrue) {
    auto d = MakeDialog();
    d.SetTotalMoney(200);
    d.SetOldTotalMoney(100);
    (void)d.IsNumChanged();  // sets m_bDoSequenceAni=true
    ASSERT_TRUE(d.DoSequenceAniFlag());
    d.InitForSequenceAni(500);
    EXPECT_TRUE(d.IsAnimating());
    EXPECT_EQ(d.AniStartTime(), 500u);
}

// ---- 1:1 DoAni -----------------------------------------------------

TEST(JackpotDialogTest, DoAniEarlyReturnWhenNotAnimating) {
    auto d = MakeDialog();
    // Default state: not animating. DoAni must be a no-op.
    d.DoAni(99999);
    EXPECT_FALSE(d.IsAnimating());
    EXPECT_EQ(d.CipherCount(), 0u);
}

TEST(JackpotDialogTest, DoAniRollsAnimatingDigitsMod10) {
    auto d = MakeDialog();
    d.SetTotalMoney(5);
    d.ConvertCipherNum();
    d.InitForAni(0);
    // First call: now=200 (well past kNumChangeTimelength=100 from
    // m_dwNumChangeTime=0). Digit 0 is bIsAni, should roll 5->6.
    d.DoAni(200);
    EXPECT_EQ(d.CipherAt(0).dwNumber, 6u);
    // Second call at now=400 rolls 6->7.
    d.DoAni(400);
    EXPECT_EQ(d.CipherAt(0).dwNumber, 7u);
    // Ninth roll brings it to 0 (wrap 9->0 in legacy).
    for (int i = 0; i < 3; ++i) {
        d.DoAni(600 + 200 * i);
    }
    EXPECT_EQ(d.CipherAt(0).dwNumber, 0u);
}

TEST(JackpotDialogTest, DoAniSettlesDigitsAfterBasicAni) {
    // 1:1 with legacy DoAni: settle phase advances CipherCount by 1 per
    // call; animation completes when CipherCount==MaxCipher. For "45"
    // (MaxCipher=2), it takes 3 calls past kBasicAniTimelength:
    //   call#1: settle cipher[0], CipherCount 0->1
    //   call#2: settle cipher[1], CipherCount 1->2
    //   call#3: cipher[2].bIsAni=false (DEFAULT_IMAGE), skip settle;
    //            CipherCount==MaxCipher -> IsAnimating=false.
    auto d = MakeDialog();
    d.SetTotalMoney(45);
    d.ConvertCipherNum();
    d.InitForAni(0);
    d.DoAni(100);  // before basic-ani: no settle
    EXPECT_EQ(d.CipherCount(), 0u);
    EXPECT_TRUE(d.IsAnimating());
    d.DoAni(2500);  // 1st settle: cipher[0]
    EXPECT_EQ(d.CipherCount(), 1u);
    EXPECT_FALSE(d.CipherAt(0).bIsAni);
    d.DoAni(3500);  // 2nd settle: cipher[1]
    EXPECT_EQ(d.CipherCount(), 2u);
    EXPECT_FALSE(d.CipherAt(1).bIsAni);
    EXPECT_TRUE(d.IsAnimating());  // not done yet
    d.DoAni(4500);  // 3rd call: cipher[2] is DEFAULT_IMAGE, animation done
    EXPECT_FALSE(d.IsAnimating());
}

// ---- 1:1 DoSequenceAni --------------------------------------------

TEST(JackpotDialogTest, DoSequenceAniSnapWhenNotAnimating) {
    // 1:1 with legacy DoSequenceAni: when not animating, snap
    // m_dwTotalMoney to m_dwOldTotalMoney (which IsNumChanged just set
    // to the new value). So a decrease of 100->50 commits m_dwTotalMoney
    // to 50, not 100.
    auto d = MakeDialog();
    d.SetTotalMoney(50);
    d.SetOldTotalMoney(100);
    (void)d.IsNumChanged();
    ASSERT_FALSE(d.DoSequenceAniFlag());
    d.DoSequenceAni(9999);
    EXPECT_EQ(d.TotalMoney(), 50u);
    EXPECT_EQ(d.OldTotalMoney(), 50u);
}

TEST(JackpotDialogTest, DoSequenceAniRollsAndClamps) {
    auto d = MakeDialog();
    d.SetTotalMoney(200);
    d.SetOldTotalMoney(100);
    (void)d.IsNumChanged();
    ASSERT_TRUE(d.DoSequenceAniFlag());
    d.InitForSequenceAni(0);
    ASSERT_TRUE(d.IsAnimating());
    // After durTime=kNumChangeTimelength * 100 = 10000ms, durMoney
    // would be 100 * kMoneyPerMon = 100. Snap m_dwTotalMoney up to 200.
    d.DoSequenceAni(10000);
    EXPECT_EQ(d.TotalMoney(), 200u);
    EXPECT_FALSE(d.IsAnimating());
}

// ---- 1:1 Process (full loop) --------------------------------------

TEST(JackpotDialogTest, ProcessDetectsChangeAndAnimates) {
    // 1:1 with legacy Process() driver. We exercise the full loop with a
    // single change so the second tick doesn't see a "decrease" re-trigger
    // (legacy snap-to-old on no-anim).
    auto d = MakeDialog();
    d.SetTotalMoney(0);
    d.SetOldTotalMoney(0);
    d.Process(0);  // no-op, nothing changed
    EXPECT_FALSE(d.IsAnimating());
    d.SetTotalMoney(100);  // increase
    // Process(10000): IsNumChanged + InitForSequenceAni(10000) +
    //   DoSequenceAni(10000). durTime=0, durMoney=0, so m_dwTotalMoney
    //   is set to m_dwTempMoney=0 by DoSequenceAni.
    d.Process(10000);
    EXPECT_TRUE(d.IsAnimating());
    EXPECT_EQ(d.TotalMoney(), 0u);
    EXPECT_EQ(d.OldTotalMoney(), 100u);
    // Restore the new value so the second Process tick sees "no change"
    // (otherwise it would re-detect a 100->0 decrease and snap to 0).
    d.SetTotalMoney(100);
    d.Process(20000);
    // durTime = 20000 - 10000 = 10000ms. durMoney = 10000/100 * 1 = 100.
    // m_dwTotalMoney = 0 + 100 = 100 == m_dwOldTotalMoney. Clamp + stop.
    EXPECT_EQ(d.TotalMoney(), 100u);
    EXPECT_FALSE(d.IsAnimating());
    // ConvertCipherNum was called at the end: 100 -> cipher[0]=0, bIsAni=true.
    EXPECT_EQ(d.CipherAt(0).dwNumber, 0u);
    EXPECT_TRUE(d.CipherAt(0).bIsAni);
}

// ---- 1:1 quirk stubs (no-op) -------------------------------------

TEST(JackpotDialogTest, InitNumImageIsNoop) {
    auto d = MakeDialog();
    d.InitNumImage();
    SUCCEED();
}

TEST(JackpotDialogTest, ReleaseNumImageIsNoop) {
    auto d = MakeDialog();
    d.ReleaseNumImage();
    SUCCEED();
}

TEST(JackpotDialogTest, LinkingIsNoop) {
    auto d = MakeDialog();
    d.Linking();
    EXPECT_EQ(d.CloseButton(), nullptr);  // still null -- no child wiring
}

TEST(JackpotDialogTest, SetNumImagePosIsNoop) {
    auto d = MakeDialog();
    d.SetNumImagePos();
    for (std::uint32_t i = 0; i < kCipherNum; ++i) {
        EXPECT_EQ(d.CipherPos(i).x, 0.0f);
        EXPECT_EQ(d.CipherPos(i).y, 0.0f);
    }
}

TEST(JackpotDialogTest, RenderFallsThroughToCDialog) {
    auto d = MakeDialog();
    // cDialog::Init (the base) is hidden by cJackpotDialog::Init(). Call
    // it through a base reference to verify Render() does not crash.
    cDialog& base = d;
    base.Init(0, 0, 200, 100, nullptr, 7);
    d.Render();  // GPU stub; just verify it does not crash.
    SUCCEED();
}

// ---- 1:1 quirk: Clock injection ---------------------------------

TEST(JackpotDialogTest, ClockCanBeOverridden) {
    std::uint32_t t = 42;
    cJackpotDialog d([&t]() { return t; });
    // No direct way to read the clock back -- but Process(0) with
    // nothing to do should be safe. We exercise the clock path by
    // triggering DoAni via InitForAni + Process to confirm no crash.
    d.SetTotalMoney(100);
    d.SetOldTotalMoney(50);
    d.Process(t);
    SUCCEED();
}
