// pet_manager.hpp - Phase D5 1:1 port of legacy [Server]Map/PetManager.h
// State machine for player pet ownership (summoned pets, equipment, buffs,
// grade-up probability, skill recharge). All fields mirror legacy
// CPetManager members in CamelCase so byte-level diff against the
// reference exe is possible when paired with the side-by-side harness.

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace mxh::server {

// ---- Constants (1:1 PetManager.h + Pet.h defines) ----

inline constexpr std::uint32_t PET_SKILLCHARGE_CHECKTIME = 1000u;
inline constexpr std::uint32_t PET_MAX_SKILL_CHARGE      = 10000u;
inline constexpr std::uint32_t PET_MAX_LEVEL             = 3u;
inline constexpr std::uint32_t PET_GRADEUP_PROB_1TO2     = 80u;
inline constexpr std::uint32_t PET_GRADEUP_PROB_2TO3     = 80u;
inline constexpr std::uint32_t PET_RESUMMON_VALID_TIME   = 30000u;
inline constexpr std::uint32_t CRISTMAS_EVENTPET         = 8u;
inline constexpr std::uint32_t CRISTMAS_EVENTPET_SUMMONNING_TIME = 60000u * 30u;

// Pet constants mirrored from Pet.h
inline constexpr std::uint32_t PET_DEFAULT_FRIENDLY = 3000000u;
inline constexpr std::uint32_t PET_REVIVAL_FRIENDLY = 2000000u;
inline constexpr std::uint32_t PET_MAX_FRIENDLY     = 10000000u;
inline constexpr std::uint32_t PET_STATE_CHECK_TIME = 10000u;
inline constexpr std::uint32_t PET_MAX_GRADE        = 3u;

// ---- Enumerations ----

enum class PetUpgradeResult : std::uint8_t {
    UpgradeSucess              = 0,
    UpgradeFailforProb         = 1,
    UpgradeFailforEtc          = 2,
    UpgradeFailfor3rdUp        = 3,
    UpgradeFailforSamePetSummoned = 4,
};

enum class PetFeedResult : std::uint8_t {
    Sucess       = 0,
    Unsummoned   = 1,
    StaminaFull  = 2,
};

enum class PetBuffKind : std::uint8_t {
    None               = 0,
    DemagePercent      = 1,
    Dodge              = 2,
    MasterAllStatUp    = 3,
    ItemDoubleChance   = 4,
    NoForeAtkMonster   = 5,
    ReduceCriticalDmg  = 6,
    MasterAllStatRound = 7,
    ItemRareProbUp     = 8,
    MussangTimeIncrease = 9,
    KindMax,
};

enum class PetSummonning : std::uint8_t {
    ReleaseSummon = 0,
    SaveSummon    = 1,
};

enum class PetKind : std::uint16_t {
    None       = 0,
    CommonPet  = 1,
    ShopItemPet = 2,
    EventPet   = 4,
};

enum class PetBasicState : std::uint8_t {
    None   = 0,
    Moving = 1,
    Ungi   = 2,
};

// ---- POD structures ----

// Mirrors legacy BuffData.
struct PetBuffData {
    std::uint32_t Prob = 0;
    std::uint32_t BuffValueData = 0;
    std::uint32_t BuffAdditionalData = 0;
};

// Mirrors legacy PETEQUIP_ITEMOPTION.
struct PetEquipOption {
    int   iPetStaminaReductionDecrease   = 0;  // stamina reduction %
    int   iPetStaminaMaxIncreaseAmount   = 0;  // +max stamina value
    int   iPetStaminaRecoverateIncrease  = 0;  // recovery rate %
    int   iPetStaminaRecoverateAmount    = 0;  // recovery value
    int   iPetFriendshipIncrease         = 0;  // friendship gain %
    float fPetFriendshipProtectionRate   = 0.0f; // friendship loss guard %
};

// Mirrors legacy PET_TOTALINFO (subset we use for state machine tests).
struct PetTotalInfo {
    std::uint32_t PetSummonItemDBIdx = 0;
    std::uint16_t PetKind = 0;
    std::uint16_t PetGrade = 0;
    std::uint32_t PetStamina = 0;
    std::uint32_t PetFriendly = 0;
    std::uint8_t  bAlive = 0;
    std::uint8_t  bRest = 0;
    std::uint8_t  bSummonning = 0;
};

// Mirrors legacy CPetManager state.
struct PetManagerState {
    std::uint32_t m_dwPetObjectID = 1;
    std::uint32_t m_dwSkillRechargeCheckTime = 0;
    std::uint32_t m_dwSkillRechargeAmount = 0;
    bool          m_bSkillGuageFull = false;
    bool          m_bPetStaminaZero = false;
    std::uint32_t m_dwReleaseDelayTime = 0;
    std::uint32_t m_dwResummonDelayTime = 0;
    int           m_BuffFlag = 0;
    std::uint16_t m_wPetKind = 0;
    int           m_iFriendshipReduceAmount = 0;
    std::uint32_t m_dwEventPetSummonRemainTime = 0;
    std::uint32_t m_dwEventPetCheckTime = 0;
    std::uint32_t m_dwPetValidDistPosCheckTime = 0;
    PetEquipOption m_PetEquipOption{};
    std::vector<PetTotalInfo> m_PetInfoList;
    std::optional<std::uint32_t> m_curSummonItemDBIdx;
    std::array<PetBuffData, static_cast<std::size_t>(PetBuffKind::KindMax)> m_BuffData{};
};

// ---- Free functions (mirroring CPetManager methods) ----

inline PetManagerState make_pet_manager() {
    PetManagerState s;
    return s;
}

inline void init_pet_manager(PetManagerState& s) {
    s.m_dwSkillRechargeCheckTime = 0;
    s.m_dwSkillRechargeAmount = 0;
    s.m_bSkillGuageFull = false;
    s.m_bPetStaminaZero = false;
    s.m_dwReleaseDelayTime = 0;
    s.m_dwResummonDelayTime = 0;
    s.m_BuffFlag = static_cast<int>(PetBuffKind::None);
    s.m_wPetKind = 0;
    s.m_iFriendshipReduceAmount = 0;
    s.m_dwEventPetSummonRemainTime = 0;
    s.m_dwEventPetCheckTime = 0;
    s.m_dwPetValidDistPosCheckTime = 0;
    s.m_PetEquipOption = PetEquipOption{};
    s.m_PetInfoList.clear();
    s.m_curSummonItemDBIdx.reset();
}

inline void add_pet_total_info(PetManagerState& s, const PetTotalInfo& info) {
    s.m_PetInfoList.push_back(info);
}

inline void remove_pet_total_info(PetManagerState& s, std::uint32_t summon_item_db_idx) {
    for (auto it = s.m_PetInfoList.begin(); it != s.m_PetInfoList.end(); ++it) {
        if (it->PetSummonItemDBIdx == summon_item_db_idx) {
            s.m_PetInfoList.erase(it);
            if (s.m_curSummonItemDBIdx && *s.m_curSummonItemDBIdx == summon_item_db_idx) {
                s.m_curSummonItemDBIdx.reset();
            }
            return;
        }
    }
}

inline PetTotalInfo* find_pet_total_info(PetManagerState& s, std::uint32_t summon_item_db_idx) {
    for (auto& p : s.m_PetInfoList) {
        if (p.PetSummonItemDBIdx == summon_item_db_idx) return &p;
    }
    return nullptr;
}

// Grade-up probability: returns 0..10000 (basis points). 8000 = 80.00%.
inline std::uint32_t gradeup_probability_basis_points(std::uint16_t current_grade) {
    if (current_grade == 1) return PET_GRADEUP_PROB_1TO2 * 100u;
    if (current_grade == 2) return PET_GRADEUP_PROB_2TO3 * 100u;
    return 0u;
}

// Try upgrading a pet. Probability gate uses basis_points (0..10000). If
// roll < prob, succeeds. Returns PetUpgradeResult.
inline PetUpgradeResult upgrade_pet(PetTotalInfo& pet, std::uint32_t roll_basis_points, bool is_third_upgrade_blocked = false) {
    if (pet.PetGrade >= PET_MAX_GRADE) return PetUpgradeResult::UpgradeFailfor3rdUp;
    if (is_third_upgrade_blocked)      return PetUpgradeResult::UpgradeFailforSamePetSummoned;
    const std::uint32_t prob = gradeup_probability_basis_points(pet.PetGrade);
    if (prob == 0u) return PetUpgradeResult::UpgradeFailforEtc;
    if (roll_basis_points >= prob) return PetUpgradeResult::UpgradeFailforProb;
    pet.PetGrade += 1;
    return PetUpgradeResult::UpgradeSucess;
}

// Friendship delta applied by an event (eFIA). Always clamps to [0, MAX].
inline void add_friendship(PetTotalInfo& pet, int delta) {
    std::int64_t v = static_cast<std::int64_t>(pet.PetFriendly) + delta;
    if (v < 0) v = 0;
    if (v > static_cast<std::int64_t>(PET_MAX_FRIENDLY)) v = PET_MAX_FRIENDLY;
    pet.PetFriendly = static_cast<std::uint32_t>(v);
}

inline bool is_pet_max_friendship(const PetTotalInfo& pet) {
    return pet.PetFriendly >= PET_MAX_FRIENDLY;
}

inline bool is_pet_stamina_zero(const PetTotalInfo& pet) {
    return pet.PetStamina == 0u;
}

inline bool is_pet_above_default_friendly(const PetTotalInfo& pet) {
    return pet.PetFriendly > PET_DEFAULT_FRIENDLY;
}

// Skill recharge tick: increments recharge amount by `amount_per_tick`,
// capped at PET_MAX_SKILL_CHARGE; if cap reached the gauge flips to full.
inline void tick_skill_recharge(PetManagerState& s, std::uint32_t amount_per_tick) {
    s.m_dwSkillRechargeAmount += amount_per_tick;
    if (s.m_dwSkillRechargeAmount >= PET_MAX_SKILL_CHARGE) {
        s.m_dwSkillRechargeAmount = PET_MAX_SKILL_CHARGE;
        s.m_bSkillGuageFull = true;
    }
}

inline void set_skill_ready(PetManagerState& s) {
    s.m_dwSkillRechargeAmount = PET_MAX_SKILL_CHARGE;
    s.m_bSkillGuageFull = true;
}

// Stamina add (e.g. from feed). Clamps to max stamina; marks m_bPetStaminaZero
// if state goes from zero to non-zero.
inline void add_stamina(PetManagerState& s, PetTotalInfo& pet, int delta, std::uint32_t max_stamina) {
    if (delta < 0 && static_cast<std::int64_t>(pet.PetStamina) + delta < 0) {
        pet.PetStamina = 0u;
    } else if (delta > 0 && pet.PetStamina + static_cast<std::uint32_t>(delta) > max_stamina) {
        pet.PetStamina = max_stamina;
    } else {
        pet.PetStamina = static_cast<std::uint32_t>(static_cast<std::int64_t>(pet.PetStamina) + delta);
    }
    if (pet.PetStamina > 0u && s.m_bPetStaminaZero) {
        s.m_bPetStaminaZero = false;
    } else if (pet.PetStamina == 0u && !s.m_bPetStaminaZero) {
        s.m_bPetStaminaZero = true;
    }
}

// Resummon timer: caller can call this once at release; CheckResummonAvailable
// reports true only after PET_RESUMMON_VALID_TIME has elapsed since release.
inline void begin_resummon_cooldown(PetManagerState& s, std::uint32_t now_ms) {
    s.m_dwResummonDelayTime = now_ms;
}

inline bool check_resummon_available(const PetManagerState& s, std::uint32_t now_ms) {
    if (s.m_dwResummonDelayTime == 0u) return true;
    return (now_ms - s.m_dwResummonDelayTime) >= PET_RESUMMON_VALID_TIME;
}

inline void begin_release_delay(PetManagerState& s, std::uint32_t now_ms) {
    s.m_dwReleaseDelayTime = now_ms;
}

inline void clear_release_delay(PetManagerState& s) {
    s.m_dwReleaseDelayTime = 0u;
}

// Summon a pet by item DB index. Returns true if the pet exists and was
// not already summoned. Sets curSummonItemDBIdx.
inline bool summon_pet(PetManagerState& s, std::uint32_t item_db_idx) {
    auto* p = find_pet_total_info(s, item_db_idx);
    if (!p) return false;
    if (s.m_curSummonItemDBIdx && *s.m_curSummonItemDBIdx == item_db_idx) return false;
    s.m_curSummonItemDBIdx = item_db_idx;
    s.m_wPetKind = p->PetKind;
    return true;
}

inline void unsummon_pet(PetManagerState& s) {
    s.m_curSummonItemDBIdx.reset();
    s.m_wPetKind = 0;
}

inline bool has_cur_summon(const PetManagerState& s) {
    return s.m_curSummonItemDBIdx.has_value();
}

// Feed pet: if stamina full returns ePFR_StaminaFull; if no current pet
// returns ePFR_Unsummoned; else adds feed amount to stamina capped at
// max_stamina. Note: legacy allows overflow saturation to zero on negative.
inline PetFeedResult feed_up_pet(PetManagerState& s, std::uint32_t feed_amount, std::uint32_t max_stamina) {
    if (!has_cur_summon(s)) return PetFeedResult::Unsummoned;
    auto* p = find_pet_total_info(s, *s.m_curSummonItemDBIdx);
    if (!p) return PetFeedResult::Unsummoned;
    if (p->PetStamina >= max_stamina) return PetFeedResult::StaminaFull;
    add_stamina(s, *p, static_cast<int>(feed_amount), max_stamina);
    return PetFeedResult::Sucess;
}

// Seal pet: returns to inventory (alive flag flips to false).
inline void seal_pet(PetManagerState& s) {
    if (!has_cur_summon(s)) return;
    auto* p = find_pet_total_info(s, *s.m_curSummonItemDBIdx);
    if (!p) return;
    p->bAlive = 0;
    unsummon_pet(s);
}

// Revival: sets a pet's friendly to PET_REVIVAL_FRIENDLY.
inline void revival_pet(PetTotalInfo& pet) {
    pet.PetFriendly = PET_REVIVAL_FRIENDLY;
    pet.bAlive = 1;
}

// Event pet timer: ticks down by `now_ms` delta, returns true when expired.
inline bool tick_event_pet_remain(PetManagerState& s, std::uint32_t elapsed_ms) {
    if (s.m_dwEventPetSummonRemainTime == 0u) return false;
    if (elapsed_ms >= s.m_dwEventPetSummonRemainTime) {
        s.m_dwEventPetSummonRemainTime = 0u;
        return true;
    }
    s.m_dwEventPetSummonRemainTime -= elapsed_ms;
    return false;
}

inline void start_event_pet(PetManagerState& s) {
    s.m_dwEventPetSummonRemainTime = CRISTMAS_EVENTPET_SUMMONNING_TIME;
    s.m_dwEventPetCheckTime = 0u;
}

// Buff flag helpers: bitmask-style; buff kinds (1..9) are individual bits.
inline void add_pet_buff_flag(PetManagerState& s, PetBuffKind kind) {
    if (kind == PetBuffKind::None || kind == PetBuffKind::KindMax) return;
    s.m_BuffFlag |= (1 << static_cast<int>(kind));
}

inline void remove_pet_buff_flag(PetManagerState& s, PetBuffKind kind) {
    if (kind == PetBuffKind::None || kind == PetBuffKind::KindMax) return;
    s.m_BuffFlag &= ~(1 << static_cast<int>(kind));
}

inline bool has_pet_buff_flag(const PetManagerState& s, PetBuffKind kind) {
    if (kind == PetBuffKind::None || kind == PetBuffKind::KindMax) return false;
    return (s.m_BuffFlag & (1 << static_cast<int>(kind))) != 0;
}

// Friendship guard (SW070531). Reduces by raw amount but capped by
// `fPetFriendshipProtectionRate` of max friendly when protection rate is
// non-zero (i.e. reduced_amount * (1 - protectionRate)).
inline std::int64_t apply_friendship_protection(std::uint32_t raw_loss, float protection_rate) {
    if (protection_rate <= 0.0f) return static_cast<std::int64_t>(raw_loss);
    if (protection_rate >= 1.0f) return 0;
    const double mult = 1.0 - static_cast<double>(protection_rate);
    return static_cast<std::int64_t>(static_cast<double>(raw_loss) * mult);
}

// Add buff data (slot keyed by buff kind).
inline void set_buff_data(PetManagerState& s, PetBuffKind kind, const PetBuffData& data) {
    const auto idx = static_cast<std::size_t>(kind);
    if (idx >= s.m_BuffData.size()) return;
    s.m_BuffData[idx] = data;
}

inline std::optional<PetBuffData> get_buff_data(const PetManagerState& s, PetBuffKind kind) {
    const auto idx = static_cast<std::size_t>(kind);
    if (idx >= s.m_BuffData.size()) return std::nullopt;
    return s.m_BuffData[idx];
}

} // namespace mxh::server
