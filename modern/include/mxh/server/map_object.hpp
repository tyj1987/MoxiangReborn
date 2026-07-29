#pragma once

// map_object.hpp - Phase 6.2 MapObject 1:1 port (subset).
//
// Source-of-truth: legacy [Server]Map/MapObject.h + .cpp.
//
// Small structural port: MAPOBJECT_INFO wire-format struct,
// ATTRIBUTE_VAL<float> template (mirror legacy), and the
// MapObject class that derives from Object and adds the
// map-object-specific life/shield/defense accessors.  The
// CastleGate wiring (SetCastlegateAddMsg / SIEGEWARMGR delete)
// is intentionally deferred to a later commit.

#include <array>
#include <cstdint>
#include <cstring>

#include "mxh/server/object.hpp"

namespace mxh::server {

// ---- Element enum (mirror legacy ATTR_*) ----
enum class Element : std::uint8_t {
    None  = 0,
    Fire  = 1,
    Earth = 2,
    Iron  = 3,
    Water = 4,
    Tree  = 5,
    Max   = 5,
    NoAttr = 6,
    AllAttr = 7,
};

inline constexpr std::size_t ATTR_VAL_COUNT = 6;  // Element::Max + 1

// ---- ATTRIBUTE_VAL<T> (mirror legacy union ATTRIBUTE_VAL) ----
//
// Legacy defines this as a union with both an anonymous struct
// (FireElement/WaterElement/TreeElement/GoldElement/EarthElement/
// NoneElement) and an Element[] array view.  Modern replicates
// the layout as a struct whose size and offsets match the legacy
// union so wire-format compatibility is preserved.
template <typename T>
struct AttributeVal {
    T fire_element   = T{};
    T water_element  = T{};
    T tree_element   = T{};
    T gold_element   = T{};
    T earth_element  = T{};
    T none_element   = T{};

    // Element[]-style array view (legacy uses a union for this).
    // Out-of-range access returns a default-constructed T.
    T& at(std::uint8_t attrib) {
        static T dummy{};
        if (attrib == 0) return dummy;
        switch (attrib) {
            case 1: return fire_element;
            case 2: return earth_element;
            case 3: return gold_element;  // legacy: ATTR_IRON
            case 4: return water_element;
            case 5: return tree_element;
            case 6: return none_element;
            default: return dummy;
        }
    }
    const T& at(std::uint8_t attrib) const {
        return const_cast<AttributeVal*>(this)->at(attrib);
    }

    // Mirror legacy GetElement_Val / SetElement_Val / AddATTRIBUTE_VAL.
    T get_element_val(std::uint8_t attrib) const {
        if (attrib == 0) return T{};
        if (attrib > ATTR_VAL_COUNT) return T{};
        return at(attrib);
    }
    void set_element_val(std::uint8_t attrib, T val) {
        if (attrib == 0) return;
        if (attrib > ATTR_VAL_COUNT) return;
        at(attrib) = val;
    }
    void add(const AttributeVal& other) {
        fire_element  += other.fire_element;
        water_element += other.water_element;
        tree_element  += other.tree_element;
        gold_element  += other.gold_element;
        earth_element += other.earth_element;
        none_element  += other.none_element;
    }
};

using AttributeRegist = AttributeVal<float>;

// ---- MAPOBJECT_INFO (mirror legacy struct MAPOBJECT_INFO) ----
#pragma pack(push, 1)
struct MapObjectInfo {
    std::uint32_t   life        = 0;
    std::uint32_t   max_life    = 0;
    std::uint32_t   shield      = 0;
    std::uint32_t   max_shield  = 0;
    std::uint32_t   phy_defence = 0;
    float           radius      = 0.0f;
    AttributeRegist attr_regist {};
};
#pragma pack(pop)

// ---- MapObject (mirror legacy CMapObject : public CObject) ----
class MapObject : public Object {
public:
    MapObject() = default;
    ~MapObject() override = default;

    MapObject(const MapObject&) = default;
    MapObject& operator=(const MapObject&) = default;

    // ---- Init (mirror CMapObject::InitMapObject) ----
    void init_map_object(const MapObjectInfo* info) {
        if (info != nullptr) {
            map_object_info_ = *info;
        } else {
            map_object_info_ = MapObjectInfo{};
        }
        level_ = 0;
    }

    // ---- Level ----
    void set_level(std::uint32_t level) { level_ = level; }
    std::uint32_t get_level() const { return level_; }

    // ---- Radius (from MapObjectInfo) ----
    float get_radius() const { return map_object_info_.radius; }

    // ---- Life / Shield (override Object's virtual hooks) ----
    std::uint32_t get_life() const { return map_object_info_.life; }
    void set_life(std::uint32_t life) { map_object_info_.life = life; }

    std::uint32_t get_shield() const { return map_object_info_.shield; }
    void set_shield(std::uint32_t shield) { map_object_info_.shield = shield; }

    // ---- Max / defense (mirror DoGetMax* / DoGetPhyDefense) ----
    std::uint32_t do_get_max_life() const { return map_object_info_.max_life; }
    std::uint32_t do_get_max_shield() const { return map_object_info_.max_shield; }
    std::uint32_t do_get_phy_defense() const { return map_object_info_.phy_defence; }

    float do_get_att_defense(std::uint16_t attrib) const {
        return map_object_info_.attr_regist.get_element_val(
            static_cast<std::uint8_t>(attrib));
    }

    // ---- MapObjectInfo accessors ----
    const MapObjectInfo& get_map_object_info() const { return map_object_info_; }
    MapObjectInfo&       mutable_map_object_info() { return map_object_info_; }

protected:
    void do_die(Object* p_attacker) override;

private:
    MapObjectInfo  map_object_info_{};
    std::uint32_t  level_ = 0;
};

}  // namespace mxh::server
