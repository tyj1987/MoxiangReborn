// ggsrv25_vendor_stub_test.cpp - Phase 6.3 nProtect GameGuard v2.5 vendor stub.
#include "mxh/crypto/ggsrv25_vendor_stub.hpp"
#include <gtest/gtest.h>
using namespace mxh::crypto;
TEST(GGSrv25VendorStubTest, AuthMinLenIs16){ EXPECT_EQ(kGGSrv25AuthMinLen,16u); }
TEST(GGSrv25VendorStubTest, AuthMaxLenIs64){ EXPECT_EQ(kGGSrv25AuthMaxLen,64u); }
TEST(GGSrv25VendorStubTest, ErrorOkIs0){ EXPECT_EQ(kGGSrv25ErrorOk,0u); }
TEST(GGSrv25VendorStubTest, InitSucceedsAndClearsEndedFlag){ EXPECT_FALSE(ggsrv25_is_ended()); EXPECT_EQ(ggsrv25_init(),1); EXPECT_FALSE(ggsrv25_is_ended()); }
TEST(GGSrv25VendorStubTest, EndSetsEndedFlag){ ggsrv25_init(); ggsrv25_end(); EXPECT_TRUE(ggsrv25_is_ended()); }
TEST(GGSrv25VendorStubTest, EndIsIdempotent){ ggsrv25_init(); ggsrv25_end(); ggsrv25_end(); EXPECT_TRUE(ggsrv25_is_ended()); }
TEST(GGSrv25VendorStubTest, CheckAuthEmptyReturns0){ ggsrv25_init(); EXPECT_EQ(ggsrv25_check_auth(""),0); }
TEST(GGSrv25VendorStubTest, CheckAuthNullptrReturns0){ ggsrv25_init(); EXPECT_EQ(ggsrv25_check_auth(nullptr),0); }
TEST(GGSrv25VendorStubTest, CheckAuthShortReturns0){ ggsrv25_init(); EXPECT_EQ(ggsrv25_check_auth("abcd"),0); }
TEST(GGSrv25VendorStubTest, CheckAuthAtMinLenReturns1){ ggsrv25_init(); EXPECT_EQ(ggsrv25_check_auth("0123456789abcdef"),1); }
TEST(GGSrv25VendorStubTest, CheckAuthAtMaxLenReturns1){ ggsrv25_init(); EXPECT_EQ(ggsrv25_check_auth("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"),1); }
TEST(GGSrv25VendorStubTest, CheckAuthBeyondMaxLenReturns0){ ggsrv25_init(); EXPECT_EQ(ggsrv25_check_auth("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdefA"),0); }
TEST(GGSrv25VendorStubTest, GetErrorCodeIsAlways0){ ggsrv25_init(); EXPECT_EQ(ggsrv25_get_error_code(),0); ggsrv25_end(); EXPECT_EQ(ggsrv25_get_error_code(),0); }