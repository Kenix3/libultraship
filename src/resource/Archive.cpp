#include "Archive.h"
#include "Resource.h"
#include "OtrFile.h"
#include "core/Window.h"
#include "resource/ResourceMgr.h"
#include <spdlog/spdlog.h>
#include "Utils/StringHelper.h"
#include <StrHash64.h>
#include <filesystem>
#include "binarytools/BinaryReader.h"
#include "binarytools/MemoryStream.h"
#include "binarytools/FileHelper.h"

#ifdef __SWITCH__
#include "port/switch/SwitchImpl.h"
#endif

namespace Ship {
Archive::Archive(const std::string& mainPath, bool enableWriting)
    : Archive(mainPath, "", std::unordered_set<uint32_t>(), enableWriting) {
    mMainMpq = nullptr;
}

Archive::Archive(const std::string& mainPath, const std::string& patchesPath,
                 const std::unordered_set<uint32_t>& validHashes, bool enableWriting, bool generateCrcMap)
    : mMainPath(mainPath), mPatchesPath(patchesPath), mOtrArchives({}), mValidHashes(validHashes) {
    mMainMpq = nullptr;
    Load(enableWriting, generateCrcMap);
}

Archive::Archive(const std::vector<std::string>& fileList, const std::unordered_set<uint32_t>& validHashes,
                 bool enableWriting, bool generateCrcMap)
    : mOtrArchives(fileList), mValidHashes(validHashes) {
    mMainMpq = nullptr;
    Load(enableWriting, generateCrcMap);
}

Archive::~Archive() {
    Unload();
}

bool Archive::IsMainMPQValid() {
    return mMainMpq != nullptr;
}

std::shared_ptr<Archive> Archive::CreateArchive(const std::string& archivePath, int fileCapacity) {
    auto archive = std::make_shared<Archive>(archivePath, true);

    TCHAR* fileName = new TCHAR[archivePath.size() + 1];
    fileName[archivePath.size()] = 0;
    std::copy(archivePath.begin(), archivePath.end(), fileName);

    bool success = SFileCreateArchive(fileName, MPQ_CREATE_LISTFILE | MPQ_CREATE_ATTRIBUTES | MPQ_CREATE_ARCHIVE_V2,
                                      fileCapacity, &archive->mMainMpq);
    int32_t error = GetLastError();

    delete[] fileName;

    if (success) {
        archive->mMpqHandles[archivePath] = archive->mMainMpq;
        return archive;
    } else {
        SPDLOG_ERROR("({}) We tried to create an archive, but it has fallen and cannot get up.", error);
        return nullptr;
    }
}

std::shared_ptr<OtrFile> Archive::LoadFileFromHandle(const std::string& filePath, bool includeParent,
                                                     HANDLE mpqHandle) {
    HANDLE fileHandle = NULL;

    std::shared_ptr<OtrFile> fileToLoad = std::make_shared<OtrFile>();
    fileToLoad->Path = filePath;

    if (mpqHandle == nullptr) {
        mpqHandle = mMainMpq;
    }

#if _DEBUG
    if (FileHelper::Exists("TestData/" + filePath)) {
        auto byteData = FileHelper::ReadAllBytes("TestData/" + filePath);
        fileToLoad->Buffer.resize(byteData.size() + 1);
        memcpy(fileToLoad->Buffer.data(), byteData.data(), byteData.size() + 1);

        // Throw in a null terminator at the end incase we're loading a text file...
        fileToLoad->Buffer[byteData.size()] = '\0';

        fileToLoad->Parent = includeParent ? shared_from_this() : nullptr;
        fileToLoad->IsLoaded = true;
    } else {
#endif
        bool attempt = SFileOpenFileEx(mpqHandle, filePath.c_str(), 0, &fileHandle);

        if (!attempt) {
            SPDLOG_ERROR("({}) Failed to open file {} from mpq archive  {}.", GetLastError(), filePath, mMainPath);
            return nullptr;
        }

        DWORD fileSize = SFileGetFileSize(fileHandle, 0);
        fileToLoad->Buffer.resize(fileSize);
        DWORD countBytes;

        if (!SFileReadFile(fileHandle, fileToLoad->Buffer.data(), fileSize, &countBytes, NULL)) {
            SPDLOG_ERROR("({}) Failed to read file {} from mpq archive {}", GetLastError(), filePath, mMainPath);
            if (!SFileCloseFile(fileHandle)) {
                SPDLOG_ERROR("({}) Failed to close file {} from mpq after read failure in archive {}", GetLastError(),
                             filePath, mMainPath);
            }
            return nullptr;
        }

        if (!SFileCloseFile(fileHandle)) {
            SPDLOG_ERROR("({}) Failed to close file {} from mpq archive {}", GetLastError(), filePath, mMainPath);
        }

        fileToLoad->Parent = includeParent ? shared_from_this() : nullptr;
        fileToLoad->IsLoaded = true;
#if _DEBUG
    }
#endif

    return fileToLoad;
}

std::shared_ptr<OtrFile> Archive::LoadFile(const std::string& filePath, bool includeParent) {
    return LoadFileFromHandle(filePath, includeParent, nullptr);
}

bool Archive::AddFile(const std::string& path, uintptr_t fileData, DWORD fileSize) {
    HANDLE hFile;
#ifdef _WIN32
    SYSTEMTIME sysTime;
    GetSystemTime(&sysTime);
    FILETIME t;
    SystemTimeToFileTime(&sysTime, &t);
    ULONGLONG theTime = static_cast<uint64_t>(t.dwHighDateTime) << (sizeof(t.dwHighDateTime) * 8) | t.dwLowDateTime;
#else
    time_t theTime;
    time(&theTime);
#endif

    std::string updatedPath = path;

    StringHelper::ReplaceOriginal(updatedPath, "\\", "/");

    if (!SFileCreateFile(mMainMpq, updatedPath.c_str(), theTime, fileSize, 0, MPQ_FILE_COMPRESS, &hFile)) {
        SPDLOG_ERROR("({}) Failed to create file of {} bytes {} in archive {}", GetLastError(), fileSize, updatedPath,
                     mMainPath);
        return false;
    }

    if (!SFileWriteFile(hFile, (void*)fileData, fileSize, MPQ_COMPRESSION_ZLIB)) {
        SPDLOG_ERROR("({}) Failed to write {} bytes to {} in archive {}", GetLastError(), fileSize, updatedPath,
                     mMainPath);
        if (!SFileCloseFile(hFile)) {
            SPDLOG_ERROR("({}) Failed to close file {} after write failure in archive {}", GetLastError(), updatedPath,
                         mMainPath);
        }
        return false;
    }

    if (!SFileFinishFile(hFile)) {
        SPDLOG_ERROR("({}) Failed to finish file {} in archive {}", GetLastError(), updatedPath, mMainPath);
        if (!SFileCloseFile(hFile)) {
            SPDLOG_ERROR("({}) Failed to close file {} after finish failure in archive {}", GetLastError(), updatedPath,
                         mMainPath);
        }
        return false;
    }
    // SFileFinishFile already frees the handle, so no need to close it again.

    mAddedFiles.push_back(updatedPath);
    mHashes[CRC64(updatedPath.c_str())] = updatedPath;

    return true;
}

bool Archive::RemoveFile(const std::string& path) {
    // TODO: Notify the resource manager and child Files

    if (!SFileRemoveFile(mMainMpq, path.c_str(), 0)) {
        SPDLOG_ERROR("({}) Failed to remove file {} in archive {}", GetLastError(), path, mMainPath);
        return false;
    }

    return true;
}

bool Archive::RenameFile(const std::string& oldPath, const std::string& newPath) {
    // TODO: Notify the resource manager and child Files

    if (!SFileRenameFile(mMainMpq, oldPath.c_str(), newPath.c_str())) {
        SPDLOG_ERROR("({}) Failed to rename file {} to {} in archive {}", GetLastError(), oldPath, newPath, mMainPath);
        return false;
    }

    return true;
}

std::vector<SFILE_FIND_DATA> Archive::ListFiles(const std::string& searchMask) const {
    auto fileList = std::vector<SFILE_FIND_DATA>();
    SFILE_FIND_DATA findContext;
    for (auto& path : mMpqHandles) {
        HANDLE hFind;

        hFind = SFileFindFirstFile(path.second, searchMask.c_str(), &findContext, nullptr);
        // if (hFind && GetLastError() != ERROR_NO_MORE_FILES) {
        if (hFind != nullptr) {
            fileList.push_back(findContext);

            bool fileFound;
            do {
                fileFound = SFileFindNextFile(hFind, &findContext);

                if (fileFound) {
                    fileList.push_back(findContext);
                } else if (!fileFound && GetLastError() != ERROR_NO_MORE_FILES)
                // else if (!fileFound)
                {
                    SPDLOG_ERROR("({}), Failed to search with mask {} in archive {}", GetLastError(), searchMask,
                                 mMainPath);
                    if (!SListFileFindClose(hFind)) {
                        SPDLOG_ERROR("({}) Failed to close file search {} after failure in archive {}", GetLastError(),
                                     searchMask, mMainPath);
                    }
                    return fileList;
                }
            } while (fileFound);
        } else if (GetLastError() != ERROR_NO_MORE_FILES) {
            SPDLOG_ERROR("({}), Failed to search with mask {} in archive {}", GetLastError(), searchMask, mMainPath);
            return fileList;
        }

        if (hFind != nullptr) {
            if (!SFileFindClose(hFind)) {
                SPDLOG_ERROR("({}) Failed to close file search {} in archive {}", GetLastError(), searchMask,
                             mMainPath);
            }
        }
    }

    return fileList;
}

bool Archive::HasFile(const std::string& filename) const {
    bool result = false;
    auto start = std::chrono::steady_clock::now();

    auto lst = ListFiles(filename);

    for (const auto& item : lst) {
        if (item.cFileName == filename) {
            result = true;
            break;
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    return result;
}

const std::string* Archive::HashToString(uint64_t hash) const {
    auto it = mHashes.find(hash);
    return it != mHashes.end() ? &it->second : nullptr;
}

bool Archive::Load(bool enableWriting, bool generateCrcMap) {
    return LoadMainMPQ(enableWriting, generateCrcMap) && LoadPatchMPQs();
}

bool Archive::Unload() {
    bool success = true;
    for (const auto& mpqHandle : mMpqHandles) {
        if (!SFileCloseArchive(mpqHandle.second)) {
            SPDLOG_ERROR("({}) Failed to close mpq {}", GetLastError(), mpqHandle.first);
            success = false;
        }
    }

    mMainMpq = nullptr;

    return success;
}

bool Archive::LoadPatchMPQs() {
    // OTRTODO: We also want to periodically scan the patch directories for new MPQs. When new MPQs are found we will
    // load the contents to fileCache and then copy over to gameResourceAddresses
    if (mPatchesPath.length() > 0) {
        if (std::filesystem::is_directory(mPatchesPath)) {
            for (const auto& p : std::filesystem::recursive_directory_iterator(mPatchesPath)) {
                if (StringHelper::IEquals(p.path().extension().string(), ".otr") ||
                    StringHelper::IEquals(p.path().extension().string(), ".mpq")) {
                    SPDLOG_ERROR("Reading {} mpq patch", p.path().string());
                    if (!LoadPatchMPQ(p.path().string())) {
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

void Archive::GenerateCrcMap() {
    auto listFile = LoadFile("(listfile)", false);

    // Use std::string_view to avoid unnecessary string copies
    std::vector<std::string_view> lines =
        StringHelper::Split(std::string_view(listFile->Buffer.data(), listFile->Buffer.size()), "\n");

    for (size_t i = 0; i < lines.size(); i++) {
        // Use std::string_view to avoid unnecessary string copies
        std::string_view line = lines[i].substr(0, lines[i].length() - 1); // Trim \r
        std::string lineStr = std::string(line);

        // Not NULL terminated str
        uint64_t hash = ~crc64(line.data(), line.length());
        mHashes.emplace(hash, std::move(lineStr));
    }
}

bool Archive::ProcessOtrVersion(HANDLE mpqHandle) {
    auto t = LoadFileFromHandle("version", false, mpqHandle);
    if (t != nullptr && t->IsLoaded) {
        auto stream = std::make_shared<MemoryStream>(t->Buffer.data(), t->Buffer.size());
        auto reader = std::make_shared<BinaryReader>(stream);
        Ship::Endianness endianness = (Ship::Endianness)reader->ReadUByte();
        reader->SetEndianness(endianness);
        uint32_t version = reader->ReadUInt32();
        if (mValidHashes.empty() || mValidHashes.contains(version)) {
            PushGameVersion(version);
            return true;
        }
    }
    return false;
}

bool Archive::LoadMainMPQ(bool enableWriting, bool generateCrcMap) {
    HANDLE mpqHandle = NULL;
    if (mOtrArchives.empty()) {
        if (mMainPath.length() > 0) {
            if (std::filesystem::is_directory(mMainPath)) {
                for (const auto& p : std::filesystem::recursive_directory_iterator(mMainPath)) {
                    if (StringHelper::IEquals(p.path().extension().string(), ".otr")) {
                        SPDLOG_ERROR("Reading {} mpq", p.path().string());
                        mOtrArchives.push_back(p.path().string());
                    }
                }
            } else if (std::filesystem::is_regular_file(mMainPath)) {
                mOtrArchives.push_back(mMainPath);
            } else {
                SPDLOG_ERROR("The directory {} does not exist", mMainPath);
                return false;
            }
        } else {
            SPDLOG_ERROR("No OTR file list or Main Path provided.");
            return false;
        }
        if (mOtrArchives.empty()) {
            SPDLOG_ERROR("No OTR files present in {}", mMainPath);
            return false;
        }
    }
    bool baseLoaded = false;
    size_t i = 0;
    while (!baseLoaded && i < mOtrArchives.size()) {
#if defined(__SWITCH__) || defined(__WIIU__)
        std::string fullPath = mOtrArchives[i];
#else
        std::string fullPath = std::filesystem::absolute(mOtrArchives[i]).string();
#endif
        if (SFileOpenArchive(fullPath.c_str(), 0, enableWriting ? 0 : MPQ_OPEN_READ_ONLY, &mpqHandle)) {
            SPDLOG_INFO("Opened mpq file {}.", fullPath);
            mMainMpq = mpqHandle;
            mMainPath = fullPath;
            if (!ProcessOtrVersion(mMainMpq)) {
                SPDLOG_WARN("Attempted to load invalid OTR file {}", mOtrArchives[i]);
                SFileCloseArchive(mpqHandle);
                mMainMpq = nullptr;
            } else {
                mMpqHandles[fullPath] = mpqHandle;
                if (generateCrcMap) {
                    GenerateCrcMap();
                }
                baseLoaded = true;
            }
        }
        i++;
    }
    // If we exited the above loop without setting baseLoaded to true, then we've
    // attemtped to load all the OTRs available to us.
    if (!baseLoaded) {
        SPDLOG_ERROR("No valid OTR file was provided.");
        return false;
    }
    for (size_t j = i; j < mOtrArchives.size(); j++) {
#if defined(__SWITCH__) || defined(__WIIU__)
        std::string fullPath = mOtrArchives[j];
#else
        std::string fullPath = std::filesystem::absolute(mOtrArchives[j]).string();
#endif
        if (LoadPatchMPQ(fullPath, true)) {
            SPDLOG_INFO("({}) Patched in mpq file.", fullPath);
        }
        if (generateCrcMap) {
            GenerateCrcMap();
        }
    }

    return true;
}

bool Archive::LoadPatchMPQ(const std::string& path, bool validateVersion) {
    HANDLE patchHandle = NULL;
#if defined(__SWITCH__) || defined(__WIIU__)
    std::string fullPath = path;
#else
    std::string fullPath = std::filesystem::absolute(path).string();
#endif
    if (mMpqHandles.contains(fullPath)) {
        return true;
    }
    if (!SFileOpenArchive(fullPath.c_str(), 0, MPQ_OPEN_READ_ONLY, &patchHandle)) {
        SPDLOG_ERROR("({}) Failed to open patch mpq file {} while applying to {}.", GetLastError(), path, mMainPath);
        return false;
    } else {
        // We don't always want to validate the "version" file, only when we're loading standalone OTRs as patches
        // i.e. Ocarina of Time along with Master Quest.
        if (validateVersion) {
            if (!ProcessOtrVersion(patchHandle)) {
                SPDLOG_INFO("({}) Missing version file. Attempting to apply patch anyway.", path);
            }
        }
    }
    if (!SFileOpenPatchArchive(mMainMpq, fullPath.c_str(), "", 0)) {
        SPDLOG_ERROR("({}) Failed to apply patch mpq file {} to main mpq {}.", GetLastError(), path, mMainPath);
        return false;
    }

    mMpqHandles[fullPath] = patchHandle;

    return true;
}

std::vector<uint32_t> Archive::GetGameVersions() {
    return mGameVersions;
}

void Archive::PushGameVersion(uint32_t newGameVersion) {
    mGameVersions.push_back(newGameVersion);
}
} // namespace Ship
