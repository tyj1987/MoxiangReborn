// MoxianPacker - Modern CLI tool for Moxian resource packing
//
// Replaces legacy PackingTool.exe with a modern C++ CLI application.
// Supports:
//   - Packing files into .pak archives
//   - Extracting files from .pak archives
//   - Listing .pak contents
//   - Verifying .pak integrity
//
// Usage:
//   MoxianPacker pack <input_dir> <output.pak>
//   MoxianPacker extract <input.pak> <output_dir>
//   MoxianPacker list <input.pak>
//   MoxianPacker verify <input.pak>

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iomanip>

namespace fs = std::filesystem;

// ============================================================================
// PAK file format structures (compatible with 4DyuchiFileStorage)
// ============================================================================

#pragma pack(push, 1)
struct PakHeader {
    char magic[4];          // "PACK"
    uint32_t version;       // 1
    uint32_t file_count;    // Number of files in archive
    uint32_t index_offset;  // Offset to file index
    uint32_t index_size;    // Size of file index
    uint32_t data_offset;   // Offset to first file data
    uint32_t reserved[2];   // Reserved for future use
};

struct PakEntry {
    char name[256];         // Null-terminated file name
    uint32_t offset;        // Offset from start of archive
    uint32_t size;          // Uncompressed size
    uint32_t compressed_size; // Compressed size (0 if not compressed)
    uint32_t checksum;      // CRC32 checksum
    uint32_t flags;         // Compression flags
};
#pragma pack(pop)

static_assert(sizeof(PakHeader) == 32, "PakHeader must be 32 bytes");
static_assert(sizeof(PakEntry) == 276, "PakEntry must be 276 bytes");

// ============================================================================
// CRC32 implementation
// ============================================================================

uint32_t crc32(const uint8_t* data, size_t length) {
    static uint32_t table[256];
    static bool initialized = false;

    if (!initialized) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t crc = i;
            for (int j = 0; j < 8; ++j) {
                crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
            }
            table[i] = crc;
        }
        initialized = true;
    }

    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFF;
}

// ============================================================================
// Pack command
// ============================================================================

int pack(const fs::path& input_dir, const fs::path& output_file) {
    if (!fs::exists(input_dir) || !fs::is_directory(input_dir)) {
        std::cerr << "Error: Input directory does not exist: " << input_dir << std::endl;
        return 1;
    }

    std::cout << "Packing directory: " << input_dir << std::endl;
    std::cout << "Output file: " << output_file << std::endl;

    // Collect all files
    std::vector<fs::path> files;
    for (const auto& entry : fs::recursive_directory_iterator(input_dir)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path());
        }
    }

    std::cout << "Found " << files.size() << " files" << std::endl;

    // Calculate sizes
    uint32_t data_offset = sizeof(PakHeader);
    uint32_t index_size = static_cast<uint32_t>(files.size() * sizeof(PakEntry));
    uint32_t current_offset = data_offset + index_size;

    // Prepare entries
    std::vector<PakEntry> entries(files.size());
    for (size_t i = 0; i < files.size(); ++i) {
        PakEntry& entry = entries[i];

        // Get relative path
        fs::path relative = fs::relative(files[i], input_dir);
        std::string name = relative.string();

        // Convert backslashes to forward slashes
        for (char& c : name) {
            if (c == '\\') c = '/';
        }

        // Copy name (truncate if too long)
        memset(entry.name, 0, sizeof(entry.name));
        strncpy(entry.name, name.c_str(), sizeof(entry.name) - 1);

        // Read file data
        std::ifstream file(files[i], std::ios::binary | std::ios::ate);
        entry.size = static_cast<uint32_t>(file.tellg());
        entry.compressed_size = 0;  // No compression for now
        entry.offset = current_offset;
        entry.flags = 0;

        // Calculate checksum
        file.seekg(0);
        std::vector<uint8_t> data(entry.size);
        file.read(reinterpret_cast<char*>(data.data()), entry.size);
        entry.checksum = crc32(data.data(), entry.size);

        current_offset += entry.size;

        std::cout << "  " << name << " (" << entry.size << " bytes)" << std::endl;
    }

    // Write archive
    std::ofstream out(output_file, std::ios::binary);

    // Write header
    PakHeader header;
    memcpy(header.magic, "PACK", 4);
    header.version = 1;
    header.file_count = static_cast<uint32_t>(files.size());
    header.index_offset = sizeof(PakHeader);
    header.index_size = index_size;
    header.data_offset = data_offset + index_size;
    memset(header.reserved, 0, sizeof(header.reserved));
    out.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // Write index
    out.write(reinterpret_cast<const char*>(entries.data()), index_size);

    // Write file data
    for (size_t i = 0; i < files.size(); ++i) {
        std::ifstream file(files[i], std::ios::binary);
        std::vector<char> buffer(entries[i].size);
        file.read(buffer.data(), entries[i].size);
        out.write(buffer.data(), entries[i].size);
    }

    out.close();

    std::cout << "Archive created successfully!" << std::endl;
    std::cout << "Total size: " << fs::file_size(output_file) << " bytes" << std::endl;

    return 0;
}

// ============================================================================
// Extract command
// ============================================================================

int extract(const fs::path& input_file, const fs::path& output_dir) {
    if (!fs::exists(input_file)) {
        std::cerr << "Error: Input file does not exist: " << input_file << std::endl;
        return 1;
    }

    std::cout << "Extracting archive: " << input_file << std::endl;
    std::cout << "Output directory: " << output_dir << std::endl;

    // Create output directory
    fs::create_directories(output_dir);

    // Open archive
    std::ifstream in(input_file, std::ios::binary);

    // Read header
    PakHeader header;
    in.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (memcmp(header.magic, "PACK", 4) != 0) {
        std::cerr << "Error: Invalid PAK file" << std::endl;
        return 1;
    }

    std::cout << "Archive contains " << header.file_count << " files" << std::endl;

    // Read index
    std::vector<PakEntry> entries(header.file_count);
    in.seekg(header.index_offset);
    in.read(reinterpret_cast<char*>(entries.data()), header.file_count * sizeof(PakEntry));

    // Extract files
    for (size_t i = 0; i < entries.size(); ++i) {
        const PakEntry& entry = entries[i];
        fs::path output_path = output_dir / entry.name;

        // Create parent directories
        fs::create_directories(output_path.parent_path());

        // Read file data
        in.seekg(entry.offset);
        std::vector<char> data(entry.size);
        in.read(data.data(), entry.size);

        // Write file
        std::ofstream out(output_path, std::ios::binary);
        out.write(data.data(), entry.size);
        out.close();

        // Verify checksum
        uint32_t actual_crc = crc32(reinterpret_cast<const uint8_t*>(data.data()), entry.size);
        if (actual_crc != entry.checksum) {
            std::cerr << "  WARNING: Checksum mismatch for " << entry.name << std::endl;
        }

        std::cout << "  " << entry.name << " (" << entry.size << " bytes)" << std::endl;
    }

    in.close();

    std::cout << "Extraction completed!" << std::endl;

    return 0;
}

// ============================================================================
// List command
// ============================================================================

int list(const fs::path& input_file) {
    if (!fs::exists(input_file)) {
        std::cerr << "Error: Input file does not exist: " << input_file << std::endl;
        return 1;
    }

    // Open archive
    std::ifstream in(input_file, std::ios::binary);

    // Read header
    PakHeader header;
    in.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (memcmp(header.magic, "PACK", 4) != 0) {
        std::cerr << "Error: Invalid PAK file" << std::endl;
        return 1;
    }

    std::cout << "Archive: " << input_file << std::endl;
    std::cout << "Version: " << header.version << std::endl;
    std::cout << "Files: " << header.file_count << std::endl;
    std::cout << std::endl;

    // Read index
    std::vector<PakEntry> entries(header.file_count);
    in.seekg(header.index_offset);
    in.read(reinterpret_cast<char*>(entries.data()), header.file_count * sizeof(PakEntry));

    // Print table
    std::cout << std::left
              << std::setw(50) << "Name"
              << std::setw(15) << "Size"
              << std::setw(15) << "Compressed"
              << std::setw(10) << "CRC32"
              << std::endl;
    std::cout << std::string(90, '-') << std::endl;

    uint64_t total_size = 0;
    for (const auto& entry : entries) {
        std::cout << std::left
                  << std::setw(50) << entry.name
                  << std::setw(15) << entry.size
                  << std::setw(15) << (entry.compressed_size > 0 ? entry.compressed_size : entry.size)
                  << std::setw(10) << std::hex << entry.checksum << std::dec
                  << std::endl;
        total_size += entry.size;
    }

    std::cout << std::string(90, '-') << std::endl;
    std::cout << "Total: " << entries.size() << " files, " << total_size << " bytes" << std::endl;

    in.close();

    return 0;
}

// ============================================================================
// Verify command
// ============================================================================

int verify(const fs::path& input_file) {
    if (!fs::exists(input_file)) {
        std::cerr << "Error: Input file does not exist: " << input_file << std::endl;
        return 1;
    }

    std::cout << "Verifying archive: " << input_file << std::endl;

    // Open archive
    std::ifstream in(input_file, std::ios::binary);

    // Read header
    PakHeader header;
    in.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (memcmp(header.magic, "PACK", 4) != 0) {
        std::cerr << "Error: Invalid PAK file" << std::endl;
        return 1;
    }

    // Read index
    std::vector<PakEntry> entries(header.file_count);
    in.seekg(header.index_offset);
    in.read(reinterpret_cast<char*>(entries.data()), header.file_count * sizeof(PakEntry));

    // Verify each file
    int errors = 0;
    for (size_t i = 0; i < entries.size(); ++i) {
        const PakEntry& entry = entries[i];

        // Read file data
        in.seekg(entry.offset);
        std::vector<char> data(entry.size);
        in.read(data.data(), entry.size);

        // Verify checksum
        uint32_t actual_crc = crc32(reinterpret_cast<const uint8_t*>(data.data()), entry.size);
        if (actual_crc != entry.checksum) {
            std::cerr << "  FAILED: " << entry.name << " (checksum mismatch)" << std::endl;
            ++errors;
        } else {
            std::cout << "  OK: " << entry.name << std::endl;
        }
    }

    in.close();

    if (errors == 0) {
        std::cout << "Verification passed! All " << entries.size() << " files are valid." << std::endl;
    } else {
        std::cerr << "Verification failed! " << errors << " files have errors." << std::endl;
    }

    return errors;
}

// ============================================================================
// Main
// ============================================================================

void print_usage() {
    std::cout << "MoxianPacker - Modern CLI tool for Moxian resource packing" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  MoxianPacker pack <input_dir> <output.pak>" << std::endl;
    std::cout << "  MoxianPacker extract <input.pak> <output_dir>" << std::endl;
    std::cout << "  MoxianPacker list <input.pak>" << std::endl;
    std::cout << "  MoxianPacker verify <input.pak>" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  MoxianPacker pack ./resources/ game.pak" << std::endl;
    std::cout << "  MoxianPacker extract game.pak ./extracted/" << std::endl;
    std::cout << "  MoxianPacker list game.pak" << std::endl;
    std::cout << "  MoxianPacker verify game.pak" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string command = argv[1];

    if (command == "pack" && argc == 4) {
        return pack(argv[2], argv[3]);
    } else if (command == "extract" && argc == 4) {
        return extract(argv[2], argv[3]);
    } else if (command == "list" && argc == 3) {
        return list(argv[2]);
    } else if (command == "verify" && argc == 3) {
        return verify(argv[2]);
    } else {
        print_usage();
        return 1;
    }
}
