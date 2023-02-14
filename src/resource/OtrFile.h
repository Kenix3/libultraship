#pragma once

#include <string>
#include <vector>
#include <memory>

namespace Ship {
class Archive;

struct OtrFile {
    std::shared_ptr<Archive> Parent;
    std::string Path;
    std::vector<char> Buffer;
    bool IsLoaded = false;
};
} // namespace Ship
