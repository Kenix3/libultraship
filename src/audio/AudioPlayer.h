#pragma once
#include "stdint.h"
#include "stddef.h"
#include <string>

namespace Ship {

struct AudioSettings {
    int mSampleRate = 44100;
    int mSampleLength = 1024;
    int mDesiredBuffered = 2480;
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
        return this->mAudioSettings.mSampleRate;
    }

    constexpr int GetSampleLength() const {
        return this->mAudioSettings.mSampleLength;
    }

    constexpr int GetDesiredBuffered() const {
        return this->mAudioSettings.mDesiredBuffered;
    }

    void SetSampleRate(int rate) {
        this->mAudioSettings.mSampleRate = rate;
    }

    void SetSampleLength(int length) {
        this->mAudioSettings.mSampleLength = length;
    }

    void SetDesiredBuffered(int size) {
        this->mAudioSettings.mDesiredBuffered = size;
    }

  protected:
    virtual bool DoInit(void) = 0;

  private:
    bool mInitialized;
    AudioSettings mAudioSettings;
};
} // namespace Ship

#ifdef _WIN32
#include "WasapiAudioPlayer.h"
#endif

#include "SDLAudioPlayer.h"
