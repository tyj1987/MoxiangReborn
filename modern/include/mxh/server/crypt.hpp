#pragma once
#include "mxh/crypto/hsel_stream.hpp"
namespace mxh::server {
class Crypt {
public:
 void create();
 void init(const mxh::crypto::HselInit& en,const mxh::crypto::HselInit& de);
 bool encrypt(char* data,int size); bool decrypt(char* data,int size);
 char encrypt_crc() const; char decrypt_crc() const; void set_init(bool value) noexcept { initialized_=value; }
 bool initialized() const noexcept { return initialized_; }
 const mxh::crypto::HselInit& encrypt_key() const noexcept { return en_init_; }
 const mxh::crypto::HselInit& decrypt_key() const noexcept { return de_init_; }
private: mxh::crypto::HselInit en_init_{}; mxh::crypto::HselInit de_init_{}; mxh::crypto::HselStream en_stream_{}; mxh::crypto::HselStream de_stream_{}; bool initialized_=false;
};
}