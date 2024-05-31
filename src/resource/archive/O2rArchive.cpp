#include "O2rArchive.h"

#include "Context.h"
#include "spdlog/spdlog.h"

namespace Ship {
O2rArchive::O2rArchive(const std::string& archivePath) : Archive(archivePath) {
}

O2rArchive::~O2rArchive() {
    SPDLOG_TRACE("destruct o2rarchive: {}", GetPath());
}

std::shared_ptr<Ship::File> O2rArchive::LoadFileRaw(uint64_t hash) {
    const std::string& filePath =
        *Context::GetInstance()->GetResourceManager()->GetArchiveManager()->HashToString(hash);
    return LoadFileRaw(filePath);
}

std::shared_ptr<Ship::File> O2rArchive::LoadFileRaw(const std::string& filePath) {
    if (mZipArchive == nullptr) {
        SPDLOG_TRACE("Failed to open file {} from zip archive {}. Archive not open.", filePath, GetPath());
        return nullptr;
    }

    auto zipEntryIndex = zip_name_locate(mZipArchive, filePath.c_str(), 0);
    if (zipEntryIndex < 0) {
        SPDLOG_TRACE("Failed to find file {} in zip archive  {}.", filePath, GetPath());
        return nullptr;
    }

    struct zip_stat zipEntryStat;
    zip_stat_init(&zipEntryStat);
    if (zip_stat_index(mZipArchive, zipEntryIndex, 0, &zipEntryStat) != 0) {
        SPDLOG_TRACE("Failed to get entry information for file {} in zip archive  {}.", filePath, GetPath());
        return nullptr;
    }

    struct zip_file* zipEntryFile = zip_fopen_index(mZipArchive, zipEntryIndex, 0);
    if (!zipEntryFile) {
        SPDLOG_TRACE("Failed to open file {} in zip archive  {}.", filePath, GetPath());
        return nullptr;
    }

    auto fileToLoad = std::make_shared<File>();
    fileToLoad->Buffer = std::make_shared<std::vector<char>>(zipEntryStat.size);

    if (zip_fread(zipEntryFile, fileToLoad->Buffer->data(), zipEntryStat.size) < 0) {
        SPDLOG_TRACE("Error reading file {} in zip archive  {}.", filePath, GetPath());
    }

    if (zip_fclose(zipEntryFile) != 0) {
        SPDLOG_TRACE("Error closing file {} in zip archive  {}.", filePath, GetPath());
    }

    fileToLoad->IsLoaded = true;

    return fileToLoad;
}

bool O2rArchive::Open() {
    mZipArchive = zip_open(GetPath().c_str(), ZIP_RDONLY, nullptr);
    if (mZipArchive == nullptr) {
        SPDLOG_ERROR("Failed to load zip file \"{}\"", GetPath());
        return false;
    }

    auto zipNumEntries = zip_get_num_entries(mZipArchive, 0);
    for (auto i = 0; i < zipNumEntries; i++) {
        auto zipEntryName = zip_get_name(mZipArchive, i, 0);

        // It is possible for directories to have entries in a zip
        // file, we don't want those indexed as files in the archive
        if (zipEntryName[strlen(zipEntryName) - 1] == '/') {
            continue;
        }

        IndexFile(zipEntryName);
    }

    return true;
}

bool O2rArchive::Close() {
    if (zip_close(mZipArchive) == -1) {
        SPDLOG_ERROR("Failed to close zip file \"{}\"", GetPath());
        return false;
    }

    return true;
}
} // namespace Ship
