#ifdef __APPLE__
#include "ship/audio/CoreAudioAudioPlayer.h"
#include <spdlog/spdlog.h>
#include <cstring>

namespace Ship {

CoreAudioAudioPlayer::CoreAudioAudioPlayer(AudioSettings settings) : AudioPlayer(settings), mInitialized(false) {
    pthread_mutex_init(&mMutex, NULL);
}

CoreAudioAudioPlayer::~CoreAudioAudioPlayer() {
    SPDLOG_TRACE("destruct CoreAudio audio player");
    DoClose();
    pthread_mutex_destroy(&mMutex);
}

void CoreAudioAudioPlayer::DoClose() {
    if (mInitialized) {
        AudioOutputUnitStop(mAudioUnit);
        AudioUnitUninitialize(mAudioUnit);
        AudioComponentInstanceDispose(mAudioUnit);
        mInitialized = false;
    }

    if (mRingBuffer) {
        delete[] mRingBuffer;
        mRingBuffer = nullptr;
    }
}

bool CoreAudioAudioPlayer::DoInit() {
    OSStatus status;

    mNumChannels = this->GetAudioChannels() == AudioChannelsSetting::audioStereo ? 2 : 6;

    const size_t bytesPerSample = sizeof(int16_t);
    const size_t bytesPerFrame = bytesPerSample * mNumChannels;

    mRingBufferSize = 6000 * bytesPerFrame;
    mRingBuffer = new uint8_t[mRingBufferSize];
    mRingBufferReadPos = 0;
    mRingBufferWritePos = 0;

    AudioComponentDescription desc;
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_HALOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    AudioComponent component = AudioComponentFindNext(NULL, &desc);
    if (component == NULL) {
        SPDLOG_ERROR("CoreAudio: Failed to find audio component");
        return false;
    }

    status = AudioComponentInstanceNew(component, &mAudioUnit);
    if (status != noErr) {
        SPDLOG_ERROR("CoreAudio: Failed to create audio component instance: {}", status);
        return false;
    }

    UInt32 flag = 1;
    status = AudioUnitSetProperty(mAudioUnit, kAudioOutputUnitProperty_EnableIO, kAudioUnitScope_Output, 0, &flag,
                                  sizeof(flag));
    if (status != noErr) {
        SPDLOG_ERROR("CoreAudio: Failed to enable output: {}", status);
        return false;
    }

    AudioStreamBasicDescription format;
    format.mSampleRate = this->GetSampleRate();
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
    format.mBytesPerPacket = bytesPerFrame;
    format.mFramesPerPacket = 1;
    format.mBytesPerFrame = bytesPerFrame;
    format.mChannelsPerFrame = mNumChannels;
    format.mBitsPerChannel = 16;

    status = AudioUnitSetProperty(mAudioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &format,
                                  sizeof(format));
    if (status != noErr) {
        SPDLOG_ERROR("CoreAudio: Failed to set stream format: {}", status);
        return false;
    }

    AURenderCallbackStruct callbackStruct;
    callbackStruct.inputProc = CoreAudioRenderCallback;
    callbackStruct.inputProcRefCon = this;

    status = AudioUnitSetProperty(mAudioUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0,
                                  &callbackStruct, sizeof(callbackStruct));
    if (status != noErr) {
        SPDLOG_ERROR("CoreAudio: Failed to set render callback: {}", status);
        return false;
    }

    status = AudioUnitInitialize(mAudioUnit);
    if (status != noErr) {
        SPDLOG_ERROR("CoreAudio: Failed to initialize audio unit: {}", status);
        return false;
    }

    status = AudioOutputUnitStart(mAudioUnit);
    if (status != noErr) {
        SPDLOG_ERROR("CoreAudio: Failed to start audio unit: {}", status);
        return false;
    }

    mInitialized = true;
    return true;
}

int CoreAudioAudioPlayer::Buffered() {
    pthread_mutex_lock(&mMutex);
    size_t buffered;

    if (mRingBufferWritePos >= mRingBufferReadPos) {
        buffered = mRingBufferWritePos - mRingBufferReadPos;
    } else {
        buffered = mRingBufferSize - (mRingBufferReadPos - mRingBufferWritePos);
    }

    const size_t bytesPerFrame = sizeof(int16_t) * mNumChannels;
    int samples = buffered / bytesPerFrame;

    pthread_mutex_unlock(&mMutex);
    return samples;
}

void CoreAudioAudioPlayer::DoPlay(const uint8_t* buf, size_t len) {
    pthread_mutex_lock(&mMutex);

    const size_t bytesPerFrame = sizeof(int16_t) * mNumChannels;
    const size_t maxBuffered = 6000 * bytesPerFrame;

    size_t available;
    if (mRingBufferWritePos >= mRingBufferReadPos) {
        available = mRingBufferSize - (mRingBufferWritePos - mRingBufferReadPos);
    } else {
        available = mRingBufferReadPos - mRingBufferWritePos;
    }

    if (available >= len) {
        size_t writeEnd = mRingBufferWritePos + len;

        if (writeEnd <= mRingBufferSize) {
            memcpy(mRingBuffer + mRingBufferWritePos, buf, len);
        } else {
            size_t firstChunk = mRingBufferSize - mRingBufferWritePos;
            memcpy(mRingBuffer + mRingBufferWritePos, buf, firstChunk);
            memcpy(mRingBuffer, buf + firstChunk, len - firstChunk);
        }

        mRingBufferWritePos = (mRingBufferWritePos + len) % mRingBufferSize;
    }

    pthread_mutex_unlock(&mMutex);
}

OSStatus CoreAudioAudioPlayer::CoreAudioRenderCallback(void* inRefCon, AudioUnitRenderActionFlags* ioActionFlags,
                                                       const AudioTimeStamp* inTimeStamp, UInt32 inBusNumber,
                                                       UInt32 inNumberFrames, AudioBufferList* ioData) {
    CoreAudioAudioPlayer* player = static_cast<CoreAudioAudioPlayer*>(inRefCon);

    for (UInt32 i = 0; i < ioData->mNumberBuffers; i++) {
        AudioBuffer* buffer = &ioData->mBuffers[i];
        UInt8* outputBuffer = static_cast<UInt8*>(buffer->mData);
        UInt32 bytesToWrite = buffer->mDataByteSize;

        pthread_mutex_lock(&player->mMutex);

        size_t available;
        if (player->mRingBufferWritePos >= player->mRingBufferReadPos) {
            available = player->mRingBufferWritePos - player->mRingBufferReadPos;
        } else {
            available = player->mRingBufferSize - (player->mRingBufferReadPos - player->mRingBufferWritePos);
        }

        UInt32 bytesToCopy = bytesToWrite;
        if (bytesToCopy > available) {
            bytesToCopy = available;
        }

        if (bytesToCopy > 0) {
            size_t readEnd = player->mRingBufferReadPos + bytesToCopy;

            if (readEnd <= player->mRingBufferSize) {
                memcpy(outputBuffer, player->mRingBuffer + player->mRingBufferReadPos, bytesToCopy);
            } else {
                size_t firstChunk = player->mRingBufferSize - player->mRingBufferReadPos;
                memcpy(outputBuffer, player->mRingBuffer + player->mRingBufferReadPos, firstChunk);
                memcpy(outputBuffer + firstChunk, player->mRingBuffer, bytesToCopy - firstChunk);
            }

            player->mRingBufferReadPos = (player->mRingBufferReadPos + bytesToCopy) % player->mRingBufferSize;
        }

        if (bytesToCopy < bytesToWrite) {
            memset(outputBuffer + bytesToCopy, 0, bytesToWrite - bytesToCopy);
        }

        pthread_mutex_unlock(&player->mMutex);
    }

    return noErr;
}

} // namespace Ship
#endif
