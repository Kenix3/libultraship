#ifdef _WIN32
#include "ship/audio/WasapiAudioPlayer.h"
#include "ship/utils/HResultException.h"
#include <spdlog/spdlog.h>

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

void WasapiAudioPlayer::ThrowIfFailed(HRESULT res) {
    if (FAILED(res)) {
        throw HResultException(res);
    }
}

WasapiAudioPlayer::~WasapiAudioPlayer() {
    DoClose();

    if (mDeviceEnumerator) {
        mDeviceEnumerator->UnregisterEndpointNotificationCallback(this);
        mDeviceEnumerator.Reset();
    }
}

bool WasapiAudioPlayer::SetupStream() {
    try {
        ThrowIfFailed(mDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &mDevice));
        ThrowIfFailed(mDevice->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr, IID_PPV_ARGS_Helper(&mClient)));

        // Use GetNumOutputChannels() to determine stereo vs surround
        mNumChannels = this->GetNumOutputChannels();

        if (mNumChannels == 2) {
            WAVEFORMATEX desired;
            desired.wFormatTag = WAVE_FORMAT_PCM;
            desired.nChannels = mNumChannels; // Stereo audio
            desired.wBitsPerSample = 16;      // 16-bit audio
            desired.nSamplesPerSec = this->GetSampleRate();
            desired.nBlockAlign = desired.nChannels * desired.wBitsPerSample / 8;
            desired.nAvgBytesPerSec = desired.nSamplesPerSec * desired.nBlockAlign;
            desired.cbSize = 0;

            ThrowIfFailed(mClient->Initialize(
                AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
                2000000, 0, &desired, nullptr));
        } else if (mNumChannels == 6) {
            // 5.1 surround (6 channels)
            WAVEFORMATEXTENSIBLE desired;
            desired.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
            desired.Format.nChannels = mNumChannels; // 6 channels for 5.1 audio
            desired.Format.wBitsPerSample = 16;      // 16-bit audio
            desired.Format.nSamplesPerSec = this->GetSampleRate();
            desired.Format.nBlockAlign = desired.Format.nChannels * desired.Format.wBitsPerSample / 8;
            desired.Format.nAvgBytesPerSec = desired.Format.nSamplesPerSec * desired.Format.nBlockAlign;
            desired.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
            desired.dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
            desired.Samples.wValidBitsPerSample = 16;
            desired.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

            ThrowIfFailed(mClient->Initialize(
                AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
                2000000, 0, (WAVEFORMATEX*)&desired, nullptr));
        }

        ThrowIfFailed(mClient->GetBufferSize(&mBufferFrameCount));
        ThrowIfFailed(mClient->GetService(IID_PPV_ARGS(&mRenderClient)));

        mStarted = false;
        mInitialized = true;
    } catch (const HResultException& e) {
        SPDLOG_ERROR("WasapiAudioPlayer::SetupStream failed: {}", e.what());
        return false;
    }

    return true;
}

bool WasapiAudioPlayer::DoInit() {
    try {
        if (!mDeviceEnumerator) {
            ThrowIfFailed(
                CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&mDeviceEnumerator)));
            ThrowIfFailed(mDeviceEnumerator->RegisterEndpointNotificationCallback(this));
        }
    } catch (const HResultException& e) {
        SPDLOG_ERROR("WasapiAudioPlayer::DoInit failed: {}", e.what());
        return false;
    }

    return true;
}

void WasapiAudioPlayer::DoClose() {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mClient) {
        mClient->Stop();
    }
    mRenderClient.Reset();
    mClient.Reset();
    mDevice.Reset();
    mInitialized = false;
    mStarted = false;
}

int WasapiAudioPlayer::Buffered() {
    std::lock_guard<std::mutex> lock(mMutex);
    if (!mInitialized) {
        if (!SetupStream()) {
            return GetDesiredBuffered();
        }
    }
    try {
        UINT32 padding;
        ThrowIfFailed(mClient->GetCurrentPadding(&padding));
        // Initialize may allocate a buffer smaller than the caller's target, which the
        // padding could then never reach; report the target as met once the endpoint
        // buffer is genuinely full, the same way an unusable device does below.
        return padding >= mBufferFrameCount ? GetDesiredBuffered() : static_cast<int>(padding);
    } catch (const HResultException& e) {
        SPDLOG_ERROR("WasapiAudioPlayer::Buffered failed: {}", e.what());
        return GetDesiredBuffered();
    }
}

void WasapiAudioPlayer::DoPlay(const uint8_t* buf, size_t len) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (!mInitialized) {
        if (!SetupStream()) {
            return;
        }
    }
    try {
        UINT32 frames = len / (mNumChannels * sizeof(int16_t));
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
        memcpy(data, buf, frames * mNumChannels * sizeof(int16_t));
        ThrowIfFailed(mRenderClient->ReleaseBuffer(frames, 0));

        // Initialize may allocate a smaller buffer than it was asked for, so a fixed prefill
        // target can exceed the whole buffer and the stream then never starts at all.
        const UINT32 prefill = mBufferFrameCount / 2 < 1500 ? mBufferFrameCount / 2 : 1500;
        if (!mStarted && padding + frames > prefill) {
            mStarted = true;
            ThrowIfFailed(mClient->Start());
        }
    } catch (const HResultException& e) { SPDLOG_ERROR("WasapiAudioPlayer::DoPlay failed: {}", e.what()); }
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
        // This callback runs on a separate thread, so we need to protect mInitialized
        std::lock_guard<std::mutex> lock(mMutex);
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
    // No delete on zero: Audio owns this through a shared_ptr.
    return InterlockedDecrement(&mRefCount);
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
