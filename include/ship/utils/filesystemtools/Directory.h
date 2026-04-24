#pragma once

#include <string>
#include <vector>
#include <iostream>

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#undef GetCurrentDirectory
#undef CreateDirectory

/**
 * @brief Utility class providing static helpers for common directory operations.
 *
 * Wraps std::filesystem calls for querying, creating, and listing directories.
 */
class Directory {
  public:
#ifndef PATH_HACK
    /**
     * @brief Returns the current working directory.
     * @return Absolute path string of the current working directory.
     */
    static std::string GetCurrentDirectory() {
        return fs::current_path().string();
    }
#endif

    /**
     * @brief Checks whether a directory (or file) exists at the given path.
     * @param path Filesystem path to check.
     * @return true if the path exists, false otherwise.
     */
    static bool Exists(const fs::path& path) {
        return fs::exists(path);
    }

    /**
     * @brief Creates a directory and any missing parent directories.
     *
     * Alias for CreateDirectory() to work around the Windows.h macro conflict.
     *
     * @param path Directory path to create.
     */
    static void MakeDirectory(const std::string& path) {
        CreateDirectory(path);
    }

    /**
     * @brief Creates a directory and any missing parent directories.
     * @param path Directory path to create.
     */
    static void CreateDirectory(const std::string& path) {
        try {
            fs::create_directories(path);
        } catch (...) {}
    }

    /**
     * @brief Recursively lists all files in a directory.
     * @param dir Path to the directory to enumerate.
     * @return Vector of absolute file paths found under @p dir; empty if the directory does not exist.
     */
    static std::vector<std::string> ListFiles(const std::string& dir) {
        std::vector<std::string> lst;

        if (Exists(dir)) {
            for (auto& p : fs::recursive_directory_iterator(dir)) {
                if (!p.is_directory())
                    lst.push_back(p.path().generic_string());
            }
        }

        return lst;
    }
};
