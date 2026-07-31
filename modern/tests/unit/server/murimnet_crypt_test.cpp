#include "mxh/server/murimnet_crypt.hpp"
#include "mxh/crypto/hsel_stream.hpp"
#include <gtest/gtest.h>
#include <cstring>
#include <cstdint>
using namespace mxh::server;
using mxh::crypto::HselInit;
TEST(MurimNetCrypt, CreateSucceedsAndIsInitialized){MurimNetCrypt c;EXPECT_FALSE(c.IsInitialized());EXPECT_TRUE(c.Create());EXPECT_TRUE(c.IsInitialized());}
TEST(MurimNetCrypt, NotInitializedEncryptIsNoOp){MurimNetCrypt c;char buf[16]={};std::memcpy(buf,"0123456789ABCDEF",16);EXPECT_TRUE(c.Encrypt(buf,16));EXPECT_EQ(std::memcmp(buf,"0123456789ABCDEF",16),0);}
TEST(MurimNetCrypt, NotInitializedDecryptIsNoOp){MurimNetCrypt c;char buf[16]={};std::memcpy(buf,"0123456789ABCDEF",16);EXPECT_TRUE(c.Decrypt(buf,16));EXPECT_EQ(std::memcmp(buf,"0123456789ABCDEF",16),0);}
TEST(MurimNetCrypt, RoundTripAfterCreateChangesBytesThenRestores){MurimNetCrypt c;ASSERT_TRUE(c.Create());char plain[32];for(int i=0;i<32;++i)plain[i]=static_cast<char>(i);char enc[32];std::memcpy(enc,plain,32);ASSERT_TRUE(c.Encrypt(enc,32));bool changed=false;for(int i=0;i<32;++i)if(enc[i]!=plain[i]){changed=true;break;}EXPECT_TRUE(changed);char dec[32];std::memcpy(dec,enc,32);ASSERT_TRUE(c.Decrypt(dec,32));EXPECT_EQ(std::memcmp(dec,plain,32),0);}
TEST(MurimNetCrypt, InitWithExplicitKeysRoundTrips){MurimNetCrypt c;HselInit a,b;a.iEncryptType=mxh::crypto::HSEL_ENCRYPTTYPE_1;a.iDesCount=mxh::crypto::HSEL_DES_SINGLE;a.iSwapFlag=mxh::crypto::HSEL_SWAP_FLAG_ON;a.iCustomize=mxh::crypto::HSEL_KEY_TYPE_DEFAULT;b=a;ASSERT_TRUE(c.Init(a,b));EXPECT_TRUE(c.IsInitialized());char plain[16];for(int i=0;i<16;++i)plain[i]=static_cast<char>(0x30+i);char enc[16];std::memcpy(enc,plain,16);ASSERT_TRUE(c.Encrypt(enc,16));ASSERT_TRUE(c.Decrypt(enc,16));EXPECT_EQ(std::memcmp(enc,plain,16),0);}
TEST(MurimNetCrypt, CrcAccessorsReturnValueAfterEncrypt){MurimNetCrypt c;ASSERT_TRUE(c.Create());char buf[16]={};std::memcpy(buf,"ABCDEFGHIJKLMNOP",16);ASSERT_TRUE(c.Encrypt(buf,16));std::int8_t en=c.GetEnCrcChar();std::int8_t de=c.GetDeCrcChar();(void)en;(void)de;SUCCEED();}
