#ifdef _WIN32
#include "WasapiAudioPlayer.h"
#include <spdlog/spdlog.h>
#include <cmath>
#include <thread>
#include <iostream>

// These constants are currently missing from the MinGW headers.
#ifndef AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
#define AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM 0x80000000
#endif
#ifndef AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY
#define AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY 0x08000000
#endif

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);

namespace Ship {

const double PI = 3.14159265358979323846;

void GenerateSineWave(int16_t* buffer, int sampleRate, int frequency, int duration, int numChannels, int channel) {
    int numSamples = sampleRate * duration;
    double amplitude = 32760; // Max amplitude for 16-bit audio
    double phaseIncrement = 2.0 * PI * frequency / sampleRate;

    for (int i = 0; i < numSamples; ++i) {
        double sample = amplitude * sin(phaseIncrement * i);
        for (int ch = 0; ch < numChannels; ++ch) {
            if (ch == channel) {
                buffer[i * numChannels + ch] = static_cast<int16_t>(sample);
            } else {
                buffer[i * numChannels + ch] = 0;
            }
        }
    }
}

void WasapiAudioPlayer::ThrowIfFailed(HRESULT res) {
    if (FAILED(res)) {
        throw res;
    }
}

bool WasapiAudioPlayer::SetupStream() {
    try {
        ThrowIfFailed(mDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &mDevice));
        ThrowIfFailed(mDevice->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, IID_PPV_ARGS_Helper(&mClient)));

        WAVEFORMATEXTENSIBLE desired;
        desired.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        desired.Format.nChannels = 6; // 6 channels for 5.1 audio
        desired.Format.wBitsPerSample = 16; // 16-bit audio
        desired.Format.nSamplesPerSec = this->GetSampleRate();

        std::cout << "Sample rate: " << desired.Format.nSamplesPerSec << std::endl;

        desired.Format.nBlockAlign = desired.Format.nChannels * desired.Format.wBitsPerSample / 8;
        desired.Format.nAvgBytesPerSec = desired.Format.nSamplesPerSec * desired.Format.nBlockAlign; // 2 bytes per sample (16-bit audio)
        desired.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
        desired.dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
        desired.Samples.wValidBitsPerSample = 16;
        desired.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

        ThrowIfFailed(mClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                          AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
                                          2000000, 0, (WAVEFORMATEX*) &desired, nullptr));

        std::cout << "Audio stream initialized" << std::endl;

        ThrowIfFailed(mClient->GetBufferSize(&mBufferFrameCount));
        ThrowIfFailed(mClient->GetService(IID_PPV_ARGS(&mRenderClient)));

        mStarted = false;
        mInitialized = true;

        // // Test 6-channel audio by playing a tone on each channel for 1 second
        // int sampleRate = this->GetSampleRate();
        // int duration = 1;
        // int16_t* buffer = new int16_t[sampleRate * desired.Format.nChannels * duration];
        // for (int ch = 0; ch < desired.Format.nChannels; ++ch) {
        //     GenerateSineWave(buffer, sampleRate, 440, duration, desired.Format.nChannels, ch); // 440 Hz tone
        //     Play(reinterpret_cast<uint8_t*>(buffer), sampleRate * desired.Format.nChannels * 2 * duration);
        //     std::this_thread::sleep_for(std::chrono::milliseconds(duration * 1000));
        // }
        // delete[] buffer;

    } catch (HRESULT res) {    
        std::cout << "Audio stream not initialized" << std::endl;
        return false;
    }

    return true;
}

bool WasapiAudioPlayer::DoInit() {
    try {
        ThrowIfFailed(
            CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&mDeviceEnumerator)));
    } catch (HRESULT res) { return false; }

    ThrowIfFailed(mDeviceEnumerator->RegisterEndpointNotificationCallback(this));

    return true;
}

int WasapiAudioPlayer::Buffered() {
    if (!mInitialized) {
        if (!SetupStream()) {
            return 0;
        }
    }
    try {
        UINT32 padding;
        ThrowIfFailed(mClient->GetCurrentPadding(&padding));
        return padding;
    } catch (HRESULT res) { return 0; }
}

void WasapiAudioPlayer::Play(const uint8_t* buf, size_t len) {
    if (!mInitialized) {
        if (!SetupStream()) {
            return;
        }
    }
    try {
        // Calculate the number of frames based on 6 channels (5.1 audio)
        UINT32 frames = len / (6 * 2); // 6 channels, 2 bytes per sample (16-bit audio)

        UINT32 padding;
        ThrowIfFailed(mClient->GetCurrentPadding(&padding));

        UINT32 available = mBufferFrameCount - padding;
        if (available < frames) {
            frames = available;
        }
        if (available == 0) {
            return;
        }

        BYTE* data;
        ThrowIfFailed(mRenderClient->GetBuffer(frames, &data));
        memcpy(data, buf, frames * 6 * 2); // 6 channels, 2 bytes per sample
        ThrowIfFailed(mRenderClient->ReleaseBuffer(frames, 0));

        if (!mStarted && padding + frames > 1500) {
            mStarted = true;
            ThrowIfFailed(mClient->Start());
        }
    } catch (HRESULT res) {}
}

HRESULT STDMETHODCALLTYPE WasapiAudioPlayer::OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WasapiAudioPlayer::OnDeviceAdded(LPCWSTR pwstrDeviceId) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WasapiAudioPlayer::OnDeviceRemoved(LPCWSTR pwstrDeviceId) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WasapiAudioPlayer::OnDefaultDeviceChanged(EDataFlow flow, ERole role,
                                                                    LPCWSTR pwstrDefaultDeviceId) {
    if (flow == eRender && role == eConsole) {
        // This callback runs on a separate thread,
        // but it's not important how fast this write takes effect.
        mInitialized = false;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WasapiAudioPlayer::OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key) {
    return S_OK;
}

ULONG STDMETHODCALLTYPE WasapiAudioPlayer::AddRef() {
    return InterlockedIncrement(&mRefCount);
}

ULONG STDMETHODCALLTYPE WasapiAudioPlayer::Release() {
    ULONG rc = InterlockedDecrement(&mRefCount);
    if (rc == 0) {
        delete this;
    }
    return rc;
}

HRESULT STDMETHODCALLTYPE WasapiAudioPlayer::QueryInterface(REFIID riid, VOID** ppvInterface) {
    if (riid == __uuidof(IUnknown)) {
        AddRef();
        *ppvInterface = (IUnknown*)this;
    } else if (riid == __uuidof(IMMNotificationClient)) {
        AddRef();
        *ppvInterface = (IMMNotificationClient*)this;
    } else {
        *ppvInterface = nullptr;
        return E_NOINTERFACE;
    }
    return S_OK;
}
} // namespace Ship
#endif
