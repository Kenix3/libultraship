#pragma once

#ifdef _WIN32

#include "AudioPlayer.h"
#include <wrl/client.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <mutex>

using namespace Microsoft::WRL;

namespace Ship {
/**
 * @brief AudioPlayer implementation backed by the Windows Audio Session API (WASAPI).
 *
 * WasapiAudioPlayer opens a shared-mode WASAPI audio client and also implements
 * IMMNotificationClient so it can detect when the default audio device changes at
 * runtime and seamlessly switch to the new device without requiring a restart.
 *
 * This backend is only compiled on Windows (`#ifdef _WIN32`).
 */
class WasapiAudioPlayer : public AudioPlayer, public IMMNotificationClient {
  public:
    /**
     * @brief Constructs a WasapiAudioPlayer with the given audio settings.
     * @param settings Sample rate, buffer size, desired buffered frames, and channel mode.
     */
    WasapiAudioPlayer(AudioSettings settings) : AudioPlayer(settings) {
    }

    /**
     * @brief Returns the number of frames currently queued in the WASAPI render buffer.
     *
     * Used by the audio subsystem to pace audio production.
     */
    int Buffered() override;

  protected:
    /**
     * @brief Opens the WASAPI shared-mode audio client and registers device notifications.
     * @return true if the device was initialised successfully.
     */
    bool DoInit() override;

    /**
     * @brief Stops playback, unregisters device notifications, and releases COM objects.
     */
    void DoClose() override;

    /**
     * @brief Writes interleaved PCM samples to the WASAPI render buffer.
     * @param buf Sample data (stereo or surround, depending on channel setting).
     * @param len Length of @p buf in bytes.
     */
    void DoPlay(const uint8_t* buf, size_t len) override;

    // IMMNotificationClient overrides — used to detect audio device changes.

    /** @brief Called when the state of an audio endpoint device changes. */
    virtual HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState);

    /** @brief Called when a new audio endpoint device is added to the system. */
    virtual HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId);

    /** @brief Called when an audio endpoint device is removed from the system. */
    virtual HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId);

    /**
     * @brief Called when the default audio endpoint for a given data-flow or role changes.
     *
     * WasapiAudioPlayer uses this callback to tear down the current stream and
     * reopen it on the new default device.
     */
    virtual HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role,
                                                             LPCWSTR pwstrDefaultDeviceId);

    /** @brief Called when a property value of an audio endpoint device changes. */
    virtual HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key);

    /** @brief Increments the IUnknown reference count. */
    virtual ULONG STDMETHODCALLTYPE AddRef();

    /** @brief Decrements the IUnknown reference count. */
    virtual ULONG STDMETHODCALLTYPE Release();

    /** @brief Returns the requested interface if supported; otherwise E_NOINTERFACE. */
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID** ppvInterface);

    /**
     * @brief Throws a std::runtime_error if @p res is a failed HRESULT.
     * @param res HRESULT to check.
     */
    void ThrowIfFailed(HRESULT res);

    /**
     * @brief (Re-)opens the WASAPI audio stream on the current default device.
     * @return true on success.
     */
    bool SetupStream();

  private:
    ComPtr<IMMDeviceEnumerator> mDeviceEnumerator; ///< Enumerates available audio endpoints.
    ComPtr<IMMDevice> mDevice;                     ///< Currently active audio endpoint.
    ComPtr<IAudioClient> mClient;                  ///< WASAPI audio client for the active device.
    ComPtr<IAudioRenderClient> mRenderClient;       ///< WASAPI render client for writing PCM data.
    LONG mRefCount = 1;                            ///< IUnknown reference count.
    UINT32 mBufferFrameCount = 0;                  ///< Size of the WASAPI render buffer in frames.
    bool mInitialized = false;                     ///< True after DoInit() succeeds.
    bool mStarted = false;                         ///< True after IAudioClient::Start() is called.
    int32_t mNumChannels = 2;                      ///< Number of output channels (2 or 6).
    std::mutex mMutex;                             ///< Serialises DoPlay() and device-change callbacks.
};
} // namespace Ship
#endif
