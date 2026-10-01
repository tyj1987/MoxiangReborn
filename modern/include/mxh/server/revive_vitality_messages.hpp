#pragma once
#include "mxh/server/player.hpp"
#include "mxh/net/net.hpp"
#include "mxh/proto/protocol.hpp"
#include "mxh/proto/character_revive.hpp"
#include <limits>
#include <optional>

namespace mxh::server {
// Prepare before committing; publish only after commit. Penalty and pet messages
// belong between position and vitality and are deliberately separate here.
struct ReviveVitalityMessages {
    mxh::net::Message position;
    std::vector<mxh::net::Message> vitality;
};
inline std::optional<ReviveVitalityMessages> prepare_revive_vitality_messages(
    const Player& original,const Player& revived) {
    const auto& before=original.state(); const auto& after=revived.state();
    if(original.lifecycle()!=PlayerLifecycle::Dead || !revived.is_active() ||
        before.player_id!=after.player_id || before.user_id!=after.user_id ||
        before.map_num!=after.map_num) return std::nullopt;
    const auto encoded=mxh::proto::encode_character_revive(after.player_id,after.pos_x,after.pos_z);
    if(!encoded) return std::nullopt;
    ReviveVitalityMessages result;
    result.position.header.category=static_cast<std::uint8_t>(mxh::proto::Category::UserConn);
    result.position.header.protocol=44;
    result.position.header.object_id=after.player_id;
    result.position.payload.assign(encoded->begin(),encoded->end());
    const auto append=[&](std::uint32_t old_value,std::uint32_t new_value,
        mxh::proto::CharacterProtocol protocol) {
        const auto delta=std::int64_t(new_value)-old_value;
        if(delta<std::numeric_limits<std::int32_t>::min() ||
            delta>std::numeric_limits<std::int32_t>::max()) return false;
        if(!delta) return true;
        mxh::net::Message message;
        message.header.category=static_cast<std::uint8_t>(mxh::proto::Category::Character);
        message.header.protocol=static_cast<std::uint8_t>(protocol);
        message.header.object_id=after.player_id;
        const auto bits=static_cast<std::uint32_t>(delta);
        for(unsigned i=0;i<4;++i) message.payload.push_back(static_cast<std::uint8_t>(bits>>(8*i)));
        result.vitality.push_back(std::move(message));
        return true;
    };
    using P=mxh::proto::CharacterProtocol;
    if(!append(before.vitals.current_hp,after.vitals.current_hp,P::LifeAck) ||
       !append(before.vitals.current_mp,after.vitals.current_mp,P::NaeryukAck) ||
       !append(before.vitals.current_shield,after.vitals.current_shield,P::ShieldAck)) return std::nullopt;
    return result;
}
} // namespace mxh::server
