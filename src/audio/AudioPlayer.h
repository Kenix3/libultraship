#pragma once
#include "stdint.h"
#include "stddef.h"
#include <string>

namespace Ship {

struct AudioSettings {
    int32_t SampleRate = 44100;
    int32_t SampleLength = 1024;
    int32_t DesiredBuffered = 2480;
};

class AudioPlayer {

  public:
    AudioPlayer(AudioSettings settings) : mAudioSettings(settings) {
    }
    ~AudioPlayer();

    bool Init(void);
    virtual int32_t Buffered(void) = 0;
    virtual void Play(const uint8_t* buf, size_t len) = 0;

    bool IsInitialized(void);

    int32_t GetSampleRate() const;

    int32_t GetSampleLength() const;

    int32_t GetDesiredBuffered() const;

    void SetSampleRate(int32_t rate);

    void SetSampleLength(int32_t length);

    void SetDesiredBuffered(int32_t size);

  protected:
    virtual bool DoInit(void) = 0;

  private:
    bool mInitialized = false;
    AudioSettings mAudioSettings;
};
} // namespace Ship

#ifdef _WIN32
#include "WasapiAudioPlayer.h"
#endif

#include "SDLAudioPlayer.h"
