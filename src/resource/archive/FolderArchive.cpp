#pragma once

#undef _DLL

#include "FolderArchive.h"

#include "Context.h"
#include "spdlog/spdlog.h"
#include "utils/filesystemtools/FileHelper.h"
#include "resource/ResourceManager.h"

namespace Ship {
FolderArchive::FolderArchive(const std::string& archivePath) : Archive(archivePath) {
    mArchiveBasePath = archivePath + "/";
}

Ship::FolderArchive::~FolderArchive() {
    SPDLOG_TRACE("destruct folderarchive: {}", GetPath());
}

bool FolderArchive::Open() {

    auto fileEntries = Directory::ListFiles(mArchiveBasePath);

    for (auto i = 0; i < fileEntries.size(); i++) {
        auto filePath = StringHelper::Split(fileEntries[i], mArchiveBasePath)[1];
        IndexFile(filePath);
    }

    return true;
}

bool FolderArchive::Close() {
    return true;
}

bool FolderArchive::WriteFile(const std::string& filename, const std::vector<uint8_t>& data) {
    Ship::FileHelper::WriteAllBytes(mArchiveBasePath + filename, data);
    return true;
}

std::shared_ptr<File> Ship::FolderArchive::LoadFile(const std::string& filePath) {
    return LoadFileRaw(filePath);
}

std::shared_ptr<File> Ship::FolderArchive::LoadFile(uint64_t hash) {
    const std::string& filePath =
        *Context::GetInstance()->GetResourceManager()->GetArchiveManager()->HashToString(hash);

    return LoadFileRaw(filePath);
}

std::shared_ptr<File> FolderArchive::LoadFileRaw(const std::string& filePath) {
    if (Ship::FileHelper::Exists(mArchiveBasePath + filePath)) {
        auto data = Ship::FileHelper::ReadAllBytes(mArchiveBasePath + filePath);
        auto fileToLoad = std::make_shared<File>();

        fileToLoad->Buffer = std::make_shared<std::vector<char>>(data.size());
        memcpy(fileToLoad->Buffer->data(), data.data(), data.size());

        fileToLoad->IsLoaded = true;

        return fileToLoad;
    } else {
        return nullptr;
    }
}

std::shared_ptr<File> FolderArchive::LoadFileRaw(uint64_t hash) {
    const std::string& filePath =
        *Context::GetInstance()->GetResourceManager()->GetArchiveManager()->HashToString(hash);

    return LoadFileRaw(filePath);
}
} // namespace Ship
