#pragma once
#include "mxh/ui/cDialog.hpp"
#include <cstdint>
#include <functional>
#include <vector>
namespace mxh::ui {struct CreateMaterial{std::uint32_t id{};std::uint16_t count{};};class cRareCreateDialog final:public cDialog{public:using CreateCallback=std::function<bool(std::uint32_t)>;void SetRecipe(std::uint32_t result,std::vector<CreateMaterial> materials);bool SetMaterial(std::size_t slot,CreateMaterial material);bool CanCreate()const noexcept;bool Create();void SetCreateCallback(CreateCallback cb){m_callback=std::move(cb);}std::uint32_t ResultId()const noexcept{return m_result;}const std::vector<CreateMaterial>& Materials()const noexcept{return m_materials;}private:std::uint32_t m_result{};std::vector<CreateMaterial>m_materials;std::vector<bool>m_filled;CreateCallback m_callback;};}
