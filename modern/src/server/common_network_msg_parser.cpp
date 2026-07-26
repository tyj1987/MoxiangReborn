#include "mxh/server/common_network_msg_parser.hpp"
namespace mxh::server {
CommonParserTables::CommonParserTables():server(mp_max),user(mp_max){}
bool CommonParserTables::set_server(std::size_t c,MessageParser p){if(c>=mp_max||!p)return false;server[c]=std::move(p);return true;}
bool CommonParserTables::set_user(std::size_t c,MessageParser p){if(c>=mp_max||!p)return false;user[c]=std::move(p);return true;}
bool CommonParserTables::invoke_server(std::size_t c,std::uint32_t i,const std::vector<std::uint8_t>&b)const{if(c>=mp_max)return false;if(!server[c])return true;server[c](i,b);return true;}
bool CommonParserTables::invoke_user(std::size_t c,std::uint32_t i,const std::vector<std::uint8_t>&b)const{if(c>=mp_max)return false;if(!user[c])return true;user[c](i,b);return true;}
}
[[maybe_unused]] constexpr int common_network_msg_parser_translation_unit_anchor=0;