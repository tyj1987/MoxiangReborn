#pragma once
#include <mxh/db/legacy_shop_appearance.hpp>
#include <string>
#include <unordered_set>

namespace mxh::db {
// Modern-owned data only. Legacy rows require an explicit identity-mapped import.
// Uses the existing character_info.character_data column; unknown blobs are never
// overwritten. Wire v1: MXSH, u32 version/player/count, five u16 skins, 20B rows.
// v2 appends u32 life/shield/naeryuk. Shop-only saves preserve existing vitals.
// v3 adds a u32 pet count and explicit 19-byte modern pet records; ordinary
// saves preserve them. Older readers reject v3; rollback must pair binaries/data.
struct PersistedVitals {
    std::uint32_t life = 0, shield = 0, naeryuk = 0;
};
struct PersistedPet {
    std::uint32_t summon_item = 0;
    std::uint16_t kind = 0, grade = 0;
    std::uint32_t stamina = 0, friendship = 0;
    std::uint8_t alive = 0, rest = 0, summoned = 0;
};
struct ModernShopState {
    LegacyShopAppearanceRows rows;
    std::optional<PersistedVitals> vitals;
    std::optional<std::vector<PersistedPet>> pets;
    Value original; // exact optimistic-concurrency token, including SQL NULL
};
inline std::optional<std::vector<std::uint8_t>> encode_modern_shop_state(
    std::uint32_t player, const LegacyShopAppearanceRows& rows,
    std::optional<PersistedVitals> vitals = std::nullopt,
    const std::optional<std::vector<PersistedPet>>& pets = std::nullopt) {
    if (!player || rows.used_items.size()>50) return std::nullopt;
    if (pets && (!vitals || pets->size()>4096)) return std::nullopt;
    std::vector<std::uint8_t> data{'M','X','S','H'};
    const auto put=[&](std::uint32_t value,int bytes) {
        for(int i=0;i<bytes;++i) data.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    };
    put(pets ? 3 : (vitals ? 2 : 1),4); put(player,4); put(static_cast<std::uint32_t>(rows.used_items.size()),4);
    for(auto skin:rows.skin) put(skin,2);
    std::unordered_set<std::uint16_t> icons;
    for(const auto& row:rows.used_items) {
        if(!row.item_id || !icons.insert(row.item_id).second) return std::nullopt;
        put(row.item_id,2); put(row.position,2); put(row.database_id,4);
        put(row.parameter,4); put(row.begin_time,4); put(row.remaining_time,4);
    }
    if(vitals) { put(vitals->life,4); put(vitals->shield,4); put(vitals->naeryuk,4); }
    if(pets) {
        put(static_cast<std::uint32_t>(pets->size()),4);
        std::unordered_set<std::uint32_t> identities;
        for(const auto& pet:*pets) {
            if(!pet.summon_item || !identities.insert(pet.summon_item).second ||
                pet.alive>1 || pet.rest>1 || pet.summoned>1) return std::nullopt;
            put(pet.summon_item,4); put(pet.kind,2); put(pet.grade,2);
            put(pet.stamina,4); put(pet.friendship,4);
            put(pet.alive,1); put(pet.rest,1); put(pet.summoned,1);
        }
    }
    return data;
}
inline std::optional<ModernShopState> load_modern_shop_state(
    IDbAdapter& db,std::uint32_t player,std::uint32_t account) {
    if(!player || !account) return std::nullopt;
    ResultSet result;
    if(!db.query("SELECT character_data FROM character_info WHERE chrid=? AND userid=?",
        {mxh::db::bind(static_cast<std::int64_t>(player)),mxh::db::bind(std::to_string(account))},result).ok() ||
        result.rows.size()!=1 || result.rows[0].size()!=1) return std::nullopt;
    ModernShopState state; state.original=result.rows[0][0];
    if(std::holds_alternative<std::monostate>(state.original)) return state;
    const auto* data=std::get_if<std::vector<std::uint8_t>>(&state.original);
    if(!data || data->size()<26 || (*data)[0]!='M' || (*data)[1]!='X' ||
        (*data)[2]!='S' || (*data)[3]!='H') return std::nullopt;
    std::size_t offset=4;
    const auto get=[&](int bytes) {
        std::uint32_t value=0;
        for(int i=0;i<bytes;++i) value|=std::uint32_t((*data)[offset++])<<(8*i);
        return value;
    };
    const auto version=get(4);
    if((version<1 || version>3) || get(4)!=player) return std::nullopt;
    const auto count=get(4);
    const auto fixed_size=26+20*count+(version>=2 ? 12 : 0);
    if(count>50 || (version<3 && data->size()!=fixed_size) ||
        (version==3 && data->size()<fixed_size+4)) return std::nullopt;
    for(auto& skin:state.rows.skin) skin=static_cast<std::uint16_t>(get(2));
    std::unordered_set<std::uint16_t> icons;
    for(std::uint32_t i=0;i<count;++i) {
        LegacyUsedShopItem row;
        row.item_id=static_cast<std::uint16_t>(get(2)); row.position=static_cast<std::uint16_t>(get(2));
        row.database_id=get(4); row.parameter=get(4); row.begin_time=get(4); row.remaining_time=get(4);
        if(!row.item_id || !icons.insert(row.item_id).second) return std::nullopt;
        state.rows.used_items.push_back(row);
    }
    if(version>=2) state.vitals=PersistedVitals{get(4),get(4),get(4)};
    if(version==3) {
        const auto pet_count=get(4);
        if(pet_count>4096 || data->size()!=offset+19*pet_count) return std::nullopt;
        state.pets.emplace();
        std::unordered_set<std::uint32_t> identities;
        for(std::uint32_t i=0;i<pet_count;++i) {
            PersistedPet pet;
            pet.summon_item=get(4); pet.kind=static_cast<std::uint16_t>(get(2));
            pet.grade=static_cast<std::uint16_t>(get(2)); pet.stamina=get(4); pet.friendship=get(4);
            pet.alive=static_cast<std::uint8_t>(get(1)); pet.rest=static_cast<std::uint8_t>(get(1));
            pet.summoned=static_cast<std::uint8_t>(get(1));
            if(!pet.summon_item || !identities.insert(pet.summon_item).second ||
                pet.alive>1 || pet.rest>1 || pet.summoned>1) return std::nullopt;
            state.pets->push_back(pet);
        }
    }
    return state;
}
inline bool save_modern_shop_state(IDbAdapter& db,std::uint32_t player,std::uint32_t account,
                                   const ModernShopState& previous,const LegacyShopAppearanceRows& rows,
                                   std::optional<PersistedVitals> vitals = std::nullopt,
                                   const std::optional<std::vector<PersistedPet>>& pets = std::nullopt) {
    if(!account) return false;
    // Revalidate the snapshot's provenance and current owner; the UPDATE also
    // compares the exact old value to reject concurrent changes after this read.
    const auto current=load_modern_shop_state(db,player,account);
    if(!current || current->original!=previous.original) return false;
    const auto data=encode_modern_shop_state(player,rows,vitals ? vitals : current->vitals,
        pets ? pets : current->pets);
    if(!data) return false;
    const bool was_null=std::holds_alternative<std::monostate>(previous.original);
    std::vector<Bind> params{mxh::db::bind(*data),mxh::db::bind(static_cast<std::int64_t>(player)),mxh::db::bind(std::to_string(account))};
    std::string sql="UPDATE character_info SET character_data=? WHERE chrid=? AND userid=? AND character_data ";
    if(was_null) sql+="IS NULL";
    else { sql+="=?"; params.push_back(mxh::db::bind(std::get<std::vector<std::uint8_t>>(previous.original))); }
    const auto result=db.execute(sql,params);
    return result.ok() && result.rows_affected==1;
}
} // namespace mxh::db
