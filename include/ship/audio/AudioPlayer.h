#pragma once
#include "stdint.h"
#include "stddef.h"
#include <string>
#include "public/bridge/audiobridge.h"

namespace Ship {

struct AudioSettings {
    int32_t SampleRate = 44100;
    int32_t SampleLength = 1024;
    int32_t DesiredBuffered = 2480;
    AudioChannelsSetting AudioSurround = AudioChannelsSetting::audioStereo;
};

class AudioPlayer {

  public:
    AudioPlayer(AudioSettings settings) : mAudioSettings(settings) {
    }
    ~AudioPlayer();

    bool Init();
    virtual int32_t Buffered() = 0;
    virtual void Play(const uint8_t* buf, size_t len) = 0;

    bool IsInitialized();

    int32_t GetSampleRate() const;

    int32_t GetSampleLength() const;

    int32_t GetDesiredBuffered() const;

    AudioChannelsSetting GetAudioChannels() const;

    void SetSampleRate(int32_t rate);

    void SetSampleLength(int32_t length);

    void SetDesiredBuffered(int32_t size);

    void SetAudioChannels(AudioChannelsSetting surround);

  protected:
    virtual bool DoInit() = 0;

  private:
    bool mInitialized = false;
    AudioSettings mAudioSettings;
};
} // namespace Ship

#ifdef _WIN32
#include "WasapiAudioPlayer.h"
#endif

#include "SDLAudioPlayer.h"
