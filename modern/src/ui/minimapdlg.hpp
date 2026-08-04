#pragma once

#include "bigmapdlg.hpp"
#include "cdialog.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <unordered_map>

namespace mxh::ui {

inline constexpr std::int32_t kMiniMapFullWidth = 140;
inline constexpr std::int32_t kMiniMapFullHeight = 155;

enum class MiniMapMode : std::uint8_t { Full = 0, Small = 1, Max = 2 };

class cMiniMapDlg final : public cDialog {
public:
    using MapViewFn = std::function<bool(std::uint16_t)>;
    using SurvivalMapFn = std::function<bool(std::uint16_t)>;
    using BigMapActiveFn = std::function<bool()>;
    using SetBigMapActiveFn = std::function<void(bool)>;
    using PartyPositionFn = std::function<void(std::uint32_t, std::int32_t, std::int32_t)>;
    using QuestMarkFn = std::function<void(std::uint32_t, std::int32_t)>;
    using MainBarStateFn = std::function<void(bool)>;

    cMiniMapDlg();
    ~cMiniMapDlg() override = default;

    void Linking() noexcept;
    void InitMiniMap(std::uint16_t map_num);
    void SetActive(bool val) noexcept override;
    void Process() noexcept;
    void Render() override;
    void RefreshMode() noexcept;
    void ToggleMinimapMode();

    bool CanActive() const noexcept;
    void SetMapRules(MapViewFn map_view, SurvivalMapFn survival_map) noexcept;
    void SetGameInInitKind(cBigMapDlg::GameInInitKind kind) noexcept { m_initKind = kind; }
    void SetBigMapCallbacks(BigMapActiveFn active, SetBigMapActiveFn set_active) noexcept;
    void SetBigMapForwarders(PartyPositionFn object_pos, PartyPositionFn target_pos, QuestMarkFn quest_mark) noexcept;
    void SetMainBarStateCallback(MainBarStateFn callback) noexcept { m_mainBarState = std::move(callback); }

    void AddHeroIcon(std::uint32_t object_id, std::int32_t x = 0, std::int32_t z = 0);
    void AddIcon(BigMapIconKind kind, std::uint32_t object_id, std::int32_t x = 0, std::int32_t z = 0);
    void AddPartyMemberIcon(std::uint32_t player_id, std::int32_t x = 0, std::int32_t z = 0);
    void RemoveIcon(std::uint32_t object_id) noexcept;
    void SetPartyIconObjectPos(std::uint32_t player_id, std::int32_t x, std::int32_t z) noexcept;
    void SetPartyIconTargetPos(std::uint32_t player_id, std::int32_t x, std::int32_t z) noexcept;
    void ShowQuestMarkIcon(std::uint32_t object_id, std::int32_t kind) noexcept;

    std::uint16_t map_num() const noexcept { return m_mapNum; }
    MiniMapMode current_mode() const noexcept { return m_curMode; }
    bool zoom_pushed() const noexcept { return m_zoomPushed; }
    bool icons_initialized() const noexcept { return m_iconsInitialized; }
    bool map_images_requested() const noexcept { return m_mapImagesRequested; }
    std::size_t icon_count() const noexcept { return m_icons.size(); }
    const BigMapIcon* find_icon(std::uint32_t object_id) const noexcept;
    const std::optional<BigMapIcon>& hero_icon() const noexcept { return m_heroIcon; }
    std::int32_t origin_x() const noexcept { return m_originX; }
    std::int32_t origin_y() const noexcept { return m_originY; }
    std::uint32_t process_count() const noexcept { return m_processCount; }

private:
    static bool is_event_map_exception(std::uint16_t map_num) noexcept;

    std::unordered_map<std::uint32_t, BigMapIcon> m_icons;
    std::optional<BigMapIcon> m_heroIcon;
    MapViewFn m_mapView;
    SurvivalMapFn m_survivalMap;
    BigMapActiveFn m_bigMapActive;
    SetBigMapActiveFn m_setBigMapActive;
    PartyPositionFn m_forwardObjectPos;
    PartyPositionFn m_forwardTargetPos;
    QuestMarkFn m_forwardQuestMark;
    MainBarStateFn m_mainBarState;
    cBigMapDlg::GameInInitKind m_initKind = cBigMapDlg::GameInInitKind::Normal;
    std::uint16_t m_mapNum = 0;
    MiniMapMode m_curMode = MiniMapMode::Full;
    bool m_zoomPushed = false;
    bool m_iconsInitialized = false;
    bool m_mapImagesRequested = false;
    std::int32_t m_originX = 0;
    std::int32_t m_originY = 0;
    std::uint32_t m_processCount = 0;
};

}  // namespace mxh::ui