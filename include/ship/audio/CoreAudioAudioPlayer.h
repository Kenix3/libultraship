#pragma once

#ifdef __APPLE__

#include "AudioPlayer.h"
#include <AudioToolbox/AudioToolbox.h>
#include <pthread.h>

namespace Ship {
class CoreAudioAudioPlayer : public AudioPlayer {
  public:
    CoreAudioAudioPlayer(AudioSettings settings);
    ~CoreAudioAudioPlayer();

    int Buffered() override;

  protected:
    bool DoInit() override;
    void DoClose() override;
    void DoPlay(const uint8_t* buf, size_t len) override;

  private:
    static OSStatus CoreAudioRenderCallback(void* inRefCon, AudioUnitRenderActionFlags* ioActionFlags,
                                            const AudioTimeStamp* inTimeStamp, UInt32 inBusNumber,
                                            UInt32 inNumberFrames, AudioBufferList* ioData);

    AudioUnit mAudioUnit;
    int32_t mNumChannels;
    uint8_t* mRingBuffer;
    size_t mRingBufferSize;
    size_t mRingBufferReadPos;
    size_t mRingBufferWritePos;
    pthread_mutex_t mMutex;
    bool mInitialized;
};
} // namespace Ship
#endif
