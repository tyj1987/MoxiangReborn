#include "mxh/server/mh_file_reader.hpp"
#include "mxh/compat/mh_file_ex.hpp"
#include <cctype>
#include <cstdlib>
#include <fstream>
namespace mxh::server {
bool MhFileReader::init_text(const std::string&t){data_.assign(t.begin(),t.end());cursor_=0;initialized_=true;return true;}
bool MhFileReader::init_bin(const std::string&p){auto r=mxh::compat::read_mh_bin(p);if(!r)return false;data_=std::move(r.value.data);cursor_=0;initialized_=true;return true;}
void MhFileReader::release(){data_.clear();cursor_=0;initialized_=false;}
std::string MhFileReader::token(){while(cursor_<data_.size()&&(data_[cursor_]==' '||data_[cursor_]=='\t'||data_[cursor_]=='\r'||data_[cursor_]=='\n'))++cursor_;auto b=cursor_;while(cursor_<data_.size()&&data_[cursor_]!=' '&&data_[cursor_]!='\t'&&data_[cursor_]!='\r'&&data_[cursor_]!='\n')++cursor_;return std::string(data_.begin()+b,data_.begin()+cursor_);}
std::string MhFileReader::get_string(){return token();}
std::string MhFileReader::get_quoted(){while(cursor_<data_.size()&&data_[cursor_]!='"'&&data_[cursor_]!='\n')++cursor_;if(cursor_>=data_.size()||data_[cursor_]!='"')return {};auto b=++cursor_;while(cursor_<data_.size()&&data_[cursor_]!='"'&&data_[cursor_]!='\n')++cursor_;auto out=std::string(data_.begin()+b,data_.begin()+cursor_);if(cursor_<data_.size()&&data_[cursor_]=='"')++cursor_;return out;}
std::string MhFileReader::get_line(){if(cursor_>=data_.size())return {};auto b=cursor_;while(cursor_<data_.size()&&data_[cursor_]!='\n')++cursor_;auto out=std::string(data_.begin()+b,data_.begin()+cursor_);if(cursor_<data_.size())++cursor_;if(!out.empty()&&out.back()=='\r')out.pop_back();return out;}
int MhFileReader::get_int(){return std::atoi(token().c_str());}std::int32_t MhFileReader::get_long(){return static_cast<std::int32_t>(get_int());}float MhFileReader::get_float(){return std::strtof(token().c_str(),nullptr);}std::uint32_t MhFileReader::get_dword(){return static_cast<std::uint32_t>(std::strtoul(token().c_str(),nullptr,10));}std::uint16_t MhFileReader::get_word(){return static_cast<std::uint16_t>(get_dword());}std::uint8_t MhFileReader::get_byte(){return static_cast<std::uint8_t>(get_dword());}bool MhFileReader::get_bool(){return get_int()!=0;}bool MhFileReader::seek(std::int32_t n){if(n<0&&static_cast<std::size_t>(-n)>cursor_)return false;auto next=n<0?cursor_-static_cast<std::size_t>(-n):cursor_+static_cast<std::size_t>(n);if(next>data_.size())return false;cursor_=next;return true;}
}
[[maybe_unused]] constexpr int mh_file_reader_translation_unit_anchor=0;