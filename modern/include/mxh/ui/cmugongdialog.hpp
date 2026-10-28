#pragma once
#include "mxh/ui/cDialog.hpp"
#include "mxh/services/ISkillService.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace mxh::ui {struct MugongEntry{std::uint32_t id{};std::string name;bool enabled{};};class cMugongDialog final:public cDialog{public:static constexpr std::size_t kSlots=12;bool SetSlot(std::size_t slot,MugongEntry entry);bool ClearSlot(std::size_t slot);bool SetEnabled(std::size_t slot,bool enabled);bool Select(std::size_t slot)noexcept;const MugongEntry* Selected()const noexcept;const std::vector<MugongEntry>& Slots()const noexcept{return m_slots;}void SetSkillService(const mxh::services::ISkillService* service)noexcept{m_skills=service;}void RefreshEnabledFromSkillService();private:std::vector<MugongEntry>m_slots=std::vector<MugongEntry>(kSlots);std::size_t m_selected{static_cast<std::size_t>(-1)};const mxh::services::ISkillService* m_skills{};};}
