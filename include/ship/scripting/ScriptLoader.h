#pragma once

#include "ship/resource/archive/Archive.h"
#include "ship/scripting/LibraryLoader.h"

#include <unordered_map>
#include <string>
#include <optional>
#include <functional>

namespace Ship {

enum class SafeLevel { DISABLE_SCRIPTS, ONLY_TRUSTED_SCRIPTS, WARN_UNTRUSTED_SCRIPTS, ALLOW_ALL_SCRIPTS };

class ScriptLoader {
  public:
    ScriptLoader(const std::unordered_map<std::string, std::string>& compileDefines, const uint32_t codeVersion,
                 const std::string& buildOptions, const std::vector<std::string>& includePaths,
                 const std::vector<std::string>& libraryPaths, const std::vector<std::string>& libraries)
        : mCodeVersion(codeVersion), mBuildOptions(buildOptions), mIncludePaths(includePaths),
          mLibraryPaths(libraryPaths), mLibraries(libraries), mCompileDefines(compileDefines) {
    }

    void Compile(const std::shared_ptr<Archive>& archive);
    void
    CompileAll(const std::optional<std::function<void(const std::shared_ptr<Archive>&)>>& preCallback = std::nullopt,
               const std::optional<std::function<void()>>& postCallback = std::nullopt);
    void LoadAll();
    void UnloadAll();

    void* GetFunction(const std::string& name, const std::string& function);
    std::vector<std::string> GetLoadersInDependencyOrder() const;

    void SetSafeLevel(SafeLevel level);

  private:
    uint32_t mCodeVersion;
    SafeLevel mSafeLevel = SafeLevel::WARN_UNTRUSTED_SCRIPTS;
    std::string mBuildOptions = "-g -Wl";
    std::vector<std::string> mIncludePaths;
    std::vector<std::string> mLibraryPaths;
    std::vector<std::string> mLibraries;
    std::unordered_map<std::string, std::string> mCompileDefines;
    std::unordered_map<std::string, Scripting::LibraryLoader> mLoadedScripts;
    std::vector<std::shared_ptr<Archive>> mLoadedArchives;
};

} // namespace Ship