#pragma once

#include <string>
#include <vector>

namespace Ship {
typedef void (*ScriptFunc_t)(void);
typedef void* ScriptHandle_t;

class ScriptLoader {
  public:
    ScriptLoader() : mHandle(nullptr), mTempFile("") {
    }

    std::string GenerateTempFile();
    void Init(const std::string& path);
    void* GetFunction(const std::string& name);
    void Unload();

  private:
    ScriptHandle_t mHandle;
    std::string mTempFile;
};
} // namespace Ship