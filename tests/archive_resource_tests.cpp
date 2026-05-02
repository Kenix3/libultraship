#include <gtest/gtest.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ship/resource/File.h"
#include "ship/resource/Resource.h"
#include "ship/resource/ResourceManager.h"
#include "ship/resource/archive/Archive.h"
#include "ship/resource/archive/ArchiveManager.h"
#include "ship/resource/type/Blob.h"
#include "ship/utils/StrHash64.h"

// ============================================================
// TestRamArchive — in-memory archive that does not require Context
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

    // Resolve hash → path locally so we don't need Context::GetInstance()
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
std::shared_ptr<TestRamArchive> LoadedArchive(const std::string& path,
                                              const std::unordered_map<std::string, std::string>& files) {
    auto archive = std::make_shared<TestRamArchive>(path, files);
    archive->Load();
    return archive;
}

} // anonymous namespace

// ============================================================
// ArchiveManager — empty state
// ============================================================

TEST(ArchiveManager, FreshManagerIsNotLoaded) {
    Ship::ArchiveManager am;
    EXPECT_FALSE(am.IsLoaded());
}

TEST(ArchiveManager, FreshManagerHasNoArchives) {
    Ship::ArchiveManager am;
    EXPECT_TRUE(am.GetArchives()->empty());
}

TEST(ArchiveManager, FreshManagerHasNoFiles) {
    Ship::ArchiveManager am;
    EXPECT_FALSE(am.HasFile("anything"));
    EXPECT_FALSE(am.HasFile(uint64_t(0)));
}

TEST(ArchiveManager, FreshManagerListFilesReturnsEmpty) {
    Ship::ArchiveManager am;
    EXPECT_TRUE(am.ListFiles()->empty());
}

// ============================================================
// ArchiveManager — AddArchive
// ============================================================

TEST(ArchiveManager, AddLoadedArchiveSucceeds) {
    auto archive = LoadedArchive("ram://a", { { "file.dat", "hello" } });
    ASSERT_TRUE(archive->IsLoaded());

    Ship::ArchiveManager am;
    auto result = am.AddArchive(archive);
    EXPECT_NE(result, nullptr);
    EXPECT_TRUE(am.IsLoaded());
}

TEST(ArchiveManager, AddUnloadedArchiveReturnsNull) {
    auto archive = std::make_shared<TestRamArchive>("ram://unloaded", std::unordered_map<std::string, std::string>{});
    // Do NOT call archive->Load() — IsLoaded() == false

    Ship::ArchiveManager am;
    auto result = am.AddArchive(archive);
    EXPECT_EQ(result, nullptr);
    EXPECT_FALSE(am.IsLoaded());
}

TEST(ArchiveManager, AddTwoArchivesCountsCorrectly) {
    auto a1 = LoadedArchive("ram://a1", { { "f1.dat", "x" } });
    auto a2 = LoadedArchive("ram://a2", { { "f2.dat", "y" } });

    Ship::ArchiveManager am;
    am.AddArchive(a1);
    am.AddArchive(a2);

    EXPECT_EQ(am.GetArchives()->size(), 2u);
}

// ============================================================
// ArchiveManager — HasFile / LoadFile
// ============================================================

TEST(ArchiveManager, HasFileReturnsTrueForIndexedFile) {
    auto archive = LoadedArchive("ram://test", { { "textures/link.bin", "data" } });
    Ship::ArchiveManager am;
    am.AddArchive(archive);

    EXPECT_TRUE(am.HasFile("textures/link.bin"));
}

TEST(ArchiveManager, HasFileReturnsFalseForAbsentFile) {
    auto archive = LoadedArchive("ram://test", { { "a.bin", "data" } });
    Ship::ArchiveManager am;
    am.AddArchive(archive);

    EXPECT_FALSE(am.HasFile("b.bin"));
}

TEST(ArchiveManager, HasFileByHashMatchesHasFileByPath) {
    std::string path = "textures/sprite.bin";
    auto archive = LoadedArchive("ram://test", { { path, "pixels" } });
    Ship::ArchiveManager am;
    am.AddArchive(archive);

    EXPECT_EQ(am.HasFile(path), am.HasFile(CRC64(path.c_str())));
}

TEST(ArchiveManager, LoadFileByPathReturnsContent) {
    auto archive = LoadedArchive("ram://test", { { "audio/bgm.bin", "music" } });
    Ship::ArchiveManager am;
    am.AddArchive(archive);

    auto file = am.LoadFile("audio/bgm.bin");
    ASSERT_NE(file, nullptr);
    EXPECT_TRUE(file->IsLoaded);
    std::string content(file->Buffer->begin(), file->Buffer->end());
    EXPECT_EQ(content, "music");
}

TEST(ArchiveManager, LoadFileMissingReturnsNull) {
    auto archive = LoadedArchive("ram://test", {});
    Ship::ArchiveManager am;
    am.AddArchive(archive);

    EXPECT_EQ(am.LoadFile("doesnotexist.bin"), nullptr);
}

TEST(ArchiveManager, LoadFileEmptyPathReturnsNull) {
    Ship::ArchiveManager am;
    EXPECT_EQ(am.LoadFile(""), nullptr);
}

// ============================================================
// ArchiveManager — ListFiles / ListDirectories
// ============================================================

TEST(ArchiveManager, ListFilesAllReturnsAll) {
    auto archive = LoadedArchive("ram://test", {
                                                   { "audio/sfx.bin", "a" },
                                                   { "textures/link.bin", "b" },
                                                   { "textures/ganon.bin", "c" },
                                               });
    Ship::ArchiveManager am;
    am.AddArchive(archive);

    auto files = am.ListFiles();
    EXPECT_EQ(files->size(), 3u);
}

TEST(ArchiveManager, ListFilesWithGlobMask) {
    auto archive = LoadedArchive("ram://test", {
                                                   { "textures/hero.bin", "a" },
                                                   { "textures/villain.bin", "b" },
                                                   { "audio/theme.bin", "c" },
                                               });
    Ship::ArchiveManager am;
    am.AddArchive(archive);

    auto files = am.ListFiles("textures/*");
    EXPECT_EQ(files->size(), 2u);
    for (const auto& path : *files) {
        EXPECT_EQ(path.substr(0, 8), "textures");
    }
}

TEST(ArchiveManager, ListFilesWithIncludeExclude) {
    auto archive = LoadedArchive("ram://test", {
                                                   { "textures/hero.bin", "a" },
                                                   { "textures/villain.bin", "b" },
                                                   { "audio/theme.bin", "c" },
                                               });
    Ship::ArchiveManager am;
    am.AddArchive(archive);

    // Include textures, but exclude villain
    auto files = am.ListFiles({ "textures/*" }, { "*villain*" });
    EXPECT_EQ(files->size(), 1u);
    EXPECT_EQ(files->at(0), "textures/hero.bin");
}

TEST(ArchiveManager, ListDirectoriesFindsDirectories) {
    auto archive = LoadedArchive("ram://test", {
                                                   { "textures/hero.bin", "a" },
                                                   { "audio/theme.bin", "b" },
                                               });
    Ship::ArchiveManager am;
    am.AddArchive(archive);

    // Use "*" to match all directories
    auto dirs = am.ListDirectories("*");
    EXPECT_GE(dirs->size(), 2u);
}

// ============================================================
// ArchiveManager — HashToString / HashToCString
// ============================================================

TEST(ArchiveManager, HashToStringReturnsCorrectPath) {
    std::string path = "models/link.bin";
    auto archive = LoadedArchive("ram://test", { { path, "data" } });
    Ship::ArchiveManager am;
    am.AddArchive(archive);

    const std::string* result = am.HashToString(CRC64(path.c_str()));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, path);
}

TEST(ArchiveManager, HashToStringReturnsNullForUnknownHash) {
    Ship::ArchiveManager am;
    EXPECT_EQ(am.HashToString(0xDEADBEEFDEADBEEFULL), nullptr);
}

TEST(ArchiveManager, HashToCStringReturnsCorrectPath) {
    std::string path = "scripts/init.lua";
    auto archive = LoadedArchive("ram://test", { { path, "data" } });
    Ship::ArchiveManager am;
    am.AddArchive(archive);

    const char* result = am.HashToCString(CRC64(path.c_str()));
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, path.c_str());
}

TEST(ArchiveManager, HashToCStringReturnsNullForUnknownHash) {
    Ship::ArchiveManager am;
    EXPECT_EQ(am.HashToCString(0xDEADBEEFDEADBEEFULL), nullptr);
}

// ============================================================
// ArchiveManager — GetArchiveFromFile
// ============================================================

TEST(ArchiveManager, GetArchiveFromFileReturnsCorrectArchive) {
    auto archive = LoadedArchive("ram://test", { { "heroes/link.bin", "data" } });
    Ship::ArchiveManager am;
    am.AddArchive(archive);

    auto result = am.GetArchiveFromFile("heroes/link.bin");
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->GetPath(), "ram://test");
}

// ============================================================
// ArchiveManager — RemoveArchive
// ============================================================

TEST(ArchiveManager, RemoveArchiveByPointerRemovesIt) {
    auto archive = LoadedArchive("ram://test", { { "file.bin", "data" } });
    Ship::ArchiveManager am;
    am.AddArchive(archive);
    ASSERT_TRUE(am.IsLoaded());

    size_t removed = am.RemoveArchive(archive);
    EXPECT_EQ(removed, 1u);
    EXPECT_FALSE(am.IsLoaded());
}

TEST(ArchiveManager, RemoveArchiveByPathRemovesIt) {
    auto archive = LoadedArchive("ram://test", { { "file.bin", "data" } });
    Ship::ArchiveManager am;
    am.AddArchive(archive);

    size_t removed = am.RemoveArchive("ram://test");
    EXPECT_EQ(removed, 1u);
    EXPECT_FALSE(am.IsLoaded());
}

TEST(ArchiveManager, RemoveNonExistentArchiveReturnsZero) {
    Ship::ArchiveManager am;
    EXPECT_EQ(am.RemoveArchive("ram://doesnotexist"), 0u);
}

TEST(ArchiveManager, FilesUnavailableAfterArchiveRemoved) {
    auto archive = LoadedArchive("ram://test", { { "important.bin", "payload" } });
    Ship::ArchiveManager am;
    am.AddArchive(archive);
    ASSERT_TRUE(am.HasFile("important.bin"));

    am.RemoveArchive(archive);
    EXPECT_FALSE(am.HasFile("important.bin"));
}

// ============================================================
// ArchiveManager — GameVersion validation
// ============================================================

TEST(ArchiveManager, IsGameVersionValidWithEmptySetAlwaysTrue) {
    // Empty valid set means all versions are accepted
    Ship::ArchiveManager am;
    EXPECT_TRUE(am.IsGameVersionValid(0x00000000));
    EXPECT_TRUE(am.IsGameVersionValid(0xFFFFFFFF));
    EXPECT_TRUE(am.IsGameVersionValid(0x12345678));
}

TEST(ArchiveManager, IsGameVersionValidWithNonEmptySet) {
    // Simulate a manager with a specific valid-version set via Init
    Ship::ArchiveManager am;
    am.Init({}, { 0x00000001 });
    EXPECT_TRUE(am.IsGameVersionValid(0x00000001));
    EXPECT_FALSE(am.IsGameVersionValid(0x00000002));
}

TEST(ArchiveManager, GetGameVersionsEmptyOnStart) {
    Ship::ArchiveManager am;
    EXPECT_TRUE(am.GetGameVersions().empty());
}

#ifdef ENABLE_SCRIPTING
// ============================================================
// ArchiveManager — UntrustedArchiveHandler
// ============================================================

TEST(ArchiveManager, SetAndGetUntrustedArchiveHandler) {
    Ship::ArchiveManager am;
    EXPECT_EQ(am.GetUntrustedArchiveHandler(), nullptr);

    bool handlerCalled = false;
    am.SetUntrustedArchiveHandler([&](Ship::Archive&, Ship::KeystoreEntry&) {
        handlerCalled = true;
        return true;
    });

    EXPECT_NE(am.GetUntrustedArchiveHandler(), nullptr);
}
#endif

// ============================================================
// ArchiveManager — WriteFile
// ============================================================

TEST(ArchiveManager, WriteFileIndexesNewFile) {
    auto archive = LoadedArchive("ram://test", {});
    Ship::ArchiveManager am;
    am.AddArchive(archive);
    ASSERT_FALSE(am.HasFile("newfile.bin"));

    std::vector<uint8_t> data = { 0x01, 0x02, 0x03 };
    bool ok = am.WriteFile(archive, "newfile.bin", data);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(am.HasFile("newfile.bin"));
}

TEST(ArchiveManager, WriteFileOnNullArchiveReturnsFalse) {
    Ship::ArchiveManager am;
    bool ok = am.WriteFile(nullptr, "file.bin", {});
    EXPECT_FALSE(ok);
}

// ============================================================
// ResourceIdentifier — equality and hash
// ============================================================

TEST(ResourceIdentifier, EqualIdentifiersCompareEqual) {
    Ship::ResourceIdentifier a("path/to/resource", 0, nullptr);
    Ship::ResourceIdentifier b("path/to/resource", 0, nullptr);
    EXPECT_EQ(a, b);
}

TEST(ResourceIdentifier, DifferentPathsNotEqual) {
    Ship::ResourceIdentifier a("path/a", 0, nullptr);
    Ship::ResourceIdentifier b("path/b", 0, nullptr);
    EXPECT_NE(a, b);
}

TEST(ResourceIdentifier, DifferentOwnersNotEqual) {
    Ship::ResourceIdentifier a("path", 0, nullptr);
    Ship::ResourceIdentifier b("path", 1, nullptr);
    EXPECT_NE(a, b);
}

TEST(ResourceIdentifier, HasherGivesSameValueForEqualIdentifiers) {
    Ship::ResourceIdentifier a("path/to/resource", 0, nullptr);
    Ship::ResourceIdentifier b("path/to/resource", 0, nullptr);
    Ship::ResourceIdentifierHash hasher;
    EXPECT_EQ(hasher(a), hasher(b));
}

TEST(ResourceIdentifier, HasherGivesDifferentValuesForDifferentPaths) {
    Ship::ResourceIdentifier a("path/a", 0, nullptr);
    Ship::ResourceIdentifier b("path/b", 0, nullptr);
    Ship::ResourceIdentifierHash hasher;
    EXPECT_NE(hasher(a), hasher(b));
}

// ============================================================
// ResourceManager — Init and IsLoaded
// ============================================================

TEST(ResourceManager, NotLoadedWithNoArchives) {
    Ship::ResourceManager rm;
    rm.Init({}, {});
    EXPECT_FALSE(rm.IsLoaded());
}

TEST(ResourceManager, GetArchiveManagerNonNullAfterInit) {
    Ship::ResourceManager rm;
    rm.Init({}, {});
    EXPECT_NE(rm.GetArchiveManager(), nullptr);
}

TEST(ResourceManager, GetResourceLoaderNonNullAfterInit) {
    Ship::ResourceManager rm;
    rm.Init({}, {});
    EXPECT_NE(rm.GetResourceLoader(), nullptr);
}

TEST(ResourceManager, IsLoadedAfterAddingValidArchive) {
    Ship::ResourceManager rm;
    rm.Init({}, {});

    auto archive = LoadedArchive("ram://test", { { "data/hero.bin", "payload" } });
    auto added = rm.GetArchiveManager()->AddArchive(archive);
    ASSERT_NE(added, nullptr);

    EXPECT_TRUE(rm.IsLoaded());
}

// ============================================================
// ResourceManager — LoadFileProcess
// ============================================================

TEST(ResourceManager, LoadFileProcessReturnsFileForKnownPath) {
    Ship::ResourceManager rm;
    rm.Init({}, {});

    auto archive = LoadedArchive("ram://test", { { "audio/bgm.bin", "musicdata" } });
    rm.GetArchiveManager()->AddArchive(archive);

    auto file = rm.LoadFileProcess("audio/bgm.bin");
    ASSERT_NE(file, nullptr);
    EXPECT_TRUE(file->IsLoaded);
    std::string content(file->Buffer->begin(), file->Buffer->end());
    EXPECT_EQ(content, "musicdata");
}

TEST(ResourceManager, LoadFileProcessReturnsNullForMissingFile) {
    Ship::ResourceManager rm;
    rm.Init({}, {});

    auto archive = LoadedArchive("ram://test", {});
    rm.GetArchiveManager()->AddArchive(archive);

    auto file = rm.LoadFileProcess("doesnotexist.bin");
    EXPECT_EQ(file, nullptr);
}

TEST(ResourceManager, LoadFileProcessViaIdentifier) {
    Ship::ResourceManager rm;
    rm.Init({}, {});

    auto archive = LoadedArchive("ram://test", { { "sprites/link.bin", "spritedata" } });
    rm.GetArchiveManager()->AddArchive(archive);

    Ship::ResourceIdentifier id("sprites/link.bin", 0, nullptr);
    auto file = rm.LoadFileProcess(id);
    ASSERT_NE(file, nullptr);
    std::string content(file->Buffer->begin(), file->Buffer->end());
    EXPECT_EQ(content, "spritedata");
}

TEST(ResourceManager, LoadFileProcessViaIdentifierWithParentArchive) {
    Ship::ResourceManager rm;
    rm.Init({}, {});

    auto archive = LoadedArchive("ram://test", { { "map/world.bin", "mapdata" } });
    rm.GetArchiveManager()->AddArchive(archive);

    // Identifier with the archive as Parent should load directly from that archive
    Ship::ResourceIdentifier id("map/world.bin", 0, archive);
    auto file = rm.LoadFileProcess(id);
    ASSERT_NE(file, nullptr);
    std::string content(file->Buffer->begin(), file->Buffer->end());
    EXPECT_EQ(content, "mapdata");
}

// ============================================================
// ResourceManager — AltAssets
// ============================================================

TEST(ResourceManager, AltAssetsDisabledByDefault) {
    Ship::ResourceManager rm;
    rm.Init({}, {});
    EXPECT_FALSE(rm.IsAltAssetsEnabled());
}

TEST(ResourceManager, SetAltAssetsEnabledToggles) {
    Ship::ResourceManager rm;
    rm.Init({}, {});

    rm.SetAltAssetsEnabled(true);
    EXPECT_TRUE(rm.IsAltAssetsEnabled());

    rm.SetAltAssetsEnabled(false);
    EXPECT_FALSE(rm.IsAltAssetsEnabled());
}

// ============================================================
// ResourceManager — GetCachedResource
// ============================================================

TEST(ResourceManager, GetCachedResourceReturnNullForUncachedPath) {
    Ship::ResourceManager rm;
    rm.Init({}, {});
    EXPECT_EQ(rm.GetCachedResource("not/cached"), nullptr);
}

TEST(ResourceManager, GetCachedResourceByIdentifierReturnNullForUncached) {
    Ship::ResourceManager rm;
    rm.Init({}, {});

    Ship::ResourceIdentifier id("not/cached", 0, nullptr);
    EXPECT_EQ(rm.GetCachedResource(id), nullptr);
}

// ============================================================
// ResourceManager — OtrSignatureCheck
// ============================================================

TEST(ResourceManager, OtrSignatureCheckDetectsPrefix) {
    Ship::ResourceManager rm;
    rm.Init({}, {});

    EXPECT_TRUE(rm.OtrSignatureCheck("__OTR__path/to/file"));
    EXPECT_TRUE(rm.OtrSignatureCheck("__OTR__"));
    EXPECT_FALSE(rm.OtrSignatureCheck("path/to/file"));
    EXPECT_FALSE(rm.OtrSignatureCheck(""));
    EXPECT_FALSE(rm.OtrSignatureCheck("__OTR")); // prefix too short
    EXPECT_FALSE(rm.OtrSignatureCheck("_OTR__path"));
}

// ============================================================
// ResourceManager — GetResourceSize(shared_ptr)
// ============================================================

TEST(ResourceManager, GetResourceSizeNullReturnsZero) {
    Ship::ResourceManager rm;
    rm.Init({}, {});
    // Use explicit typed nullptr to avoid ambiguity with the const char* overload
    std::shared_ptr<Ship::IResource> nullRes;
    EXPECT_EQ(rm.GetResourceSize(nullRes), 0u);
}

TEST(ResourceManager, GetResourceSizeBlobWithData) {
    Ship::ResourceManager rm;
    rm.Init({}, {});

    auto blob = std::make_shared<Ship::Blob>();
    blob->Data = { 1, 2, 3, 4, 5 };
    EXPECT_EQ(rm.GetResourceSize(blob), 5u);
}

TEST(ResourceManager, GetResourceSizeBlobEmpty) {
    Ship::ResourceManager rm;
    rm.Init({}, {});

    auto blob = std::make_shared<Ship::Blob>();
    EXPECT_EQ(rm.GetResourceSize(blob), 0u);
}

// ============================================================
// ResourceManager — GetResourceIsCustom(shared_ptr)
// ============================================================

TEST(ResourceManager, GetResourceIsCustomNullReturnsFalse) {
    Ship::ResourceManager rm;
    rm.Init({}, {});
    std::shared_ptr<Ship::IResource> nullRes;
    EXPECT_FALSE(rm.GetResourceIsCustom(nullRes));
}

TEST(ResourceManager, GetResourceIsCustomFalseWhenNotCustom) {
    Ship::ResourceManager rm;
    rm.Init({}, {});

    auto initData = std::make_shared<Ship::ResourceInitData>();
    initData->IsCustom = false;
    auto blob = std::make_shared<Ship::Blob>(initData);
    EXPECT_FALSE(rm.GetResourceIsCustom(blob));
}

TEST(ResourceManager, GetResourceIsCustomTrueWhenCustom) {
    Ship::ResourceManager rm;
    rm.Init({}, {});

    auto initData = std::make_shared<Ship::ResourceInitData>();
    initData->IsCustom = true;
    auto blob = std::make_shared<Ship::Blob>(initData);
    EXPECT_TRUE(rm.GetResourceIsCustom(blob));
}

// ============================================================
// ResourceManager — GetResourceRawPointer(shared_ptr)
// ============================================================

TEST(ResourceManager, GetResourceRawPointerNullReturnsNull) {
    Ship::ResourceManager rm;
    rm.Init({}, {});
    std::shared_ptr<Ship::IResource> nullRes;
    EXPECT_EQ(rm.GetResourceRawPointer(nullRes), nullptr);
}

TEST(ResourceManager, GetResourceRawPointerBlobReturnsDataPtr) {
    Ship::ResourceManager rm;
    rm.Init({}, {});

    auto blob = std::make_shared<Ship::Blob>();
    blob->Data = { 0xAB, 0xCD };
    void* ptr = rm.GetResourceRawPointer(blob);
    EXPECT_EQ(ptr, blob->Data.data());
}

// ============================================================
// ResourceManager — WriteResource
// ============================================================

TEST(ResourceManager, WriteResourceWithNoArchiveReturnsFalse) {
    Ship::ResourceManager rm;
    rm.Init({}, {});

    // No archive registered, no Parent in identifier
    Ship::ResourceIdentifier id("file.bin", 0, nullptr);
    std::vector<uint8_t> data = { 1, 2, 3 };
    EXPECT_FALSE(rm.WriteResource(id, data, false));
}

TEST(ResourceManager, WriteResourceWithParentArchiveSucceeds) {
    Ship::ResourceManager rm;
    rm.Init({}, {});

    auto archive = LoadedArchive("ram://test", { { "existing.bin", "old" } });
    rm.GetArchiveManager()->AddArchive(archive);

    Ship::ResourceIdentifier id("new.bin", 0, archive);
    std::vector<uint8_t> data = { 0xCA, 0xFE };
    EXPECT_TRUE(rm.WriteResource(id, data, false));
    // The file should now exist in the archive manager
    EXPECT_TRUE(rm.GetArchiveManager()->HasFile("new.bin"));
}

// ============================================================
// ResourceManager — UnloadResource
// ============================================================

TEST(ResourceManager, UnloadResourceNotCachedIsNoOp) {
    Ship::ResourceManager rm;
    rm.Init({}, {});
    // Unloading a path that was never cached should not crash
    EXPECT_NO_THROW(rm.UnloadResource("path/that/was/never/loaded"));
}

TEST(ResourceManager, UnloadResourceByIdentifierIsNoOp) {
    Ship::ResourceManager rm;
    rm.Init({}, {});
    Ship::ResourceIdentifier id("never/loaded", 0, nullptr);
    EXPECT_NO_THROW(rm.UnloadResource(id));
}

// ============================================================
// ResourceFilter — construction
// ============================================================

TEST(ResourceFilter, ConstructionStoresAllFields) {
    auto archive = LoadedArchive("ram://test", {});
    Ship::ResourceFilter filter({ "textures/*" }, { "*.tmp" }, 42, archive);

    ASSERT_EQ(filter.IncludeMasks.size(), 1u);
    EXPECT_EQ(filter.IncludeMasks.front(), "textures/*");
    ASSERT_EQ(filter.ExcludeMasks.size(), 1u);
    EXPECT_EQ(filter.ExcludeMasks.front(), "*.tmp");
    EXPECT_EQ(filter.Owner, 42u);
    EXPECT_EQ(filter.Parent, archive);
}

TEST(ResourceFilter, DefaultNullParentAndZeroOwner) {
    Ship::ResourceFilter filter({}, {}, 0, nullptr);
    EXPECT_EQ(filter.Owner, 0u);
    EXPECT_EQ(filter.Parent, nullptr);
}
