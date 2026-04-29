#pragma once

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "PathHelper.h"
#include "Directory.h"

namespace Ship {
/**
 * @brief Utility class providing static helpers for reading and writing files on disk.
 *
 * Similar to DiskFile but lives inside the Ship namespace. All methods are static
 * and operate directly on filesystem paths.
 */
class FileHelper {
  public:
    /**
     * @brief Checks whether a file exists and is readable.
     * @param filePath Path to the file to check.
     * @return true if the file can be opened for reading, false otherwise.
     */
    static bool Exists(const fs::path& filePath) {
        std::ifstream file(filePath, std::ios::in | std::ios::binary | std::ios::ate);
        return file.good();
    }

    /**
     * @brief Reads the entire contents of a file into a byte vector.
     * @param filePath Path to the file to read.
     * @return Vector containing every byte of the file, or an empty vector if the file cannot be opened.
     */
    static std::vector<uint8_t> ReadAllBytes(const fs::path& filePath) {
        std::ifstream file(filePath, std::ios::in | std::ios::binary | std::ios::ate);

        if (!file) {
            return std::vector<uint8_t>();
        }

        int32_t fileSize = (int32_t)file.tellg();
        file.seekg(0);
        char* data = nullptr;
        std::vector<uint8_t> result;

        try {
            data = new char[fileSize];
            file.read(data, fileSize);
            result = std::vector<uint8_t>(data, data + fileSize);
        } catch (const std::exception& e) {
            delete[] data;
            throw e;
        }

        delete[] data;

        return result;
    };

    /**
     * @brief Reads the entire contents of a file as a text string.
     * @param filePath Path to the file to read.
     * @return String containing the full file contents.
     */
    static std::string ReadAllText(const fs::path& filePath) {
        std::ifstream file(filePath, std::ios::in | std::ios::binary | std::ios::ate);
        int32_t fileSize = (int32_t)file.tellg();
        file.seekg(0);
        char* data = nullptr;
        std::string str;

        try {
            data = new char[fileSize + 1];
            memset(data, 0, fileSize + 1);
            file.read(data, fileSize);
            str = std::string((const char*)data);
        } catch (const std::exception& e) {
            delete[] data;
            throw e;
        }

        delete[] data;

        return str;
    };

    /**
     * @brief Reads a file and splits it into lines.
     * @param filePath Path to the file to read.
     * @return Vector of strings, one per line.
     */
    static std::vector<std::string> ReadAllLines(const fs::path& filePath) {
        std::string text = ReadAllText(filePath);
        std::vector<std::string> lines = StringHelper::Split(text, "\n");

        return lines;
    };

    /**
     * @brief Writes a byte vector to a file, overwriting any existing content.
     * @note Creates parent directories if they do not already exist.
     * @param filePath Path to the file to write.
     * @param data     Bytes to write.
     */
    static void WriteAllBytes(const fs::path& filePath, const std::vector<uint8_t>& data) {
        if (!Directory::Exists(PathHelper::GetDirectoryName(filePath))) {
            Directory::MakeDirectory(PathHelper::GetDirectoryName(filePath).string());
        }
        std::ofstream file(filePath, std::ios::binary);
        file.write((char*)data.data(), data.size());
    };

    /**
     * @brief Writes a char vector to a file, overwriting any existing content.
     * @note Creates parent directories if they do not already exist.
     * @param filePath Path to the file to write.
     * @param data     Characters to write.
     */
    static void WriteAllBytes(const std::string& filePath, const std::vector<char>& data) {
        if (!Directory::Exists(PathHelper::GetDirectoryName(filePath))) {
            Directory::MakeDirectory(PathHelper::GetDirectoryName(filePath).string());
        }

        std::ofstream file(filePath, std::ios::binary);
        file.write((char*)data.data(), data.size());
    };

    /**
     * @brief Writes raw data to a file from a pointer and size, overwriting any existing content.
     * @note Creates parent directories if they do not already exist.
     * @param filePath Path to the file to write.
     * @param data     Pointer to the data to write.
     * @param dataSize Number of bytes to write.
     */
    static void WriteAllBytes(const std::string& filePath, const char* data, int dataSize) {
        if (!Directory::Exists(PathHelper::GetDirectoryName(filePath))) {
            Directory::MakeDirectory(PathHelper::GetDirectoryName(filePath).string());
        }
        std::ofstream file(filePath, std::ios::binary);
        file.write((char*)data, dataSize);
    };

    /**
     * @brief Writes a string to a file as text, overwriting any existing content.
     * @param filePath Path to the file to write.
     * @param text     String to write.
     */
    static void WriteAllText(const fs::path& filePath, const std::string& text) {
        std::ofstream file(filePath, std::ios::out | std::ios::binary);
        file.write(text.c_str(), text.size());
    }
};
} // namespace Ship