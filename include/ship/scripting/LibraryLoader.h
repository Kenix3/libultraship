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
 * platform's dynamic-linker API, and resolves exported symbols by name.
 *
 * Typical usage:
 *   1. Call GenerateTempFile() to allocate a writable temp path.
 *   2a. Call WriteToTempFile() to populate it from raw bytes, OR
 *   2b. Have an external tool (e.g. TCC) write the library to the path returned
 *       by GenerateTempFile() directly.
 *   3. Call Init() with that path to load the library into the process.
 *
 * This is used by the scripting subsystem to hot-reload compiled script modules.
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
     * @brief Writes raw library bytes to the temp file created by GenerateTempFile().
     * @param data Raw bytes of the shared library to write to disk.
     * @throws std::runtime_error if the temp file cannot be opened for writing.
     * @note GenerateTempFile() must be called before this method.
     */
    void WriteToTempFile(const std::vector<uint8_t>& data);

    /**
     * @brief Loads the shared library already written at @p path via the platform dynamic linker.
     * @param path Filesystem path to the shared library to load.  The file must
     *             already exist on disk (written by WriteToTempFile() or an
     *             external tool such as TCC); Init() does not copy or create it.
     * @note On Linux and macOS the file at @p path is unlinked from the
     *       filesystem immediately after a successful dlopen() call so that the
     *       temporary file is removed without waiting for Unload().  The library
     *       remains mapped in memory until Unload() is called.
     * @note On Windows the file is kept on disk and is deleted by Unload().
     * @throws std::runtime_error if the library cannot be loaded.
     */
    void Init(const std::string& path);

    /**
     * @brief Resolves an exported symbol from the loaded library.
     * @param name The symbol name to look up.
     * @return Pointer to the symbol, or nullptr if not found or no library is loaded.
     */
    void* GetFunction(const std::string& name);

    /**
     * @brief Unloads the library from memory and cleans up platform resources.
     * @note On Windows, FreeLibrary() is called and the associated temporary
     *       file is deleted from disk.
     * @note On Linux and macOS, dlclose() is not called (the handle is simply
     *       cleared); the temporary file was already unlinked by Init(), so no
     *       filesystem cleanup is needed here.
     */
    void Unload();

  private:
    LibraryHandle_t mHandle;
    std::string mTempFile;
};
} // namespace Ship::Scripting