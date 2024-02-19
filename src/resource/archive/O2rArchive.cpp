#include "O2rArchive.h"

#include "spdlog/spdlog.h"

namespace LUS {
O2rArchive::O2rArchive(const std::string& archivePath) : Archive(archivePath) {
}

O2rArchive::~O2rArchive() {
    SPDLOG_TRACE("destruct o2rarchive: {}", GetPath());
}

std::shared_ptr<File> O2rArchive::LoadFileRaw(uint64_t hash) {
    return nullptr;
}

std::shared_ptr<File> O2rArchive::LoadFileRaw(const std::string& filePath) {
    return nullptr;
}

bool O2rArchive::LoadRaw() {
    return false;
}

bool O2rArchive::UnloadRaw() {
    return false;
}
} // namespace LUS
