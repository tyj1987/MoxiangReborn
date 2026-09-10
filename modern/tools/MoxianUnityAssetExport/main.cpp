#include "mxh/compat/hfl_height_field.hpp"
#include <windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

static std::string sha256(const std::vector<std::uint8_t>& bytes) {
    BCRYPT_ALG_HANDLE algorithm{};
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
        throw std::runtime_error("SHA256 provider unavailable");
    std::uint8_t hash[32]{};
    const auto result = BCryptHash(algorithm, nullptr, 0,
        const_cast<PUCHAR>(bytes.data()), static_cast<ULONG>(bytes.size()), hash, sizeof(hash));
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (result < 0) throw std::runtime_error("SHA256 failed");
    std::ostringstream out;
    for (const auto byte : hash) out << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    return out.str();
}

static std::string json_string(const std::string& value) {
    std::ostringstream out;
    out << '"';
    for (const unsigned char c : value) {
        if (c == '"' || c == '\\') out << '\\' << c;
        else if (c < 32 || c >= 127) out << "\\u00" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
        else out << c;
    }
    out << '"';
    return out.str();
}

int main(int argc, char** argv) {
    try {
        if (argc != 5) {
            std::cerr << "usage: mxh_unity_asset_export input.hfl output.mxhasset expected-sha256 source-id\n";
            return 2;
        }
        const fs::path input = fs::absolute(argv[1]);
        const fs::path output = fs::absolute(argv[2]);
        if (output.extension() != ".mxhasset" || fs::exists(output))
            throw std::runtime_error("output must be a new .mxhasset path");
        if (fs::file_size(input) > 256 * 1024 * 1024)
            throw std::runtime_error("input exceeds converter limit");
        std::ifstream source(input, std::ios::binary);
        if (!source) throw std::runtime_error("cannot open input");
        std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(source), {}};
        const auto digest = sha256(bytes);
        if (digest != argv[3]) throw std::runtime_error("source SHA256 mismatch");
        mxh::compat::HflHeightField hfl;
        std::string error;
        if (!mxh::compat::parse_hfl(bytes, hfl, &error)) throw std::runtime_error(error);
        const auto& d = hfl.desc;
        if (d.height_count_x < 2 || d.height_count_z < 2 ||
            !std::isfinite(d.width) || !std::isfinite(d.height) || !std::isfinite(d.face_size) ||
            d.width <= 0 || d.height <= 0 || d.face_size <= 0 ||
            !std::all_of(hfl.heights.begin(), hfl.heights.end(), [](float h) { return std::isfinite(h); }))
            throw std::runtime_error("invalid finite terrain geometry");
        const auto binary = output.parent_path() / (output.stem().string() + ".mxhheight");
        if (fs::exists(binary)) throw std::runtime_error("height output already exists");
        fs::create_directories(output.parent_path());
        std::vector<std::uint8_t> heights(hfl.heights.size() * sizeof(float));
        std::memcpy(heights.data(), hfl.heights.data(), heights.size());
        std::ofstream data(binary, std::ios::binary);
        data.write(reinterpret_cast<const char*>(heights.data()), static_cast<std::streamsize>(heights.size()));
        data.close();
        if (!data) throw std::runtime_error("height output write failed");
        std::ofstream json(output);
        json << std::setprecision(9)
            << "{\n\"schemaVersion\":1,\"kind\":\"heightfield\",\"profileId\":\"unity-remaster-v1\","
            << "\"converterVersion\":\"hfl-native-1\",\"releaseReady\":false,\n"
            << "\"sourceId\":" << json_string(argv[4]) << ",\"sourceSha256\":" << json_string(digest) << ",\n"
            << "\"heightFile\":" << json_string(binary.filename().string()) << ",\"heightSha256\":" << json_string(sha256(heights)) << ",\n"
            << "\"heightCountX\":" << d.height_count_x << ",\"heightCountZ\":" << d.height_count_z
            << ",\"width\":" << d.width << ",\"depth\":" << d.height << ",\"faceSize\":" << d.face_size
            << ",\"tileCountX\":" << d.tile_count_x << ",\"tileCountZ\":" << d.tile_count_z
            << ",\"facesPerTile\":" << d.faces_per_tile_axis << ",\n\"textureNames\":[";
        for (std::size_t i = 0; i < hfl.textures.size(); ++i) {
            if (i) json << ',';
            json << json_string(hfl.textures[i].name);
        }
        json << "],\n\"tiles\":[";
        for (std::size_t i = 0; i < hfl.tiles.size(); ++i) { if (i) json << ','; json << hfl.tiles[i]; }
        json << "]\n}\n";
        json.close();
        if (!json) throw std::runtime_error("manifest output write failed");
        std::cout << "MXH_HFL_EXPORT_OK vertices=" << hfl.heights.size()
                  << " source_sha256=" << digest << " release_ready=false\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "MXH_HFL_EXPORT_FAILED " << error.what() << '\n';
        return 1;
    }
}
