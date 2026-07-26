#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
namespace mxh::ui {
enum class QuestStatus:std::uint8_t{Available,Active,Completed,Claimed};
struct QuestEntry{std::uint32_t id{};std::string title;QuestStatus status{QuestStatus::Available};std::uint32_t reward{};};
class cQuestDialog final:public cDialog{public:using ClaimCallback=std::function<bool(const QuestEntry&)>;void AddQuest(QuestEntry q);bool UpdateQuest(std::uint32_t id,QuestStatus s);bool Select(std::size_t i)noexcept;bool ClaimSelected();void SetClaimCallback(ClaimCallback cb){m_claim=std::move(cb);}const QuestEntry* Selected()const noexcept;const std::vector<QuestEntry>& Quests()const noexcept{return m_quests;}private:std::vector<QuestEntry>m_quests;std::size_t m_selected{static_cast<std::size_t>(-1)};ClaimCallback m_claim;};}
