#include <gtest/gtest.h>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ship/resource/File.h"
#include "ship/resource/Resource.h"
#include "ship/resource/ResourceLoader.h"
#include "ship/resource/ResourceManager.h"
#include "ship/resource/ResourceType.h"
#include "ship/resource/archive/Archive.h"
#include "ship/resource/archive/ArchiveManager.h"
#include "ship/resource/factory/BlobFactory.h"
#include "ship/resource/type/Blob.h"
#include "ship/utils/StrHash64.h"
#include "ship/utils/binarytools/endianness.h"

#include "archive_resource_fixtures.h"

namespace {

const std::string kBase = "textures/base.bin";
const std::string kBaseMeta = kBase + ".meta";
const std::string kAlias = "textures/alias.bin";

constexpr size_t kBlobPadding = 16;

std::string BlobBody(const std::string& payload) {
    const uint32_t size = static_cast<uint32_t>(payload.size());
    std::string body(sizeof(uint32_t), '\0');
    std::memcpy(body.data(), &size, sizeof(uint32_t));
    return body + payload;
}

std::string BareBlob(const std::string& payload) {
    return BlobBody(payload);
}

std::string HeaderedBlob(const std::string& payload) {
    std::string header(OTR_HEADER_SIZE, '\0');
    header[0] = static_cast<char>(Ship::Endianness::Native);
    const uint32_t type = static_cast<uint32_t>(Ship::ResourceType::Blob);
    std::memcpy(header.data() + 4, &type, sizeof(uint32_t));
    return header + BlobBody(payload);
}

std::string MetaJson(const std::string& targetPath, bool isCustom = false) {
    std::string json = R"({"type":"Blob","format":"Binary","version":0)";
    if (!targetPath.empty()) {
        json += R"(,"path":")" + targetPath + R"(")";
    }
    if (isCustom) {
        json += R"(,"isCustom":true)";
    }
    return json + "}";
}

class UnreadableFileArchive : public LusTest::TestRamArchive {
  public:
    UnreadableFileArchive(const std::string& path, const std::unordered_map<std::string, std::string>& files,
                          std::string unreadable)
        : LusTest::TestRamArchive(path, files), mUnreadable(std::move(unreadable)) {
    }

    using LusTest::TestRamArchive::LoadFile;

    std::shared_ptr<Ship::File> LoadFile(const std::string& filePath) override {
        return filePath == mUnreadable ? nullptr : LusTest::TestRamArchive::LoadFile(filePath);
    }

  private:
    std::string mUnreadable;
};

struct MetaAliasHarness {
    MetaAliasHarness()
        : base(), threadPool(std::make_shared<Ship::ThreadPool>(1)),
          manager(std::make_shared<Ship::ResourceManager>(threadPool)) {
        manager->Init({ { "archivePaths", std::vector<std::string>{ base.GetPath().string() } },
                        { "validHashes", std::vector<uint32_t>{} } });
        manager->GetResourceLoader()->RegisterResourceFactory(std::make_shared<Ship::ResourceFactoryBinaryBlobV0>(),
                                                              RESOURCE_FORMAT_BINARY, "Blob",
                                                              static_cast<uint32_t>(Ship::ResourceType::Blob), 0);
    }

    void Mount(const std::string& name, const std::unordered_map<std::string, std::string>& files) {
        manager->GetArchiveManager()->AddArchive(LusTest::LoadedArchive("ram://" + name, files));
    }

    void Mount(const std::shared_ptr<Ship::Archive>& archive) {
        archive->Load();
        manager->GetArchiveManager()->AddArchive(archive);
    }

    std::string Load(const std::string& path) {
        return PayloadOf(manager->LoadResourceProcess(path));
    }

    std::string Load(const Ship::ResourceIdentifier& identifier) {
        return PayloadOf(manager->LoadResourceProcess(identifier));
    }

    static std::string PayloadOf(const std::shared_ptr<Ship::IResource>& resource) {
        auto blob = std::dynamic_pointer_cast<Ship::Blob>(resource);
        if (blob == nullptr) {
            return "<load failed>";
        }
        if (blob->Data.size() < kBlobPadding) {
            return "<truncated>";
        }
        return std::string(blob->Data.begin(), blob->Data.end() - kBlobPadding);
    }

    LusTest::TempDirectoryArchive base;
    std::shared_ptr<Ship::ThreadPool> threadPool;
    std::shared_ptr<Ship::ResourceManager> manager;
};

} // namespace

// ============================================================
// Single-archive cases
// ============================================================

// One archive ships the real asset, the `.meta` and the target all at once.
//
//   vanilla   base.bin
//   mod       base.bin, alias.bin, base.bin.meta -> alias.bin
//
// Loads alias.bin: one archive supplies both candidates, so its `.meta` decides.
TEST(MetaAliasResolution, OwnMetaOutranksOwnRealAsset) {
    MetaAliasHarness h;
    h.Mount("vanilla", { { kBase, HeaderedBlob("vanilla") } });
    h.Mount("mod",
            { { kBase, HeaderedBlob("mod real") }, { kBaseMeta, MetaJson(kAlias) }, { kAlias, BareBlob("aliased") } });

    EXPECT_EQ(h.Load(kBase), "aliased");
}

// A `.meta` whose target is nowhere to be found.
//
//   vanilla   base.bin
//   mod       base.bin, base.bin.meta -> alias.bin
//
// Loads mod's base.bin: with no target there is nothing to alias to, so the real asset remains.
TEST(MetaAliasResolution, FallsBackToRealAssetWhenTargetAbsent) {
    MetaAliasHarness h;
    h.Mount("vanilla", { { kBase, HeaderedBlob("vanilla") } });
    h.Mount("mod", { { kBase, HeaderedBlob("mod real") }, { kBaseMeta, MetaJson(kAlias) } });

    EXPECT_EQ(h.Load(kBase), "mod real");
}

// The higher archive ships no real asset at the requested path, only the alias and its target.
//
//   vanilla   base.bin
//   mod       alias.bin, base.bin.meta -> alias.bin
//
// Loads alias.bin: the alias ranks at mod, which outranks vanilla's real asset.
TEST(MetaAliasResolution, AliasResolvesWithNoRealAssetInItsArchive) {
    MetaAliasHarness h;
    h.Mount("vanilla", { { kBase, HeaderedBlob("vanilla") } });
    h.Mount("mod", { { kBaseMeta, MetaJson(kAlias) }, { kAlias, BareBlob("aliased") } });

    EXPECT_EQ(h.Load(kBase), "aliased");
}

// An archive with only a `.meta`, pointing at a file that does not exist.
//
//   vanilla   base.bin
//   mod       base.bin.meta -> alias.bin
//
// Loads vanilla's base.bin.
TEST(MetaAliasResolution, MetaOnlyArchiveDoesNotShadowLowerRealAsset) {
    MetaAliasHarness h;
    h.Mount("vanilla", { { kBase, HeaderedBlob("vanilla") } });
    h.Mount("mod", { { kBaseMeta, MetaJson(kAlias) } });

    EXPECT_EQ(h.Load(kBase), "vanilla");
}

// No `.meta` involved at all — plain archive priority.
//
//   vanilla   base.bin
//   mod       base.bin
//
// Loads mod's base.bin: the highest-ranked real asset.
TEST(MetaAliasResolution, RealAssetLoadsWhenNoMetaExists) {
    MetaAliasHarness h;
    h.Mount("vanilla", { { kBase, HeaderedBlob("vanilla") } });
    h.Mount("mod", { { kBase, HeaderedBlob("mod real") } });

    EXPECT_EQ(h.Load(kBase), "mod real");
}

// An alias target that still carries its OTR header.
//
//   vanilla   base.bin
//   mod       alias.bin (headered), base.bin.meta -> alias.bin
//
// Loads alias.bin's payload: init data from a `.meta` skips the legacy header parse, so the
// header has to be stepped over or the factory reads it as payload.
TEST(MetaAliasResolution, HeaderedAliasTargetSkipsOtrHeader) {
    MetaAliasHarness h;
    h.Mount("vanilla", { { kBase, HeaderedBlob("vanilla") } });
    h.Mount("mod", { { kBaseMeta, MetaJson(kAlias) }, { kAlias, HeaderedBlob("aliased") } });

    EXPECT_EQ(h.Load(kBase), "aliased");
}

// ============================================================
// Layered cases
// ============================================================

// The `.meta`, the real asset and the target spread across three archives.
//
//   vanilla     base.bin
//   10-meta     base.bin.meta -> alias.bin
//   20-real     base.bin
//   30-target   alias.bin
//
// Loads alias.bin: the alias ranks where its target lives (30), above the real asset at 20.
TEST(MetaAliasResolution, AliasTargetInHigherArchiveOutranksRealAsset) {
    MetaAliasHarness h;
    h.Mount("vanilla", { { kBase, HeaderedBlob("vanilla") } });
    h.Mount("10-meta", { { kBaseMeta, MetaJson(kAlias) } });
    h.Mount("20-real", { { kBase, HeaderedBlob("mod real") } });
    h.Mount("30-target", { { kAlias, BareBlob("aliased") } });

    EXPECT_EQ(h.Load(kBase), "aliased");
}

// The same stack with the target archive removed and the `.meta` left untouched.
//
//   vanilla   base.bin
//   10-meta   base.bin.meta -> alias.bin
//   20-real   base.bin
//
// Loads 20-real's base.bin.
TEST(MetaAliasResolution, AliasFallsBackWhenTargetArchiveDropped) {
    MetaAliasHarness h;
    h.Mount("vanilla", { { kBase, HeaderedBlob("vanilla") } });
    h.Mount("10-meta", { { kBaseMeta, MetaJson(kAlias) } });
    h.Mount("20-real", { { kBase, HeaderedBlob("mod real") } });

    EXPECT_EQ(h.Load(kBase), "mod real");
}

// A real asset ranked above the alias target.
//
//   vanilla              base.bin
//   10-meta-and-target   alias.bin, base.bin.meta -> alias.bin
//   20-real              base.bin
//
// Loads 20-real's base.bin: the alias ranks where its target lives (10), which loses to a real
// asset at 20.
TEST(MetaAliasResolution, RealAssetOutranksLowerAliasTarget) {
    MetaAliasHarness h;
    h.Mount("vanilla", { { kBase, HeaderedBlob("vanilla") } });
    h.Mount("10-meta-and-target", { { kBaseMeta, MetaJson(kAlias) }, { kAlias, BareBlob("aliased") } });
    h.Mount("20-real", { { kBase, HeaderedBlob("mod real") } });

    EXPECT_EQ(h.Load(kBase), "mod real");
}

// A `.meta` pointing down the stack at a target below itself.
//
//   vanilla     base.bin
//   10-target   alias.bin
//   20-meta     base.bin.meta -> alias.bin
//
// Loads alias.bin: the target is resolved across every mounted archive, not just from the
// `.meta`'s own archive upward.
TEST(MetaAliasResolution, AliasTargetBelowTheMetaStillResolves) {
    MetaAliasHarness h;
    h.Mount("vanilla", { { kBase, HeaderedBlob("vanilla") } });
    h.Mount("10-target", { { kAlias, BareBlob("aliased") } });
    h.Mount("20-meta", { { kBaseMeta, MetaJson(kAlias) } });

    EXPECT_EQ(h.Load(kBase), "aliased");
}

// ============================================================
// Resolution details
// ============================================================

// A `.meta` with no "path" key, beside a resource that carries no init data of its own.
//
//   archive   base.bin (no OTR header), base.bin.meta (no "path")
//
// Loads base.bin: the `.meta` provides its init data.
TEST(MetaAliasResolution, MetaProvidesInitDataForHeaderlessResource) {
    MetaAliasHarness h;
    h.Mount("archive", { { kBase, BareBlob("base") }, { kBaseMeta, MetaJson("") } });

    EXPECT_EQ(h.Load(kBase), "base");
}

// A `.meta` declaring its target custom.
//
//   archive   alias.bin, base.bin.meta -> alias.bin ("isCustom": true)
//
// Loads alias.bin as custom.
TEST(MetaAliasResolution, MetaCanDeclareIsCustom) {
    MetaAliasHarness h;
    h.Mount("archive", { { kBaseMeta, MetaJson(kAlias, /*isCustom=*/true) }, { kAlias, BareBlob("aliased") } });

    auto resource = h.manager->LoadResourceProcess(kBase);
    ASSERT_NE(resource, nullptr);
    EXPECT_TRUE(h.manager->GetResourceIsCustom(resource));
}

// The same `.meta` without an "isCustom" key.
//
//   archive   alias.bin, base.bin.meta -> alias.bin
//
// Loads alias.bin as non-custom.
TEST(MetaAliasResolution, AliasedResourceIsNotCustomByDefault) {
    MetaAliasHarness h;
    h.Mount("archive", { { kBaseMeta, MetaJson(kAlias) }, { kAlias, BareBlob("aliased") } });

    auto resource = h.manager->LoadResourceProcess(kBase);
    ASSERT_NE(resource, nullptr);
    EXPECT_FALSE(h.manager->GetResourceIsCustom(resource));
}

// A request by CRC64 rather than by path.
//
//   archive   alias.bin, base.bin.meta -> alias.bin
//
// Loads alias.bin.
TEST(MetaAliasResolution, HashIdentifiedRequestResolvesMeta) {
    MetaAliasHarness h;
    h.Mount("archive", { { kBaseMeta, MetaJson(kAlias) }, { kAlias, BareBlob("aliased") } });

    EXPECT_EQ(h.Load(Ship::ResourceIdentifier(CRC64(kBase.c_str()), 0, nullptr)), "aliased");
}

// An alias target that is indexed but cannot be served — a zero-byte zip entry, or a folder
// entry deleted since it was indexed.
//
//   vanilla   base.bin
//   mod       alias.bin (indexed, unreadable), base.bin.meta -> alias.bin
//
// Loads vanilla's base.bin.
TEST(MetaAliasResolution, UnreadableAliasTargetFallsBackToRealAsset) {
    MetaAliasHarness h;
    h.Mount("vanilla", { { kBase, HeaderedBlob("vanilla") } });
    h.Mount(
        std::make_shared<UnreadableFileArchive>("ram://mod",
                                                std::unordered_map<std::string, std::string>{
                                                    { kBaseMeta, MetaJson(kAlias) }, { kAlias, BareBlob("aliased") } },
                                                kAlias));

    EXPECT_EQ(h.Load(kBase), "vanilla");
}

// Unloading an archive whose `.meta` produced a cached resource.
//
//   archive   alias.bin, base.bin.meta -> alias.bin
//
// Evicts the resource cached at base.bin.
TEST(MetaAliasResolution, UnloadEvictsResourceReachedThroughMeta) {
    MetaAliasHarness h;
    h.Mount("archive", { { kBaseMeta, MetaJson(kAlias) }, { kAlias, BareBlob("aliased") } });

    ASSERT_EQ(h.Load(kBase), "aliased");
    ASSERT_NE(h.manager->GetCachedResource(kBase), nullptr);

    h.manager->UnloadResources("*");

    EXPECT_EQ(h.manager->GetCachedResource(kBase), nullptr);
}

// An alias shipped under alt/, unloaded by directory the way a loading zone does. The request has
// no alt/ prefix — LoadResourceProcess prepends it, so the resource caches under the alt path.
//
//   vanilla   base.bin
//   mod       alt/alias.bin, alt/base.bin.meta -> alt/alias.bin
//
// Evicts the resource cached at alt/base.bin.
TEST(MetaAliasResolution, AltDirectoryUnloadEvictsAliasedResource) {
    const std::string altBase = Ship::IResource::gAltAssetPrefix + kBase;
    const std::string altAlias = Ship::IResource::gAltAssetPrefix + kAlias;

    MetaAliasHarness h;
    h.Mount("vanilla", { { kBase, HeaderedBlob("vanilla") } });
    h.Mount("mod", { { altBase + ".meta", MetaJson(altAlias) }, { altAlias, BareBlob("aliased") } });
    h.manager->SetAltAssetsEnabled(true);

    ASSERT_EQ(h.Load(kBase), "aliased");
    ASSERT_NE(h.manager->GetCachedResource(altBase), nullptr);

    h.manager->UnloadResources(Ship::IResource::gAltAssetPrefix + "*");

    EXPECT_EQ(h.manager->GetCachedResource(altBase), nullptr);
}
