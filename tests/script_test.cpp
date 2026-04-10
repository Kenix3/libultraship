#include <gtest/gtest.h>
#include <cstring>
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
    RamArchive()
        : Archive("ram://") {}
    ~RamArchive() = default;

    bool Open() override { return true; }
    bool Close() override { return true; }
    bool WriteFile(const std::string& filename, const std::vector<uint8_t>& data) override { return true; }

    std::shared_ptr<File> LoadFile(uint64_t hash) override {
        return nullptr;
    }
    std::shared_ptr<File> LoadFile(const std::string& filePath) override {
        auto file = std::make_shared<File>();
        if (filePath == "version") {
            file->Buffer = std::make_shared<std::vector<char>>(std::vector<char>{0x01, 0x00, 0x00, 0x00});
            file->IsLoaded = true;
        } else if (filePath == "manifest.json") {
            const std::string manifestContent = R"({
                "name": "Test Script",
                "code_version": 1,
                "game_version": 1,
                "main": "build.gen"
            })";
            file->Buffer = std::make_shared<std::vector<char>>(manifestContent.begin(), manifestContent.end());
            file->IsLoaded = true;
        } else if (filePath == "build.gen") {
            const std::string scriptContent = R"(test.c)";
            file->Buffer = std::make_shared<std::vector<char>>(scriptContent.begin(), scriptContent.end());
            file->IsLoaded = true;
        } else if (filePath == "test.c") {
            const std::string sourceCode = R"(
                #include <stdio.h>                
                #ifdef __FIBx2__
                int fib(int n) {
                    return 2 * fib(n);
                }
                #else
                int fib(int n) {
                    if (n <= 1) return n;
                    return fib(n - 1) + fib(n - 2);
                }
                #endif
                void ModInit() {
                    printf("Hello from test.c!\n");
                }
                void ModExit() {
                    printf("Goodbye from test.c!\n");
                }
            )";
            file->Buffer = std::make_shared<std::vector<char>>(sourceCode.begin(), sourceCode.end());
            file->IsLoaded = true;
        } else {
            file->IsLoaded = false;
        }
        return file;
    }
};
};

TEST(ScriptSystem, FibFunction) {
    auto archive = std::make_shared<Ship::RamArchive>();
    printf("Creating ScriptSystem...\n");
    Ship::ScriptSystem system({}, 1, "-g -Wl", {}, {}, {});
    printf("Compiling script...\n");
    system.Compile(archive);
    printf("Loading script...\n");
    system.LoadAll();
    printf("Getting function pointer...\n");
    
    auto fib = reinterpret_cast<int (*)(int)>(system.GetFunction("Test Script", "fib"));
    EXPECT_NE(fib, nullptr);
    EXPECT_EQ(fib(10), 55);
}

TEST(ScriptSystem, ModInitAndExit) {
    auto archive = std::make_shared<Ship::RamArchive>();
    printf("Creating ScriptSystem...\n");
    Ship::ScriptSystem system({}, 1, "-g -Wl", {}, {}, {});
    printf("Compiling script...\n");
    system.Compile(archive);
    printf("Loading script...\n");
    system.LoadAll();
    printf("Getting function pointer...\n");

    auto init = reinterpret_cast<void (*)()>(system.GetFunction("Test Script", "ModInit"));
    auto exit = reinterpret_cast<void (*)()>(system.GetFunction("Test Script", "ModExit"));
    EXPECT_NE(init, nullptr);
    EXPECT_NE(exit, nullptr);

    init();
    exit();
}

TEST(ScriptSystem, CompileDefines) {
    auto archive = std::make_shared<Ship::RamArchive>();
    printf("Creating ScriptSystem...\n");
    Ship::ScriptSystem system({{"__FIBx2__", "1"}}, 1, "-g -Wl", {}, {}, {});
    printf("Compiling script...\n");
    system.Compile(archive);
    printf("Loading script...\n");
    system.LoadAll();
    printf("Getting function pointer...\n");

    auto fib = reinterpret_cast<int (*)(int)>(system.GetFunction("Test Script", "fib"));
    EXPECT_NE(fib, nullptr);
    EXPECT_EQ(fib(10), 110);
}