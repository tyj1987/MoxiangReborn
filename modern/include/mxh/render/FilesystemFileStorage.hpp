#pragma once

#include "mxh/render/IFileStorage.hpp"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace mxh::compat { class PackFile; }

namespace mxh::gx {

// Header family of a direct-child .pak, classified by the uint32 at file
// offset 4 (little-endian). Nonzero is the PlayDH 92-byte header; zero is
// the original 96-byte header. The numeric value is the header size and is
// what Initialize prints in the mount-order line.
enum class PakFormatTier : std::uint32_t {
    PlayDh92 = 92,
    Original96 = 96,
};

// One .pak considered for mounting. `path` is the archive.
// `root_index` 0 is the primary root. Each extra_pak_roots entry uses
// 1, 2, ... in list order. Ordering is tier, then root_index, then the
// filename (not the directory). Same-named paks from different roots
// both stay in the list.
struct PakMountCandidate {
    std::filesystem::path path;
    PakFormatTier tier = PakFormatTier::PlayDh92;
    std::uint32_t root_index = 0;
};

// Subdirectory of the primary resource root that holds original 96-byte
// paks (TYJ-120). The primary pak scan is not recursive, so this directory
// is mounted only when it is listed in extra_pak_roots. The mount-order
// log spells that directory with this exact text and a forward slash on
// every platform, for example `Original/Map.pak(96)`. deploy_client.ps1
// must use this same name.
inline constexpr std::string_view kOriginalPakSubdir = "Original";

// Pak tier priority for FilesystemFileStorage. This is the config item:
// the mount loop does not hard-code which family wins.
//
// Loose files are always searched first and are not in this list. They
// cannot be moved behind a pak by editing the vector.
//
// Default, highest priority first: PlayDH 92-byte paks, then original
// 96-byte paks. The production client
// (modern/tools/MoxianClient/main.cpp) constructs FilesystemFileStorage
// with this default and does not pass an override. To mount original
// paks before PlayDH paks, pass a FilesystemFileStorageOptions whose
// pak_tier_order is {PakFormatTier::Original96, PakFormatTier::PlayDh92}.
// That options value is the only change required.
//
// A tier omitted from the vector still mounts, after every listed tier.
// Within one tier, root_index is the next key: the primary root (0),
// then each extra_pak_roots entry in list order. Filenames break that
// tie. Filename order uses the UTF-8 bytes of path.filename().u8string()
// (C++17 returns std::string; C++20 returns std::u8string, which is
// copied into std::string as raw bytes). ASCII A-Z is folded to a-z.
// When that comparison ties, including names that differ only in case
// such as Data.pak and data.pak, those same UTF-8 bytes decide, compared
// as unsigned char. Uppercase ASCII therefore mounts first (0x41 < 0x61),
// so ABC.pak comes before abc.pak. A shorter name that is a
// case-insensitive prefix of a longer one sorts first; raw bytes are
// consulted only when the folded names have equal length.
//
// extra_pak_roots is empty by default. Leave it empty and discovery,
// mount order, and the mount-order log stay byte-identical to a single
// primary root. Loose files are always read from the primary root only.
//
// Each extra root is scanned non-recursively for direct-child .pak files.
// A relative entry resolves against the primary root, so
// kOriginalPakSubdir selects <primary>/Original. Absolute entries are
// used as given. The primary scan does not recurse, so a subdirectory
// is not mounted twice. Paks from every root are classified by header
// and placed into pak_tier_order the same way as paks from the primary
// root. A missing or unreadable extra root logs a warning and is
// skipped; Initialize does not throw for that.
//
// When extra_pak_roots is non-empty, the mount-order log disambiguates
// paks that did not come from the primary root. The label is the pak
// path relative to the primary root when that path does not escape it,
// otherwise `<extra-directory>/<filename>`. Separators are `/`. A path
// component that matches kOriginalPakSubdir ignoring ASCII case is
// spelled with that constant, so the Original subdirectory logs as
// `Original/Map.pak(96)` on every platform. Primary-root paks stay
// filename-only, matching the unset log.
struct FilesystemFileStorageOptions {
    std::vector<PakFormatTier> pak_tier_order{
        PakFormatTier::PlayDh92,
        PakFormatTier::Original96,
    };
    std::vector<std::filesystem::path> extra_pak_roots{};
};

// Read the uint32 at offset 4. nullopt when the file is missing or shorter
// than 8 bytes. Does not open the archive and does not depend on PackFile.
[[nodiscard]] std::optional<PakFormatTier> classify_pak_header(
    const std::filesystem::path& path);

// Filename order used inside a tier. See FilesystemFileStorageOptions.
[[nodiscard]] bool pak_filename_less(std::string_view a, std::string_view b) noexcept;

[[nodiscard]] bool pak_mount_less(const PakMountCandidate& a,
                                  const PakMountCandidate& b,
                                  const std::vector<PakFormatTier>& tier_order);

[[nodiscard]] std::vector<PakMountCandidate> order_pak_mounts(
    std::vector<PakMountCandidate> candidates,
    const std::vector<PakFormatTier>& tier_order);

// Read-only I4DyuchiFileStorage backed by the PlayDH directory and its
// original .pak archives.
// The legacy renderer consumes this COM-style interface; modern ownership
// remains ref-counted so it can be passed directly to IRenderer::Create.
class FilesystemFileStorage final : public I4DyuchiFileStorage {
public:
    explicit FilesystemFileStorage(
        std::filesystem::path root,
        FilesystemFileStorageOptions options = {});

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
    FilesystemFileStorageOptions options_{};
    std::vector<std::unique_ptr<mxh::compat::PackFile>> packs_;
};

}  // namespace mxh::gx
