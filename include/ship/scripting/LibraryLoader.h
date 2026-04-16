#pragma once

#include <string>
#include <vector>

namespace Ship::Scripting {

/** @brief Function pointer type for library entry points with no parameters and no return value. */
typedef void (*LibraryFunc_t)(void);

/** @brief Opaque handle to a dynamically loaded shared library. */
typedef void* LibraryHandle_t;

/**
 * @brief Loads and manages a single shared library (DLL / .so / .dylib) at runtime.
 *
 * LibraryLoader copies a compiled library to a temporary file, loads it via the
 * platform's dynamic-linker API, and resolves exported symbols by name. This is
 * used by the scripting subsystem to hot-reload compiled script modules.
 */
class LibraryLoader {
  public:
    /**
     * @brief Constructs an unloaded LibraryLoader with no associated library.
     */
    LibraryLoader() : mHandle(nullptr), mTempFile("") {
    }

    /**
     * @brief Creates a temporary file path suitable for holding a library copy.
     * @return Absolute path to the generated temporary file.
     */
    std::string GenerateTempFile();

    /**
     * @brief Copies the library at @p path to a temporary file and loads it.
     * @param path Filesystem path to the shared library to load.
     */
    void Init(const std::string& path);

    /**
     * @brief Resolves an exported symbol from the loaded library.
     * @param name The symbol name to look up.
     * @return Pointer to the symbol, or nullptr if not found or no library is loaded.
     */
    void* GetFunction(const std::string& name);

    /**
     * @brief Unloads the library and removes the temporary file.
     */
    void Unload();

  private:
    LibraryHandle_t mHandle;
    std::string mTempFile;
};
} // namespace Ship::Scripting