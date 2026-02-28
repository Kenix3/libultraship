#pragma once
#include "AudioPlayer.h"

namespace Ship {
class NullAudioPlayer final : public AudioPlayer {
  public:
    NullAudioPlayer(AudioSettings settings) : AudioPlayer(settings) {
    }
    ~NullAudioPlayer();

    int Buffered() override;

    void SetMuted(bool muted) override;
    bool IsMuted() const override;

  protected:
    bool DoInit() override;
    void DoClose() override;
    void DoPlay(const uint8_t* buf, size_t len) override;
};
} // namespace Ship
