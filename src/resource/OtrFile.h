#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace Ship {
class Archive;

class OtrFile {
  public:
    std::shared_ptr<Archive> Parent;
    std::string Path;
    std::shared_ptr<char[]> Buffer;
    uint32_t BufferSize;
    bool IsLoaded = false;
    bool HasLoadError = false;
    std::condition_variable FileLoadNotifier;
    std::mutex FileLoadMutex;
};
} // namespace Ship
