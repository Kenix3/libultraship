#pragma once
#include "AudioPlayer.h"
#include <SDL2/SDL.h>

namespace Ship {
class SDLAudioPlayer : public AudioPlayer {
  public:
    SDLAudioPlayer(AudioSettings settings) : AudioPlayer(settings) {
    }
    ~SDLAudioPlayer();

    int Buffered();
    void Play(const uint8_t* buf, size_t len);

  protected:
    bool DoInit();

  private:
    SDL_AudioDeviceID mDevice;
};
} // namespace Ship
