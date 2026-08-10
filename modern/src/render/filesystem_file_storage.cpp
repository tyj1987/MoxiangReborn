#include "mxh/render/FilesystemFileStorage.hpp"
#include "mxh/compat/pack_file.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <string>

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

FilesystemFileStorage::FilesystemFileStorage(std::filesystem::path root)
    : root_(std::filesystem::weakly_canonical(std::move(root))) {}

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
    if (rel.is_absolute()) return {};
    const auto candidate = std::filesystem::weakly_canonical(root_ / rel);
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
    std::error_code ec;
    for (const auto& item : std::filesystem::directory_iterator(root_, ec)) {
        if (!item.is_regular_file(ec) || item.path().extension() != ".pak") continue;
        if (auto pack = mxh::compat::PackFile::open(item.path()))
            packs_.push_back(std::move(pack));
    }
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
