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
 * LibraryLoader loads a compiled library from a caller-supplied path via the
 * platform's dynamic-linker API, and resolves exported symbols by name. The
 * caller is responsible for copying the library to a temporary file (via
 * GenerateTempFile()) before calling Init(). This is used by the scripting
 * subsystem to hot-reload compiled script modules.
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
     * @brief Loads the shared library at @p path using the platform dynamic linker.
     * @param path Filesystem path to the shared library to load.
     * @note On Linux and macOS the file at @p path is unlinked from the filesystem
     *       after a successful dlopen() call (the library stays loaded in memory).
     *       On Windows the file is left on disk until Unload() is called.
     */
    void Init(const std::string& path);

    /**
     * @brief Resolves an exported symbol from the loaded library.
     * @param name The symbol name to look up.
     * @return Pointer to the symbol, or nullptr if not found or no library is loaded.
     */
    void* GetFunction(const std::string& name);

    /**
     * @brief Unloads the library.
     * @note On Windows, also deletes the associated temporary file from disk.
     *       On Linux and macOS the temp file is already removed during Init().
     */
    void Unload();

  private:
    LibraryHandle_t mHandle;
    std::string mTempFile;
};
} // namespace Ship::Scripting