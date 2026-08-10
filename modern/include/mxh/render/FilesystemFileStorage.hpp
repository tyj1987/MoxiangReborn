#pragma once

#include "mxh/render/IFileStorage.hpp"

#include <atomic>
#include <filesystem>
#include <memory>
#include <vector>

namespace mxh::compat { class PackFile; }

namespace mxh::gx {

// Read-only I4DyuchiFileStorage backed by the PlayDH directory and its
// original .pak archives.
// The legacy renderer consumes this COM-style interface; modern ownership
// remains ref-counted so it can be passed directly to IRenderer::Create.
class FilesystemFileStorage final : public I4DyuchiFileStorage {
public:
    explicit FilesystemFileStorage(std::filesystem::path root);

    const std::filesystem::path& root() const noexcept { return root_; }

    STDMETHODIMP QueryInterface(REFIID, void**) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    BOOL __stdcall Initialize(std::uint32_t, std::uint32_t, std::uint32_t,
                              FILE_ACCESS_METHOD) override;
    void* __stdcall MapPackFile(char*) override;
    void __stdcall UnmapPackFile(void*) override;
    std::uint32_t __stdcall GetFileNum(void*) override;
    std::uint32_t __stdcall CreateFileInfoList(void*, FSFILE_ATOM_INFO**,
                                               std::uint32_t) override;
    void __stdcall DeleteFileInfoList(void*, FSFILE_ATOM_INFO*) override;
    BOOL __stdcall IsExistInFileStorage(char*) override;
    BOOL __stdcall LockPackFile(void*, std::uint32_t) override;
    BOOL __stdcall InsertFileToPackFile(void*, char*) override;
    BOOL __stdcall DeleteFileFromPackFile(char*) override;
    BOOL __stdcall UnlockPackFile(void*, LOAD_CALLBACK_FUNC) override;
    BOOL __stdcall ExtractFile(char*) override;
    BOOL __stdcall ExtractAllFiles() override;
    std::uint32_t __stdcall ExtractAllFilesFromPackFile(void*) override;
    void* __stdcall FSOpenFile(char*, std::uint32_t) override;
    int __stdcall FSScanf(void*, char*, ...) override;
    std::uint32_t __stdcall FSRead(void*, void*, std::uint32_t) override;
    std::uint32_t __stdcall FSSeek(void*, std::uint32_t, FSFILE_SEEK) override;
    BOOL __stdcall FSCloseFile(void*) override;
    BOOL __stdcall GetPackFileInfo(void*, FSPACK_FILE_INFO*) override;
    BOOL __stdcall BeginLogging(char*, std::uint32_t) override;
    BOOL __stdcall EndLogging() override;

private:
    ~FilesystemFileStorage() = default;
    std::filesystem::path resolve(const char* relative) const;

    std::atomic<ULONG> ref_count_{1};
    std::filesystem::path root_;
    std::vector<std::unique_ptr<mxh::compat::PackFile>> packs_;
};

}  // namespace mxh::gx
