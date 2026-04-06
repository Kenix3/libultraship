#pragma once

#include "ship/resource/archive/Archive.h"
#include "ship/scripting/ScriptLoader.h"

#include <unordered_map>
#include <string>
#include <optional>
#include <functional>

namespace Ship {

enum class SafeLevel { ONLY_TRUSTED_SCRIPTS, WARN_UNTRUSTED_SCRIPTS, ALLOW_ALL_SCRIPTS };

class ScriptSystem {
  public:
    ScriptSystem(const std::unordered_map<std::string, std::string>& compileDefines, const uint32_t codeVersion)
        : mCodeVersion(codeVersion), mCompileDefines(compileDefines) {
    }

    void Compile(const std::shared_ptr<Archive>& archive);
    void CompileAll(std::optional<std::function<void(const std::shared_ptr<Archive>&)>> pre_callback = std::nullopt,
                    std::optional<std::function<void()>> post_callback = std::nullopt);
    void LoadAll();
    void UnloadAll();
    void* GetFunction(const std::string& name, const std::string& function);
    std::vector<ScriptLoader*> GetLoadersInDependencyOrder();

  private:
    uint32_t mCodeVersion;
    std::unordered_map<std::string, std::string> mCompileDefines;
    std::unordered_map<std::string, ScriptLoader> mLoadedScripts;
};

} // namespace Ship