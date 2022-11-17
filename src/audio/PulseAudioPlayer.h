#pragma once

#if defined(__linux__) || defined(__BSD__)

#include "AudioPlayer.h"
#include <pulse/pulseaudio.h>

namespace Ship {
class PulseAudioPlayer : public AudioPlayer {
  public:
    PulseAudioPlayer();
    int Buffered() override;
    int GetDesiredBuffered() override;
    void Play(const uint8_t* buff, uint32_t len) override;

  protected:
    bool doInit() override;

  private:
    pa_context* mContext = nullptr;
    pa_stream* mStream = nullptr;
    pa_mainloop* mMainLoop = nullptr;
    bool mWriteComplete = false;
    pa_buffer_attr mAttr = { 0 };
};
} // namespace Ship
#endif
