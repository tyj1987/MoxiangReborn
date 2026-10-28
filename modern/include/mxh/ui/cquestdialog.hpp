#pragma once
#include "mxh/ui/cDialog.hpp"
#include "mxh/services/IQuestService.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
namespace mxh::ui {
class cListDialog;
enum class QuestStatus:std::uint8_t{Available,Active,Completed,Claimed};
struct QuestEntry{std::uint32_t id{};std::string title;QuestStatus status{QuestStatus::Available};std::uint32_t reward{};};
class cQuestDialog final:public cDialog{public:using ClaimCallback=std::function<bool(const QuestEntry&)>;void Linking();void AddQuest(QuestEntry q);void ClearQuests();bool RemoveQuest(std::uint32_t id);bool UpdateQuest(std::uint32_t id,QuestStatus s);bool Select(std::size_t i)noexcept;bool ClaimSelected();std::uint32_t ActionEvent(std::int32_t mouseX,std::int32_t mouseY,std::uint32_t mouseFlags)override;void SetClaimCallback(ClaimCallback cb){m_claim=std::move(cb);}void SetQuestService(mxh::services::IQuestService* service)noexcept{m_quest_service=service;}const QuestEntry* Selected()const noexcept;const std::vector<QuestEntry>& Quests()const noexcept{return m_quests;}cListDialog* QuestList()const noexcept{return m_quest_list;}private:void syncQuestList();std::vector<QuestEntry>m_quests;std::size_t m_selected{static_cast<std::size_t>(-1)};cListDialog* m_quest_list{};ClaimCallback m_claim;mxh::services::IQuestService* m_quest_service{};};}
