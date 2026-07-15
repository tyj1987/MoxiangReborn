// MoxianProtocolDoc - Protocol documentation generator
//
// Parses Protocol.h and generates protocol documentation.
// Supports:
//   - Extracting MP_CATEGORY enum values
//   - Extracting MP_PROTOCOL_* enum values
//   - Generating Markdown documentation
//   - Generating JSON protocol schema
//
// Usage:
//   MoxianProtocolDoc <Protocol.h> [--output <output.md>]
//   MoxianProtocolDoc <Protocol.h> --json [--output <output.json>]

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

// ============================================================================
// Protocol Data Structures
// ============================================================================

struct ProtocolEnum {
    std::string name;
    std::vector<std::pair<std::string, std::string>> values; // name, comment
};

struct ProtocolCategory {
    std::string name;
    int value;
    std::string comment;
    std::vector<ProtocolEnum> protocols;
};

// ============================================================================
// Protocol Parser
// ============================================================================

class ProtocolParser {
public:
    ProtocolParser() = default;
    ~ProtocolParser() = default;

    // Parse Protocol.h file
    bool parse(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file: " << path << std::endl;
            return false;
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());

        // Parse MP_CATEGORY enum
        parseCategoryEnum(content);

        // Parse MP_PROTOCOL_* enums
        parseProtocolEnums(content);

        return true;
    }

    // Get parsed categories
    const std::vector<ProtocolCategory>& getCategories() const {
        return categories_;
    }

    // Generate Markdown documentation
    std::string generateMarkdown() const {
        std::ostringstream ss;

        ss << "# Moxian Protocol Documentation\n\n";
        ss << "> Auto-generated from Protocol.h\n\n";
        ss << "## Table of Contents\n\n";

        // Generate TOC
        for (const auto& cat : categories_) {
            ss << "- [" << cat.name << "](#" << cat.name << ") - " << cat.comment << "\n";
        }
        ss << "\n";

        // Generate detailed sections
        for (const auto& cat : categories_) {
            ss << "## " << cat.name << "\n\n";
            ss << "**Value:** " << cat.value << "\n\n";

            if (!cat.comment.empty()) {
                ss << "**Description:** " << cat.comment << "\n\n";
            }

            // Find matching protocol enum
            for (const auto& proto : protocols_) {
                if (proto.name.find(cat.name.substr(3)) != std::string::npos) {
                    ss << "### Protocols\n\n";
                    ss << "| Protocol | Value | Description |\n";
                    ss << "|----------|-------|-------------|\n";

                    int value = 0;
                    for (const auto& val : proto.values) {
                        ss << "| " << val.first << " | " << value << " | " << val.second << " |\n";
                        value++;
                    }
                    ss << "\n";
                    break;
                }
            }
        }

        return ss.str();
    }

    // Generate JSON schema
    std::string generateJSON() const {
        std::ostringstream ss;

        ss << "{\n";
        ss << "  \"version\": \"1.0\",\n";
        ss << "  \"categories\": [\n";

        for (size_t i = 0; i < categories_.size(); ++i) {
            const auto& cat = categories_[i];
            ss << "    {\n";
            ss << "      \"name\": \"" << cat.name << "\",\n";
            ss << "      \"value\": " << cat.value << ",\n";
            ss << "      \"comment\": \"" << escapeJSON(cat.comment) << "\",\n";
            ss << "      \"protocols\": [\n";

            // Find matching protocol enum
            for (const auto& proto : protocols_) {
                if (proto.name.find(cat.name.substr(3)) != std::string::npos) {
                    int value = 0;
                    for (size_t j = 0; j < proto.values.size(); ++j) {
                        const auto& val = proto.values[j];
                        ss << "        {\n";
                        ss << "          \"name\": \"" << val.first << "\",\n";
                        ss << "          \"value\": " << value << ",\n";
                        ss << "          \"comment\": \"" << escapeJSON(val.second) << "\"\n";
                        ss << "        }";
                        if (j < proto.values.size() - 1) ss << ",";
                        ss << "\n";
                        value++;
                    }
                    break;
                }
            }

            ss << "      ]\n";
            ss << "    }";
            if (i < categories_.size() - 1) ss << ",";
            ss << "\n";
        }

        ss << "  ]\n";
        ss << "}\n";

        return ss.str();
    }

    // Print summary
    void printSummary() const {
        std::cout << "Protocol Summary:" << std::endl;
        std::cout << "  Categories: " << categories_.size() << std::endl;
        std::cout << "  Protocol Enums: " << protocols_.size() << std::endl;

        size_t totalProtocols = 0;
        for (const auto& proto : protocols_) {
            totalProtocols += proto.values.size();
        }
        std::cout << "  Total Protocols: " << totalProtocols << std::endl;
    }

private:
    // Parse MP_CATEGORY enum
    void parseCategoryEnum(const std::string& content) {
        std::regex categoryRegex(R"(enum\s+MP_CATEGORY\s*\{([^}]+)\})");
        std::smatch match;

        if (std::regex_search(content, match, categoryRegex)) {
            std::string enumBody = match[1];
            std::regex valueRegex(R"((\w+)\s*(?:=\s*(\d+))?\s*(?://\s*(.*))?)");

            int currentValue = 1;
            auto begin = std::sregex_iterator(enumBody.begin(), enumBody.end(), valueRegex);
            auto end = std::sregex_iterator();

            for (auto it = begin; it != end; ++it) {
                std::smatch valueMatch = *it;
                ProtocolCategory cat;
                cat.name = valueMatch[1];

                if (valueMatch[2].matched) {
                    currentValue = std::stoi(valueMatch[2]);
                }

                cat.value = currentValue;
                cat.comment = valueMatch[3].matched ? valueMatch[3].str() : "";

                categories_.push_back(cat);
                currentValue++;
            }
        }
    }

    // Parse MP_PROTOCOL_* enums
    void parseProtocolEnums(const std::string& content) {
        std::regex protocolRegex(R"(enum\s+(MP_PROTOCOL_\w+)\s*\{([^}]+)\})");
        auto begin = std::sregex_iterator(content.begin(), content.end(), protocolRegex);
        auto end = std::sregex_iterator();

        for (auto it = begin; it != end; ++it) {
            std::smatch match = *it;
            ProtocolEnum proto;
            proto.name = match[1];

            std::string enumBody = match[2];
            std::regex valueRegex(R"((\w+)\s*(?://\s*(.*))?)");

            auto valueBegin = std::sregex_iterator(enumBody.begin(), enumBody.end(), valueRegex);
            auto valueEnd = std::sregex_iterator();

            for (auto vit = valueBegin; vit != valueEnd; ++vit) {
                std::smatch valueMatch = *vit;
                std::string name = valueMatch[1];
                std::string comment = valueMatch[2].matched ? valueMatch[2].str() : "";

                // Skip empty matches
                if (name.empty() || name == "enum" || name == "MP_PROTOCOL_") continue;

                proto.values.push_back({name, comment});
            }

            protocols_.push_back(proto);
        }
    }

    // Escape string for JSON
    std::string escapeJSON(const std::string& str) const {
        std::string result;
        for (char c : str) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c;
            }
        }
        return result;
    }

    std::vector<ProtocolCategory> categories_;
    std::vector<ProtocolEnum> protocols_;
};

// ============================================================================
// Command Line Interface
// ============================================================================

void printUsage() {
    std::cout << "MoxianProtocolDoc - Protocol documentation generator" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  MoxianProtocolDoc <Protocol.h> [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --output <file>   Output file (default: stdout)" << std::endl;
    std::cout << "  --json            Generate JSON format" << std::endl;
    std::cout << "  --summary         Print summary only" << std::endl;
    std::cout << "  --help, -h        Show this help" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  MoxianProtocolDoc Protocol.h --output protocol.md" << std::endl;
    std::cout << "  MoxianProtocolDoc Protocol.h --json --output protocol.json" << std::endl;
    std::cout << "  MoxianProtocolDoc Protocol.h --summary" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string inputFile;
    std::string outputFile;
    bool jsonFormat = false;
    bool summaryOnly = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else if (arg == "--json") {
            jsonFormat = true;
        } else if (arg == "--summary") {
            summaryOnly = true;
        } else if (arg == "--output" && i + 1 < argc) {
            outputFile = argv[++i];
        } else if (inputFile.empty()) {
            inputFile = arg;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage();
            return 1;
        }
    }

    if (inputFile.empty()) {
        std::cerr << "Error: No input file specified" << std::endl;
        printUsage();
        return 1;
    }

    // Parse protocol file
    ProtocolParser parser;
    if (!parser.parse(inputFile)) {
        return 1;
    }

    // Print summary if requested
    if (summaryOnly) {
        parser.printSummary();
        return 0;
    }

    // Generate output
    std::string output;
    if (jsonFormat) {
        output = parser.generateJSON();
    } else {
        output = parser.generateMarkdown();
    }

    // Write output
    if (!outputFile.empty()) {
        std::ofstream file(outputFile);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot create output file: " << outputFile << std::endl;
            return 1;
        }
        file << output;
        std::cout << "Documentation generated: " << outputFile << std::endl;
    } else {
        std::cout << output;
    }

    return 0;
}