#include "mxh/server/murimnet_wire_serializer.hpp"
#include "mxh/server/murimnet_channel.hpp"
#include "mxh/server/murimnet_play_room.hpp"
#include "mxh/server/murimnet_play_room_manager.hpp"
#include <algorithm>
#include <cstring>
#include <string>
namespace mxh::server {
namespace {
template <std::size_t N>
void copy_truncated(std::array<char, N>& dst, const std::string& src) {
    dst.fill(0);
    const std::size_t n = std::min(src.size(), N - 1);
    std::memcpy(dst.data(), src.data(), n);
}
}  // namespace
std::vector<std::uint8_t> mnh_build_channel_list_wire(const MurimNetChannelManager& mgr) {
    MnhMsgChannelBaseInfoList msg{};
    msg.Category = MNH_CATEGORY_MURIMNET;
    msg.Protocol = static_cast<std::uint8_t>(MurimNetProtocol::Chnl_ChannelInfoList);
    msg.dwObjectID = 0;
    const std::size_t n = mgr.channel_count();
    if (n > MNH_MAX_CHANNEL_IN_MURIMNET) return {};
    msg.dwTotalChannelNum = static_cast<std::uint32_t>(n);
    std::size_t i = 0;
    mgr.for_each_channel([&](const MurimNetChannel& c) {
        if (i >= n) return;
        auto& dst = msg.ChannelInfo[i++];
        dst.dwChannelIndex = c.channel_index();
        copy_truncated(dst.strChannelTitle, c.title());
        dst.cbChannelKind = static_cast<std::int8_t>(c.kind());
        dst.wMaxPlayer = c.max_players();
        dst.wPlayerNum = static_cast<std::uint16_t>(c.player_count());
    });
    const std::size_t total = sizeof(MnhWireBase) + sizeof(std::uint32_t) + n * sizeof(MnhChannelBaseInfo);
    std::vector<std::uint8_t> out(total);
    std::memcpy(out.data(), &msg, total);
    return out;
}
std::vector<std::uint8_t> mnh_build_channel_info_wire(const MurimNetChannel& c, std::uint8_t protocol) {
    MnhMsgChannelBaseInfo msg{};
    msg.Category = MNH_CATEGORY_MURIMNET;
    msg.Protocol = protocol;
    msg.dwObjectID = c.channel_index();
    auto& dst = msg.Info;
    dst.dwChannelIndex = c.channel_index();
    copy_truncated(dst.strChannelTitle, c.title());
    dst.cbChannelKind = static_cast<std::int8_t>(c.kind());
    dst.wMaxPlayer = c.max_players();
    dst.wPlayerNum = static_cast<std::uint16_t>(c.player_count());
    std::vector<std::uint8_t> out(sizeof(MnhMsgChannelBaseInfo));
    std::memcpy(out.data(), &msg, out.size());
    return out;
}

std::vector<std::uint8_t> mnh_build_playroom_info_wire(const MurimNetPlayRoom& r, std::uint8_t protocol) {
    MnhMsgPlayRoomBaseInfo msg{};
    msg.Category = MNH_CATEGORY_MURIMNET;
    msg.Protocol = protocol;
    msg.dwObjectID = r.room_index();
    auto& dst = msg.Info;
    dst.dwPlayRoomIndex = r.room_index();
    copy_truncated(dst.strPlayRoomTitle, r.title());
    dst.cbPlayRoomKind = static_cast<std::int8_t>(r.kind());
    dst.wMaxObserver = r.max_observers();
    dst.wMaxPlayerPerTeam = r.max_players_per_team();
    dst.MoneyForPlay = r.money_for_play();
    dst.wPlayerNum = static_cast<std::uint16_t>(r.player_count());
    dst.cbStart = r.started() ? 1 : 0;
    dst.wMapNum = r.map_num();
    std::vector<std::uint8_t> out(sizeof(MnhMsgPlayRoomBaseInfo));
    std::memcpy(out.data(), &msg, out.size());
    return out;
}

std::vector<std::uint8_t> mnh_build_player_info_wire(const MurimNetPlayer& p, std::uint8_t protocol) {
    MnhMsgPlayerBaseInfo msg{};
    msg.Category = MNH_CATEGORY_MURIMNET;
    msg.Protocol = protocol;
    msg.dwObjectID = p.id();
    const auto& info = p.info();
    auto& dst = msg.Info;
    dst.dwObjectID = info.player_id;
    dst.wRankPoint = info.rank_point;
    copy_truncated(dst.strPlayerName, info.name);
    dst.Level = info.level;
    copy_truncated(dst.strNick, info.nick);
    dst.wPlayCount = info.play_count;
    dst.wWin = info.win;
    dst.wLose = info.lose;
    copy_truncated(dst.strMunpa, info.munpa);
    dst.cbPositionInMunpa = info.position_in_munpa;
    dst.cbTeam = static_cast<std::int8_t>(info.team);
    std::vector<std::uint8_t> out(sizeof(MnhMsgPlayerBaseInfo));
    std::memcpy(out.data(), &msg, out.size());
    return out;
}

std::vector<std::uint8_t> mnh_build_chat_wire(std::uint32_t sender_id, const std::string& text, std::uint8_t protocol) {
    MnhMsgChat msg{};
    msg.Category = MNH_CATEGORY_MURIMNET;
    msg.Protocol = protocol;
    msg.dwObjectID = sender_id;
    const std::size_t n = std::min(text.size(), MNH_MAX_CHAT_LENGTH);
    std::memcpy(msg.Msg.data(), text.data(), n);
    msg.Msg[n] = 0;
    const std::size_t total = sizeof(MnhWireBase) + n + 1;
    std::vector<std::uint8_t> out(total);
    std::memcpy(out.data(), &msg, total);
    return out;
}

std::vector<std::uint8_t> mnh_build_player_list_wire(const std::function<void(const std::function<void(const MurimNetPlayer&)>&)>& visitor, std::uint8_t protocol) {
    MnhMsgPlayerBaseInfoList msg{};
    msg.Category = MNH_CATEGORY_MURIMNET;
    msg.Protocol = protocol;
    msg.dwObjectID = 0;
    std::size_t i = 0;
    if (visitor) {
        visitor([&](const MurimNetPlayer& p) {
            if (i >= MNH_MAX_PLAYER_IN_CHANNEL) return;
            const auto& info = p.info();
            auto& dst = msg.PlayerInfo[i++];
            dst.dwObjectID = info.player_id;
            dst.wRankPoint = info.rank_point;
            copy_truncated(dst.strPlayerName, info.name);
            dst.Level = info.level;
            copy_truncated(dst.strNick, info.nick);
            dst.wPlayCount = info.play_count;
            dst.wWin = info.win;
            dst.wLose = info.lose;
            copy_truncated(dst.strMunpa, info.munpa);
            dst.cbPositionInMunpa = info.position_in_munpa;
            dst.cbTeam = static_cast<std::int8_t>(info.team);
        });
    }
    msg.dwTotalPlayerNum = static_cast<std::uint32_t>(i);
    const std::size_t total = sizeof(MnhWireBase) + sizeof(std::uint32_t) + i * sizeof(MnhPlayerBaseInfo);
    std::vector<std::uint8_t> out(total);
    std::memcpy(out.data(), &msg, total);
    return out;
}

std::vector<std::uint8_t> mnh_build_playroom_list_wire(const MurimNetPlayRoomManager& mgr) {
    MnhMsgPlayRoomBaseInfoList msg{};
    msg.Category = MNH_CATEGORY_MURIMNET;
    msg.Protocol = static_cast<std::uint8_t>(MurimNetProtocol::Chnl_PlayRoomInfoList);
    msg.dwObjectID = 0;
    const std::size_t n = mgr.room_count();
    if (n > MNH_MAX_PLAYROOM_IN_MURIMNET) return {};
    msg.dwTotalPlayRoomNum = static_cast<std::uint32_t>(n);
    std::size_t i = 0;
    mgr.for_each_room([&](const MurimNetPlayRoom& r) {
        if (i >= n) return;
        auto& dst = msg.PlayRoomInfo[i++];
        dst.dwPlayRoomIndex = r.room_index();
        copy_truncated(dst.strPlayRoomTitle, r.title());
        dst.cbPlayRoomKind = static_cast<std::int8_t>(r.kind());
        dst.wMaxObserver = r.max_observers();
        dst.wMaxPlayerPerTeam = r.max_players_per_team();
        dst.MoneyForPlay = r.money_for_play();
        dst.wPlayerNum = static_cast<std::uint16_t>(r.player_count());
        dst.cbStart = r.started() ? 1 : 0;
        dst.wMapNum = r.map_num();
    });
    const std::size_t total = sizeof(MnhWireBase) + sizeof(std::uint32_t) + n * sizeof(MnhPlayRoomBaseInfo);
    std::vector<std::uint8_t> out(total);
    std::memcpy(out.data(), &msg, total);
    return out;
}
}
