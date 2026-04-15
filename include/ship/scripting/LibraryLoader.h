#pragma once

#include <string>
#include <vector>

namespace Ship::Scripting {
typedef void (*LibraryFunc_t)(void);
typedef void* LibraryHandle_t;

class LibraryLoader {
  public:
    LibraryLoader() : mHandle(nullptr), mTempFile("") {
    }

    std::string GenerateTempFile();
    void Init(const std::string& path);
    void* GetFunction(const std::string& name);
    void Unload();

  private:
    LibraryHandle_t mHandle;
    std::string mTempFile;
};
} // namespace Ship::Scripting