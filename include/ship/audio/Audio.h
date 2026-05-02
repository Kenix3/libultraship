#pragma once

#include <string>
#include <memory>
#include <vector>
#include "ship/audio/AudioPlayer.h"
#include "ship/Component.h"

namespace Ship {
/** @brief Identifies the audio backend implementation in use. */
enum class AudioBackend { WASAPI, SDL, COREAUDIO, NUL };

/**
 * @brief Manages audio playback through a platform-specific AudioPlayer.
 *
 * Audio selects and initialises an AudioPlayer backend based on the AudioSettings
 * provided at construction. The backend can be switched at runtime via
 * SetCurrentAudioBackend(); the channel layout can be changed via SetAudioChannels()
 * without restarting the application.
 *
 * **Required Context children (looked up at runtime):**
 * - **Config** — queried during Init() and SetCurrentAudioBackend() to load/persist
 *   the selected audio backend. Config must be added to the Context **before** calling
 *   Audio::Init().
 *
 * Obtain the instance from `Context::GetChildren().GetFirst<Audio>()`.
 */
class Audio : public Component {
  public:
    /**
     * @brief Constructs an Audio manager with the given initial settings.
     * @param settings Initial audio backend selection and channel configuration.
     */
    Audio(AudioSettings settings) : Component("Audio"), mAudioSettings(settings) {
    }
    ~Audio();

    /**
     * @brief Selects and initialises the best available audio backend.
     *
     * Called by Component::Init(). Populates the list of available backends,
     * picks the one specified in the AudioSettings (or falls back to a default),
     * and starts the AudioPlayer.
     *
     * @note Init-order dependency: Config must be present **and initialized** in
     *       the Context hierarchy before Component::Init() is called on Audio.
     *       Config self-initializes on construction, so adding it to the Context
     *       before Audio satisfies this requirement.
     *
     * @throws std::runtime_error if Config is not present in the hierarchy.
     * @throws std::runtime_error if Config is present but not yet initialized.
     */
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
     * @throws std::runtime_error if Config is not present in the hierarchy.
     * @throws std::runtime_error if Config is present but not yet initialized.
     */
    void OnInit() override;

  private:
    std::shared_ptr<AudioPlayer> mAudioPlayer;
    AudioBackend mAudioBackend;
    AudioSettings mAudioSettings;
    std::shared_ptr<std::vector<AudioBackend>> mAvailableAudioBackends;
};
} // namespace Ship
