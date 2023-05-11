#pragma once

#include <string>
#include <memory>
#include <unordered_set>
#include <vector>
#include <spdlog/spdlog.h>
#include "Mercury.h"
#include "resource/ResourceManager.h"
#include "controller/ControlDeck.h"
#include "debug/CrashHandler.h"
#include "audio/AudioPlayer.h"
#include "core/Window.h"
#include "core/ConsoleVariable.h"

namespace LUS {
class Context {
  public:
    static std::shared_ptr<Context> GetInstance();
    static std::shared_ptr<Context> CreateInstance(const std::string name, const std::string shortName,
                                                   const std::vector<std::string>& otrFiles = {},
                                                   const std::unordered_set<uint32_t>& validHashes = {},
                                                   uint32_t reservedThreadCount = 1);

    static std::string GetAppBundlePath();
    static std::string GetAppDirectoryPath();
    static std::string GetPathRelativeToAppDirectory(const char* path);
    static std::string GetPathRelativeToAppBundle(const char* path);

    Context(std::string name, std::string shortName);

    void Init(const std::vector<std::string>& otrFiles, const std::unordered_set<uint32_t>& validHashes,
              uint32_t reservedThreadCount);

    bool DoesOtrFileExist();

    std::shared_ptr<spdlog::logger> GetLogger();
    std::shared_ptr<Mercury> GetConfig();
    std::shared_ptr<ConsoleVariable> GetConsoleVariables();
    std::shared_ptr<ResourceManager> GetResourceManager();
    std::shared_ptr<ControlDeck> GetControlDeck();
    std::shared_ptr<CrashHandler> GetCrashHandler();
    std::shared_ptr<AudioPlayer> GetAudioPlayer();
    std::shared_ptr<Window> GetWindow();

    std::string GetName();
    std::string GetShortName();

    void CreateDefaultSettings();
    void InitLogging();
    void InitConfiguration();
    void InitConsoleVariables();
    void InitResourceManager(const std::vector<std::string>& otrFiles = {},
                             const std::unordered_set<uint32_t>& validHashes = {}, uint32_t reservedThreadCount = 1);
    void InitControlDeck();
    void InitCrashHandler();
    void InitAudioPlayer(std::string backend);
    void InitWindow();

  protected:
    Context() = default;

  private:
    static std::weak_ptr<Context> mContext;

    std::shared_ptr<spdlog::logger> mLogger;
    std::shared_ptr<Mercury> mConfig;
    std::shared_ptr<ConsoleVariable> mConsoleVariables;
    std::shared_ptr<ResourceManager> mResourceManager;
    std::shared_ptr<ControlDeck> mControlDeck;
    std::shared_ptr<CrashHandler> mCrashHandler;
    std::shared_ptr<AudioPlayer> mAudioPlayer;
    std::shared_ptr<Window> mWindow;

    std::string mMainPath;
    std::string mPatchesPath;
    bool mOtrFileExists;

    std::string mName;
    std::string mShortName;
};
} // namespace LUS
