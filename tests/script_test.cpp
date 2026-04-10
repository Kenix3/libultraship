#include <gtest/gtest.h>
#include <stdexcept>

#include "ship/resource/File.h"
#include "ship/resource/archive/Archive.h"
#include "ship/scripting/ScriptSystem.h"

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
                #include <stdio.h>
                #define HM_API __attribute__((visibility("default")))

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
                HM_API void ModInit() { printf("Hello from test.c!\n"); }
                HM_API void ModExit() { printf("Goodbye from test.c!\n"); }
            )";
        } else {
            return nullptr;
        }

        file->Buffer = std::make_shared<std::vector<char>>(content.c_str(), content.c_str() + content.size());
        file->IsLoaded = true;
        return file;
    }
};
}; // namespace Ship

TEST(ScriptSystem, FibFunction) {
    auto archive = std::make_shared<Ship::RamArchive>();
    archive->Load();
    Ship::ScriptSystem system({}, 1, "-g -Wl", {}, {}, {});

    system.Compile(archive);
    system.LoadAll();

    auto fib = reinterpret_cast<int (*)(int)>(system.GetFunction("Test Script", "fib"));
    ASSERT_NE(fib, nullptr) << "Failed to find 'fib' function";

    EXPECT_EQ(fib(10), 55);
}

TEST(ScriptSystem, ModInitAndExit) {
    auto archive = std::make_shared<Ship::RamArchive>();
    archive->Load();
    Ship::ScriptSystem system({}, 1, "-g -Wl", {}, {}, {});

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
    Ship::ScriptSystem system({ { "__FIBx2__", "1" } }, 1, "-g -Wl", {}, {}, {});

    system.Compile(archive);
    system.LoadAll();

    auto fib = reinterpret_cast<int (*)(int)>(system.GetFunction("Test Script", "fib"));
    ASSERT_NE(fib, nullptr);
    EXPECT_EQ(fib(10), 20);
}