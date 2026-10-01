// Read-only fixture inspection through the same parsers used by MapServer.
#include "mxh/compat/monster_catalog.hpp"
#include "mxh/server/ai_group_loader.hpp"
#include "mxh/game/skill_list_parser.hpp"
#include <fstream>
#include <iostream>
#include <set>

int main() {
    const std::string root = "modern/data/PlayDH/Resource/";
    std::ifstream input(root + "MonsterList.bin", std::ios::binary);
    const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(input), {}};
    const auto catalog = mxh::compat::MonsterCatalog::parse_bin(bytes);
    if (!catalog) return 1;
    for (const int map : {10, 2}) {
        const auto groups = mxh::server::load_ai_group_list_bin(root + "Server/Monster_" +
            (map < 10 ? "0" : "") + std::to_string(map) + ".bin", "playdh-current");
        if (!groups) return 2;
        std::set<std::uint16_t> seen;
        std::uint32_t object_id = 50000; // Same initial sequence as MapHandler.
        for (const auto& group : groups->groups) {
            for (const auto& spawn : group.spawns) {
                const auto* monster = catalog->find(spawn.monster_kind);
                if (!monster) return 3;
                if (seen.insert(spawn.monster_kind).second) {
                    std::cout << "map=" << map << " firstObjectId=" << object_id
                        << " kind=" << spawn.monster_kind << " level=" << unsigned(monster->level)
                        << " hp=" << monster->life << " defense=" << monster->defense
                        << " attack=" << monster->attack_min << ".." << monster->attack_max
                        << " position=" << spawn.pos_x << ',' << spawn.pos_z << '\n';
                }
                ++object_id;
            }
        }
    }
    const auto skills = mxh::game::load_skill_list(root + "SkillList.bin");
    if (skills.parse_errors || !skills.error_message.empty()) return 4;
    for (const auto& skill : skills.skills) {
        if (skill.SkillIdx != 1) continue;
        const auto simple = mxh::game::to_simple(skill);
        const mxh::game::PlayerCombatStats defaults;
        std::cout << "skill=1 physical=" << simple.phy_attack << " attribute=" << simple.att_attack
            << " mp=" << simple.need_nearyuk << " defaultActorAttack=" << defaults.phy_attack
            << " defaultActorDefense=" << defaults.phy_defence << '\n';
        return 0;
    }
    return 5;
}
