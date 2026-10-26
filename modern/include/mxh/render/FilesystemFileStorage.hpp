#pragma once

#include "mxh/render/IFileStorage.hpp"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace mxh::compat { class PackFile; }

namespace mxh::gx {

// Header family of a direct-child .pak. The numeric value is the header
// size and is what Initialize prints in the mount-order line. Which family
// a file belongs to is decided by classify_pak_header, not by the uint32
// at offset 4 alone.
enum class PakFormatTier : std::uint32_t {
    PlayDh92 = 92,
    Original96 = 96,
};

// One .pak considered for mounting. `path` is the archive; ordering uses
// only its filename, not the directory.
struct PakMountCandidate {
    std::filesystem::path path;
    PakFormatTier tier = PakFormatTier::PlayDh92;
};

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
// Within one rank, filenames sort by the UTF-8 bytes of
// path.filename().u8string() (C++17 returns std::string; C++20 returns
// std::u8string, which is copied into std::string as raw bytes). ASCII
// A-Z is folded to a-z. When that comparison ties, including names that
// differ only in case such as Data.pak and data.pak, those same UTF-8
// bytes decide, compared as unsigned char. Uppercase ASCII therefore
// mounts first (0x41 < 0x61), so ABC.pak comes before abc.pak. A shorter
// name that is a case-insensitive prefix of a longer one sorts first;
// raw bytes are consulted only when the folded names have equal length.
struct FilesystemFileStorageOptions {
    std::vector<PakFormatTier> pak_tier_order{
        PakFormatTier::PlayDh92,
        PakFormatTier::Original96,
    };
};

// First 8 bytes of a .pak, or an open failure. `opened == false` means the
// file could not be opened. `error` is the OS reason when the platform
// reported one (empty if it did not). This does not parse the archive and
// does not call PackFile::open.
struct PakHeaderPrefix {
    bool opened = false;
    std::string error;
    unsigned char bytes[8]{};
    std::size_t size = 0;
};

// Test seam. Production passes nullptr and reads with ifstream.
// A substitute may refuse selected paths; other paths should call
// read_pak_header_prefix so the bytes stay the real prefix.
using PakHeaderOpener = PakHeaderPrefix (*)(const std::filesystem::path&);

[[nodiscard]] PakHeaderPrefix read_pak_header_prefix(const std::filesystem::path& path);

// CannotOpen and ShorterThan8 are not tiers. PlayDh92 / Original96 match
// PakFormatTier. Unrecognized is not a tier: the file is not loaded.
// open_error is set only for CannotOpen, and only when a reason is available.
enum class PakHeaderKind : std::uint8_t {
    CannotOpen,
    ShorterThan8,
    PlayDh92,
    Original96,
    Unrecognized,
};

struct PakHeaderClassification {
    PakHeaderKind kind = PakHeaderKind::CannotOpen;
    std::string open_error;
    // Entry count of the one validating walk. 0 when kind is not a tier.
    std::uint32_t entry_count = 0;
    // Both layouts are always attempted once the file is readable and at
    // least 8 bytes. Exactly one true value selects PlayDh92 or Original96.
    // Both true, or both false, selects Unrecognized.
    bool playdh92_valid = false;
    bool original96_valid = false;
};

// Try both entry walks via mxh::compat::probe_pak_layouts. PlayDh92 uses
// the uint32 at offset 4 and the 92-byte layout walk. Original96 uses the
// uint32 at offset 92 and the strict 96-byte walk. A count of 0 does not
// validate. Exactly one walk that finishes at EOF selects that kind.
// Neither or both is Unrecognized. `opener == nullptr` uses
// read_pak_header_prefix for the open / short-header checks. Does not
// open the archive via PackFile.
[[nodiscard]] PakHeaderClassification classify_pak_header(
    const std::filesystem::path& path,
    PakHeaderOpener opener = nullptr);

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

    // Test seam. Null restores the ifstream opener. Not a mount option:
    // pak_tier_order and the constructor are unchanged.
    void set_pak_header_opener_for_test(PakHeaderOpener opener) noexcept {
        header_opener_ = opener;
    }

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
    PakHeaderOpener header_opener_ = nullptr;
    std::vector<std::unique_ptr<mxh::compat::PackFile>> packs_;
};

}  // namespace mxh::gx
