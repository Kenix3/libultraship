#include "ship/resource/archive/Archive.h"

#include "spdlog/spdlog.h"

#include "ship/Context.h"
#include "ship/resource/File.h"
#include "ship/resource/ResourceLoader.h"
#include "ship/resource/ResourceType.h"
#include "ship/utils/binarytools/MemoryStream.h"
#include "ship/utils/glob.h"
#include "ship/utils/StrHash64.h"
#include "ship/window/Window.h"
#include "ship/utils/StringHelper.h"
#ifdef ENABLE_SCRIPTING
#include "ship/security/Keystore.h"
#endif

#include <tinyxml2.h>
#include <monocypher.h>
#include <nlohmann/json.hpp>
#include <monocypher-ed25519.h>

namespace Ship {
Archive::Archive(const std::string& path)
    : mIsLoaded(false), mIsSigned(false), mIsChecksumValid(false), mHasGameVersion(false), mGameVersion(0xFFFFFFFF),
      mManifest(), mPath(path) {
    mHashes = std::make_shared<std::unordered_map<uint64_t, std::string>>();
}

Archive::~Archive() {
    SPDLOG_TRACE("destruct archive: {}", GetPath());
}

bool Archive::operator==(const Archive& rhs) const {
    return mPath == rhs.mPath;
}

void Archive::Load() {
    bool opened = Open();

    auto t = LoadFile("version");
    bool isGameVersionValid = false;
    if (t != nullptr && t->IsLoaded) {
        mHasGameVersion = true;
        auto stream = std::make_shared<MemoryStream>(t->Buffer->data(), t->Buffer->size());
        auto reader = std::make_shared<BinaryReader>(stream);
        Endianness endianness = (Endianness)reader->ReadUByte();
        reader->SetEndianness(endianness);
        SetGameVersion(reader->ReadUInt32());
        isGameVersionValid =
            Context::GetInstance()->GetChildren().GetFirst<ResourceManager>()->GetArchiveManager()->IsGameVersionValid(
                GetGameVersion());

        if (!isGameVersionValid) {
            SPDLOG_WARN("Attempting to load Archive \"{}\" with invalid version {}", GetPath(), GetGameVersion());
        }
    }

    auto m = LoadFile("manifest.json");
    if (m != nullptr && m->IsLoaded) {
        try {
            auto json = nlohmann::json::parse(m->Buffer->begin(), m->Buffer->end());
            mManifest.Name = json["name"].get<std::string>();
            mManifest.Icon = json.value("icon", "");
            mManifest.Author = json.value("author", "Unknown");
            mManifest.Version = json.value("version", "1.0");
            mManifest.Website = json.value("website", "https://github.com/Kenix3/libultraship");
            mManifest.Description = json.value("description", "No description provided.");
            mManifest.License = json.value("license", "All rights reserved");
            mManifest.CodeVersion = json.value("code_version", 1);
            mManifest.GameVersion = json.value("game_version", 0xFFFFFFFF);
            mManifest.Main = json.value("main", "");
            mManifest.Binaries = json.value("binaries", std::unordered_map<std::string, std::string>{});
            mManifest.Dependencies = json.value("dependencies", std::vector<std::string>{});
            mManifest.Checksum = json.value("checksum", "");
            mManifest.Signature = json.value("signature", "");
            mManifest.PublicKey = json.value("public_key", "");

            if (mManifest.GameVersion != 0xFFFFFFFF) {
                mHasGameVersion = true;
                SetGameVersion(mManifest.GameVersion);
                isGameVersionValid = Context::GetInstance()
                                         ->GetChildren()
                                         .GetFirst<ResourceManager>()
                                         ->GetArchiveManager()
                                         ->IsGameVersionValid(GetGameVersion());

                if (!isGameVersionValid) {
                    SPDLOG_WARN("Attempting to load Archive \"{}\" with invalid version {}", GetPath(),
                                GetGameVersion());
                }
            }

            Validate();
        } catch (const std::exception& e) {
            SPDLOG_ERROR("Failed to parse metadata.json for archive {}: {}", GetPath(), e.what());
        }
    }

    SetLoaded(opened && (!mHasGameVersion || isGameVersionValid));

    if (!IsLoaded()) {
        Unload();
    }
}

void Archive::Unload() {
    Close();
    SetLoaded(false);
}

std::shared_ptr<File> Archive::LoadFile(uint64_t hash) {
    const std::string& filePath =
        *Context::GetInstance()->GetChildren().GetFirst<ResourceManager>()->GetArchiveManager()->HashToString(hash);
    return LoadFile(filePath);
}

std::shared_ptr<std::unordered_map<uint64_t, std::string>> Archive::ListFiles() {
    return mHashes;
}

std::shared_ptr<std::unordered_map<uint64_t, std::string>> Archive::ListFiles(const std::string& filter) {
    auto result = std::make_shared<std::unordered_map<uint64_t, std::string>>();

    std::copy_if(mHashes->begin(), mHashes->end(), std::inserter(*result, result->begin()),
                 [filter](const std::pair<const int64_t, const std::string&> entry) {
                     return glob_match(filter.c_str(), entry.second.c_str());
                 });

    return result;
}

bool Archive::HasFile(const std::string& filePath) {
    return HasFile(CRC64(filePath.c_str()));
}

bool Archive::HasFile(uint64_t hash) {
    return mHashes->count(hash) > 0;
}

bool Archive::HasGameVersion() {
    return mHasGameVersion;
}

uint32_t Archive::GetGameVersion() {
    return mGameVersion;
}

const ArchiveManifest& Archive::GetManifest() {
    return mManifest;
}

bool Archive::IsSigned() {
    return mIsSigned;
}

bool Archive::IsChecksumValid() {
    return mIsChecksumValid;
}

const std::string& Archive::GetPath() {
    return mPath;
}

bool Archive::IsLoaded() {
    return mIsLoaded;
}

void Archive::SetLoaded(bool isLoaded) {
    mIsLoaded = isLoaded;
}

void Archive::SetGameVersion(uint32_t gameVersion) {
    mGameVersion = gameVersion;
}

void Archive::IndexFile(const std::string& filePath) {
    if (filePath.length() > 5 && filePath.substr(filePath.length() - 5) == ".meta") {
        IndexFile(filePath.substr(0, filePath.length() - 5));
        return;
    }

    (*mHashes)[CRC64(filePath.c_str())] = filePath;
}

void Archive::Validate() {
#ifdef ENABLE_SCRIPTING
    if (mManifest.Checksum.empty()) {
        SPDLOG_WARN("Archive {} does not have a checksum in its metadata, skipping validation", GetPath());
        return;
    }

    auto keystore = Context::GetInstance()->GetChildren().GetFirst<Keystore>();
    auto manager = Context::GetInstance()->GetChildren().GetFirst<ResourceManager>()->GetArchiveManager();
    std::vector<uint8_t> manifestKey = StringHelper::HexToBytes(mManifest.PublicKey);

    if (!keystore->HasKey(manifestKey)) {
        auto callback = manager->GetUntrustedArchiveHandler();
        if (callback != nullptr) {
            auto key = KeystoreEntry{ mManifest.Author, manifestKey, KeyOrigin::User };
            bool isTrusted = callback(*this, key);
            if (!isTrusted) {
                SPDLOG_ERROR("Archive {} is untrusted and was rejected by the user.", GetPath());
                return;
            }
            keystore->AddKey(mManifest.Author, manifestKey, key.Origin);
            SPDLOG_INFO("Added new public key for author {} to keystore.", mManifest.Author);
        } else {
            SPDLOG_ERROR("Archive {} is signed by an unknown author, and no handler is available to approve it.",
                         GetPath());
            return;
        }
    }

    std::vector<std::tuple<std::string, std::shared_ptr<File>>> files;

    for (const auto& [hash, filePath] : *mHashes) {
        std::string normalizedPath = filePath;
        std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');

        if (normalizedPath == "manifest.json" || normalizedPath.back() == '/') {
            continue;
        }

        auto file = LoadFile(filePath);
        if (file == nullptr || !file->IsLoaded) {
            SPDLOG_ERROR("Failed to load file {} from archive {} during validation", filePath, GetPath());
            return;
        }

        files.emplace_back(normalizedPath, file);
    }

    std::sort(files.begin(), files.end(),
              [](const std::tuple<std::string, std::shared_ptr<File>>& a,
                 const std::tuple<std::string, std::shared_ptr<File>>& b) { return std::get<0>(a) < std::get<0>(b); });

    crypto_blake2b_ctx ctx;
    crypto_blake2b_init(&ctx, 64);

    for (const auto& [normalizedPath, file] : files) {
        crypto_blake2b_update(&ctx, reinterpret_cast<const uint8_t*>(normalizedPath.c_str()), normalizedPath.length());
        if (file->Buffer->size() > 0) {
            crypto_blake2b_update(&ctx, reinterpret_cast<const uint8_t*>(file->Buffer->data()), file->Buffer->size());
        }
    }

    std::vector<uint8_t> rawHash(64);
    crypto_blake2b_final(&ctx, rawHash.data());
    std::string calculatedChecksumHex = StringHelper::BytesToHex(rawHash);
    if (calculatedChecksumHex != mManifest.Checksum) {
        SPDLOG_ERROR("Checksum validation failed for archive {}. Expected {}, got {}", GetPath(), mManifest.Checksum,
                     calculatedChecksumHex);
        return;
    }

    mIsChecksumValid = true;

    if (mManifest.Signature.empty()) {
        SPDLOG_WARN("Archive {} is marked as signed but does not have a signature in its metadata, skipping signature "
                    "validation",
                    GetPath());
        return;
    }

    std::vector<uint8_t> signature = StringHelper::HexToBytes(mManifest.Signature);

    if (signature.size() != 64) {
        SPDLOG_ERROR("Invalid signature size for archive {}. Expected 64 bytes, got {}", GetPath(), signature.size());
        return;
    }

    bool validSignature = false;

    for (const auto& key : keystore->GetAllKeys()) {
        const int status = crypto_ed25519_check(signature.data(), key.Data.data(), rawHash.data(), rawHash.size());

        if (status == 0) {
            validSignature = true;
            break;
        }
    }

    if (!validSignature) {
        SPDLOG_ERROR(
            "Signature validation failed for archive {}. The archive may have been tampered with or corrupted.",
            GetPath());
        return;
    }

    mIsSigned = true;
    SPDLOG_INFO("Archive {} successfully authenticated.", GetPath());
#endif // ENABLE_SCRIPTING
}

} // namespace Ship
