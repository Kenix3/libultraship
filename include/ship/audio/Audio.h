#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <memory>
#include <vector>
#include "ship/audio/AudioPlayer.h"

namespace Ship {
class Config;

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
 * Obtain the instance from Context::GetAudio().
 */
class Audio {
  public:
    /**
     * @brief Constructs an Audio manager with the given initial settings.
     * @param settings Initial audio backend selection and channel configuration.
     */
    Audio(AudioSettings settings) : mAudioSettings(settings) {
    }
    ~Audio();

    /**
     * @brief Selects and initialises the best available audio backend.
     *
     * Populates the list of available backends, picks the one specified in the
     * AudioSettings (or falls back to a default), and starts the AudioPlayer.
     */
    void Init();

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

    /** @brief Returns whether the float (HD) audio pipeline is currently active. */
    bool IsUsingFloatPipeline() const;

    /**
     * @brief Single authority for the float-pipeline mode.
     *
     * Updates the live flag, the settings new players inherit, and the current
     * player (reopening its device in the matching format). Everything else --
     * the producer's PlayF32-vs-Play choice and any newly constructed player --
     * derives from this, so there is exactly one place the mode is owned.
     *
     * @return true if applied; false if the current player refused float mode
     *         (in which case the authority is reverted to the player's actual mode).
     */
    bool SetUseFloatPipeline(bool enabled);

    /**
     * @brief Registers a callback invoked whenever a new AudioPlayer is
     * initialised (backend switch, fallback to Null, startup).
     *
     * The new player already inherits the float-pipeline mode; this hook exists
     * so the host can re-attach instance-bound state the player cannot carry
     * across a rebuild (e.g. a FluidSynth mix source). Pass an empty function to
     * clear it.
     */
    void SetOnAudioPlayerInitialized(std::function<void()> callback);

  protected:
    /** @brief (Re)initialises the AudioPlayer for the current backend and channel settings. */
    void InitAudioPlayer();

    /**
     * @brief Reads and validates the audio backend from the persisted config.
     *
     * Reads `Window.AudioBackend`, maps the stored string to an AudioBackend enum value,
     * handles the "pulse" → SDL migration, and returns a platform-appropriate default when
     * the stored value is absent or unrecognised.
     */
    AudioBackend GetSavedAudioBackend();

    /**
     * @brief Reads and validates the audio channel layout from the persisted config.
     *
     * Reads the channel setting CVar and maps it to a valid AudioChannelsSetting,
     * defaulting to stereo when the stored value is absent or unrecognised.
     */
    AudioChannelsSetting GetSavedAudioChannelsSetting();

  private:
    std::shared_ptr<AudioPlayer> mAudioPlayer;
    AudioBackend mAudioBackend;
    AudioSettings mAudioSettings;
    std::shared_ptr<std::vector<AudioBackend>> mAvailableAudioBackends;
    std::shared_ptr<Config> mConfig;

    // Single source of truth for the float-pipeline mode. Lock-free so the audio
    // producer can read it cheaply; written only by SetUseFloatPipeline, which
    // also mirrors it into mAudioSettings (the template new players inherit).
    std::atomic<bool> mUseFloatPipeline{ false };
    std::function<void()> mOnAudioPlayerInitialized;
};
} // namespace Ship
