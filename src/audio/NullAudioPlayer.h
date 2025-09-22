#pragma once
#include "AudioPlayer.h"

namespace Ship {
class NullAudioPlayer final : public AudioPlayer {
  public:
    NullAudioPlayer(AudioSettings settings) : AudioPlayer(settings) {
    }
    ~NullAudioPlayer();

    int Buffered();
    void Play(const uint8_t* buf, size_t len);

  protected:
    bool DoInit();
};
} // namespace Ship
