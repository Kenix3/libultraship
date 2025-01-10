#include "Archive.h"

#include "spdlog/spdlog.h"

#include "Context.h"
#include "resource/File.h"
#include "resource/ResourceLoader.h"
#include "resource/ResourceType.h"
#include "utils/binarytools/MemoryStream.h"
#include "utils/glob.h"
#include "utils/StrHash64.h"
#include <nlohmann/json.hpp>

namespace Ship {
Archive::Archive(const std::string& path)
    : mIsLoaded(false), mHasGameVersion(false), mGameVersion(0xFFFFFFFF), mPath(path) {
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

    SetLoaded(opened && (!mHasGameVersion || isGameVersionValid));

    if (!IsLoaded()) {
        Unload();
    }
}

void Archive::Unload() {
    Close();
    SetLoaded(false);
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

std::shared_ptr<File> Archive::LoadFile(uint64_t hash) {
    const std::string& filePath =
        *Context::GetInstance()->GetResourceManager()->GetArchiveManager()->HashToString(hash);
    return LoadFile(filePath);
}

} // namespace Ship
