#pragma once
#include "AudioPlayer.h"
#include <SDL2/SDL.h>

namespace Ship {
class SDLAudioPlayer : public AudioPlayer {
  public:
    bool Init(void);
    int Buffered(void);
    int GetDesiredBuffered(void);
    void Play(const uint8_t* buf, uint32_t len);

  private:
    SDL_AudioDeviceID mDevice;
};
} // namespace Ship
