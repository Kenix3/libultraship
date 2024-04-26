#include "ArchiveManager.h"

#include <filesystem>
#include "spdlog/spdlog.h"

#include "resource/archive/Archive.h"
#include "resource/archive/OtrArchive.h"
#include "resource/archive/O2rArchive.h"
#include "Utils/StringHelper.h"
#include <StrHash64.h>

// TODO: Delete me and find an implementation
// Comes from stormlib. May not be the most efficient, but it's also important to be consistent.
// NOLINTNEXTLINE
extern bool SFileCheckWildCard(const char* szString, const char* szWildCard);

namespace LUS {
ArchiveManager::ArchiveManager() {
}

void ArchiveManager::Init(const std::vector<std::string>& archivePaths) {
    Init(archivePaths, {});
}

void ArchiveManager::Init(const std::vector<std::string>& archivePaths,
                          const std::unordered_set<uint32_t>& validGameVersions) {
    mValidGameVersions = validGameVersions;
    auto archives = GetArchiveListInPaths(archivePaths);
    for (const auto archive : archives) {
        AddArchive(archive);
    }
}

ArchiveManager::~ArchiveManager() {
    SPDLOG_TRACE("destruct archive manager");
    SetArchives({});
}

bool ArchiveManager::IsArchiveLoaded() {
    return !mArchives.empty();
}

std::shared_ptr<File> ArchiveManager::LoadFile(const std::string& filePath,
                                               std::shared_ptr<ResourceInitData> initData) {
    if (filePath == "") {
        return nullptr;
    }

    return LoadFile(CRC64(filePath.c_str()), initData);
}

std::shared_ptr<File> ArchiveManager::LoadFile(uint64_t hash, std::shared_ptr<ResourceInitData> initData) {
    const auto archive = mFileToArchive[hash];
    if (archive == nullptr) {
        return nullptr;
    }

    auto file = archive->LoadFile(hash, initData);
    file->Parent = archive;
    return file;
}

bool ArchiveManager::HasFile(const std::string& filePath) {
    return HasFile(CRC64(filePath.c_str()));
}

bool ArchiveManager::HasFile(uint64_t hash) {
    return mFileToArchive.count(hash) > 0;
}

std::shared_ptr<std::vector<std::string>> ArchiveManager::ListFiles(const std::string& filter) {
    auto list = ListFiles();
    auto result = std::make_shared<std::vector<std::string>>();

    std::copy_if(list->begin(), list->end(), std::back_inserter(*result), [filter](const std::string& filePath) {
        return SFileCheckWildCard(filePath.c_str(), filter.c_str());
    });

    return result;
}

std::shared_ptr<std::vector<std::string>> ArchiveManager::ListFiles() {
    auto list = std::make_shared<std::vector<std::string>>();
    for (const auto& [hash, path] : mHashes) {
        list->push_back(path);
    }
    return list;
}

std::vector<uint32_t> ArchiveManager::GetGameVersions() {
    return mGameVersions;
}

void ArchiveManager::AddGameVersion(uint32_t newGameVersion) {
    mGameVersions.push_back(newGameVersion);
}

std::vector<std::shared_ptr<Archive>> ArchiveManager::GetArchives() {
    return mArchives;
}

void ArchiveManager::SetArchives(const std::vector<std::shared_ptr<Archive>>& archives) {
    for (const auto& archive : mArchives) {
        archive->Unload();
    }
    mArchives.clear();
    mGameVersions.clear();
    mHashes.clear();
    mFileToArchive.clear();
    for (const auto& archive : archives) {
        if (!archive->IsLoaded()) {
            archive->Load();
        }
        AddArchive(archive);
    }
}

const std::string* ArchiveManager::HashToString(uint64_t hash) const {
    auto it = mHashes.find(hash);
    return it != mHashes.end() ? &it->second : nullptr;
}

std::vector<std::string> ArchiveManager::GetArchiveListInPaths(const std::vector<std::string>& archivePaths) {
    std::vector<std::string> fileList = {};

    for (const auto& archivePath : archivePaths) {
        if (archivePath.length() > 0) {
            if (std::filesystem::is_directory(archivePath)) {
                for (const auto& p : std::filesystem::recursive_directory_iterator(archivePath)) {
                    if (StringHelper::IEquals(p.path().extension().string(), ".otr") ||
                        StringHelper::IEquals(p.path().extension().string(), ".zip") ||
                        StringHelper::IEquals(p.path().extension().string(), ".mpq") ||
                        StringHelper::IEquals(p.path().extension().string(), ".o2r")) {
                        fileList.push_back(std::filesystem::absolute(p).string());
                    }
                }
            } else if (std::filesystem::is_regular_file(archivePath)) {
                fileList.push_back(std::filesystem::absolute(archivePath).string());
            } else {
                SPDLOG_WARN("The archive at path {} does not exist", std::filesystem::absolute(archivePath).string());
            }
        } else {
            SPDLOG_WARN("No archive path supplied");
        }
    }

    return fileList;
}

std::shared_ptr<Archive> ArchiveManager::AddArchive(const std::string& archivePath) {
    const std::filesystem::path path = archivePath;
    const std::string extension = path.extension().string();
    std::shared_ptr<Archive> archive = nullptr;

    SPDLOG_INFO("Reading archive: {}", path.string());

    if (StringHelper::IEquals(extension, ".zip") || StringHelper::IEquals(extension, ".zip")) {
        archive = dynamic_pointer_cast<Archive>(std::make_shared<O2rArchive>(archivePath));
    } else if (StringHelper::IEquals(extension, ".otr") || StringHelper::IEquals(extension, ".mpq")) {
        archive = dynamic_pointer_cast<Archive>(std::make_shared<OtrArchive>(archivePath));
    } else {
        // Not recognized file extension, trying with o2r
        SPDLOG_WARN("File extension \"{}\" not recognized, trying to create an o2r archive.", extension);
        archive = std::make_shared<O2rArchive>(archivePath);
    }

    archive->Load();
    return AddArchive(archive);
}

std::shared_ptr<Archive> ArchiveManager::AddArchive(std::shared_ptr<Archive> archive) {
    if (!archive->IsLoaded()) {
        SPDLOG_WARN("Attempting to add unloaded Archive at {} to Archive Manager", archive->GetPath());
        return nullptr;
    }

    if (!mValidGameVersions.empty() && !mValidGameVersions.contains(archive->GetGameVersion())) {
        SPDLOG_WARN("Attempting to add Archive at {} with invalid Game Version {} to Archive Manager",
                    archive->GetPath(), archive->GetGameVersion());
        return nullptr;
    }

    SPDLOG_INFO("Adding Archive {} to Archive Manager", archive->GetPath());

    mArchives.push_back(archive);
    if (archive->HasGameVersion()) {
        mGameVersions.push_back(archive->GetGameVersion());
    }
    const auto fileList = archive->ListFiles();
    for (auto& [hash, filename] : *fileList.get()) {
        mHashes[hash] = filename;
        mFileToArchive[hash] = archive;
    }
    return archive;
}

bool ArchiveManager::IsGameVersionValid(uint32_t gameVersion) {
    return mValidGameVersions.empty() || mValidGameVersions.contains(gameVersion);
}

} // namespace LUS
