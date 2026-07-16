#pragma once

#include <string>
#include <memory>
#include <vector>
#include "ship/audio/AudioBackend.h"
#include "ship/audio/AudioPlayer.h"
#include "ship/Component.h"
#include "ship/config/Config.h"

namespace Ship {
class Config;

/**
 * @brief Manages audio playback through a platform-specific AudioPlayer.
 *
 * Audio selects and initialises an AudioPlayer backend based on the AudioSettings
 * provided at construction. The backend can be switched at runtime via
 * SetCurrentAudioBackend(); the channel layout can be changed via SetAudioChannels()
 * without restarting the application.
 *
 * **Required dependencies (constructor-injected):**
 * - **Config** — cached on the class and used by SetCurrentAudioBackend() to
 *   load/persist the selected audio backend. Any code path that uses the cached
 *   Config validates that it exists and is initialized before use.
 *
 * Obtain the instance from `Context::GetChildren().GetFirst<Audio>()`.
 */
class Audio : public Component {
  public:
    /**
     * @brief Constructs an Audio manager with the given initial settings.
     * @param settings Initial audio backend selection and channel configuration.
     */
    Audio(AudioSettings settings, std::shared_ptr<Config> config = nullptr)
        : Component("Audio"), mAudioSettings(settings), mConfig(config) {
    }
    ~Audio();

    /** @brief Returns the currently active AudioPlayer instance. */
    std::shared_ptr<AudioPlayer> GetAudioPlayer();

    /** @brief Returns the identifier of the currently active audio backend. */
    AudioBackend GetCurrentAudioBackend();

    /** @brief Returns all audio backends available on the current platform. */
    std::shared_ptr<std::vector<AudioBackend>> GetAvailableAudioBackends();

    /**
     * @brief Switches to a different audio backend, reinitialising the AudioPlayer.
     * @param backend The backend to activate.
     */
    void SetCurrentAudioBackend(AudioBackend backend);

    /**
     * @brief Changes the channel layout and reinitialises the audio player.
     *
     * Safe to call at runtime without restarting the game.
     *
     * @param channels New channel configuration (stereo, surround, etc.).
     */
    void SetAudioChannels(AudioChannelsSetting channels);

    /**
     * @brief Returns the current audio channel configuration.
     */
    AudioChannelsSetting GetAudioChannels() const;

  protected:
    /** @brief (Re)initialises the AudioPlayer for the current backend and channel settings. */
    void InitAudioPlayer();

    /**
     * @brief Implements audio initialization. Called by Component::Init().
     *
     */
    void OnInit(const nlohmann::json& initArgs = nlohmann::json::object()) override;

    /**
     * @brief Reads and validates the audio backend from the persisted config.
     */
    AudioBackend GetSavedAudioBackend();

    /**
     * @brief Reads and validates the audio channel layout from the persisted config.
     */
    AudioChannelsSetting GetSavedAudioChannelsSetting();

  private:
    std::shared_ptr<AudioPlayer> mAudioPlayer;
    AudioBackend mAudioBackend;
    AudioSettings mAudioSettings;
    std::shared_ptr<std::vector<AudioBackend>> mAvailableAudioBackends;
    std::shared_ptr<Config> mConfig;

    /** @brief Returns the cached Config component. */
    std::shared_ptr<Config> GetConfig() const;
};
} // namespace Ship
