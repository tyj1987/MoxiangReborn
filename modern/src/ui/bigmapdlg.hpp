#pragma once

#include "cdialog.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <unordered_map>

namespace mxh::ui {

inline constexpr std::int32_t kBigMapWidth = 512;
inline constexpr std::int32_t kBigMapHeight = 512;
inline constexpr std::uint32_t kBigMapIconColor = 0xDCFFFFFFu;

enum class BigMapIconKind : std::uint16_t {
    Self = 0,
    PartyMember,
    Login,
    MapChange,
    Weapon,
    Dress,
    Doctor,
    Book,
    Warehouse,
    Pyoguk,
    Munpa,
    Yeonmu,
    Accessory,
    Gwanjol,
    CastleManager,
    Etc,
    QuestExclamation1,
    QuestExclamation2,
    QuestExclamation3,
    Camera,
    Max,
};

struct BigMapIcon {
    std::uint32_t object_id = 0;
    BigMapIconKind kind = BigMapIconKind::Etc;
    std::int32_t object_x = 0;
    std::int32_t object_z = 0;
    std::int32_t target_x = 0;
    std::int32_t target_z = 0;
    std::int32_t quest_mark_kind = 0;
    std::uint32_t color = kBigMapIconColor;
    bool always_tooltip = true;
};

class cBigMapDlg final : public cDialog {
public:
    enum class GameInInitKind : std::uint8_t { Normal, SuryunEnter, EventMapEnter };
    using MapViewFn = std::function<bool(std::uint16_t)>;
    using SurvivalMapFn = std::function<bool(std::uint16_t)>;
    using RefreshMiniMapFn = std::function<void()>;

    cBigMapDlg();
    ~cBigMapDlg() override = default;

    void Linking() noexcept;
    void InitBigMap(std::uint16_t map_num);
    void SetActive(bool val) noexcept override;
    void Process() noexcept;
    void Render() override;

    bool CanActive() const noexcept;
    void SetMapRules(MapViewFn map_view, SurvivalMapFn survival_map) noexcept;
    void SetGameInInitKind(GameInInitKind kind) noexcept { m_initKind = kind; }
    void SetRefreshMiniMapCallback(RefreshMiniMapFn callback) noexcept { m_refreshMiniMap = std::move(callback); }

    void AddHeroIcon(std::uint32_t object_id, std::int32_t x = 0, std::int32_t z = 0);
    void AddIcon(BigMapIconKind kind, std::uint32_t object_id, std::int32_t x = 0, std::int32_t z = 0);
    void AddPartyMemberIcon(std::uint32_t player_id, std::int32_t x = 0, std::int32_t z = 0);
    void RemoveIcon(std::uint32_t object_id) noexcept;
    void SetPartyIconObjectPos(std::uint32_t player_id, std::int32_t x, std::int32_t z) noexcept;
    void SetPartyIconTargetPos(std::uint32_t player_id, std::int32_t x, std::int32_t z) noexcept;
    void ShowQuestMarkIcon(std::uint32_t object_id, std::int32_t kind) noexcept;

    std::uint16_t map_num() const noexcept { return m_mapNum; }
    bool icons_initialized() const noexcept { return m_iconsInitialized; }
    bool map_image_requested() const noexcept { return m_mapImageRequested; }
    std::size_t icon_count() const noexcept { return m_icons.size(); }
    const BigMapIcon* find_icon(std::uint32_t object_id) const noexcept;
    const std::optional<BigMapIcon>& hero_icon() const noexcept { return m_heroIcon; }
    std::uint32_t process_count() const noexcept { return m_processCount; }

private:
    static bool is_event_map_exception(std::uint16_t map_num) noexcept;

    std::unordered_map<std::uint32_t, BigMapIcon> m_icons;
    std::optional<BigMapIcon> m_heroIcon;
    MapViewFn m_mapView;
    SurvivalMapFn m_survivalMap;
    RefreshMiniMapFn m_refreshMiniMap;
    GameInInitKind m_initKind = GameInInitKind::Normal;
    std::uint16_t m_mapNum = 0;
    bool m_iconsInitialized = false;
    bool m_mapImageRequested = false;
    std::uint32_t m_processCount = 0;
};

}  // namespace mxh::ui