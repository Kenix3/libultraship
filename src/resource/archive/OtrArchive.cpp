#include "OtrArchive.h"

#include "Context.h"
#include "utils/binarytools/FileHelper.h"
#include "resource/ResourceManager.h"
#include "resource/archive/ArchiveManager.h"

#include "spdlog/spdlog.h"

namespace LUS {
OtrArchive::OtrArchive(const std::string& archivePath) : Archive(archivePath) {
}

OtrArchive::~OtrArchive() {
    SPDLOG_TRACE("destruct otrarchive: {}", GetPath());
}

std::shared_ptr<File> OtrArchive::LoadFileRaw(const std::string& filePath) {
    HANDLE fileHandle;
    bool attempt = SFileOpenFileEx(mHandle, filePath.c_str(), 0, &fileHandle);
    if (!attempt) {
        SPDLOG_TRACE("({}) Failed to open file {} from mpq archive  {}.", GetLastError(), filePath, GetPath());
        return nullptr;
    }

    auto fileToLoad = std::make_shared<File>();
    fileToLoad->InitData = std::make_shared<ResourceInitData>();
    fileToLoad->InitData->Path = filePath;
    DWORD fileSize = SFileGetFileSize(fileHandle, 0);
    DWORD readBytes;
    fileToLoad->Buffer = std::make_shared<std::vector<char>>();
    fileToLoad->Buffer->resize(fileSize);
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

    fileToLoad->Parent = dynamic_pointer_cast<Archive>(std::make_shared<OtrArchive>(std::move(*this)));
    fileToLoad->IsLoaded = true;

    return fileToLoad;
}

std::shared_ptr<File> OtrArchive::LoadFileRaw(uint64_t hash) {
    const std::string& filePath = *Context::GetInstance()->GetResourceManager()->GetArchiveManager()->HashToString(hash);
    return LoadFileRaw(filePath);
}

std::shared_ptr<ResourceInitData> OtrArchive::LoadFileMeta(const std::string& filePath) {
    return nullptr;
}

std::shared_ptr<ResourceInitData> OtrArchive::LoadFileMeta(uint64_t hash) {
    return nullptr;
}

bool OtrArchive::LoadRaw() {
    const bool opened = SFileOpenArchive(GetPath().c_str(), 0, MPQ_OPEN_READ_ONLY, &mHandle);
    if (opened) {
        SPDLOG_INFO("Opened mpq file \"{}\"", GetPath());
        // SFileCloseArchive(mHandle);
    } else {
        SPDLOG_ERROR("Failed to load mpq file \"{}\"", GetPath());
        mHandle = nullptr;
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

        AddFile(lineStr);
    }

    return opened;
}

bool OtrArchive::UnloadRaw() {
    bool closed = SFileCloseArchive(mHandle);
    if (!closed) {
        SPDLOG_ERROR("({}) Failed to close mpq {}", GetLastError(), mHandle);
    }

    return closed;
}

} // namespace LUS
