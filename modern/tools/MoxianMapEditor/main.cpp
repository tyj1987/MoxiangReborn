// MoxianMapEditor - Modern map editor for Moxian game
//
// Replaces legacy 4DyuchiGXMapEditor with a modern C++ application.
// Supports:
//   - Loading and viewing .bmhm map files
//   - Editing tile properties
//   - Placing and managing map objects
//   - Exporting to legacy format for compatibility
//
// Usage:
//   MoxianMapEditor <map.bmhm>
//   MoxianMapEditor --new <width> <height> <output.bmhm>
//   MoxianMapEditor --export <input.bmhm> <output.png>

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <memory>
#include <cstdint>
#include <cstring>

namespace fs = std::filesystem;

// ============================================================================
// BMHM file format structures (compatible with 4DyuchiGXMapEditor)
// ============================================================================

#pragma pack(push, 1)

struct BMHMHeader {
    char magic[4];           // "BMHM"
    uint32_t version;        // Version (1)
    uint32_t width;          // Map width in tiles
    uint32_t height;         // Map height in tiles
    uint32_t tileSize;       // Tile size in pixels (32)
    uint32_t numLayers;      // Number of layers
    uint32_t numObjects;     // Number of objects
    uint32_t reserved[8];    // Reserved for future use
};

struct BMTile {
    uint16_t textureId;      // Texture index
    uint8_t  tileX;          // Tile X in texture atlas
    uint8_t  tileY;          // Tile Y in texture atlas
    uint8_t  flags;          // Tile flags (walkable, etc.)
    uint8_t  height;         // Height level (0-255)
    uint16_t reserved;       // Reserved
};

struct BMObject {
    uint32_t id;             // Object ID
    uint32_t objectId;       // Object type ID
    float x, y, z;           // Position
    float rotX, rotY, rotZ;  // Rotation
    float scaleX, scaleY, scaleZ; // Scale
    uint32_t flags;          // Object flags
    char name[32];           // Object name
};

#pragma pack(pop)

// ============================================================================
// Map Editor Core
// ============================================================================

class MoxianMapEditor {
public:
    MoxianMapEditor() = default;
    ~MoxianMapEditor() = default;

    // Load a BMHM file
    bool load(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file: " << path << std::endl;
            return false;
        }

        // Read header
        BMHMHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));

        if (std::memcmp(header.magic, "BMHM", 4) != 0) {
            std::cerr << "Error: Invalid BMHM file format" << std::endl;
            return false;
        }

        width_ = header.width;
        height_ = header.height;
        tileSize_ = header.tileSize;

        // Read tiles
        size_t numTiles = width_ * height_;
        tiles_.resize(numTiles);
        file.read(reinterpret_cast<char*>(tiles_.data()), numTiles * sizeof(BMTile));

        // Read objects
        objects_.resize(header.numObjects);
        file.read(reinterpret_cast<char*>(objects_.data()), header.numObjects * sizeof(BMObject));

        path_ = path;
        modified_ = false;

        std::cout << "Loaded map: " << path << std::endl;
        std::cout << "  Size: " << width_ << "x" << height_ << " tiles" << std::endl;
        std::cout << "  Objects: " << objects_.size() << std::endl;

        return true;
    }

    // Save to BMHM file
    bool save(const std::string& path = "") {
        std::string savePath = path.empty() ? path_ : path;
        if (savePath.empty()) {
            std::cerr << "Error: No file path specified" << std::endl;
            return false;
        }

        std::ofstream file(savePath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot create file: " << savePath << std::endl;
            return false;
        }

        // Write header
        BMHMHeader header;
        std::memcpy(header.magic, "BMHM", 4);
        header.version = 1;
        header.width = width_;
        header.height = height_;
        header.tileSize = tileSize_;
        header.numLayers = 1;
        header.numObjects = static_cast<uint32_t>(objects_.size());
        std::memset(header.reserved, 0, sizeof(header.reserved));

        file.write(reinterpret_cast<const char*>(&header), sizeof(header));

        // Write tiles
        file.write(reinterpret_cast<const char*>(tiles_.data()), tiles_.size() * sizeof(BMTile));

        // Write objects
        if (!objects_.empty()) {
            file.write(reinterpret_cast<const char*>(objects_.data()), objects_.size() * sizeof(BMObject));
        }

        path_ = savePath;
        modified_ = false;

        std::cout << "Saved map: " << savePath << std::endl;
        return true;
    }

    // Create new empty map
    bool createNew(uint32_t width, uint32_t height, uint32_t tileSize = 32) {
        width_ = width;
        height_ = height;
        tileSize_ = tileSize;

        size_t numTiles = width_ * height_;
        tiles_.resize(numTiles);
        std::memset(tiles_.data(), 0, numTiles * sizeof(BMTile));

        objects_.clear();
        path_.clear();
        modified_ = true;

        std::cout << "Created new map: " << width_ << "x" << height_ << " tiles" << std::endl;
        return true;
    }

    // Get tile at position
    BMTile* getTile(uint32_t x, uint32_t y) {
        if (x >= width_ || y >= height_) return nullptr;
        return &tiles_[y * width_ + x];
    }

    // Set tile at position
    bool setTile(uint32_t x, uint32_t y, const BMTile& tile) {
        if (x >= width_ || y >= height_) return false;
        tiles_[y * width_ + x] = tile;
        modified_ = true;
        return true;
    }

    // Add object
    uint32_t addObject(const BMObject& obj) {
        uint32_t id = static_cast<uint32_t>(objects_.size());
        objects_.push_back(obj);
        objects_.back().id = id;
        modified_ = true;
        return id;
    }

    // Remove object
    bool removeObject(uint32_t id) {
        if (id >= objects_.size()) return false;
        objects_.erase(objects_.begin() + id);
        // Update IDs
        for (uint32_t i = id; i < objects_.size(); ++i) {
            objects_[i].id = i;
        }
        modified_ = true;
        return true;
    }

    // Get object
    BMObject* getObject(uint32_t id) {
        if (id >= objects_.size()) return nullptr;
        return &objects_[id];
    }

    // Print map info
    void printInfo() const {
        std::cout << "Map Information:" << std::endl;
        std::cout << "  Path: " << (path_.empty() ? "(new)" : path_) << std::endl;
        std::cout << "  Size: " << width_ << "x" << height_ << " tiles" << std::endl;
        std::cout << "  Tile Size: " << tileSize_ << " pixels" << std::endl;
        std::cout << "  Tiles: " << tiles_.size() << std::endl;
        std::cout << "  Objects: " << objects_.size() << std::endl;
        std::cout << "  Modified: " << (modified_ ? "Yes" : "No") << std::endl;
    }

    // Export to simple text format
    bool exportToText(const std::string& path) {
        std::ofstream file(path);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot create text file: " << path << std::endl;
            return false;
        }

        file << "# Moxian Map Export" << std::endl;
        file << "width=" << width_ << std::endl;
        file << "height=" << height_ << std::endl;
        file << "tilesize=" << tileSize_ << std::endl;
        file << std::endl;

        file << "[tiles]" << std::endl;
        for (uint32_t y = 0; y < height_; ++y) {
            for (uint32_t x = 0; x < width_; ++x) {
                const BMTile& tile = tiles_[y * width_ + x];
                file << x << "," << y << "," << tile.textureId << "," << tile.flags << "," << tile.height << std::endl;
            }
        }

        file << std::endl;
        file << "[objects]" << std::endl;
        for (const auto& obj : objects_) {
            file << obj.id << "," << obj.objectId << "," << obj.x << "," << obj.y << "," << obj.z << "," << obj.name << std::endl;
        }

        std::cout << "Exported to text: " << path << std::endl;
        return true;
    }

    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    size_t objectCount() const { return objects_.size(); }
    bool isModified() const { return modified_; }

private:
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t tileSize_ = 32;
    std::vector<BMTile> tiles_;
    std::vector<BMObject> objects_;
    std::string path_;
    bool modified_ = false;
};

// ============================================================================
// Command Line Interface
// ============================================================================

void printUsage() {
    std::cout << "MoxianMapEditor - Modern map editor for Moxian game" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  MoxianMapEditor <map.bmhm>              Load and edit map" << std::endl;
    std::cout << "  MoxianMapEditor --new <w> <h> <out.bmhm> Create new map" << std::endl;
    std::cout << "  MoxianMapEditor --info <map.bmhm>        Show map info" << std::endl;
    std::cout << "  MoxianMapEditor --export <in.bmhm> <out.txt> Export to text" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  MoxianMapEditor --new 100 100 new_map.bmhm" << std::endl;
    std::cout << "  MoxianMapEditor --info map.bmhm" << std::endl;
    std::cout << "  MoxianMapEditor --export map.bmhm map.txt" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string arg1 = argv[1];

    if (arg1 == "--help" || arg1 == "-h") {
        printUsage();
        return 0;
    }

    if (arg1 == "--new") {
        if (argc < 5) {
            std::cerr << "Error: --new requires <width> <height> <output.bmhm>" << std::endl;
            return 1;
        }

        uint32_t width = std::stoul(argv[2]);
        uint32_t height = std::stoul(argv[3]);
        std::string output = argv[4];

        MoxianMapEditor editor;
        if (!editor.createNew(width, height)) {
            return 1;
        }
        if (!editor.save(output)) {
            return 1;
        }

        std::cout << "Map created successfully: " << output << std::endl;
        return 0;
    }

    if (arg1 == "--info") {
        if (argc < 3) {
            std::cerr << "Error: --info requires <map.bmhm>" << std::endl;
            return 1;
        }

        MoxianMapEditor editor;
        if (!editor.load(argv[2])) {
            return 1;
        }

        editor.printInfo();
        return 0;
    }

    if (arg1 == "--export") {
        if (argc < 4) {
            std::cerr << "Error: --export requires <input.bmhm> <output.txt>" << std::endl;
            return 1;
        }

        MoxianMapEditor editor;
        if (!editor.load(argv[2])) {
            return 1;
        }

        if (!editor.exportToText(argv[3])) {
            return 1;
        }

        return 0;
    }

    // Default: load map and show info
    MoxianMapEditor editor;
    if (!editor.load(arg1)) {
        return 1;
    }

    editor.printInfo();
    return 0;
}