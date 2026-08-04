#include "minimapdlg.hpp"

namespace mxh::ui {

cMiniMapDlg::cMiniMapDlg() {
    m_mapView = [](std::uint16_t) { return true; };
    m_survivalMap = [](std::uint16_t) { return false; };
}

void cMiniMapDlg::Linking() noexcept {
    m_iconsInitialized = true;
}

bool cMiniMapDlg::is_event_map_exception(std::uint16_t map_num) noexcept {
    return map_num == 23 || map_num == 24 || map_num == 26 || map_num == 29;
}

void cMiniMapDlg::InitMiniMap(std::uint16_t map_num) {
    bool active = isActive();
    if (m_initKind == cBigMapDlg::GameInInitKind::SuryunEnter ||
        m_initKind == cBigMapDlg::GameInInitKind::EventMapEnter) {
        active = is_event_map_exception(map_num);
    }
    if (m_mapView && !m_mapView(map_num)) active = false;
    if (m_survivalMap && m_survivalMap(map_num)) active = is_event_map_exception(map_num);
    m_mapNum = map_num;
    m_curMode = MiniMapMode::Full;
    m_mapImagesRequested = true;
    cDialog::SetActive(active);
}

bool cMiniMapDlg::CanActive() const noexcept {
    return !m_mapView || m_mapView(m_mapNum);
}

void cMiniMapDlg::SetActive(bool val) noexcept {
    if (!CanActive()) val = false;
    cDialog::SetActive(val);
    if (m_mainBarState) m_mainBarState(val);
}

void cMiniMapDlg::SetMapRules(MapViewFn map_view, SurvivalMapFn survival_map) noexcept {
    m_mapView = std::move(map_view);
    m_survivalMap = std::move(survival_map);
}

void cMiniMapDlg::SetBigMapCallbacks(BigMapActiveFn active, SetBigMapActiveFn set_active) noexcept {
    m_bigMapActive = std::move(active);
    m_setBigMapActive = std::move(set_active);
}

void cMiniMapDlg::SetBigMapForwarders(PartyPositionFn object_pos, PartyPositionFn target_pos,
                                      QuestMarkFn quest_mark) noexcept {
    m_forwardObjectPos = std::move(object_pos);
    m_forwardTargetPos = std::move(target_pos);
    m_forwardQuestMark = std::move(quest_mark);
}

void cMiniMapDlg::Process() noexcept {
    ++m_processCount;
    if (m_curMode == MiniMapMode::Full || !m_heroIcon) {
        m_originX = 0;
        m_originY = 0;
    } else {
        m_originX = m_heroIcon->object_x - kMiniMapFullWidth / 2;
        m_originY = m_heroIcon->object_z - kMiniMapFullHeight / 2;
    }
}

void cMiniMapDlg::Render() {
    if (!isActive() || !m_mapImagesRequested) return;
    cDialog::Render();
}

void cMiniMapDlg::RefreshMode() noexcept {
    m_zoomPushed = m_bigMapActive ? m_bigMapActive() : false;
}

void cMiniMapDlg::ToggleMinimapMode() {
    if (m_setBigMapActive) {
        const bool active = m_bigMapActive ? m_bigMapActive() : false;
        m_setBigMapActive(!active);
    }
    RefreshMode();
}

void cMiniMapDlg::AddHeroIcon(std::uint32_t object_id, std::int32_t x, std::int32_t z) {
    m_heroIcon = BigMapIcon{object_id, BigMapIconKind::Self, x, z, x, z};
}

void cMiniMapDlg::AddIcon(BigMapIconKind kind, std::uint32_t object_id, std::int32_t x, std::int32_t z) {
    RemoveIcon(object_id);
    m_icons.emplace(object_id, BigMapIcon{object_id, kind, x, z, x, z});
}

void cMiniMapDlg::AddPartyMemberIcon(std::uint32_t player_id, std::int32_t x, std::int32_t z) {
    AddIcon(BigMapIconKind::PartyMember, player_id, x, z);
}

void cMiniMapDlg::RemoveIcon(std::uint32_t object_id) noexcept {
    m_icons.erase(object_id);
}

void cMiniMapDlg::SetPartyIconObjectPos(std::uint32_t player_id, std::int32_t x, std::int32_t z) noexcept {
    auto it = m_icons.find(player_id);
    if (it != m_icons.end()) {
        it->second.object_x = x;
        it->second.object_z = z;
        it->second.target_x = x;
        it->second.target_z = z;
    }
    if (m_forwardObjectPos) m_forwardObjectPos(player_id, x, z);
}

void cMiniMapDlg::SetPartyIconTargetPos(std::uint32_t player_id, std::int32_t x, std::int32_t z) noexcept {
    auto it = m_icons.find(player_id);
    if (it != m_icons.end()) {
        it->second.target_x = x;
        it->second.target_z = z;
    }
    if (m_forwardTargetPos) m_forwardTargetPos(player_id, x, z);
}

void cMiniMapDlg::ShowQuestMarkIcon(std::uint32_t object_id, std::int32_t kind) noexcept {
    auto it = m_icons.find(object_id);
    if (it != m_icons.end()) it->second.quest_mark_kind = kind;
    if (m_forwardQuestMark) m_forwardQuestMark(object_id, kind);
}

const BigMapIcon* cMiniMapDlg::find_icon(std::uint32_t object_id) const noexcept {
    const auto it = m_icons.find(object_id);
    return it == m_icons.end() ? nullptr : &it->second;
}

}  // namespace mxh::ui