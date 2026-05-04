#ifdef __APPLE__
#include "ship/audio/CoreAudioAudioPlayer.h"
#include <spdlog/spdlog.h>
#include <cstring>

namespace Ship {

CoreAudioAudioPlayer::CoreAudioAudioPlayer(AudioSettings settings)
    : AudioPlayer(settings), mAudioUnit(nullptr), mNumChannels(0), mRingBuffer(nullptr), mRingBufferSize(0),
      mRingBufferReadPos(0), mRingBufferWritePos(0), mInitialized(false) {
}

CoreAudioAudioPlayer::~CoreAudioAudioPlayer() {
    SPDLOG_TRACE("destruct CoreAudio audio player");
    DoClose();
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

    mNumChannels = GetNumOutputChannels();

    const size_t bytesPerSample = sizeof(int16_t);
    const size_t bytesPerFrame = bytesPerSample * mNumChannels;

    // Reserve one extra frame so write == read unambiguously means "empty";
    // a buffer is full when (write + frame) % size == read. Usable capacity
    // stays at 6000 frames, matching the prior implementation.
    mRingBufferSize = (6000 + 1) * bytesPerFrame;
    mRingBuffer = new uint8_t[mRingBufferSize];
    mRingBufferReadPos.store(0, std::memory_order_relaxed);
    mRingBufferWritePos.store(0, std::memory_order_relaxed);

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
    size_t r = mRingBufferReadPos.load(std::memory_order_acquire);
    size_t w = mRingBufferWritePos.load(std::memory_order_acquire);
    size_t buffered = (w >= r) ? (w - r) : (mRingBufferSize - (r - w));
    return buffered / (sizeof(int16_t) * mNumChannels);
}

void CoreAudioAudioPlayer::DoPlay(const uint8_t* buf, size_t len) {
    const size_t bytesPerFrame = sizeof(int16_t) * mNumChannels;
    size_t r = mRingBufferReadPos.load(std::memory_order_acquire);
    size_t w = mRingBufferWritePos.load(std::memory_order_relaxed);
    // free space leaves one frame reserved so write can never collide with read
    size_t freeSpace = (w >= r) ? (mRingBufferSize - (w - r) - bytesPerFrame)
                                : (r - w - bytesPerFrame);

    size_t toWrite = len <= freeSpace ? len : freeSpace;
    if (toWrite == 0) {
        return;
    }

    size_t writeEnd = w + toWrite;
    if (writeEnd <= mRingBufferSize) {
        memcpy(mRingBuffer + w, buf, toWrite);
    } else {
        size_t firstChunk = mRingBufferSize - w;
        memcpy(mRingBuffer + w, buf, firstChunk);
        memcpy(mRingBuffer, buf + firstChunk, toWrite - firstChunk);
    }
    mRingBufferWritePos.store((w + toWrite) % mRingBufferSize, std::memory_order_release);
}

OSStatus CoreAudioAudioPlayer::CoreAudioRenderCallback(void* inRefCon, AudioUnitRenderActionFlags* ioActionFlags,
                                                       const AudioTimeStamp* inTimeStamp, UInt32 inBusNumber,
                                                       UInt32 inNumberFrames, AudioBufferList* ioData) {
    CoreAudioAudioPlayer* player = static_cast<CoreAudioAudioPlayer*>(inRefCon);

    for (UInt32 i = 0; i < ioData->mNumberBuffers; i++) {
        AudioBuffer* buffer = &ioData->mBuffers[i];
        UInt8* outputBuffer = static_cast<UInt8*>(buffer->mData);
        UInt32 bytesToWrite = buffer->mDataByteSize;

        size_t r = player->mRingBufferReadPos.load(std::memory_order_relaxed);
        size_t w = player->mRingBufferWritePos.load(std::memory_order_acquire);
        size_t available = (w >= r) ? (w - r) : (player->mRingBufferSize - (r - w));

        UInt32 bytesToCopy = bytesToWrite;
        if (bytesToCopy > available) {
            bytesToCopy = available;
        }

        if (bytesToCopy > 0) {
            size_t readEnd = r + bytesToCopy;
            if (readEnd <= player->mRingBufferSize) {
                memcpy(outputBuffer, player->mRingBuffer + r, bytesToCopy);
            } else {
                size_t firstChunk = player->mRingBufferSize - r;
                memcpy(outputBuffer, player->mRingBuffer + r, firstChunk);
                memcpy(outputBuffer + firstChunk, player->mRingBuffer, bytesToCopy - firstChunk);
            }
            player->mRingBufferReadPos.store((r + bytesToCopy) % player->mRingBufferSize,
                                             std::memory_order_release);
        }

        if (bytesToCopy < bytesToWrite) {
            memset(outputBuffer + bytesToCopy, 0, bytesToWrite - bytesToCopy);
        }
    }

    return noErr;
}

} // namespace Ship
#endif
