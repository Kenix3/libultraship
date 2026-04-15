#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ship/resource/File.h"
#include "ship/resource/archive/Archive.h"
#include "ship/utils/StrHash64.h"

// ============================================================
// Minimal in-memory Archive for testing Archive's own methods
// ============================================================

namespace {

class TestArchive final : public Ship::Archive {
  public:
    explicit TestArchive(const std::string& path = "ram://arch",
                         const std::unordered_map<std::string, std::string>& files = {},
                         const std::string& manifest = R"({"name":"TestArchive","code_version":1})")
        : Ship::Archive(path), mFiles(files), mManifest(manifest) {
    }

    bool Open() override {
        for (const auto& [path, _] : mFiles) {
            IndexFile(path);
        }
        return true;
    }

    bool Close() override {
        return true;
    }

    bool WriteFile(const std::string&, const std::vector<uint8_t>&) override {
        return false;
    }

    std::shared_ptr<Ship::File> LoadFile(const std::string& filePath) override {
        if (filePath == "manifest.json") {
            return Make(mManifest);
        }
        auto it = mFiles.find(filePath);
        return it != mFiles.end() ? Make(it->second) : nullptr;
    }

    std::shared_ptr<Ship::File> LoadFile(uint64_t hash) override {
        for (const auto& [path, _] : mFiles) {
            if (CRC64(path.c_str()) == hash) {
                return LoadFile(path);
            }
        }
        return nullptr;
    }

  private:
    std::unordered_map<std::string, std::string> mFiles;
    std::string mManifest;

    static std::shared_ptr<Ship::File> Make(const std::string& content) {
        auto f = std::make_shared<Ship::File>();
        f->Buffer = std::make_shared<std::vector<char>>(content.begin(), content.end());
        f->IsLoaded = true;
        return f;
    }
};

} // anonymous namespace

// ============================================================
// Archive::GetPath
// ============================================================

TEST(Archive, GetPathReturnsConstructorArg) {
    TestArchive arch("ram://my-archive");
    EXPECT_EQ(arch.GetPath(), "ram://my-archive");
}

// ============================================================
// Archive::IsLoaded  (before and after Load)
// ============================================================

TEST(Archive, IsLoadedFalseBeforeLoad) {
    TestArchive arch("ram://arch", { { "file.bin", "data" } });
    EXPECT_FALSE(arch.IsLoaded());
}

TEST(Archive, IsLoadedTrueAfterSuccessfulLoad) {
    TestArchive arch("ram://arch", { { "file.bin", "data" } });
    arch.Load();
    EXPECT_TRUE(arch.IsLoaded());
}

TEST(Archive, UnloadSetsNotLoaded) {
    TestArchive arch("ram://arch", {});
    arch.Load();
    ASSERT_TRUE(arch.IsLoaded());
    arch.Unload();
    EXPECT_FALSE(arch.IsLoaded());
}

// ============================================================
// Archive::operator==
// ============================================================

TEST(Archive, EqualToSelfByPath) {
    TestArchive a("ram://same");
    TestArchive b("ram://same");
    EXPECT_TRUE(a == b);
}

TEST(Archive, NotEqualWithDifferentPath) {
    TestArchive a("ram://alpha");
    TestArchive b("ram://beta");
    EXPECT_FALSE(a == b);
}

// ============================================================
// Archive::HasFile  (string and hash overloads)
// ============================================================

TEST(Archive, HasFileReturnsTrueForIndexedFile) {
    TestArchive arch("ram://arch", { { "textures/hero.bin", "pixels" } });
    arch.Load();
    EXPECT_TRUE(arch.HasFile("textures/hero.bin"));
}

TEST(Archive, HasFileReturnsFalseForAbsentFile) {
    TestArchive arch("ram://arch", { { "textures/hero.bin", "pixels" } });
    arch.Load();
    EXPECT_FALSE(arch.HasFile("audio/bgm.bin"));
}

TEST(Archive, HasFileByHashMatchesPath) {
    std::string path = "models/link.bin";
    TestArchive arch("ram://arch", { { path, "data" } });
    arch.Load();

    uint64_t hash = CRC64(path.c_str());
    EXPECT_EQ(arch.HasFile(path), arch.HasFile(hash));
}

TEST(Archive, HasFileReturnsFalseBeforeLoad) {
    TestArchive arch("ram://arch", { { "file.bin", "data" } });
    // Do NOT call Load — files not indexed yet
    EXPECT_FALSE(arch.HasFile("file.bin"));
}

// ============================================================
// Archive::ListFiles
// ============================================================

TEST(Archive, ListFilesNoFilterReturnsAll) {
    TestArchive arch("ram://arch", {
                                       { "a/x.bin", "1" },
                                       { "a/y.bin", "2" },
                                       { "b/z.bin", "3" },
                                   });
    arch.Load();
    auto list = arch.ListFiles();
    ASSERT_NE(list, nullptr);
    EXPECT_EQ(list->size(), 3u);
}

TEST(Archive, ListFilesWithGlobFilter) {
    TestArchive arch("ram://arch", {
                                       { "textures/hero.bin", "a" },
                                       { "textures/villain.bin", "b" },
                                       { "audio/bgm.bin", "c" },
                                   });
    arch.Load();

    auto list = arch.ListFiles("textures/*");
    ASSERT_NE(list, nullptr);
    EXPECT_EQ(list->size(), 2u);
    for (auto& [hash, path] : *list) {
        EXPECT_EQ(path.substr(0, 8), "textures");
    }
}

TEST(Archive, ListFilesEmptyArchiveReturnsEmpty) {
    TestArchive arch("ram://arch", {});
    arch.Load();
    auto list = arch.ListFiles();
    ASSERT_NE(list, nullptr);
    EXPECT_TRUE(list->empty());
}

// ============================================================
// Archive::HasGameVersion / GetManifest
// ============================================================

TEST(Archive, HasGameVersionFalseWhenNoVersionInManifest) {
    // Our default manifest has no "game_version" field
    TestArchive arch("ram://arch", {});
    arch.Load();
    EXPECT_FALSE(arch.HasGameVersion());
}

TEST(Archive, GetManifestNameFromJson) {
    TestArchive arch("ram://arch", {}, R"({"name":"MyGame","code_version":2})");
    arch.Load();
    EXPECT_EQ(std::string(arch.GetManifest().Name), "MyGame");
}

TEST(Archive, GetManifestCodeVersion) {
    TestArchive arch("ram://arch", {}, R"({"name":"X","code_version":7})");
    arch.Load();
    EXPECT_EQ(arch.GetManifest().CodeVersion, 7);
}

// ============================================================
// Archive::IsSigned / IsChecksumValid
// ============================================================

TEST(Archive, IsSignedFalseWhenNoChecksum) {
    TestArchive arch("ram://arch", {});
    arch.Load();
    EXPECT_FALSE(arch.IsSigned());
}

TEST(Archive, IsChecksumValidFalseWhenNotSigned) {
    TestArchive arch("ram://arch", {});
    arch.Load();
    EXPECT_FALSE(arch.IsChecksumValid());
}
