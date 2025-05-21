#include "O2rArchive.h"

#include "Context.h"
#include "window/Window.h"
#include "spdlog/spdlog.h"

namespace Ship {
O2rArchive::O2rArchive(const std::string& archivePath) : Archive(archivePath) {
}

O2rArchive::~O2rArchive() {
    SPDLOG_TRACE("destruct o2rarchive: {}", GetPath());
    Close();
}

std::shared_ptr<File> O2rArchive::LoadFile(uint64_t hash) {
    const std::string& filePath =
        *Context::GetInstance()->GetResourceManager()->GetArchiveManager()->HashToString(hash);
    return LoadFile(filePath);
}

std::shared_ptr<File> O2rArchive::LoadFile(const std::string& filePath) {
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

    // Filesize 0, no logging needed
    if (zipEntryStat.size == 0) {
        SPDLOG_TRACE("Failed to load file {}; filesize 0", filePath, GetPath());
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
    mZipArchive = zip_open(GetPath().c_str(), ZIP_CREATE, nullptr);
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
    if (mZipArchive == nullptr) {
        SPDLOG_ERROR("Cannot close zip file. Zip file not loaded. \"{}\"", GetPath());
        return false;
    }

    if (zip_close(mZipArchive) == -1) {
        SPDLOG_ERROR("Failed to close zip file \"{}\"", GetPath());
        return false;
    }

    return true;
}

bool O2rArchive::WriteFile(const std::string& filePath, const std::vector<uint8_t>& data) {
    if (!mZipArchive) {
        SPDLOG_ERROR("Cannot write to zip: Archive is not open.");
        return false;
    }

    // Create a new zip source from the data buffer
    zip_source_t* source = zip_source_buffer(mZipArchive, data.data(), data.size(), 0);
    if (!source) {
        SPDLOG_ERROR("Failed to create zip source for file \"{}\"", filePath);
        return false;
    }

    // Add or replace the file in the zip archive
    if (zip_file_add(mZipArchive, filePath.c_str(), source, ZIP_FL_ENC_UTF_8 | ZIP_FL_OVERWRITE) < 0) {
        SPDLOG_ERROR("Failed to add file \"{}\" to ZIP", filePath);
        zip_source_free(source);
        return false;
    }

    // Save changes to disk
    if (zip_close(mZipArchive) < 0) {
        zip_error_t* error = zip_get_error(mZipArchive);
        SPDLOG_ERROR("Failed to save changes to zip archive: {} ({})", zip_error_strerror(error),
                     zip_error_code_zip(error));
        zip_discard(mZipArchive); // Close zip and discard changes
        return false;
    }

    SPDLOG_INFO("Successfully wrote file: {}", filePath);

    // Reopen the zip file so that it may continued to be used by libultraship
    mZipArchive = zip_open(GetPath().c_str(), ZIP_CREATE, nullptr);
    if (mZipArchive == nullptr) {
        SPDLOG_ERROR("Failed to reopen zip file after writing.");
        return false;
    }

    IndexFile(filePath);

    // Success
    return true;
}

} // namespace Ship
