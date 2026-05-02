#pragma once

#ifdef ENABLE_SCRIPTING

#include "ship/resource/archive/Archive.h"
#include "ship/scripting/LibraryLoader.h"
#include "ship/Component.h"

#include <unordered_map>
#include <string>
#include <optional>
#include <functional>

namespace Ship {

/**
 * @brief Security level controlling which scripts are allowed to load and execute.
 */
enum class SafeLevel {
    DISABLE_SCRIPTS,        ///< No scripts are loaded or executed.
    ONLY_TRUSTED_SCRIPTS,   ///< Only scripts from signed/trusted archives are loaded.
    WARN_UNTRUSTED_SCRIPTS, ///< Untrusted scripts are loaded with a warning.
    ALLOW_ALL_SCRIPTS       ///< All scripts are loaded without restriction.
};

/**
 * @brief Manages compilation, loading, and lifetime of runtime scripts.
 *
 * ScriptLoader compiles C/C++ source files found in mounted archives using TCC
 * (Tiny C Compiler), then loads the resulting shared objects at runtime so their
 * exported functions can be called by the engine via GetFunction().
 *
 * **Required Context children (looked up at runtime):**
 * - **ResourceManager** — queried during script loading to access the ArchiveManager
 *   for enumerating script source files. ResourceManager must be added to the Context
 *   before any ScriptLoader load methods are called.
 */
class ScriptLoader : public Component {
  public:
    /**
     * @brief Constructs a ScriptLoader with the given compiler configuration.
     * @param compileDefines Preprocessor defines passed to the compiler.
     * @param codeVersion    Version tag embedded in compiled modules for compatibility checks.
     * @param buildOptions   Raw compiler flags string (e.g. "-g -Wl").
     * @param includePaths   Additional include search directories.
     * @param libraryPaths   Additional library search directories.
     * @param libraries      Libraries to link against compiled scripts.
     */
    ScriptLoader(const std::unordered_map<std::string, std::string>& compileDefines, const uint32_t codeVersion,
                 const std::string& buildOptions, const std::vector<std::string>& includePaths,
                 const std::vector<std::string>& libraryPaths, const std::vector<std::string>& libraries)
        : Component("ScriptLoader"), mCodeVersion(codeVersion), mBuildOptions(buildOptions),
          mIncludePaths(includePaths), mLibraryPaths(libraryPaths), mLibraries(libraries),
          mCompileDefines(compileDefines) {
    }

    /**
     * @brief Compiles script sources from a single archive.
     * @param archive The archive containing script source files to compile.
     */
    void Compile(const std::shared_ptr<Archive>& archive);

    /**
     * @brief Compiles scripts from all mounted archives.
     * @param preCallback  Optional callback invoked before each archive is compiled.
     * @param postCallback Optional callback invoked after all archives have been compiled.
     */
    void
    CompileAll(const std::optional<std::function<void(const std::shared_ptr<Archive>&)>>& preCallback = std::nullopt,
               const std::optional<std::function<void()>>& postCallback = std::nullopt);

    /**
     * @brief Loads all previously compiled script modules into the runtime.
     */
    void LoadAll();

    /**
     * @brief Unloads all currently loaded script modules.
     */
    void UnloadAll();

    /**
     * @brief Looks up a function exported by a loaded script module.
     * @param name     Name of the script module.
     * @param function Name of the exported function.
     * @return Opaque function pointer, or nullptr if the module or function is not found.
     */
    void* GetFunction(const std::string& name, const std::string& function);

    /**
     * @brief Returns the names of all loaded script modules sorted in dependency order.
     * @return Ordered vector of module names, with dependencies listed before dependents.
     */
    std::vector<std::string> GetLoadersInDependencyOrder() const;

    /**
     * @brief Sets the security level for script loading.
     * @param level The SafeLevel policy to enforce.
     */
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

#endif // ENABLE_SCRIPTING