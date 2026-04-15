#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

#include "ship/utils/filesystemtools/PathHelper.h"
#include "ship/utils/filesystemtools/FileHelper.h"
#include "ship/utils/filesystemtools/Directory.h"

namespace fs = std::filesystem;

// ============================================================
// PathHelper::GetFileName
// ============================================================

TEST(PathHelperGetFileName, ExtractsBaseName) {
    EXPECT_EQ(Ship::PathHelper::GetFileName("/some/dir/file.txt"), "file.txt");
}

TEST(PathHelperGetFileName, ExtractsBaseNameNoDir) {
    EXPECT_EQ(Ship::PathHelper::GetFileName("file.txt"), "file.txt");
}

TEST(PathHelperGetFileName, ExtractsBaseNameNoExtension) {
    EXPECT_EQ(Ship::PathHelper::GetFileName("/dir/filename"), "filename");
}

// ============================================================
// PathHelper::GetFileNameWithoutExtension
// ============================================================

TEST(PathHelperGetStem, RemovesExtension) {
    EXPECT_EQ(Ship::PathHelper::GetFileNameWithoutExtension("/dir/file.txt"), "file");
}

TEST(PathHelperGetStem, MultipleDotsKeepsLastAsExtension) {
    EXPECT_EQ(Ship::PathHelper::GetFileNameWithoutExtension("/dir/file.tar.gz"), "file.tar");
}

TEST(PathHelperGetStem, NoExtensionReturnsStem) {
    EXPECT_EQ(Ship::PathHelper::GetFileNameWithoutExtension("/dir/filename"), "filename");
}

// ============================================================
// PathHelper::GetFileNameExtension
// ============================================================

TEST(PathHelperGetExtension, ReturnsExtensionWithDot) {
    EXPECT_EQ(Ship::PathHelper::GetFileNameExtension("file.txt"), ".txt");
}

TEST(PathHelperGetExtension, ReturnsLastExtensionForMultipleDots) {
    EXPECT_EQ(Ship::PathHelper::GetFileNameExtension("file.tar.gz"), ".gz");
}

// ============================================================
// PathHelper::GetDirectoryName
// ============================================================

TEST(PathHelperGetDirectoryName, ReturnsParent) {
    fs::path p = "/some/dir/file.txt";
    EXPECT_EQ(Ship::PathHelper::GetDirectoryName(p), fs::path("/some/dir"));
}

TEST(PathHelperGetDirectoryName, RootParentOfTopLevel) {
    fs::path p = "/file.txt";
    EXPECT_EQ(Ship::PathHelper::GetDirectoryName(p), fs::path("/"));
}

// ============================================================
// PathHelper::GetPath
// ============================================================

TEST(PathHelperGetPath, FiltersOutFileSegments) {
    // Segments with a '.' are treated as files and excluded
    fs::path result = Ship::PathHelper::GetPath("textures/player/link.png");
    EXPECT_EQ(result, fs::path("textures/player"));
}

TEST(PathHelperGetPath, PureDirectoryPathIsUnchanged) {
    fs::path result = Ship::PathHelper::GetPath("assets/textures");
    EXPECT_EQ(result, fs::path("assets/textures"));
}

TEST(PathHelperGetPath, SingleFileNameReturnsEmpty) {
    // A single segment with a '.' is filtered out, leaving an empty path
    fs::path result = Ship::PathHelper::GetPath("file.txt");
    EXPECT_TRUE(result.empty());
}

// ============================================================
// Directory helper fixture — creates/cleans up a temp test dir
// ============================================================

class DirectoryTest : public ::testing::Test {
  protected:
    fs::path mTestDir;

    void SetUp() override {
        mTestDir = fs::temp_directory_path() / "lus_dir_test";
        fs::remove_all(mTestDir);
    }

    void TearDown() override {
        fs::remove_all(mTestDir);
    }
};

TEST_F(DirectoryTest, ExistsReturnsFalseForNonexistent) {
    EXPECT_FALSE(Directory::Exists(mTestDir));
}

TEST_F(DirectoryTest, CreateDirectoryMakesItExist) {
    Directory::CreateDirectory(mTestDir.string());
    EXPECT_TRUE(Directory::Exists(mTestDir));
}

TEST_F(DirectoryTest, MakeDirectoryAlsoWorks) {
    Directory::MakeDirectory(mTestDir.string());
    EXPECT_TRUE(Directory::Exists(mTestDir));
}

TEST_F(DirectoryTest, CreateDirectoryIsIdempotent) {
    Directory::CreateDirectory(mTestDir.string());
    EXPECT_NO_THROW(Directory::CreateDirectory(mTestDir.string()));
    EXPECT_TRUE(Directory::Exists(mTestDir));
}

TEST_F(DirectoryTest, ListFilesEmptyDirectory) {
    Directory::CreateDirectory(mTestDir.string());
    auto files = Directory::ListFiles(mTestDir.string());
    EXPECT_TRUE(files.empty());
}

TEST_F(DirectoryTest, ListFilesReturnsAllFiles) {
    Directory::CreateDirectory(mTestDir.string());
    // Create two files
    std::ofstream(mTestDir / "a.txt").close();
    std::ofstream(mTestDir / "b.txt").close();
    auto files = Directory::ListFiles(mTestDir.string());
    EXPECT_EQ(files.size(), 2u);
}

TEST_F(DirectoryTest, ListFilesIgnoresSubdirectories) {
    Directory::CreateDirectory(mTestDir.string());
    std::ofstream(mTestDir / "file.txt").close();
    Directory::CreateDirectory((mTestDir / "subdir").string());
    std::ofstream(mTestDir / "subdir" / "nested.txt").close();
    auto files = Directory::ListFiles(mTestDir.string());
    // Recursive iterator finds both files, not the directory itself
    EXPECT_EQ(files.size(), 2u);
}

TEST_F(DirectoryTest, ListFilesNonexistentDirectoryReturnsEmpty) {
    auto files = Directory::ListFiles((mTestDir / "doesnotexist").string());
    EXPECT_TRUE(files.empty());
}

TEST_F(DirectoryTest, GetCurrentDirectoryReturnsNonEmpty) {
    std::string cwd = Directory::GetCurrentDirectory();
    EXPECT_FALSE(cwd.empty());
}

// ============================================================
// FileHelper fixture
// ============================================================

class FileHelperTest : public ::testing::Test {
  protected:
    fs::path mTestDir;
    fs::path mTestFile;

    void SetUp() override {
        mTestDir = fs::temp_directory_path() / "lus_file_test";
        fs::remove_all(mTestDir);
        Directory::CreateDirectory(mTestDir.string());
        mTestFile = mTestDir / "test.bin";
    }

    void TearDown() override {
        fs::remove_all(mTestDir);
    }
};

TEST_F(FileHelperTest, ExistsReturnsFalseForMissingFile) {
    EXPECT_FALSE(Ship::FileHelper::Exists(mTestDir / "nothere.txt"));
}

TEST_F(FileHelperTest, ExistsReturnsTrueAfterWrite) {
    Ship::FileHelper::WriteAllBytes(mTestFile.string(),
                                    std::vector<char>{ 0x01, 0x02, 0x03 });
    EXPECT_TRUE(Ship::FileHelper::Exists(mTestFile));
}

TEST_F(FileHelperTest, WriteAndReadAllBytesRoundTrip) {
    std::vector<uint8_t> data = { 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE };
    Ship::FileHelper::WriteAllBytes(mTestFile, data);
    auto read = Ship::FileHelper::ReadAllBytes(mTestFile);
    EXPECT_EQ(read, data);
}

TEST_F(FileHelperTest, WriteAndReadAllBytesVectorCharOverload) {
    std::vector<char> data = { 'h', 'e', 'l', 'l', 'o' };
    Ship::FileHelper::WriteAllBytes(mTestFile.string(), data);
    auto read = Ship::FileHelper::ReadAllBytes(mTestFile);
    ASSERT_EQ(read.size(), data.size());
    for (size_t i = 0; i < data.size(); i++) {
        EXPECT_EQ(static_cast<char>(read[i]), data[i]);
    }
}

TEST_F(FileHelperTest, WriteAndReadAllBytesRawPointerOverload) {
    const char data[] = "rawdata";
    Ship::FileHelper::WriteAllBytes(mTestFile.string(), data, sizeof(data) - 1);
    auto read = Ship::FileHelper::ReadAllBytes(mTestFile);
    ASSERT_EQ(read.size(), sizeof(data) - 1);
    EXPECT_EQ(std::string(read.begin(), read.end()), "rawdata");
}

TEST_F(FileHelperTest, WriteAndReadAllTextRoundTrip) {
    std::string content = "Hello, World!\nLine 2\n";
    Ship::FileHelper::WriteAllText(mTestFile, content);
    std::string result = Ship::FileHelper::ReadAllText(mTestFile);
    EXPECT_EQ(result, content);
}

TEST_F(FileHelperTest, ReadAllLinesProducesCorrectCount) {
    std::string content = "line1\nline2\nline3\n";
    Ship::FileHelper::WriteAllText(mTestFile, content);
    auto lines = Ship::FileHelper::ReadAllLines(mTestFile);
    // Split on '\n': "line1", "line2", "line3", "" (trailing)
    ASSERT_GE(lines.size(), 3u);
    EXPECT_EQ(lines[0], "line1");
    EXPECT_EQ(lines[1], "line2");
    EXPECT_EQ(lines[2], "line3");
}

TEST_F(FileHelperTest, ReadAllBytesNonExistentFileReturnsEmpty) {
    auto data = Ship::FileHelper::ReadAllBytes(mTestDir / "missing.bin");
    EXPECT_TRUE(data.empty());
}

TEST_F(FileHelperTest, WriteAllBytesCreatesParentDirectory) {
    fs::path nested = mTestDir / "subdir" / "nested.bin";
    std::vector<char> data = { 0x01, 0x02 };
    Ship::FileHelper::WriteAllBytes(nested.string(), data);
    EXPECT_TRUE(Ship::FileHelper::Exists(nested));
}
