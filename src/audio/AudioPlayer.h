#pragma once
#include "stdint.h"
#include "stddef.h"
#include <string>

namespace Ship {
enum class AudioSurroundSetting { stereo, surround51 };

struct AudioSettings {
    int32_t SampleRate = 44100;
    int32_t SampleLength = 1024;
    int32_t DesiredBuffered = 2480;
    AudioSurroundSetting AudioSurround = AudioSurroundSetting::stereo;
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
    
    AudioSurroundSetting GetAudioSurround() const;

    void SetSampleRate(int32_t rate);

    void SetSampleLength(int32_t length);

    void SetDesiredBuffered(int32_t size);
    
    void SetAudioSurround(AudioSurroundSetting surround);

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
