#ifndef EXCLUDE_MPQ_SUPPORT

#include "OtrArchive.h"

#include "Context.h"
#include "utils/filesystemtools/FileHelper.h"
#include "resource/ResourceManager.h"
#include "resource/archive/ArchiveManager.h"

#include "spdlog/spdlog.h"

namespace Ship {
OtrArchive::OtrArchive(const std::string& archivePath) : Archive(archivePath) {
    mHandle = nullptr;
}

OtrArchive::~OtrArchive() {
    SPDLOG_TRACE("destruct otrarchive: {}", GetPath());
}

std::shared_ptr<Ship::File> OtrArchive::LoadFileRaw(const std::string& filePath) {
    if (mHandle == nullptr) {
        SPDLOG_TRACE("Failed to open file {} from mpq archive {}. Archive not open.", filePath, GetPath());
        return nullptr;
    }

    HANDLE fileHandle;
    bool attempt = SFileOpenFileEx(mHandle, filePath.c_str(), 0, &fileHandle);
    if (!attempt) {
        SPDLOG_TRACE("({}) Failed to open file {} from mpq archive  {}.", GetLastError(), filePath, GetPath());
        return nullptr;
    }

    auto fileToLoad = std::make_shared<File>();
    DWORD fileSize = SFileGetFileSize(fileHandle, 0);
    DWORD readBytes;
    fileToLoad->Buffer = std::make_shared<std::vector<char>>(fileSize);
    bool readFileSuccess = SFileReadFile(fileHandle, fileToLoad->Buffer->data(), fileSize, &readBytes, NULL);

    if (!readFileSuccess) {
        SPDLOG_ERROR("({}) Failed to read file {} from mpq archive {}", GetLastError(), filePath, GetPath());
        bool closeFileSuccess = SFileCloseFile(fileHandle);
        if (!closeFileSuccess) {
            SPDLOG_ERROR("({}) Failed to close file {} from mpq after read failure in archive {}", GetLastError(),
                         filePath, GetPath());
        }
        return nullptr;
    }

    bool closeFileSuccess = SFileCloseFile(fileHandle);
    if (!closeFileSuccess) {
        SPDLOG_ERROR("({}) Failed to close file {} from mpq archive {}", GetLastError(), filePath, GetPath());
    }

    fileToLoad->IsLoaded = true;

    return fileToLoad;
}

std::shared_ptr<Ship::File> OtrArchive::LoadFileRaw(uint64_t hash) {
    const std::string& filePath =
        *Context::GetInstance()->GetResourceManager()->GetArchiveManager()->HashToString(hash);
    return LoadFileRaw(filePath);
}

bool OtrArchive::Open() {
    const bool opened = SFileOpenArchive(GetPath().c_str(), 0, MPQ_OPEN_READ_ONLY, &mHandle);
    if (opened) {
        SPDLOG_INFO("Opened mpq file \"{}\"", GetPath());
    } else {
        SPDLOG_ERROR("Failed to load mpq file \"{}\"", GetPath());
        mHandle = nullptr;
        return false;
    }

    // Generate the file list by reading the list file.
    // This can also be done via the StormLib API, but this was copied from the LUS1.x implementation in GenerateCrcMap.
    auto listFile = LoadFileRaw("(listfile)");

    // Use std::string_view to avoid unnecessary string copies
    std::vector<std::string_view> lines =
        StringHelper::Split(std::string_view(listFile->Buffer->data(), listFile->Buffer->size()), "\n");

    for (size_t i = 0; i < lines.size(); i++) {
        // Use std::string_view to avoid unnecessary string copies
        std::string_view line = lines[i].substr(0, lines[i].length() - 1); // Trim \r
        std::string lineStr = std::string(line);

        IndexFile(lineStr);
    }

    return opened;
}

bool OtrArchive::Close() {
    bool closed = SFileCloseArchive(mHandle);
    if (!closed) {
        SPDLOG_ERROR("({}) Failed to close mpq {}", GetLastError(), mHandle);
    }

    return closed;
}

} // namespace Ship

#endif // EXCLUDE_MPQ_SUPPORT
