#include "mxh/server/crypt.hpp"
#include <gtest/gtest.h>
#include <cstring>
using namespace mxh::server; using namespace mxh::crypto;
TEST(Crypt, UninitializedOperationsAreNoOpSuccess){Crypt c;char b[]="abc";EXPECT_TRUE(c.encrypt(b,3));EXPECT_STREQ(b,"abc");EXPECT_EQ(c.encrypt_crc(),0);}
TEST(Crypt, InitSwapsServerKeys){Crypt c;HselInit a,b;a.iDesCount=HSEL_DES_SINGLE;b.iDesCount=HSEL_DES_TRIPLE;c.init(a,b);EXPECT_TRUE(c.initialized());EXPECT_EQ(c.encrypt_key().iDesCount,b.iDesCount);EXPECT_EQ(c.decrypt_key().iDesCount,a.iDesCount);}
TEST(Crypt, InitializedRoundTrip){Crypt c;HselInit i;i.iDesCount=HSEL_DES_SINGLE;i.iEncryptType=HSEL_ENCRYPTTYPE_1;i.iCustomize=HSEL_KEY_TYPE_CUSTOMIZE;i.Keys.iLeftKey=1;i.Keys.iRightKey=2;i.Keys.iMiddleKey=3;i.Keys.iTotalKey=4;i.Keys.iLeftMultiGab=1;i.Keys.iRightMultiGab=1;i.Keys.iMiddleMultiGab=1;i.Keys.iTotalMultiGab=1;i.Keys.iLeftPlusGab=1;i.Keys.iRightPlusGab=1;i.Keys.iMiddlePlusGab=1;i.Keys.iTotalPlusGab=1;c.init(i,i);char b[]="hello world";char original[12];std::memcpy(original,b,12);EXPECT_TRUE(c.encrypt(b,11));EXPECT_TRUE(c.decrypt(b,11));EXPECT_EQ(std::memcmp(b,original,11),0);}