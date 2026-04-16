#pragma once
#include "AudioPlayer.h"

namespace Ship {
/**
 * @brief A no-op AudioPlayer used when no audio device is available or audio is disabled.
 *
 * NullAudioPlayer silently discards all audio data. DoInit() succeeds immediately,
 * Buffered() always returns the desired buffered frame count so the game continues to
 * produce audio at the expected rate, and DoPlay() is a no-op.
 *
 * This backend is always compiled in and is selected as a fallback when every other
 * backend fails to initialise.
 */
class NullAudioPlayer final : public AudioPlayer {
  public:
    /**
     * @brief Constructs a NullAudioPlayer with the given audio settings.
     * @param settings Audio configuration (rates are accepted but otherwise unused).
     */
    NullAudioPlayer(AudioSettings settings) : AudioPlayer(settings) {
    }
    ~NullAudioPlayer();

    /**
     * @brief Returns the desired buffered frame count so the game always produces audio.
     */
    int Buffered() override;

  protected:
    /** @brief Always returns true — no actual device needs to be opened. */
    bool DoInit() override;

    /** @brief No-op — nothing to close. */
    void DoClose() override;

    /** @brief Silently discards all audio data. */
    void DoPlay(const uint8_t* buf, size_t len) override;
};
} // namespace Ship
