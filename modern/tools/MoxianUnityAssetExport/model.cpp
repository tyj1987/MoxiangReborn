#include "mxh/compat/stm_static_model.hpp"
#include "nlohmann/json.hpp"
#include <windows.h>
#include <bcrypt.h>
#include <cmath>
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
    auto result = BCryptHash(algorithm, nullptr, 0, const_cast<PUCHAR>(bytes.data()),
        static_cast<ULONG>(bytes.size()), hash, sizeof(hash));
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (result < 0) throw std::runtime_error("SHA256 failed");
    std::ostringstream out;
    for (auto byte : hash) out << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    return out.str();
}

template<std::size_t N> static Json finite(const std::array<float, N>& values) {
    for (float value : values) if (!std::isfinite(value)) throw std::runtime_error("Non-finite model data");
    return values;
}
static Json vector3(const std::array<float, 3>& value) {
    (void)finite(value);
    return {{"x", value[0]}, {"y", value[1]}, {"z", value[2]}};
}

int main(int argc, char** argv) {
    try {
        if (argc != 5) throw std::runtime_error("usage: mxh_unity_model_export input.mod output.mxhmodel expected-sha256 source-id");
        const fs::path input = fs::absolute(argv[1]), output = fs::absolute(argv[2]);
        if (output.extension() != ".mxhmodel" || fs::exists(output))
            throw std::runtime_error("Output must be a new .mxhmodel path");
        if (fs::file_size(input) > 64 * 1024 * 1024) throw std::runtime_error("Model exceeds converter limit");
        std::ifstream file(input, std::ios::binary);
        if (!file) throw std::runtime_error("Cannot open model");
        std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file), {}};
        auto digest = sha256(bytes);
        if (digest != argv[3]) throw std::runtime_error("Source SHA256 mismatch");
        mxh::compat::StmStaticModel model;
        std::string error;
        if (!mxh::compat::parse_mod(bytes, model, &error)) throw std::runtime_error(error);
        if (model.meshes.empty()) throw std::runtime_error("Model contains no meshes");
        Json doc = {{"schemaVersion", 1}, {"kind", "model"}, {"profileId", "unity-remaster-v1"},
            {"converterVersion", "mod-native-1"}, {"releaseReady", false}, {"sourceId", argv[4]},
            {"sourceSha256", digest}, {"coordinateSpace", "legacy-unconverted"},
            {"materials", Json::array()}, {"bones", Json::array()}, {"meshes", Json::array()}};
        for (const auto& material : model.materials) {
            if (!std::isfinite(material.transparency)) throw std::runtime_error("Non-finite transparency");
            doc["materials"].push_back({{"index", material.index}, {"diffuse", material.diffuse},
                {"transparency", material.transparency}, {"textureName", material.texture_name}, {"flags", material.flags}});
        }
        for (const auto& bone : model.bones)
            doc["bones"].push_back({{"index", bone.index}, {"parentIndex", bone.parent_index}, {"name", bone.name},
                {"position", vector3(bone.position)}, {"rotationAxis", vector3(bone.rotation_axis)},
                {"rotationAngle", finite(std::array<float, 1>{bone.rotation_angle})[0]},
                {"scale", vector3(bone.scale)}, {"transform", finite(bone.transform)}});
        std::size_t vertices = 0, influences = 0;
        for (const auto& mesh : model.meshes) {
            Json item = {{"index", mesh.index}, {"name", mesh.name}, {"transform", finite(mesh.transform)},
                {"flags", mesh.mesh_flags}, {"positions", Json::array()}, {"normals", Json::array()},
                {"texcoords", Json::array()}, {"faceGroups", Json::array()}, {"physique", Json::array()}};
            for (const auto& value : mesh.positions) item["positions"].push_back(vector3(value));
            for (const auto& value : mesh.normals) item["normals"].push_back(vector3(value));
            for (const auto& value : mesh.texcoords) {
                (void)finite(value);
                item["texcoords"].push_back({{"x", value[0]}, {"y", value[1]}});
            }
            for (const auto& group : mesh.face_groups) {
                if (group.indices.size() % 3 != 0) throw std::runtime_error("Incomplete triangle");
                for (auto index : group.indices) if (index >= mesh.positions.size()) throw std::runtime_error("Invalid vertex index");
                item["faceGroups"].push_back({{"materialIndex", group.material_index}, {"indices", group.indices}});
            }
            for (const auto& vertex : mesh.physique) {
                Json weights = Json::array();
                for (const auto& influence : vertex) {
                    if (!std::isfinite(influence.weight) || influence.weight < 0) throw std::runtime_error("Invalid skin weight");
                    weights.push_back({{"boneIndex", influence.bone_index}, {"weight", influence.weight},
                        {"offset", vector3(influence.offset)}, {"normalOffset", vector3(influence.normal_offset)}});
                    ++influences;
                }
                item["physique"].push_back({{"influences", weights}});
            }
            vertices += mesh.positions.size();
            doc["meshes"].push_back(std::move(item));
        }
        const auto encoded = doc.dump(2); // Reject invalid UTF-8 before creating output.
        fs::create_directories(output.parent_path());
        std::ofstream destination(output, std::ios::binary);
        destination << encoded << '\n';
        destination.close();
        if (!destination) throw std::runtime_error("Model output write failed");
        std::cout << "MXH_MOD_EXPORT_OK meshes=" << model.meshes.size() << " bones=" << model.bones.size()
            << " vertices=" << vertices << " influences=" << influences << " release_ready=false\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "MXH_MOD_EXPORT_FAILED " << error.what() << '\n';
        return 1;
    }
}
