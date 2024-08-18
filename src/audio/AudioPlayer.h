#pragma once
#include "stdint.h"
#include "stddef.h"
#include <string>

namespace Ship {

struct AudioSettings {
    int SampleRate = 44100;
    int SampleLength = 1024;
    int DesiredBuffered = 2480;
};

class AudioPlayer {

  public:
    AudioPlayer(AudioSettings settings) : mAudioSettings(settings) {
    }
    ~AudioPlayer();

    bool Init(void);
    virtual int Buffered(void) = 0;
    virtual void Play(const uint8_t* buf, size_t len) = 0;

    bool IsInitialized(void);

    constexpr int GetSampleRate() const {
        return this->mAudioSettings.SampleRate;
    }

    constexpr int GetSampleLength() const {
        return this->mAudioSettings.SampleLength;
    }

    constexpr int GetDesiredBuffered() const {
        return this->mAudioSettings.DesiredBuffered;
    }

    void SetSampleRate(int rate) {
        this->mAudioSettings.SampleRate = rate;
    }

    void SetSampleLength(int length) {
        this->mAudioSettings.SampleLength = length;
    }

    void SetDesiredBuffered(int size) {
        this->mAudioSettings.DesiredBuffered = size;
    }

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
