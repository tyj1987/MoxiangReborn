#include "mxh/render/FilesystemFileStorage.hpp"
#include "mxh/compat/pack_file.hpp"
#include "mxh/log/mlog.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <optional>
#include <string>
#include <utility>

namespace mxh::gx {
namespace {
struct OpenFile {
    explicit OpenFile(const std::filesystem::path& path)
        : stream(path, std::ios::binary) {}
    explicit OpenFile(std::vector<std::uint8_t> packed)
        : bytes(std::move(packed)) {}
    std::ifstream stream;
    std::vector<std::uint8_t> bytes;
    std::size_t position = 0;
};
}

namespace {

unsigned char fold_ascii(unsigned char c) noexcept {
    if (c >= 'A' && c <= 'Z') return static_cast<unsigned char>(c - 'A' + 'a');
    return c;
}

int tier_rank(PakFormatTier tier, const std::vector<PakFormatTier>& order) {
    for (std::size_t i = 0; i < order.size(); ++i) {
        if (order[i] == tier) return static_cast<int>(i);
    }
    // Omitted tiers stay mounted, behind every tier the config listed.
    return static_cast<int>(order.size());
}

// filename().string() uses the ANSI code page on Windows, so a Chinese
// name is not stable across machines and can throw. u8string() is UTF-8.
// C++17 returns std::string; C++20 returns std::u8string. Copy the code
// units into a std::string either way so the sort and the log share one
// byte sequence.
std::string filename_utf8(const std::filesystem::path& path) {
    const auto u8 = path.filename().u8string();
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}

}  // namespace

std::optional<PakFormatTier> classify_pak_header(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return std::nullopt;
    unsigned char buf[8]{};
    in.read(reinterpret_cast<char*>(buf), 8);
    if (in.gcount() != 8) return std::nullopt;
    const std::uint32_t count_at_offset_4 =
        static_cast<std::uint32_t>(buf[4]) |
        (static_cast<std::uint32_t>(buf[5]) << 8) |
        (static_cast<std::uint32_t>(buf[6]) << 16) |
        (static_cast<std::uint32_t>(buf[7]) << 24);
    if (count_at_offset_4 != 0) return PakFormatTier::PlayDh92;
    return PakFormatTier::Original96;
}

bool pak_filename_less(std::string_view a, std::string_view b) noexcept {
    const auto n = std::min(a.size(), b.size());
    for (std::size_t i = 0; i < n; ++i) {
        const auto fa = fold_ascii(static_cast<unsigned char>(a[i]));
        const auto fb = fold_ascii(static_cast<unsigned char>(b[i]));
        if (fa != fb) return fa < fb;
    }
    if (a.size() != b.size()) return a.size() < b.size();
    for (std::size_t i = 0; i < a.size(); ++i) {
        const auto ra = static_cast<unsigned char>(a[i]);
        const auto rb = static_cast<unsigned char>(b[i]);
        if (ra != rb) return ra < rb;
    }
    return false;
}

bool pak_mount_less(const PakMountCandidate& a,
                    const PakMountCandidate& b,
                    const std::vector<PakFormatTier>& tier_order) {
    const int rank_a = tier_rank(a.tier, tier_order);
    const int rank_b = tier_rank(b.tier, tier_order);
    if (rank_a != rank_b) return rank_a < rank_b;
    const auto name_a = filename_utf8(a.path);
    const auto name_b = filename_utf8(b.path);
    return pak_filename_less(name_a, name_b);
}

std::vector<PakMountCandidate> order_pak_mounts(
    std::vector<PakMountCandidate> candidates,
    const std::vector<PakFormatTier>& tier_order) {
    std::sort(candidates.begin(), candidates.end(),
              [&](const PakMountCandidate& a, const PakMountCandidate& b) {
                  return pak_mount_less(a, b, tier_order);
              });
    return candidates;
}

FilesystemFileStorage::FilesystemFileStorage(
    std::filesystem::path root,
    FilesystemFileStorageOptions options)
    : root_(std::filesystem::weakly_canonical(std::move(root))),
      options_(std::move(options)) {}

STDMETHODIMP FilesystemFileStorage::QueryInterface(REFIID riid, void** out) {
    if (!out) return E_POINTER;
    if (riid == IID_IUnknown) {
        *out = static_cast<I4DyuchiFileStorage*>(this);
        AddRef();
        return S_OK;
    }
    *out = nullptr;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) FilesystemFileStorage::AddRef() { return ++ref_count_; }
STDMETHODIMP_(ULONG) FilesystemFileStorage::Release() {
    const ULONG remaining = --ref_count_;
    if (remaining == 0) delete this;
    return remaining;
}

std::filesystem::path FilesystemFileStorage::resolve(const char* relative) const {
    if (!relative || !*relative) return {};
    std::filesystem::path rel(relative);
    std::filesystem::path candidate;
    if (rel.is_absolute()) {
        // Some callers (e.g. cDialogLoader M-R4.1 sprite hook) hand us
        // already-absolute paths like
        // An absolute local path produced by a diagnostic run can be
        // std::filesystem::path::string() on a path built from
        // m_pathRoot / "2D/1.tif" (the join preserves the original '/' in
        // the appended portion).  We canonicalize and only accept the path
        // if it lives under root_; otherwise it would be an escape attempt.
        std::error_code ec;
        candidate = std::filesystem::weakly_canonical(rel, ec);
        if (candidate.empty() || ec) return {};
    } else {
        candidate = std::filesystem::weakly_canonical(root_ / rel);
    }
    auto root_it = root_.begin();
    auto candidate_it = candidate.begin();
    for (; root_it != root_.end(); ++root_it, ++candidate_it) {
        if (candidate_it == candidate.end() || *root_it != *candidate_it) return {};
    }
    return candidate;
}

BOOL __stdcall FilesystemFileStorage::Initialize(
    std::uint32_t, std::uint32_t, std::uint32_t, FILE_ACCESS_METHOD) {
    if (!std::filesystem::is_directory(root_)) return FALSE;
    packs_.clear();
    pack_names_.clear();
    std::error_code ec;
    std::vector<PakMountCandidate> classified;
    std::vector<std::filesystem::path> unclassified;
    for (const auto& item : std::filesystem::directory_iterator(root_, ec)) {
        if (!item.is_regular_file(ec) || item.path().extension() != ".pak") continue;
        if (const auto tier = classify_pak_header(item.path())) {
            classified.push_back(PakMountCandidate{item.path(), *tier});
        } else {
            unclassified.push_back(item.path());
        }
    }
    // Tier sequence comes from options_.pak_tier_order, not from this loop.
    const auto ordered = order_pak_mounts(std::move(classified), options_.pak_tier_order);

    struct MountedPak {
        std::string filename;
        PakFormatTier tier;
    };
    std::vector<MountedPak> mounted;
    const auto try_open = [&](const std::filesystem::path& path,
                              std::optional<PakFormatTier> tier) {
        if (auto pack = mxh::compat::PackFile::open(path)) {
            if (tier) {
                mounted.push_back(MountedPak{filename_utf8(path), *tier});
            }
            packs_.push_back(std::move(pack));
            // Same UTF-8 bytes and order as the "[pak] mount order" line (#9).
            pack_names_.push_back(filename_utf8(path));
        } else {
            const auto pak_path = path.string();
            MLOG_WARN("[pak] skipped '%s': unsupported header / not a pak",
                      pak_path.c_str());
        }
    };
    for (const auto& candidate : ordered) try_open(candidate.path, candidate.tier);
    // Shorter than 8 bytes: offset 4 cannot be read. Do not call
    // PackFile::open (that would follow directory order). Warn and skip.
    std::sort(unclassified.begin(), unclassified.end(),
              [](const std::filesystem::path& a, const std::filesystem::path& b) {
                  return pak_filename_less(filename_utf8(a), filename_utf8(b));
              });
    for (const auto& path : unclassified) {
        const auto name = filename_utf8(path);
        MLOG_WARN("[pak] skipped '%s': header shorter than 8 bytes", name.c_str());
    }

    std::string line = "[pak] mount order:";
    for (std::size_t i = 0; i < mounted.size(); ++i) {
        line.push_back(' ');
        line += std::to_string(i + 1);
        line.push_back('=');
        line += mounted[i].filename;
        line.push_back('(');
        line += std::to_string(static_cast<unsigned>(mounted[i].tier));
        line.push_back(')');
    }
    MLOG_INFO("%s", line.c_str());
    return TRUE;
}
void* __stdcall FilesystemFileStorage::MapPackFile(char*) { return nullptr; }
void __stdcall FilesystemFileStorage::UnmapPackFile(void*) {}
std::uint32_t __stdcall FilesystemFileStorage::GetFileNum(void*) { return 0; }
std::uint32_t __stdcall FilesystemFileStorage::CreateFileInfoList(
    void*, FSFILE_ATOM_INFO**, std::uint32_t) { return 0; }
void __stdcall FilesystemFileStorage::DeleteFileInfoList(void*, FSFILE_ATOM_INFO*) {}
BOOL __stdcall FilesystemFileStorage::IsExistInFileStorage(char* name) {
    const auto path = resolve(name);
    if (!path.empty() && std::filesystem::is_regular_file(path)) return TRUE;
    if (!name || !*name) return FALSE;
    std::string normalized(name);
    std::replace(normalized.begin(), normalized.end(), '/', '\\');
    const auto basename = std::filesystem::path(normalized).filename().string();
    for (const auto& pack : packs_) {
        if (pack->find(normalized) || (basename != normalized && pack->find(basename))) return TRUE;
    }
    return FALSE;
}
BOOL __stdcall FilesystemFileStorage::LockPackFile(void*, std::uint32_t) { return FALSE; }
BOOL __stdcall FilesystemFileStorage::InsertFileToPackFile(void*, char*) { return FALSE; }
BOOL __stdcall FilesystemFileStorage::DeleteFileFromPackFile(char*) { return FALSE; }
BOOL __stdcall FilesystemFileStorage::UnlockPackFile(void*, LOAD_CALLBACK_FUNC) { return FALSE; }
BOOL __stdcall FilesystemFileStorage::ExtractFile(char*) { return FALSE; }
BOOL __stdcall FilesystemFileStorage::ExtractAllFiles() { return FALSE; }
std::uint32_t __stdcall FilesystemFileStorage::ExtractAllFilesFromPackFile(void*) { return 0; }

void* __stdcall FilesystemFileStorage::FSOpenFile(char* name, std::uint32_t) {
    const auto path = resolve(name);
    if (!path.empty() && std::filesystem::is_regular_file(path)) {
        auto* file = new OpenFile(path);
        if (file->stream) return file;
        delete file;
    }
    if (!name || !*name) return nullptr;
    std::string normalized(name);
    std::replace(normalized.begin(), normalized.end(), '/', '\\');
    const std::string basename = std::filesystem::path(normalized).filename().string();
    for (const auto& pack : packs_) {
        auto bytes = pack->read(normalized);
        if (bytes.empty() && basename != normalized) bytes = pack->read(basename);
        if (!bytes.empty()) return new OpenFile(std::move(bytes));
    }
    return nullptr;
}
std::string FilesystemFileStorage::describeSource(const char* name) const {
    const auto path = resolve(name);
    if (!path.empty() && std::filesystem::is_regular_file(path)) return "loose:" + path.string();
    if (!name || !*name) return {};
    std::string normalized(name);
    std::replace(normalized.begin(), normalized.end(), '/', '\\');
    const std::string basename = std::filesystem::path(normalized).filename().string();
    for (std::size_t i = 0; i < packs_.size(); ++i) {
        if (packs_[i]->find(normalized) || (basename != normalized && packs_[i]->find(basename)))
            return "pak:" + (i < pack_names_.size() ? pack_names_[i] : std::string("?"));
    }
    return {};
}
int __stdcall FilesystemFileStorage::FSScanf(void*, char*, ...) { return 0; }
std::uint32_t __stdcall FilesystemFileStorage::FSRead(
    void* handle, void* output, std::uint32_t bytes) {
    if (!handle || !output || bytes == 0) return 0;
    auto& file = *static_cast<OpenFile*>(handle);
    if (!file.bytes.empty()) {
        const auto available = file.bytes.size() - std::min(file.position, file.bytes.size());
        const auto count = std::min<std::size_t>(bytes, available);
        std::memcpy(output, file.bytes.data() + file.position, count);
        file.position += count;
        return static_cast<std::uint32_t>(count);
    }
    auto& stream = file.stream;
    stream.read(static_cast<char*>(output), static_cast<std::streamsize>(bytes));
    return static_cast<std::uint32_t>(stream.gcount());
}
std::uint32_t __stdcall FilesystemFileStorage::FSSeek(
    void* handle, std::uint32_t offset, FSFILE_SEEK base) {
    if (!handle) return 0;
    auto& file = *static_cast<OpenFile*>(handle);
    if (!file.bytes.empty()) {
        std::size_t basePosition = 0;
        if (base == FSFILE_SEEK_CUR) basePosition = file.position;
        else if (base == FSFILE_SEEK_END) basePosition = file.bytes.size();
        file.position = std::min(file.bytes.size(), basePosition + offset);
        return static_cast<std::uint32_t>(file.position);
    }
    auto& stream = file.stream;
    std::ios_base::seekdir direction = std::ios::beg;
    if (base == FSFILE_SEEK_CUR) direction = std::ios::cur;
    else if (base == FSFILE_SEEK_END) direction = std::ios::end;
    stream.clear();
    stream.seekg(static_cast<std::streamoff>(offset), direction);
    const auto position = stream.tellg();
    return position < 0 ? 0u : static_cast<std::uint32_t>(position);
}
BOOL __stdcall FilesystemFileStorage::FSCloseFile(void* handle) {
    if (!handle) return FALSE;
    delete static_cast<OpenFile*>(handle);
    return TRUE;
}
BOOL __stdcall FilesystemFileStorage::GetPackFileInfo(void*, FSPACK_FILE_INFO*) { return FALSE; }
BOOL __stdcall FilesystemFileStorage::BeginLogging(char*, std::uint32_t) { return FALSE; }
BOOL __stdcall FilesystemFileStorage::EndLogging() { return FALSE; }

}  // namespace mxh::gx
