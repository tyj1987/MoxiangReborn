#include "bigmapdlg.hpp"

namespace mxh::ui {

cBigMapDlg::cBigMapDlg() {
    m_mapView = [](std::uint16_t) { return true; };
    m_survivalMap = [](std::uint16_t) { return false; };
}

void cBigMapDlg::Linking() noexcept {
    m_iconsInitialized = true;
}

bool cBigMapDlg::is_event_map_exception(std::uint16_t map_num) noexcept {
    return map_num == 23 || map_num == 24 || map_num == 26 || map_num == 29;
}

void cBigMapDlg::InitBigMap(std::uint16_t map_num) {
    bool active = isActive();
    if (m_initKind == GameInInitKind::SuryunEnter || m_initKind == GameInInitKind::EventMapEnter) {
        active = is_event_map_exception(map_num);
    }
    if (m_mapView && !m_mapView(map_num)) active = false;
    if (m_survivalMap && m_survivalMap(map_num)) active = is_event_map_exception(map_num);
    m_mapNum = map_num;
    m_mapImageRequested = true;
    cDialog::SetActive(active);
}

bool cBigMapDlg::CanActive() const noexcept {
    return !m_mapView || m_mapView(m_mapNum);
}

void cBigMapDlg::SetActive(bool val) noexcept {
    if (!CanActive()) val = false;
    cDialog::SetActive(val);
    if (m_refreshMiniMap) m_refreshMiniMap();
}

void cBigMapDlg::SetMapRules(MapViewFn map_view, SurvivalMapFn survival_map) noexcept {
    m_mapView = std::move(map_view);
    m_survivalMap = std::move(survival_map);
}

void cBigMapDlg::Process() noexcept {
    ++m_processCount;
}

void cBigMapDlg::Render() {
    if (!isActive() || !m_mapImageRequested) return;
    cDialog::Render();
}

void cBigMapDlg::AddHeroIcon(std::uint32_t object_id, std::int32_t x, std::int32_t z) {
    m_heroIcon = BigMapIcon{object_id, BigMapIconKind::Self, x, z, x, z};
}

void cBigMapDlg::AddIcon(BigMapIconKind kind, std::uint32_t object_id, std::int32_t x, std::int32_t z) {
    RemoveIcon(object_id);
    m_icons.emplace(object_id, BigMapIcon{object_id, kind, x, z, x, z});
}

void cBigMapDlg::AddPartyMemberIcon(std::uint32_t player_id, std::int32_t x, std::int32_t z) {
    AddIcon(BigMapIconKind::PartyMember, player_id, x, z);
}

void cBigMapDlg::RemoveIcon(std::uint32_t object_id) noexcept {
    m_icons.erase(object_id);
}

void cBigMapDlg::SetPartyIconObjectPos(std::uint32_t player_id, std::int32_t x, std::int32_t z) noexcept {
    auto it = m_icons.find(player_id);
    if (it == m_icons.end()) return;
    it->second.object_x = x;
    it->second.object_z = z;
    it->second.target_x = x;
    it->second.target_z = z;
}

void cBigMapDlg::SetPartyIconTargetPos(std::uint32_t player_id, std::int32_t x, std::int32_t z) noexcept {
    auto it = m_icons.find(player_id);
    if (it == m_icons.end()) return;
    it->second.target_x = x;
    it->second.target_z = z;
}

void cBigMapDlg::ShowQuestMarkIcon(std::uint32_t object_id, std::int32_t kind) noexcept {
    auto it = m_icons.find(object_id);
    if (it == m_icons.end()) return;
    it->second.quest_mark_kind = kind;
}

const BigMapIcon* cBigMapDlg::find_icon(std::uint32_t object_id) const noexcept {
    const auto it = m_icons.find(object_id);
    return it == m_icons.end() ? nullptr : &it->second;
}

}  // namespace mxh::ui