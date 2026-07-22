#include <gtest/gtest.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "ship/resource/File.h"
#include "ship/resource/archive/Archive.h"
#include "ship/resource/archive/ArchiveManager.h"
#include "ship/utils/StrHash64.h"

// ============================================================
// TestRamArchive — in-memory archive that does not require Context
// (same shape as the one in archive_resource_tests.cpp)
// ============================================================

namespace {

class TestRamArchive final : public Ship::Archive {
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
        {
            // The fake's own storage needs a lock so TSAN findings point at
            // the class under test, not the test double.
            std::lock_guard<std::mutex> lock(mTestFilesMutex);
            mTestFiles[path] = content;
        }
        IndexFile(path);
        return true;
    }

    std::shared_ptr<Ship::File> LoadFile(const std::string& filePath) override {
        if (filePath == "manifest.json") {
            return MakeFile(mManifestJson);
        }
        std::lock_guard<std::mutex> lock(mTestFilesMutex);
        auto it = mTestFiles.find(filePath);
        if (it == mTestFiles.end()) {
            return nullptr;
        }
        return MakeFile(it->second);
    }

    std::shared_ptr<Ship::File> LoadFile(uint64_t hash) override {
        std::string found;
        {
            std::lock_guard<std::mutex> lock(mTestFilesMutex);
            for (const auto& [path, _] : mTestFiles) {
                if (CRC64(path.c_str()) == hash) {
                    found = path;
                    break;
                }
            }
        }
        if (found.empty()) {
            return nullptr;
        }
        return LoadFile(found);
    }

  private:
    std::mutex mTestFilesMutex;
    std::unordered_map<std::string, std::string> mTestFiles;
    std::string mManifestJson;

    static std::shared_ptr<Ship::File> MakeFile(const std::string& content) {
        auto file = std::make_shared<Ship::File>();
        file->Buffer = std::make_shared<std::vector<char>>(content.begin(), content.end());
        file->IsLoaded = true;
        return file;
    }
};

std::shared_ptr<TestRamArchive> LoadedArchive(const std::string& path,
                                              const std::unordered_map<std::string, std::string>& files) {
    auto archive = std::make_shared<TestRamArchive>(path, files);
    archive->Load();
    return archive;
}

} // anonymous namespace

// ============================================================
// Misses must not mutate the file index
// ============================================================

// Lookups of nonexistent paths are hot at runtime (alt-asset probes, texture
// pack sibling probes). A miss must leave the index untouched: the historical
// operator[] lookup inserted a null entry per miss, so a missing path became
// "present" to HasFile and the map churned concurrently with other readers.
TEST(ArchiveManagerConcurrency, LoadFileMissDoesNotPolluteIndex) {
    Ship::ArchiveManager am;
    am.AddArchive(LoadedArchive("ram://a", { { "file/exists", "data" } }));

    EXPECT_EQ(am.LoadFile("file/missing"), nullptr);
    EXPECT_FALSE(am.HasFile("file/missing"));

    EXPECT_EQ(am.LoadFile(CRC64("file/also_missing")), nullptr);
    EXPECT_FALSE(am.HasFile("file/also_missing"));

    EXPECT_NE(am.LoadFile("file/exists"), nullptr);
}

TEST(ArchiveManagerConcurrency, GetArchiveFromFileMissDoesNotPolluteIndex) {
    Ship::ArchiveManager am;
    am.AddArchive(LoadedArchive("ram://a", { { "file/exists", "data" } }));

    EXPECT_EQ(am.GetArchiveFromFile("file/missing"), nullptr);
    EXPECT_FALSE(am.HasFile("file/missing"));

    EXPECT_NE(am.GetArchiveFromFile("file/exists"), nullptr);
}

// ============================================================
// Concurrent lookups vs. index writers
// ============================================================

// Worker threads (texture streaming, async resource loads) call LoadFile /
// HasFile / HashToString while the game can grow the index via WriteFile
// (controller pak saves). Every lookup of a file that exists must succeed;
// under TSAN this test also proves the index accesses are properly
// synchronized.
TEST(ArchiveManagerConcurrency, ParallelLookupsSurviveIndexWrites) {
    Ship::ArchiveManager am;

    std::unordered_map<std::string, std::string> files;
    for (int i = 0; i < 64; i++) {
        files["dir/file" + std::to_string(i)] = "data" + std::to_string(i);
    }
    auto archive = LoadedArchive("ram://a", files);
    am.AddArchive(archive);

    constexpr int kReaders = 4;
    constexpr int kIterations = 2000;
    std::atomic<bool> failed{ false };

    std::vector<std::thread> readers;
    for (int t = 0; t < kReaders; t++) {
        readers.emplace_back([&, t] {
            for (int i = 0; i < kIterations && !failed; i++) {
                const std::string hit = "dir/file" + std::to_string((t * 31 + i) % 64);
                if (am.LoadFile(hit) == nullptr || !am.HasFile(hit)) {
                    failed = true;
                }
                // Unique miss per iteration: the worst case for a lookup that
                // mutates on miss.
                am.LoadFile("missing/" + std::to_string(t) + "/" + std::to_string(i));
                am.HashToString(CRC64(hit.c_str()));
                if (i % 64 == 0) {
                    am.ListFiles("dir/*");
                }
            }
        });
    }

    std::thread writer([&] {
        for (int i = 0; i < kIterations / 4; i++) {
            am.WriteFile(archive, "written/file" + std::to_string(i), { 'x' });
        }
    });

    for (auto& r : readers) {
        r.join();
    }
    writer.join();

    EXPECT_FALSE(failed) << "a file that exists failed to resolve during concurrent index writes";
    EXPECT_TRUE(am.HasFile("written/file0"));
    EXPECT_FALSE(am.HasFile("missing/0/0"));
}
