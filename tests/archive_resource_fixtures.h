#pragma once

// Archive fixtures shared by the archive/resource test suites.

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ship/resource/File.h"
#include "ship/resource/archive/Archive.h"
#include "ship/utils/StrHash64.h"

namespace LusTest {

// In-memory archive that does not require Context.
class TestRamArchive : public Ship::Archive {
  public:
    explicit TestRamArchive(const std::string& path = "ram://test",
                            const std::unordered_map<std::string, std::string>& files = {},
                            const std::string& manifest = R"({"name":"TestArchive","code_version":1})")
        : Ship::Archive(path), mTestFiles(files), mManifestJson(manifest) {
    }

    bool Open() override {
        for (const auto& [path, _] : mTestFiles) {
            IndexFile(path);
        }
        return true;
    }

    bool Close() override {
        return true;
    }

    bool WriteFile(const std::string& path, const std::vector<uint8_t>& data) override {
        std::string content(data.begin(), data.end());
        mTestFiles[path] = content;
        IndexFile(path);
        return true;
    }

    std::shared_ptr<Ship::File> LoadFile(const std::string& filePath) override {
        // Serve manifest.json so Archive::Load() can parse it without Context
        if (filePath == "manifest.json") {
            return MakeFile(mManifestJson);
        }
        auto it = mTestFiles.find(filePath);
        if (it == mTestFiles.end()) {
            return nullptr;
        }
        return MakeFile(it->second);
    }

    // Resolve hash → path locally so we don't need Context::GetCurrent()
    std::shared_ptr<Ship::File> LoadFile(uint64_t hash) override {
        for (const auto& [path, _] : mTestFiles) {
            if (CRC64(path.c_str()) == hash) {
                return LoadFile(path);
            }
        }
        return nullptr;
    }

  private:
    std::unordered_map<std::string, std::string> mTestFiles;
    std::string mManifestJson;

    static std::shared_ptr<Ship::File> MakeFile(const std::string& content) {
        auto file = std::make_shared<Ship::File>();
        file->Buffer = std::make_shared<std::vector<char>>(content.begin(), content.end());
        file->IsLoaded = true;
        return file;
    }
};

// Helper: build and load a TestRamArchive with given files
inline std::shared_ptr<TestRamArchive> LoadedArchive(const std::string& path,
                                                     const std::unordered_map<std::string, std::string>& files) {
    auto archive = std::make_shared<TestRamArchive>(path, files);
    archive->Load();
    return archive;
}

// A real directory on disk. ArchiveManager::Init only marks itself initialized once it has
// found at least one archive in the paths it was given, so a ResourceManager needs one of
// these to get through OnInit.
class TempDirectoryArchive {
  public:
    explicit TempDirectoryArchive(const std::unordered_map<std::string, std::string>& files = {}) {
        static size_t sCounter = 0;
        mPath = std::filesystem::temp_directory_path() / ("lus_resource_manager_test_" + std::to_string(sCounter++));
        std::filesystem::create_directories(mPath);
        WriteTextFile("manifest.json", R"({"name":"TempArchive","code_version":1})");
        for (const auto& [path, content] : files) {
            WriteTextFile(path, content);
        }
    }

    ~TempDirectoryArchive() {
        std::error_code ec;
        std::filesystem::remove_all(mPath, ec);
    }

    const std::filesystem::path& GetPath() const {
        return mPath;
    }

  private:
    std::filesystem::path mPath;

    void WriteTextFile(const std::string& relativePath, const std::string& content) {
        const auto filePath = mPath / relativePath;
        std::filesystem::create_directories(filePath.parent_path());
        std::ofstream out(filePath, std::ios::binary);
        out << content;
    }
};

} // namespace LusTest
