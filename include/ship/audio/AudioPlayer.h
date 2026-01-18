#pragma once
#include "stdint.h"
#include "stddef.h"
#include <string>
#include <memory>
#include <vector>
#include "ship/audio/AudioChannelsSetting.h"
#include "ship/audio/DolbyProLogicII.h"

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

    // Play stereo audio - will decode to surround if surround mode is enabled
    // buf: interleaved stereo samples (L, R, L, R, ...)
    // len: length in bytes
    void Play(const uint8_t* buf, size_t len);

    bool IsInitialized();

    int32_t GetSampleRate() const;

    int32_t GetSampleLength() const;

    int32_t GetDesiredBuffered() const;

    AudioChannelsSetting GetAudioChannels() const;

    void SetSampleRate(int32_t rate);

    void SetSampleLength(int32_t length);

    void SetDesiredBuffered(int32_t size);

    // Change audio channels and reinitialize the audio device
    // Returns true if successful
    bool SetAudioChannels(AudioChannelsSetting channels);

    // Get the number of output channels (2 for stereo, 6 for surround)
    int32_t GetNumOutputChannels() const;

  protected:
    virtual bool DoInit() = 0;

    // Reinitialize the audio device with current settings (called after channel change)
    virtual void DoClose() = 0;

    // Internal play method - receives audio in the output format (stereo or surround)
    virtual void DoPlay(const uint8_t* buf, size_t len) = 0;

    // PLII decoder for surround mode
    std::unique_ptr<DolbyProLogicIIDecoder> mPLIIDecoder;

    // Surround output buffer
    std::vector<int16_t> mSurroundBuffer;

    AudioSettings mAudioSettings;

    // Number of audio frames to skip after channel switch (to prevent glitches)
    int mSkipFrames = 0;

  private:
    bool mInitialized = false;
};
} // namespace Ship

#ifdef _WIN32
#include "WasapiAudioPlayer.h"
#endif

#include "SDLAudioPlayer.h"
#include "NullAudioPlayer.h"
