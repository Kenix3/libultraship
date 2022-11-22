#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace Ship {
class Archive;

struct OtrFile {
    std::shared_ptr<Archive> Parent;
    std::string Path;
    std::shared_ptr<char[]> Buffer;
    size_t BufferSize;
    bool IsLoaded = false;
    bool HasLoadError = false;
    std::condition_variable FileLoadNotifier;
    std::mutex FileLoadMutex;
};
} // namespace Ship
