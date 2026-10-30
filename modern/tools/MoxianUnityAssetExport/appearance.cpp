#include "mxh/compat/character_appearance_catalog.hpp"
#include "mxh/game/item_list_parser.hpp"
#include "nlohmann/json.hpp"
#include <windows.h>
#include <bcrypt.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <set>
using Json=nlohmann::ordered_json;
namespace fs=std::filesystem;
static std::string hash(const std::vector<std::uint8_t>& bytes) {
    BCRYPT_ALG_HANDLE alg{};
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("SHA256 provider failure");
    unsigned char digest[32]{};
    auto status=BCryptHash(alg,nullptr,0,const_cast<PUCHAR>(bytes.data()),static_cast<ULONG>(bytes.size()),digest,32);
    BCryptCloseAlgorithmProvider(alg,0);
    if(status<0)throw std::runtime_error("SHA256 failure");
    std::ostringstream text;
    for(auto value:digest)text<<std::hex<<std::setw(2)<<std::setfill('0')<<static_cast<int>(value);
    return text.str();
}
int main(int argc,char** argv) {
    try {
        if(argc!=3)throw std::runtime_error("usage: mxh_unity_appearance_export PlayDH-root new-output.mxhappearance");
        fs::path root=fs::absolute(argv[1]),output=fs::absolute(argv[2]);
        if(output.extension()!=".mxhappearance" || fs::exists(output))throw std::runtime_error("Output must be a new .mxhappearance path");
        Json document={{"schemaVersion",1},{"kind","appearance-catalog"},{"profileId","unity-remaster-v1"},
            {"converterVersion","appearance-native-1"},{"releaseReady",false},{"sources",Json::array()},{"genders",Json::array()}};
        auto read=[&](const std::string& name) {
            auto path=root/name;
            auto size=fs::file_size(path);
            if(size==0 || size>32*1024*1024)throw std::runtime_error("Invalid source size: "+name);
            std::ifstream stream(path,std::ios::binary);
            std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
            if(!stream.read(reinterpret_cast<char*>(bytes.data()),bytes.size()))throw std::runtime_error("Source read failed: "+name);
            document["sources"].push_back({{"path",name},{"sha256",hash(bytes)},{"bytes",size}});
            return bytes;
        };
        for(unsigned gender=0;gender<2;++gender) {
            std::string suffix=gender==0 ? "M.bin" : "W.bin";
            auto model=mxh::compat::CharacterAppearanceCatalog::parse_mod_list_bin(read("Resource/Client/ModList_"+suffix));
            auto face=mxh::compat::CharacterAppearanceCatalog::parse_part_list_bin(read("Resource/Client/FaceList_"+suffix));
            auto hair=mxh::compat::CharacterAppearanceCatalog::parse_part_list_bin(read("Resource/Client/HairList_"+suffix));
            if(!model || !face || !hair)throw std::runtime_error("Invalid appearance list");
            document["genders"].push_back({{"gender",gender},{"baseObject",model->base_object},
                {"models",model->mod_files},{"faces",*face},{"hairs",*hair}});
        }
        auto items=mxh::game::parse_item_list_bytes(read("Resource/ItemList.bin"));
        if(items.parse_errors || !items.error_message.empty() || items.items.empty())throw std::runtime_error("ItemList parse failed: "+items.error_message);
        document["items"]=Json::array();std::set<std::uint16_t> ids;
        for(const auto& item:items.items) {
            if(!ids.insert(item.ItemIdx).second)throw std::runtime_error("Duplicate item ID");
            document["items"].push_back({{"itemId",item.ItemIdx},{"partType",item.Part3DType},
                {"modelIndex",item.Part3DModelNum},{"weaponType",item.WeaponType}});
        }
        fs::create_directories(output.parent_path());
        std::ofstream stream(output,std::ios::binary);stream<<document.dump(2)<<'\n';
        if(!stream)throw std::runtime_error("Output write failed");
        std::cout<<"sources=7 genders=2 items="<<items.items.size()<<'\n';
        return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
