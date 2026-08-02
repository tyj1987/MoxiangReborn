#include "mxh/server/crypt.hpp"
namespace mxh::server {
void Crypt::create(){mxh::crypto::HselInit i;i.iEncryptType=mxh::crypto::HSEL_ENCRYPTTYPE_RAND;i.iDesCount=mxh::crypto::HSEL_DES_TRIPLE;i.iCustomize=mxh::crypto::HSEL_KEY_TYPE_DEFAULT;i.iSwapFlag=mxh::crypto::HSEL_SWAP_FLAG_ON;en_stream_.initial(i);de_stream_.initial(i);en_init_=i;de_init_=i;}
void Crypt::init(const mxh::crypto::HselInit& en,const mxh::crypto::HselInit& de){en_init_=de;de_init_=en;en_stream_.initial(de);de_stream_.initial(en);initialized_=true;}
bool Crypt::encrypt(char*d,int n){if(!initialized_)return true;return en_stream_.encrypt(d,n);}
bool Crypt::decrypt(char*d,int n){if(!initialized_)return true;return de_stream_.decrypt(d,n);}
char Crypt::encrypt_crc()const{
  if(!initialized_)return 0;
  return static_cast<char>(en_stream_.get_crc_char());
}
char Crypt::decrypt_crc()const{
  if(!initialized_)return 0;
  return static_cast<char>(de_stream_.get_crc_char());
}
}
[[maybe_unused]] constexpr int crypt_translation_unit_anchor=0;