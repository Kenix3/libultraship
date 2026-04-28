#pragma once

#include <iostream>
#include <stdexcept>
#include <string>
#include "../StringHelper.h"

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

/**
 * @brief Utility class providing static helpers for extracting components from filesystem paths.
 */
class Path {
  public:
    /**
     * @brief Returns the filename component of a path (including extension).
     * @param input Filesystem path.
     * @return Filename string (e.g. "file.txt" from "/dir/file.txt").
     */
    static std::string GetFileName(const fs::path& input) {
        // https://en.cppreference.com/w/cpp/filesystem/path/filename
        return input.filename().string();
    };

    /**
     * @brief Returns the filename without its extension (stem).
     * @param input Filesystem path.
     * @return Stem string (e.g. "file" from "/dir/file.txt").
     */
    static std::string GetFileNameWithoutExtension(const fs::path& input) {
        // https://en.cppreference.com/w/cpp/filesystem/path/stem
        return input.stem().string();
    };

    /**
     * @brief Returns the file extension including the leading dot.
     * @param input File path or name as a string.
     * @return Extension string (e.g. ".txt").
     * @throws std::out_of_range if @p input contains no '.' character.
     */
    static std::string GetFileNameExtension(const std::string& input) {
        auto pos = input.find_last_of(".");
        if (pos == std::string::npos) {
            throw std::out_of_range("GetFileNameExtension: no '.' found in \"" + input + "\"");
        }
        return input.substr(pos);
    };

    /**
     * @brief Extracts the directory portion of a slash-separated path, stripping segments that contain a dot.
     * @param input Slash-separated path string.
     * @return Filesystem path composed of only the directory segments.
     */
    static fs::path GetPath(const std::string& input) {
        std::vector<std::string> split = StringHelper::Split(input, "/");
        fs::path output;

        for (std::string str : split) {
            if (str.find_last_of(".") == std::string::npos)
                output /= str;
        }

        return output;
    };

    /**
     * @brief Returns the parent directory of the given path.
     * @param path Filesystem path.
     * @return Parent directory path.
     */
    static fs::path GetDirectoryName(const fs::path& path) {
        return path.parent_path();
    };
};
