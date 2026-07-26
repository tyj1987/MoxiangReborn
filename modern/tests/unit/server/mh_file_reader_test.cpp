#include "mxh/server/mh_file_reader.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(MhFileReader, ReadsWhitespaceTokensAndTypes){MhFileReader f;ASSERT_TRUE(f.init_text("12 345 1.5 true\n"));EXPECT_EQ(f.get_int(),12);EXPECT_EQ(f.get_word(),345);EXPECT_FLOAT_EQ(f.get_float(),1.5f);EXPECT_EQ(f.get_string(),"true");}
TEST(MhFileReader, ReadsLinesAndQuotedStrings){MhFileReader f;ASSERT_TRUE(f.init_text("prefix \"hello world\"\r\nnext line"));EXPECT_EQ(f.get_quoted(),"hello world");EXPECT_EQ(f.get_line(),"");EXPECT_EQ(f.get_line(),"next line");}
TEST(MhFileReader, QuotedMissingReturnsEmpty){MhFileReader f;f.init_text("no quote\n");EXPECT_TRUE(f.get_quoted().empty());}
TEST(MhFileReader, EofAndRelease){MhFileReader f;f.init_text("x");EXPECT_FALSE(f.eof());EXPECT_EQ(f.get_string(),"x");EXPECT_TRUE(f.eof());f.release();EXPECT_FALSE(f.initialized());}
TEST(MhFileReader, SeekHonorsBounds){MhFileReader f;f.init_text("abc");EXPECT_TRUE(f.seek(1));EXPECT_EQ(f.get_string(),"bc");EXPECT_FALSE(f.seek(99));EXPECT_FALSE(f.seek(-99));}
TEST(MhFileReader, BoolAndUnsignedConversions){MhFileReader f;f.init_text("0 1 255");EXPECT_FALSE(f.get_bool());EXPECT_TRUE(f.get_bool());EXPECT_EQ(f.get_byte(),255);}