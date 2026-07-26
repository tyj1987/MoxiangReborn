#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>
namespace mxh::ui {struct PartyMember{std::uint32_t id{};std::string name;bool leader{};};class cPartyDialog final:public cDialog{public:using InviteCallback=std::function<bool(std::string_view)>;void SetSelfId(std::uint32_t id)noexcept{m_self=id;}void AddMember(PartyMember m);bool RemoveMember(std::uint32_t id);bool TransferLeader(std::uint32_t id);bool Invite(std::string_view name);const PartyMember* Leader()const noexcept;const std::vector<PartyMember>& Members()const noexcept{return m_members;}void SetInviteCallback(InviteCallback cb){m_invite=std::move(cb);}private:std::uint32_t m_self{};std::vector<PartyMember>m_members;InviteCallback m_invite;};}

