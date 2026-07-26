#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace mxh::server {
class MhFileReader {
public:
 bool init_text(const std::string& text); bool init_bin(const std::string& path); void release();
 bool initialized() const noexcept{return initialized_;} bool eof() const noexcept{return cursor_>=data_.size();}
 std::string get_string(); std::string get_quoted(); std::string get_line();
 int get_int(); std::int32_t get_long(); float get_float(); std::uint32_t get_dword(); std::uint16_t get_word(); std::uint8_t get_byte(); bool get_bool(); bool seek(std::int32_t offset);
 const std::vector<std::uint8_t>& data() const noexcept{return data_;}
private: std::vector<std::uint8_t> data_;std::size_t cursor_=0;bool initialized_=false;std::string token();
};
}