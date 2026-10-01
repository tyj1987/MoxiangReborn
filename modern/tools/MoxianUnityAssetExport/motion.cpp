#include "mxh/compat/anm_motion.hpp"
#include "nlohmann/json.hpp"
#include <windows.h>
#include <bcrypt.h>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>

using Json = nlohmann::ordered_json;
namespace fs = std::filesystem;
static std::string sha256(const std::vector<std::uint8_t>& bytes) {
    BCRYPT_ALG_HANDLE algorithm{};
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
        throw std::runtime_error("SHA256 provider unavailable");
    std::uint8_t hash[32]{};
    auto result = BCryptHash(algorithm, nullptr, 0, const_cast<PUCHAR>(bytes.data()), static_cast<ULONG>(bytes.size()), hash, sizeof(hash));
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (result < 0) throw std::runtime_error("SHA256 failed");
    std::ostringstream out;
    for (auto byte : hash) out << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    return out.str();
}
template<class Keys> static Json keys(const Keys& source) {
    Json result = Json::array();
    std::uint32_t previous = 0;
    bool first = true;
    for (const auto& key : source) {
        if (!first && key.frame < previous) throw std::runtime_error("Unordered motion keys");
        for (float value : key.value) if (!std::isfinite(value)) throw std::runtime_error("Non-finite motion key");
        Json value = {{"x", key.value[0]}, {"y", key.value[1]}, {"z", key.value[2]}};
        if constexpr (std::tuple_size_v<decltype(key.value)> == 4) value["w"] = key.value[3];
        result.push_back({{"ticks", key.ticks}, {"frame", key.frame}, {"value", value}});
        previous = key.frame; first = false;
    }
    return result;
}
int main(int argc, char** argv) {
    try {
        if (argc != 5) throw std::runtime_error("usage: mxh_unity_motion_export input.anm output.mxhmotion expected-sha256 source-id");
        fs::path input = fs::absolute(argv[1]), output = fs::absolute(argv[2]);
        if (output.extension() != ".mxhmotion" || fs::exists(output)) throw std::runtime_error("Output must be a new .mxhmotion");
        if (fs::file_size(input) > 64 * 1024 * 1024) throw std::runtime_error("Motion exceeds converter limit");
        std::ifstream file(input, std::ios::binary);
        if (!file) throw std::runtime_error("Cannot open motion");
        std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file), {}};
        auto digest = sha256(bytes);
        if (digest != argv[3]) throw std::runtime_error("Source SHA256 mismatch");
        std::string error;
        auto motion = mxh::compat::AnmMotion::parse(bytes, &error);
        if (!motion) throw std::runtime_error(error);
        Json doc = {{"schemaVersion", 1}, {"kind", "motion"}, {"profileId", "unity-remaster-v1"},
            {"converterVersion", "anm-native-1"}, {"releaseReady", false}, {"sourceId", argv[4]},
            {"sourceSha256", digest}, {"ticksPerFrame", motion->ticks_per_frame}, {"firstFrame", motion->first_frame},
            {"sourceHeaderBytes", std::vector<std::uint8_t>(bytes.begin(), bytes.begin() + 160)},
            {"lastFrame", motion->last_frame}, {"frameSpeed", motion->frame_speed}, {"keyFrameStep", motion->key_frame_step},
            {"objects", Json::array()}};
        std::size_t count = 0;
        std::size_t cursor = 160;
        for (const auto& object : motion->objects) {
            auto u32 = [&](std::size_t offset) { std::uint32_t value; std::memcpy(&value, bytes.data() + offset, 4); return value; };
            const auto objectEnd = cursor + 8 + u32(cursor + 4);
            const auto animatedCount = u32(cursor + 8 + 16);
            auto scales = keys(object.scales);
            auto extra = cursor + 8 + 152 + object.positions.size() * 20 + object.rotations.size() * 24;
            for (std::size_t i = 0; i < object.scales.size(); ++i) {
                std::array<float, 4> orientation{};
                std::memcpy(orientation.data(), bytes.data() + extra + 20, 16);
                for (float value : orientation) if (!std::isfinite(value)) throw std::runtime_error("Non-finite scale orientation");
                scales[i]["axis"] = {{"x", orientation[0]}, {"y", orientation[1]}, {"z", orientation[2]}};
                scales[i]["axisAngle"] = orientation[3];
                extra += 36;
            }
            // The current parser validates but does not interpret mesh animation.
            // Preserve those bytes explicitly so downstream import cannot silently lose them.
            std::vector<std::uint8_t> meshKeys(bytes.begin() + extra, bytes.begin() + objectEnd);
            doc["objects"].push_back({{"index", object.index}, {"name", object.name},
                {"sourceObjectType", u32(cursor)}, {"motionFlags", u32(cursor + 8 + 148)},
                {"positions", keys(object.positions)}, {"rotations", keys(object.rotations)}, {"scales", scales},
                {"meshAnimationKeyCount", animatedCount}, {"meshAnimationBytes", meshKeys}});
            count += object.positions.size() + object.rotations.size() + object.scales.size();
            cursor = objectEnd;
        }
        if (cursor != bytes.size()) throw std::runtime_error("Unexpected trailing motion bytes");
        auto encoded = doc.dump(2);
        fs::create_directories(output.parent_path());
        std::ofstream destination(output, std::ios::binary);
        destination << encoded << '\n'; destination.close();
        if (!destination) throw std::runtime_error("Motion output write failed");
        std::cout << "MXH_ANM_EXPORT_OK tracks=" << motion->objects.size() << " keys=" << count
            << " first=" << motion->first_frame << " last=" << motion->last_frame << " release_ready=false\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "MXH_ANM_EXPORT_FAILED " << error.what() << '\n'; return 1;
    }
}
