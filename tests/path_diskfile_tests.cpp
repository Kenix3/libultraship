#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "ship/utils/filesystemtools/Path.h"
#include "ship/utils/filesystemtools/DiskFile.h"
#include "ship/utils/filesystemtools/Directory.h"

namespace fs = std::filesystem;

// ============================================================
// Path::GetFileName
// ============================================================

TEST(PathClass, GetFileNameBasicPath) {
    EXPECT_EQ(Path::GetFileName("/some/dir/file.txt"), "file.txt");
}

TEST(PathClass, GetFileNameNoDirectory) {
    EXPECT_EQ(Path::GetFileName("file.txt"), "file.txt");
}

TEST(PathClass, GetFileNameNoExtension) {
    EXPECT_EQ(Path::GetFileName("/dir/filename"), "filename");
}

TEST(PathClass, GetFileNameTrailingSlash) {
    // A trailing slash means the filename is empty
    EXPECT_EQ(Path::GetFileName("/dir/"), "");
}

// ============================================================
// Path::GetFileNameWithoutExtension
// ============================================================

TEST(PathClass, GetStemBasic) {
    EXPECT_EQ(Path::GetFileNameWithoutExtension("/dir/file.txt"), "file");
}

TEST(PathClass, GetStemMultipleDots) {
    EXPECT_EQ(Path::GetFileNameWithoutExtension("/dir/archive.tar.gz"), "archive.tar");
}

TEST(PathClass, GetStemNoExtension) {
    EXPECT_EQ(Path::GetFileNameWithoutExtension("/dir/filename"), "filename");
}

// ============================================================
// Path::GetFileNameExtension
// ============================================================

TEST(PathClass, GetExtensionBasic) {
    EXPECT_EQ(Path::GetFileNameExtension("file.txt"), ".txt");
}

TEST(PathClass, GetExtensionMultipleDots) {
    // Returns from the last dot to end
    EXPECT_EQ(Path::GetFileNameExtension("archive.tar.gz"), ".gz");
}

TEST(PathClass, GetExtensionBin) {
    EXPECT_EQ(Path::GetFileNameExtension("link.bin"), ".bin");
}

// ============================================================
// Path::GetPath (removes filename segments that contain a dot)
// ============================================================

TEST(PathClass, GetPathFiltersOutFileSegment) {
    fs::path result = Path::GetPath("textures/player/link.png");
    EXPECT_EQ(result, fs::path("textures/player"));
}

TEST(PathClass, GetPathPureDirectoryUnchanged) {
    fs::path result = Path::GetPath("assets/textures");
    EXPECT_EQ(result, fs::path("assets/textures"));
}

TEST(PathClass, GetPathSingleFileReturnsEmpty) {
    fs::path result = Path::GetPath("file.txt");
    EXPECT_TRUE(result.empty());
}

TEST(PathClass, GetPathMultiLevelWithFile) {
    fs::path result = Path::GetPath("a/b/c/file.bin");
    EXPECT_EQ(result, fs::path("a/b/c"));
}

// ============================================================
// Path::GetDirectoryName
// ============================================================

TEST(PathClass, GetDirectoryNameBasic) {
    EXPECT_EQ(Path::GetDirectoryName("/some/dir/file.txt"), fs::path("/some/dir"));
}

TEST(PathClass, GetDirectoryNameRoot) {
    EXPECT_EQ(Path::GetDirectoryName("/file.txt"), fs::path("/"));
}

TEST(PathClass, GetDirectoryNameRelative) {
    EXPECT_EQ(Path::GetDirectoryName("dir/file.txt"), fs::path("dir"));
}

// ============================================================
// DiskFile test fixture — temp directory
// ============================================================

class DiskFileTest : public ::testing::Test {
  protected:
    fs::path mTestDir;
    fs::path mTestFile;

    void SetUp() override {
        mTestDir = fs::temp_directory_path() / "lus_diskfile_test";
        fs::remove_all(mTestDir);
        Directory::MakeDirectory(mTestDir.string());
        mTestFile = mTestDir / "test.bin";
    }

    void TearDown() override {
        fs::remove_all(mTestDir);
    }
};

// ============================================================
// DiskFile::Exists
// ============================================================

TEST_F(DiskFileTest, ExistsReturnsFalseForMissing) {
    EXPECT_FALSE(DiskFile::Exists(mTestDir / "missing.bin"));
}

TEST_F(DiskFileTest, ExistsReturnsTrueAfterWrite) {
    std::vector<uint8_t> data = { 1, 2, 3 };
    DiskFile::WriteAllBytes(mTestFile, data);
    EXPECT_TRUE(DiskFile::Exists(mTestFile));
}

// ============================================================
// DiskFile::WriteAllBytes / ReadAllBytes (uint8 vector)
// ============================================================

TEST_F(DiskFileTest, WriteReadAllBytesUint8RoundTrip) {
    std::vector<uint8_t> data = { 0xDE, 0xAD, 0xBE, 0xEF };
    DiskFile::WriteAllBytes(mTestFile, data);
    auto read = DiskFile::ReadAllBytes(mTestFile);
    EXPECT_EQ(read, data);
}

TEST_F(DiskFileTest, ReadAllBytesEmptyFile) {
    // Write empty file
    { std::ofstream f(mTestFile, std::ios::binary); }
    auto data = DiskFile::ReadAllBytes(mTestFile);
    EXPECT_TRUE(data.empty());
}

TEST_F(DiskFileTest, ReadAllBytesMissingFileReturnsEmpty) {
    auto data = DiskFile::ReadAllBytes(mTestDir / "nonexistent.bin");
    EXPECT_TRUE(data.empty());
}

// ============================================================
// DiskFile::WriteAllBytes / ReadAllBytes (char vector overload)
// ============================================================

TEST_F(DiskFileTest, WriteReadAllBytesCharVectorRoundTrip) {
    std::vector<char> data = { 'h', 'e', 'l', 'l', 'o' };
    DiskFile::WriteAllBytes(mTestFile.string(), data);
    auto read = DiskFile::ReadAllBytes(mTestFile);
    ASSERT_EQ(read.size(), data.size());
    for (size_t i = 0; i < data.size(); i++) {
        EXPECT_EQ(static_cast<char>(read[i]), data[i]);
    }
}

// ============================================================
// DiskFile::WriteAllBytes raw pointer overload
// ============================================================

TEST_F(DiskFileTest, WriteReadRawPointerRoundTrip) {
    const char payload[] = "rawdata";
    DiskFile::WriteAllBytes(mTestFile.string(), payload, static_cast<int>(sizeof(payload) - 1));
    auto read = DiskFile::ReadAllBytes(mTestFile);
    ASSERT_EQ(read.size(), sizeof(payload) - 1);
    EXPECT_EQ(std::string(reinterpret_cast<char*>(read.data()), read.size()), "rawdata");
}

TEST_F(DiskFileTest, WriteAllBytesUint8CreatesParentIfMissing) {
    fs::path nested = mTestDir / "sub" / "data.bin";
    std::vector<uint8_t> data = { 0x01, 0x02 };
    DiskFile::WriteAllBytes(nested, data);
    EXPECT_TRUE(DiskFile::Exists(nested));
}

TEST_F(DiskFileTest, WriteAllBytesCharVecCreatesParentIfMissing) {
    fs::path nested = mTestDir / "sub2" / "data.bin";
    std::vector<char> data = { 'a', 'b' };
    DiskFile::WriteAllBytes(nested.string(), data);
    EXPECT_TRUE(DiskFile::Exists(nested));
}

TEST_F(DiskFileTest, WriteAllBytesRawPtrCreatesParentIfMissing) {
    fs::path nested = mTestDir / "sub3" / "data.bin";
    const char payload[] = "hi";
    DiskFile::WriteAllBytes(nested.string(), payload, static_cast<int>(sizeof(payload) - 1));
    EXPECT_TRUE(DiskFile::Exists(nested));
}

// ============================================================
// DiskFile::WriteAllText / ReadAllText
// ============================================================

TEST_F(DiskFileTest, WriteReadAllTextRoundTrip) {
    std::string content = "Hello, World!\nLine 2\n";
    DiskFile::WriteAllText(mTestFile, content);
    std::string result = DiskFile::ReadAllText(mTestFile);
    EXPECT_EQ(result, content);
}

TEST_F(DiskFileTest, WriteAllTextCreatesParentIfMissing) {
    fs::path nested = mTestDir / "sub" / "data.txt";
    DiskFile::WriteAllText(nested, "test");
    EXPECT_TRUE(DiskFile::Exists(nested));
}

// ============================================================
// DiskFile::ReadAllLines
// ============================================================

TEST_F(DiskFileTest, ReadAllLinesProducesCorrectTokens) {
    std::string content = "line1\nline2\nline3\n";
    DiskFile::WriteAllText(mTestFile, content);
    auto lines = DiskFile::ReadAllLines(mTestFile);
    ASSERT_GE(lines.size(), 3u);
    EXPECT_EQ(lines[0], "line1");
    EXPECT_EQ(lines[1], "line2");
    EXPECT_EQ(lines[2], "line3");
}

TEST_F(DiskFileTest, ReadAllLinesSingleLine) {
    DiskFile::WriteAllText(mTestFile, "only");
    auto lines = DiskFile::ReadAllLines(mTestFile);
    ASSERT_GE(lines.size(), 1u);
    EXPECT_EQ(lines[0], "only");
}
