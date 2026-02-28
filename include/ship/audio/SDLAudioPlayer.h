#pragma once
#include "AudioPlayer.h"
#include <SDL2/SDL.h>

namespace Ship {
class SDLAudioPlayer final : public AudioPlayer {
  public:
    SDLAudioPlayer(AudioSettings settings) : AudioPlayer(settings) {
    }
    ~SDLAudioPlayer();

    int Buffered() override;

    void SetMuted(bool muted) override;
    bool IsMuted() const override;

  protected:
    bool DoInit() override;
    void DoClose() override;
    void DoPlay(const uint8_t* buf, size_t len) override;

  private:
    SDL_AudioDeviceID mDevice = 0;
    int32_t mNumChannels = 2;
    bool mMuted = false;
};
} // namespace Ship
