#pragma once

#include "ship/resource/archive/Archive.h"
#include "ship/scripting/ScriptLoader.h"

#include <unordered_map>
#include <string>

namespace Ship {

enum class SafeLevel { ONLY_TRUSTED_SCRIPTS, WARN_UNTRUSTED_SCRIPTS, ALLOW_ALL_SCRIPTS };

class ScriptSystem {
  public:
    ScriptSystem(const std::unordered_map<std::string, std::string>& compileDefines, const uint32_t codeVersion)
        : mCodeVersion(codeVersion), mCompileDefines(compileDefines) {
    }

    void Load(const std::shared_ptr<Archive>& archive);
    void LoadAll();
    void UnloadAll();
    void* GetFunction(const std::string& name, const std::string& function);

  private:
    uint32_t mCodeVersion;
    std::unordered_map<std::string, std::string> mCompileDefines;
    std::unordered_map<std::string, ScriptLoader> mLoadedScripts;
};

} // namespace Ship