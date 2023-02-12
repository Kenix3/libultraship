// Copyright (c) 2019, OpenEmu Team
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the OpenEmu Team nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY OpenEmu Team ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL OpenEmu Team BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//
// Modified by David Chavez on 18.01.2023

#import "OEGameAudio.h"
#import "OEAudioUnit.h"
#import "OERingBuffer.h"
#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreAudioKit/CoreAudioKit.h>
#include <spdlog/spdlog.h>

@implementation OEGameAudio {
    AVAudioEngine               *_engine;
    id                          _token;
    AVAudioUnitGenerator        *_gen;
    BOOL                        _outputDeviceIsDefault;
    BOOL                        _running; // specifies the expected state of OEGameAudio
    id<OEAudioBuffer>           _buffer;
    const PortAudioDescription  *_audioDescription;
}

- (id)initWithAudioDescription:(const PortAudioDescription *)description {
    self = [super init];
    if (self) {
        [OEAudioUnit registerSelf];
        _volume   = 1.0;
        _running  = NO;
        _engine   = [AVAudioEngine new];
        _outputDeviceIsDefault = YES;
        _audioDescription = description;

        return self;
    }
    return self;
}

- (void)dealloc {
    [self stopMonitoringEngineConfiguration];
}

- (void)audioSampleRateDidChange {
    if (_running) {
        [_engine stop];
        [self configureNodes];
        [_engine prepare];
        [self performSelector:@selector(resumeAudio) withObject:nil afterDelay:0.020];
    }
}

- (void)startAudio {
    [self createNodes];
    [self configureNodes];
    [self attachNodes];
    [self setOutputDeviceID:self.outputDeviceID];
    
    [_engine prepare];
    // per the following, we need to wait before resuming to allow devices to start 🤦🏻‍♂️
    //  https://github.com/AudioKit/AudioKit/blob/f2a404ff6cf7492b93759d2cd954c8a5387c8b75/Examples/macOS/OutputSplitter/OutputSplitter/Audio/Output.swift#L88-L95
    [self performSelector:@selector(resumeAudio) withObject:nil afterDelay:0.020];
    [self startMonitoringEngineConfiguration];
}

- (void)stopAudio {
    [_engine stop];
    [self detachNodes];
    [self destroyNodes];
    _running = NO;
}

- (void)pauseAudio
{
    [_engine pause];
    _running = NO;
}

- (void)resumeAudio
{
    _running = YES;
    NSError *err;
    if (![_engine startAndReturnError:&err]) {
        SPDLOG_ERROR("unable to start AVAudioEngine: {}", err.localizedDescription.UTF8String);
        return;
    }
}

- (NSUInteger)write:(const void *)buffer maxLength:(NSUInteger)length {
    [_buffer write:buffer maxLength:length];
}

- (NSUInteger)bytesBuffered {
    return [(OERingBuffer *)_buffer bytesWritten];
}

- (void)startMonitoringEngineConfiguration
{
    __weak typeof(self) weakSelf = self;
    _token = [NSNotificationCenter.defaultCenter addObserverForName:AVAudioEngineConfigurationChangeNotification object:_engine queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification * _Nonnull note) {
        __strong typeof(weakSelf) strongSelf = weakSelf;
        if (!strongSelf)
            return;
            
        SPDLOG_INFO("AVAudioEngine configuration change");
        [self setOutputDeviceID:self.outputDeviceID];
    }];
}

- (void)stopMonitoringEngineConfiguration
{
    if (_token) {
        [NSNotificationCenter.defaultCenter removeObserver:_token];
    }
}

- (OEAudioBufferReadBlock)readBlockForBuffer:(id<OEAudioBuffer>)buffer {
    if ([buffer respondsToSelector:@selector(readBlock)]) {
        return [buffer readBlock];
    }

    return ^NSUInteger(void * buf, NSUInteger max) {
        return [buffer read:buf maxLength:max];
    };
}

- (AVAudioFormat *)renderFormat {
    UInt32 channelCount      = _audioDescription->mChannelCount;
    UInt32 bytesPerSample    = _audioDescription->mBitDepth / 8;
    
    CAFFormatFlags formatFlags;
    if (bytesPerSample == 4) {
        // assume 32-bit float
        formatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagsNativeEndian | kAudioFormatFlagIsNonInterleaved | kAudioFormatFlagIsPacked;
    } else {
        formatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagsNativeEndian;
    }
    
    AudioStreamBasicDescription mDataFormat = {
        .mSampleRate       = _audioDescription->mSampleRate,
        .mFormatID         = kAudioFormatLinearPCM,
        .mFormatFlags      = formatFlags,
        .mBytesPerPacket   = bytesPerSample * channelCount,
        .mFramesPerPacket  = 1,
        .mBytesPerFrame    = bytesPerSample * channelCount,
        .mChannelsPerFrame = channelCount,
        .mBitsPerChannel   = 8 * bytesPerSample,
    };
    
    return [[AVAudioFormat alloc] initWithStreamDescription:&mDataFormat];
}

- (AudioDeviceID)defaultAudioOutputDeviceID {
    AudioObjectPropertyAddress addr = {
        .mSelector = kAudioHardwarePropertyDefaultOutputDevice,
        .mScope    = kAudioObjectPropertyScopeGlobal,
        .mElement  = kAudioObjectPropertyElementMaster,
    };
    
    AudioObjectID deviceID = kAudioDeviceUnknown;
    UInt32 size = sizeof(deviceID);
    AudioObjectGetPropertyData(kAudioObjectSystemObject, &addr, 0, nil, &size, &deviceID);
    return deviceID;
}

- (void)configureNodes {
    AVAudioFormat *renderFormat = [self renderFormat];
    OEAudioUnit     *au  = (OEAudioUnit*)_gen.AUAudioUnit;
    NSError         *err;
    AUAudioUnitBus  *bus = au.inputBusses[0];
    if (![bus setFormat:renderFormat error:&err]) {
        SPDLOG_ERROR("unable to set input bus render format: {}", err.localizedDescription.UTF8String);
        return;
    }

    OEAudioBufferReadBlock read = [self readBlockForBuffer:[self audioBuffer]];
    AudioStreamBasicDescription const *src = renderFormat.streamDescription;
    NSUInteger      bytesPerFrame = src->mBytesPerFrame;
    UInt32          channelCount  = src->mChannelsPerFrame;
    
    au.outputProvider = ^AUAudioUnitStatus(AudioUnitRenderActionFlags *actionFlags, const AudioTimeStamp *timestamp, AUAudioFrameCount frameCount, NSInteger inputBusNumber, AudioBufferList *inputData) {
        NSUInteger bytesRequested = frameCount * bytesPerFrame;
        NSUInteger bytesCopied    = read(inputData->mBuffers[0].mData, bytesRequested);
        
        inputData->mBuffers[0].mDataByteSize = (UInt32)bytesCopied;
        inputData->mBuffers[0].mNumberChannels = channelCount;
        
        return noErr;
    };
}

- (void)createNodes {
    AudioComponentDescription desc = {
        .componentType          = kAudioUnitType_Generator,
        .componentSubType       = kAudioUnitSubType_Emulator,
        .componentManufacturer  = kAudioUnitManufacturer_OpenEmu,
    };
    _gen = [[AVAudioUnitGenerator alloc] initWithAudioComponentDescription:desc];
}

- (void)destroyNodes {
    _gen = nil;
}

- (void)connectNodes {
    [_engine connect:_gen to:_engine.mainMixerNode format:nil];
    _engine.mainMixerNode.outputVolume = _volume;
}

- (void)attachNodes {
    [_engine attachNode:_gen];
}

- (void)detachNodes {
    [_engine detachNode:_gen];
}

- (AudioDeviceID)outputDeviceID
{
    return _outputDeviceIsDefault ? 0 : _engine.outputNode.AUAudioUnit.deviceID;
}

- (void)setOutputDeviceID:(AudioDeviceID)outputDeviceID
{
    if (outputDeviceID == 0) {
        outputDeviceID = [self defaultAudioOutputDeviceID];
        SPDLOG_INFO("using default audio device {}", outputDeviceID);
    } else {
        _outputDeviceIsDefault = NO;
    }
    
    [_engine stop];
    NSError *err;
    if (![_engine.outputNode.AUAudioUnit setDeviceID:outputDeviceID error:&err]) {
        SPDLOG_ERROR("unable to set output device ID {}: {}", outputDeviceID, err.localizedDescription.UTF8String);
        return;
    }
    [self connectNodes];
    
    if (_running && !_engine.isRunning) {
        [_engine prepare];
        [self performSelector:@selector(resumeAudio) withObject:nil afterDelay:0.020];
    }
}

- (id<OEAudioBuffer>)audioBuffer
{
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"
    return [self ringBuffer];
    #pragma clang diagnostic pop
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-implementations"
- (OERingBuffer *)ringBuffer
{
    OERingBuffer *result = _buffer;
    if (_buffer == nil) {
        /* ring buffer is 0.05 seconds
         * the larger the buffer, the higher the maximum possible audio lag */
        double frameSampleCount = _audioDescription->mSampleRate * 0.1;
        NSUInteger channelCount = _audioDescription->mChannelCount;
        NSUInteger bytesPerSample = _audioDescription->mBitDepth / 8;
        NSAssert(frameSampleCount, @"frameSampleCount is 0");
        NSUInteger len = channelCount * bytesPerSample * frameSampleCount;
        NSUInteger coreRequestedLen = [self audioBufferSize] * 2;
        len = MAX(coreRequestedLen, len);

        result = [[OERingBuffer alloc] initWithLength:len];
        [result setDiscardPolicy:OERingBufferDiscardPolicyOldest];
        [result setAnticipatesUnderflow:YES];
        _buffer = result;
    }

    return result;
}
#pragma clang diagnostic pop

- (NSUInteger)audioBufferSize
{
    double frameSampleCount = _audioDescription->mSampleRate / _audioDescription->mFrameInterval;
    NSUInteger channelCount = _audioDescription->mChannelCount;
    NSUInteger bytesPerSample = _audioDescription->mBitDepth / 8;
    NSAssert(frameSampleCount, @"frameSampleCount is 0");
    return channelCount * bytesPerSample * frameSampleCount;
}

- (void)setVolume:(float)aVolume
{
    _volume = aVolume;
    if (_engine) {
        _engine.mainMixerNode.outputVolume = _volume;
    }
}


@end
