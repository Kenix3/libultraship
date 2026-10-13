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
    if (mDeviceEnumerator) {
        mDeviceEnumerator->UnregisterEndpointNotificationCallback(this);
        mDeviceEnumerator.Reset();
    }

    DoClose();
}

bool WasapiAudioPlayer::SetupStream() {
    const uint32_t generation = mDeviceGeneration.load(std::memory_order_acquire);

    // Dropping a running client without stopping it leaves the old endpoint streaming.
    if (mClient) {
        mClient->Stop();
        mRenderClient.Reset();
        mClient.Reset();
    }

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
        // A device change during setup leaves this stream on the old endpoint, so discard it.
        mInitialized.store(generation == mDeviceGeneration.load(std::memory_order_acquire), std::memory_order_release);
    } catch (const HResultException& e) {
        SPDLOG_ERROR("WasapiAudioPlayer::SetupStream failed: {}", e.what());
        return false;
    }

    return mInitialized.load(std::memory_order_acquire);
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
    mInitialized.store(false, std::memory_order_release);
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
        return padding;
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

        if (!mStarted && padding + frames > 1500) {
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
        // Taking mMutex here deadlocks: DoPlay() holds it across MMDevice calls that cannot
        // complete until this callback returns.
        mDeviceGeneration.fetch_add(1, std::memory_order_acq_rel);
        mInitialized.store(false, std::memory_order_release);
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
