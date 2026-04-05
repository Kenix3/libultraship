#include "ship/scripting/ScriptSystem.h"

#include "ship/resource/ResourceManager.h"
#include "ship/resource/archive/Archive.h"
#include "ship/resource/File.h"
#include "ship/Context.h"
#include "spdlog/spdlog.h"
#include <optional>
#include <fstream>
#include <string_view>
#include <libtcc.h>
#include <memory>
#include <queue>
#include <unordered_map>

#include "ship/config/ConsoleVariable.h"

namespace Ship {
std::optional<std::vector<uint8_t>> LoadFromO2R(const std::string& path,
                                                const std::shared_ptr<Archive>& archive = nullptr) {
    const auto file = archive->LoadFile(path);
    if (file == nullptr || !file->IsLoaded) {
        SPDLOG_ERROR("Failed to load script file: {}", path);
        return std::nullopt;
    }

    return std::vector<uint8_t>(file->Buffer->begin(), file->Buffer->end());
}

constexpr std::string_view Trim(const std::string_view v) {
    constexpr std::string_view whitespace = " \t\r\n";

    const auto start = v.find_first_not_of(whitespace);
    if (start == std::string_view::npos) {
        return {};
    }

    const auto end = v.find_last_not_of(whitespace);
    return v.substr(start, end - start + 1);
}

constexpr std::string_view GetPlatform() {
#if defined(_WIN32) || defined(_WIN64)
#if defined(_M_ARM64) || defined(__aarch64__)
    return "windows_arm64";
#else
    return "windows_x64";
#endif

#elif defined(__APPLE__) || defined(__MACH__)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
    return "ios";
#elif TARGET_OS_MAC
    return "darwin";
#endif

#elif defined(__ANDROID__)
    return "android";

#elif defined(__linux__)
#if defined(__x86_64__) || defined(_M_X64)
    return "linux_x64";
#elif defined(__i386__) || defined(_M_IX86)
    return "linux_x86";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "linux_arm64";
#else
    return "linux_generic";
#endif

#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    return "bsd";
#else
#error "Unsupported Operating System"
#endif
}

void ScriptSystem::Load(const std::shared_ptr<Archive>& archive) {
    const SafeLevel lvl = static_cast<SafeLevel>(Context::GetInstance()->GetConsoleVariables()->GetInteger(
        CVAR_SCRIPT_SAFE_LEVEL, static_cast<int>(SafeLevel::WARN_UNTRUSTED_SCRIPTS)));
    const ArchiveManifest& info = archive->GetManifest();
    constexpr std::string_view platform = GetPlatform();
    const bool isCodeMod = !info.Main.empty() || !info.Binaries.empty();

    if (!isCodeMod) {
        return;
    }

    if (info.CodeVersion != mCodeVersion) {
        SPDLOG_ERROR("Incompatible code version for archive {}: expected {}, got {}", archive->GetPath(), mCodeVersion,
                     info.CodeVersion);
        return;
    }

    const bool isTrusted = archive->IsSigned() && archive->IsChecksumValid();

    if (lvl == SafeLevel::ONLY_TRUSTED_SCRIPTS && !isTrusted) {
        SPDLOG_ERROR("Script loading is disabled for untrusted scripts. Failed to load script from archive: {}",
                     archive->GetPath());
        return;
    }

    if (lvl == SafeLevel::WARN_UNTRUSTED_SCRIPTS) {
        if (isTrusted) {
            SPDLOG_INFO("Loaded trusted script from archive: {}", archive->GetPath());
        } else {
            SPDLOG_WARN(
                "Loaded untrusted script from archive: {}. This script is not signed or has an invalid checksum. "
                "This may be a security risk if you do not trust the source of this script.",
                archive->GetPath());
        }
    }

    ScriptLoader loader;

    const auto& binaries = info.Binaries;
    const std::string temp = loader.GenerateTempFile();

    if (binaries.contains(std::string(platform))) {
        const std::string& path = binaries.at(std::string(platform));
        auto data = LoadFromO2R(path, archive);
        if (!data.has_value()) {
            throw std::runtime_error("Failed to load platform-specific binary: " + path);
        }

        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            throw std::runtime_error("Failed to create temporary file for platform-specific binary: " + temp);
        }
        out.write(reinterpret_cast<const char*>(data->data()), data->size());
        out.close();
    } else if (!info.Main.empty()) {
        const auto data = LoadFromO2R(info.Main, archive);
        if (!data.has_value()) {
            throw std::runtime_error("Failed to load main script: " + info.Main);
        }

        TCCState* s = tcc_new();
        if (!s) {
            throw std::runtime_error("Failed to create TCCState");
        }

        for (const auto& [key, value] : mCompileDefines) {
            tcc_define_symbol(s, key.c_str(), value.c_str());
        }

        tcc_set_output_type(s, TCC_OUTPUT_DLL);

        const std::vector<uint8_t>& raw = data.value();
        std::string content(raw.begin(), raw.end());
        std::istringstream stream(content);
        std::string line;

        while (std::getline(stream, line)) {
            if (line.empty()) {
                continue;
            }

            line.erase(line.find_last_not_of(" \r\n\t") + 1);
            line.erase(0, line.find_first_not_of(" \r\n\t"));

            if (line.empty() || line[0] == '#') {
                continue;
            }

            std::string safePath = line;

            auto buf = LoadFromO2R(safePath, archive);
            if (!buf.has_value()) {
                tcc_delete(s);
                throw std::runtime_error("Failed to load script file: '" + safePath + "'");
            }

            std::string lineFixer = "#line 1 \"[" + info.Name + "]:" + safePath + "\"\n";
            std::string sourceCode = lineFixer + std::string(buf->begin(), buf->end());
            if (tcc_compile_string(s, sourceCode.c_str()) == -1) {
                tcc_delete(s);
                throw std::runtime_error("TCC Error in " + safePath);
            }
        }

        if (tcc_output_file(s, temp.c_str()) == -1) {
            tcc_delete(s);
            throw std::runtime_error("Failed to output compiled code for " + temp);
        }
    }

    loader.Init(temp);
    const ScriptFunc_t init = (ScriptFunc_t)loader.GetFunction("ModInit");

    if (init) {
        init();
        mLoadedScripts[info.Name] = loader;
    }
};

void ScriptSystem::LoadAll() {
    auto archive = Context::GetInstance()->GetResourceManager()->GetArchiveManager();
    auto list = archive->GetArchives();
    std::vector<std::shared_ptr<Archive>> loadOrder;
    std::unordered_map<std::string, std::shared_ptr<Archive>> archiveMap;
    std::unordered_map<std::string, int> inDegree;
    std::unordered_map<std::string, std::vector<std::string>> dependents;

    for (const auto& entry : *list) {
        const auto& info = entry->GetManifest();
        if (info.Main.empty()) {
            continue;
        }

        archiveMap[info.Name] = entry;
        inDegree[info.Name] = 0;
    }

    for (const auto& [name, entry] : archiveMap) {
        const auto& deps = entry->GetManifest().Dependencies;
        for (const std::string& depName : deps) {
            if (archiveMap.contains(depName)) {
                dependents[depName].push_back(name);
                inDegree[name]++;
            } else {
                throw std::runtime_error("Archive " + name + " depends on missing archive: " + depName);
            }
        }
    }

    std::queue<std::string> readyQueue;

    for (const auto& [name, degree] : inDegree) {
        if (degree == 0) {
            readyQueue.push(name);
        }
    }

    while (!readyQueue.empty()) {
        std::string current = readyQueue.front();
        readyQueue.pop();

        loadOrder.push_back(archiveMap[current]);

        for (const std::string& dependentName : dependents[current]) {
            inDegree[dependentName]--;

            if (inDegree[dependentName] == 0) {
                readyQueue.push(dependentName);
            }
        }
    }

    if (loadOrder.size() != archiveMap.size()) {
        throw std::runtime_error("Circular dependency detected among script archives. Failed to resolve load order.");
    }

    for (const auto& entry : loadOrder) {
        Load(entry);
    }
}

void ScriptSystem::UnloadAll() {
    for (auto& [_, loader] : mLoadedScripts) {
        const ScriptFunc_t exit = (ScriptFunc_t)loader.GetFunction("ModExit");
        if (exit) {
            exit();
        }
        loader.Unload();
    }
    mLoadedScripts.clear();
};

void* ScriptSystem::GetFunction(const std::string& name, const std::string& function) {
    if (mLoadedScripts.contains(name)) {
        return mLoadedScripts.at(name).GetFunction(function);
    }
    return nullptr;
};

} // namespace Ship