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
#include "ship/security/KeystoreSystem.h"

#include <tinyxml2.h>
#include <monocypher.h>
#include <nlohmann/json.hpp>
#include <monocypher-ed25519.h>

namespace Ship {
Archive::Archive(const std::string& path)
    : mIsLoaded(false), mIsSigned(false), mIsChecksumValid(false), mHasGameVersion(false), mGameVersion(0xFFFFFFFF),
      mMetadata(), mPath(path) {
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
            Context::GetInstance()->GetResourceManager()->GetArchiveManager()->IsGameVersionValid(GetGameVersion());

        if (!isGameVersionValid) {
            SPDLOG_WARN("Attempting to load Archive \"{}\" with invalid version {}", GetPath(), GetGameVersion());
        }
    }

    auto m = LoadFile("metadata.json");
    if (m != nullptr && m->IsLoaded) {
        try {
            auto json = nlohmann::json::parse(m->Buffer->begin(), m->Buffer->end());
            mMetadata.Name = json["name"].get<std::string>();
            mMetadata.Icon = json.value("icon", "");
            mMetadata.Author = json.value("author", "Unknown");
            mMetadata.Version = json.value("version", "1.0");
            mMetadata.Website = json.value("website", "https://github.com/Kenix3/libultraship");
            mMetadata.Description = json.value("description", "No description provided.");
            mMetadata.License = json.value("license", "MIT");
            mMetadata.CodeVersion = json.value("code_version", 1);
            mMetadata.GameVersion = json.value("game_version", 0xFFFFFFFF);
            mMetadata.Main = json.value("main", "");
            mMetadata.Binaries = json.value("binaries", std::unordered_map<std::string, std::string>{});
            mMetadata.Dependencies = json.value("dependencies", std::vector<std::string>{});

            if (mMetadata.GameVersion != 0xFFFFFFFF) {
                mHasGameVersion = true;
                SetGameVersion(mMetadata.GameVersion);
                isGameVersionValid =
                    Context::GetInstance()->GetResourceManager()->GetArchiveManager()->IsGameVersionValid(
                        GetGameVersion());

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
        *Context::GetInstance()->GetResourceManager()->GetArchiveManager()->HashToString(hash);
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

const ArchiveMetadata& Archive::GetMetadata() {
    return mMetadata;
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
    if (mMetadata.Checksum.empty()) {
        SPDLOG_WARN("Archive {} does not have a checksum in its metadata, skipping validation", GetPath());
        return;
    }

    std::vector<std::tuple<std::string, std::shared_ptr<File>>> files;

    for (const auto& [hash, filePath] : *mHashes) {
        if (filePath == "metadata.json" || filePath == "version") {
            continue;
        }

        auto file = LoadFile(hash);
        if (file == nullptr || !file->IsLoaded) {
            SPDLOG_ERROR("Failed to load file {} from archive {} during validation", filePath, GetPath());
            return;
        }

        files.emplace_back(filePath, file);
    }

    std::sort(files.begin(), files.end(),
              [](const std::tuple<std::string, std::shared_ptr<File>>& a,
                 const std::tuple<std::string, std::shared_ptr<File>>& b) { return std::get<0>(a) < std::get<0>(b); });

    crypto_blake2b_ctx ctx;
    crypto_blake2b_init(&ctx, 64);
    for (const auto& [filePath, file] : files) {
        crypto_blake2b_update(&ctx, reinterpret_cast<const uint8_t*>(file->Buffer->data()), file->Buffer->size());
    }

    std::vector<uint8_t> hash(64);
    crypto_blake2b_final(&ctx, hash.data());

    std::stringstream hex_ss;
    hex_ss << std::hex << std::setfill('0');
    for (int i = 0; i < 64; ++i) {
        hex_ss << std::setw(2) << static_cast<int>(hash[i]);
    }

    if (hex_ss.str() != mMetadata.Checksum) {
        SPDLOG_ERROR("Checksum validation failed for archive {}. Expected {}, got {}", GetPath(), mMetadata.Checksum,
                     hex_ss.str());
        return;
    }

    mIsChecksumValid = true;

    if (mMetadata.Signature.empty()) {
        SPDLOG_WARN("Archive {} is marked as signed but does not have a signature in its metadata, skipping signature "
                    "validation",
                    GetPath());
        return;
    }

    std::vector<uint8_t> signature = HexToBytes(mMetadata.Signature);

    if (signature.size() != 64) {
        SPDLOG_ERROR("Invalid signature size for archive {}. Expected 64 bytes, got {}", GetPath(), signature.size());
        return;
    }

    bool validSignature = false;

    for (const auto& key : Context::GetInstance()->GetKeystoreSystem()->GetAllKeys()) {
        const int status = crypto_ed25519_check(signature.data(), key.data(), hash.data(), hash.size());
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
}

std::vector<uint8_t> Archive::HexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

} // namespace Ship
