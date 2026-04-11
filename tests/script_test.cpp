#include <gtest/gtest.h>
#include <stdexcept>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include "ship/resource/File.h"
#include "ship/resource/archive/Archive.h"
#include "ship/scripting/ScriptSystem.h"

namespace fs = std::filesystem;

// ============================================================
// ScriptSystem Tests
// ============================================================

namespace Ship {
class RamArchive final : virtual public Archive {
  public:
    RamArchive() : Archive("ram://") {
    }

    bool Open() override {
        return true;
    }
    bool Close() override {
        return true;
    }
    bool WriteFile(const std::string&, const std::vector<uint8_t>&) override {
        return true;
    }
    std::shared_ptr<File> LoadFile(uint64_t hash) override {
        return nullptr;
    }

    std::shared_ptr<File> LoadFile(const std::string& filePath) override {
        auto file = std::make_shared<File>();
        std::string content;

        if (filePath == "manifest.json") {
            content = R"({
                "name": "Test Script",
                "code_version": 1,
                "main": "build.gen"
            })";
        } else if (filePath == "build.gen") {
            content = "test.c";
        } else if (filePath == "test.c") {
            content = R"(
                #define HM_API __attribute__((visibility("default")))
                static int status = -1;

                #ifdef __FIBx2__
                HM_API int fib(int n) {
                    if (n <= 1) return n * 2;
                    return 2 * n;
                }
                #else
                HM_API int fib(int n) {
                    if (n <= 1) return n;
                    return fib(n - 1) + fib(n - 2);
                }
                #endif
                HM_API void ModInit() {
                    status = 1;
                }
                HM_API void ModExit() {
                    status = 0;
                }
            )";
        } else {
            return nullptr;
        }

        file->Buffer = std::make_shared<std::vector<char>>(content.c_str(), content.c_str() + content.size());
        file->IsLoaded = true;
        return file;
    }
};

std::string FindLibTCC1Folder() {
    std::vector<std::string> searchPaths = { "/usr/lib", "/usr/local/lib", "/lib" };

    for (const auto& basePath : searchPaths) {
        if (!fs::exists(basePath))
            continue;

        try {
            auto options = fs::directory_options::skip_permission_denied;

            for (const auto& entry : fs::recursive_directory_iterator(basePath, options)) {
                if (entry.is_regular_file() && entry.path().filename() == "libtcc1.a") {
                    return entry.path().parent_path().string();
                }
            }
        } catch (const fs::filesystem_error& e) { continue; }
    }

    return "";
}
}; // namespace Ship

TEST(ScriptSystem, FibFunction) {
    auto archive = std::make_shared<Ship::RamArchive>();
    auto libPath = Ship::FindLibTCC1Folder();
    archive->Load();
    Ship::ScriptSystem system({}, 1, "-g -Wl", {}, { libPath }, {});

    ASSERT_NE(libPath, "");

    system.Compile(archive);
    system.LoadAll();

    auto fib = reinterpret_cast<int (*)(int)>(system.GetFunction("Test Script", "fib"));
    ASSERT_NE(fib, nullptr) << "Failed to find 'fib' function";

    EXPECT_EQ(fib(10), 55);
}

TEST(ScriptSystem, ModInitAndExit) {
    auto archive = std::make_shared<Ship::RamArchive>();
    archive->Load();
    auto libPath = Ship::FindLibTCC1Folder();
    Ship::ScriptSystem system({}, 1, "-g -Wl", {}, { libPath }, {});

    ASSERT_NE(libPath, "");

    system.Compile(archive);
    system.LoadAll();

    auto init = reinterpret_cast<void (*)()>(system.GetFunction("Test Script", "ModInit"));
    auto exit = reinterpret_cast<void (*)()>(system.GetFunction("Test Script", "ModExit"));

    ASSERT_NE(init, nullptr);
    ASSERT_NE(exit, nullptr);

    init();
    exit();
}

TEST(ScriptSystem, CompileDefines) {
    auto archive = std::make_shared<Ship::RamArchive>();
    archive->Load();
    auto libPath = Ship::FindLibTCC1Folder();
    Ship::ScriptSystem system({ { "__FIBx2__", "1" } }, 1, "-g -Wl", {}, { libPath }, {});

    ASSERT_NE(libPath, "");

    system.Compile(archive);
    system.LoadAll();

    auto fib = reinterpret_cast<int (*)(int)>(system.GetFunction("Test Script", "fib"));
    ASSERT_NE(fib, nullptr);
    EXPECT_EQ(fib(10), 20);
}