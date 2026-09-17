// Uses the existing production parsers unchanged. This is a format gate,
// NOT a DX11 renderer, gameplay, human acceptance or release test.
#include "mxh/compat/stm_static_model.hpp"
#include "mxh/compat/anm_motion.hpp"
#include "mxh/compat/chx_model.hpp"
#include "mxh/compat/hfl_height_field.hpp"
#include "mxh/compat/mh_file_ex.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
namespace fs = std::filesystem;
static void require(bool test, const std::string& message) {
    if (!test) throw std::runtime_error(message);
}
static std::vector<std::uint8_t> load(const fs::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    require(bool(stream), "cannot open " + path.string());
    auto size = stream.tellg();
    require(size > 0 && size < 128 * 1024 * 1024, "invalid input size");
    stream.seekg(0);
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    stream.read(reinterpret_cast<char*>(bytes.data()), size);
    require(bool(stream), "short input read");
    return bytes;
}
static void save(const fs::path& path, const std::vector<std::uint8_t>& bytes) {
    require(!fs::exists(path), "refusing to overwrite decoded output");
    std::ofstream stream(path, std::ios::binary);
    stream.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    require(bool(stream), "output write failed");
}
static fs::path dependency(const fs::path& root, const std::string& name) {
    require(!name.empty() && name.find('\\') == std::string::npos &&
        name.find(':') == std::string::npos, "unsafe dependency spelling");
    fs::path relative(name);
    require(!relative.is_absolute(), "absolute dependency");
    for (const auto& part : relative)
        require(part != ".." && part != ".", "dependency traversal");
    auto file = root;
    for (const auto& part : relative) {
        file /= part;
        require(!fs::is_symlink(file), "symlink dependency");
    }
    require(fs::is_regular_file(file), "missing dependency: " + name);
    return file;
}
static std::size_t checkMesh(const mxh::compat::StmStaticModel& model,
                             const fs::path& root) {
    require(!model.meshes.empty() && !model.materials.empty(), "empty geometry");
    std::set<std::uint32_t> materials, bones;
    for (const auto& m : model.materials) {
        require(materials.insert(m.index).second, "duplicate material ID");
        require(!m.texture_name.empty(), "sample lacks texture");
        dependency(root, m.texture_name);
    }
    for (const auto& b : model.bones) require(bones.insert(b.index).second, "duplicate bone ID");
    for (const auto& b : model.bones)
        require(b.parent_index == 0xffffffffu || bones.count(b.parent_index), "missing parent bone");
    std::size_t triangles = 0;
    for (const auto& mesh : model.meshes) {
        require(!mesh.positions.empty() && mesh.positions.size() <= 65535, "renderer index budget");
        require(mesh.normals.size() == mesh.positions.size(), "normal coverage");
        require(mesh.texcoords.size() == mesh.positions.size(), "UV coverage");
        for (std::size_t i = 0; i < mesh.positions.size(); ++i) {
            for (float p : mesh.positions[i]) require(std::isfinite(p), "non-finite position");
            for (float u : mesh.texcoords[i]) require(std::isfinite(u), "non-finite UV");
            float length = 0;
            for (float n : mesh.normals[i]) { require(std::isfinite(n), "non-finite normal"); length += n*n; }
            require(std::abs(length - 1) < 0.002f, "non-unit normal");
        }
        for (const auto& group : mesh.face_groups) {
            require(materials.count(group.material_index), "missing material");
            require(!group.indices.empty() && group.indices.size() % 3 == 0, "invalid triangle list");
            triangles += group.indices.size()/3;
            for (auto index : group.indices) require(index < mesh.positions.size(), "index out of range");
            for (std::size_t j = 0; j < group.indices.size(); j += 3) {
                auto a = mesh.positions[group.indices[j]], b = mesh.positions[group.indices[j+1]], c = mesh.positions[group.indices[j+2]];
                float ux=b[0]-a[0], uy=b[1]-a[1], uz=b[2]-a[2], vx=c[0]-a[0], vy=c[1]-a[1], vz=c[2]-a[2];
                float x=uy*vz-uz*vy,y=uz*vx-ux*vz,z=ux*vy-uy*vx;
                require(x*x+y*y+z*z > 1e-12f, "degenerate triangle");
            }
        }
        if (!mesh.physique.empty()) {
            require(mesh.physique.size() == mesh.positions.size(), "incomplete skin");
            for (const auto& weights : mesh.physique) {
                require(!weights.empty() && weights.size() <= 4, "skin influence budget");
                float sum=0;
                for (const auto& w : weights) {
                    require(bones.count(w.bone_index), "missing skin bone");
                    require(std::isfinite(w.weight) && w.weight > 0 && w.weight <= 1, "invalid skin weight");
                    sum += w.weight;
                    for (auto v:w.offset) require(std::isfinite(v), "invalid bind offset");
                }
                require(std::abs(sum-1)<0.0001f, "skin weights not normalized");
            }
        }
    }
    return triangles;
}
int main(int argc, char** argv) {
    try {
        require(argc >= 3, "usage: reart_probe --decode input output | --server-decode input output | --hfl input | --pack root");
        const std::string mode=argv[1];
        if (mode=="--decode" || mode=="--server-decode") {
            require(argc==4,"decode requires output");
            auto decoded= mode=="--decode" ? mxh::compat::read_mh_bin(argv[2]) : mxh::compat::read_server_mh_bin(argv[2],"playdh-current");
            require(decoded.ok(),"production BIN reader rejected input: "+std::to_string(int(decoded.error)));
            save(argv[3], decoded.value.data);
            std::cout << "{\"decoded_bytes\":" << decoded.value.data.size() << "}\n"; return 0;
        }
        if (mode=="--hfl") {
            mxh::compat::HflHeightField h; std::string error;
            require(mxh::compat::parse_hfl(load(argv[2]),h,&error),error);
            auto mm=std::minmax_element(h.heights.begin(),h.heights.end());
            require(!h.heights.empty(),"empty height field");
            std::cout<<"{\"height_count_x\":"<<h.desc.height_count_x<<",\"height_count_z\":"<<h.desc.height_count_z<<",\"width\":"<<h.desc.width<<",\"height\":"<<h.desc.height<<",\"min_y\":"<<*mm.first<<",\"max_y\":"<<*mm.second<<",\"texture_count\":"<<h.textures.size()<<",\"tile_count\":"<<h.tiles.size()<<"}\n";
            return 0;
        }
        require(mode=="--pack","unknown mode");
        fs::path root=argv[2];
        std::size_t models=0, meshes=0, triangles=0, skins=0, animations=0, manifests=0;
        for (const auto& entry : fs::recursive_directory_iterator(root / "runtime")) {
            if (!entry.is_regular_file()) continue;
            const auto ext=entry.path().extension().string();
            if (ext==".mod" || ext==".stm") {
                mxh::compat::StmStaticModel model; std::string error; auto bytes=load(entry.path());
                require(ext==".mod" ? mxh::compat::parse_mod(bytes,model,&error) : mxh::compat::parse_stm(bytes,model,&error),entry.path().string()+": "+error);
                triangles+=checkMesh(model,root); ++models; meshes+=model.meshes.size();
                for (const auto& m:model.meshes) if(!m.physique.empty()) ++skins;
                // Truncated header must be rejected by the real reader.
                bytes.resize(4); mxh::compat::StmStaticModel broken;
                require(!(ext==".mod" ? mxh::compat::parse_mod(bytes,broken) : mxh::compat::parse_stm(bytes,broken)),"truncated geometry accepted");
            } else if (ext==".anm") {
                auto bytes=load(entry.path()); std::string error;
                auto anim=mxh::compat::AnmMotion::parse(bytes,&error);
                require(bool(anim),error); require(!anim->objects.empty(),"empty animation");
                for (const auto& object:anim->objects) {
                    require(!object.rotations.empty(),"no rotation tracks");
                    auto q=object.sampleRotation(float(anim->last_frame)/2, {0,0,0,1});
                    float norm=0; for(auto v:q) {require(std::isfinite(v),"invalid animation sample");norm+=v*v;}
                    require(std::abs(norm-1)<0.001f,"non-unit sampled quaternion");
                }
                bytes.resize(159); require(!mxh::compat::AnmMotion::parse(bytes),"truncated animation accepted"); ++animations;
            } else if (ext==".chx") {
                auto chx=mxh::compat::ChxModel::parse(load(entry.path())); require(bool(chx),"CHX rejected");
                for(const auto& p:chx->mod_files) dependency(root,p);
                for(const auto& p:chx->motions) dependency(root,p);
                ++manifests;
            }
        }
        require(models>=1 && animations>=1 && manifests>=1 && skins>=1,"representative categories missing");
        std::cout<<"{\"scope\":\"current C++ parsers; NOT DX11/gameplay acceptance\",\"result\":\"pass\",\"models\":"<<models<<",\"meshes\":"<<meshes<<",\"triangles\":"<<triangles<<",\"skinned_meshes\":"<<skins<<",\"animations\":"<<animations<<",\"chx_manifests\":"<<manifests<<"}\n";
        return 0;
    } catch (const std::exception& e) {std::cerr<<e.what()<<"\n";return 1;}
}
