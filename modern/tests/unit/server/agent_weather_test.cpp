#include "mxh/server/agent_weather.hpp"
#include <gtest/gtest.h>
using namespace mxh::server;
TEST(Weather, NoUserDrops){WeatherRequest r;r.user_found=false;EXPECT_EQ(classify_weather(r).kind,WeatherActionKind::drop_no_user);}
TEST(Weather, PlayerDropsWrongUserLevel){WeatherRequest r;r.user_found=true;r.user_level=1;r.protocol=weather_set;r.map_num=5;EXPECT_EQ(classify_weather(r).kind,WeatherActionKind::drop_wrong_user_level);}
TEST(Weather, GMWithoutMasterOrEventerDrops){WeatherRequest r;r.user_found=true;r.user_level=user_level_gm_weather;r.is_gm_master_or_eventer=false;EXPECT_EQ(classify_weather(r).kind,WeatherActionKind::drop_wrong_user_level);}
TEST(Weather, GMWithMasterOrEventerForwards){WeatherRequest r;r.user_found=true;r.user_level=user_level_gm_weather;r.is_gm_master_or_eventer=true;r.protocol=weather_set;r.map_num=12;auto a=classify_weather(r);EXPECT_EQ(a.kind,WeatherActionKind::forward_to_map_by_data);EXPECT_EQ(a.target_map,12);}
TEST(Weather, ProgrammerBypassesGMPowerCheck){WeatherRequest r;r.user_found=true;r.user_level=user_level_programmer;r.is_gm_master_or_eventer=false;r.protocol=weather_exe;r.data_field=7;auto a=classify_weather(r);EXPECT_EQ(a.kind,WeatherActionKind::forward_to_map_by_data);EXPECT_EQ(a.target_map,7);}
TEST(Weather, DeveloperBypassesGMPowerCheck){WeatherRequest r;r.user_found=true;r.user_level=user_level_developer;r.protocol=weather_return;r.data_field=33;EXPECT_EQ(classify_weather(r).target_map,33u);}
TEST(Weather, SetProtocolUsesMapNum){WeatherRequest r;r.user_found=true;r.user_level=user_level_developer;r.protocol=weather_set;r.map_num=42;EXPECT_EQ(classify_weather(r).target_map,42u);}
TEST(Weather, ExeProtocolUsesDataField){WeatherRequest r;r.user_found=true;r.user_level=user_level_developer;r.protocol=weather_exe;r.data_field=99;EXPECT_EQ(classify_weather(r).target_map,99u);}
TEST(Weather, ReturnProtocolUsesDataField){WeatherRequest r;r.user_found=true;r.user_level=user_level_developer;r.protocol=weather_return;r.data_field=8;EXPECT_EQ(classify_weather(r).target_map,8u);}