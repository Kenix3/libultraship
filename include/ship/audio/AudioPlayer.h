#pragma once
#include "stdint.h"
#include "stddef.h"
#include <string>
#include <memory>
#include "ship/audio/AudioChannelsSetting.h"
#include "ship/audio/SoundMatrixDecoder.h"

namespace Ship {

struct AudioSettings {
    int32_t SampleRate = 44100;
    int32_t SampleLength = 1024;
    int32_t DesiredBuffered = 2480;
    AudioChannelsSetting ChannelSetting = AudioChannelsSetting::audioStereo;
};

class AudioPlayer {

  public:
    AudioPlayer(AudioSettings settings) : mAudioSettings(settings) {
    }
    ~AudioPlayer();

    bool Init();
    virtual int32_t Buffered() = 0;

    // Play audio
    // buf: interleaved samples in either stereo: (L, R, L, R, ...), or surround: (FL, FR, C, LFE, SL, SR, ...)
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
    // Initialize the audio device.
    virtual bool DoInit() = 0;

    // Close the current audio device.
    virtual void DoClose() = 0;

    // Internal play method - receives audio in the output format (stereo or surround)
    virtual void DoPlay(const uint8_t* buf, size_t len) = 0;

  private:
    // Sound matrix decoder for surround mode
    std::unique_ptr<SoundMatrixDecoder> mSoundMatrixDecoder;

    AudioSettings mAudioSettings;
    bool mInitialized = false;
};
} // namespace Ship

#ifdef _WIN32
#include "WasapiAudioPlayer.h"
#endif

#ifdef __APPLE__
#include "CoreAudioAudioPlayer.h"
#endif

#include "SDLAudioPlayer.h"
#include "NullAudioPlayer.h"
