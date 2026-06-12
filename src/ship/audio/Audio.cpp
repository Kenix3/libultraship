#include "ship/audio/Audio.h"

#ifdef __APPLE__
#include "ship/audio/CoreAudioAudioPlayer.h"
#endif

#include "ship/Context.h"
#include "ship/config/Config.h"
#include "ship/controller/controldeck/ControlDeck.h"

namespace Ship {

Audio::~Audio() {
    SPDLOG_TRACE("destruct audio");
}

void Audio::InitAudioPlayer() {
    switch (GetCurrentAudioBackend()) {
#ifdef _WIN32
        case AudioBackend::WASAPI:
            mAudioPlayer = std::make_shared<WasapiAudioPlayer>(this->mAudioSettings);
            break;
#endif
#ifdef __APPLE__
        case AudioBackend::COREAUDIO:
            mAudioPlayer = std::make_shared<CoreAudioAudioPlayer>(this->mAudioSettings);
            break;
#endif
        case AudioBackend::SDL:
            mAudioPlayer = std::make_shared<SDLAudioPlayer>(this->mAudioSettings);
            break;
        default:
            mAudioPlayer = std::make_shared<NullAudioPlayer>(this->mAudioSettings);
            break;
    }

    if (mAudioPlayer && !mAudioPlayer->Init()) {
        // Failed to initialize system audio player.
        // Fallback to Null if the native system player does not work. That path
        // re-enters InitAudioPlayer (and fires the hook for the Null player), so
        // return here to avoid also firing it for the failed player.
        SetCurrentAudioBackend(AudioBackend::NUL);
        return;
    }

    // A fresh AudioPlayer is live. It already inherits the float-pipeline mode
    // via mAudioSettings; the hook lets the host re-attach instance-bound state
    // the player cannot carry across a rebuild (e.g. FluidSynth's mix source).
    if (mOnAudioPlayerInitialized) {
        mOnAudioPlayerInitialized();
    }
}

bool Audio::IsUsingFloatPipeline() const {
    return mUseFloatPipeline.load(std::memory_order_acquire);
}

bool Audio::SetUseFloatPipeline(bool enabled) {
    // Authority first: update the live flag and the template new players inherit,
    // so anything constructed from here on comes up in the right mode.
    mAudioSettings.UseFloatPipeline = enabled;
    mUseFloatPipeline.store(enabled, std::memory_order_release);

    if (mAudioPlayer && !mAudioPlayer->SetUseFloatPipeline(enabled)) {
        // The player refused the requested mode; reflect what it actually settled
        // on so producer and consumer stay in agreement.
        const bool actual = mAudioPlayer->IsUsingFloatPipeline();
        mAudioSettings.UseFloatPipeline = actual;
        mUseFloatPipeline.store(actual, std::memory_order_release);
        return false;
    }
    return true;
}

void Audio::SetOnAudioPlayerInitialized(std::function<void()> callback) {
    mOnAudioPlayerInitialized = std::move(callback);
}

void Audio::Init() {
    mConfig = Context::GetInstance()->GetConfig();

    mAvailableAudioBackends = std::make_shared<std::vector<AudioBackend>>();
#ifdef _WIN32
    mAvailableAudioBackends->push_back(AudioBackend::WASAPI);
#endif
#ifdef __APPLE__
    mAvailableAudioBackends->push_back(AudioBackend::COREAUDIO);
#endif
    mAvailableAudioBackends->push_back(AudioBackend::SDL);
    mAvailableAudioBackends->push_back(AudioBackend::NUL);

    SetCurrentAudioBackend(GetSavedAudioBackend());
    SetAudioChannels(GetSavedAudioChannelsSetting());
}

std::shared_ptr<AudioPlayer> Audio::GetAudioPlayer() {
    return mAudioPlayer;
}

AudioBackend Audio::GetCurrentAudioBackend() {
    return mAudioBackend;
}

AudioBackend Audio::GetSavedAudioBackend() {
    std::string backendName = mConfig->GetString("Window.AudioBackend");
    if (backendName == "wasapi") {
        return AudioBackend::WASAPI;
    }

    // Migrate pulse player in config to sdl
    if (backendName == "pulse") {
        mConfig->SetString("Window.AudioBackend", "sdl");
        mConfig->Save();
        return AudioBackend::SDL;
    }

    if (backendName == "coreaudio") {
        return AudioBackend::COREAUDIO;
    }

    if (backendName == "sdl") {
        return AudioBackend::SDL;
    }

    if (backendName == "null") {
        return AudioBackend::NUL;
    }

    SPDLOG_TRACE("Could not find AudioBackend matching value from config file ({}). Returning default AudioBackend.",
                 backendName);
#ifdef _WIN32
    return AudioBackend::WASAPI;
#endif

#ifdef __APPLE__
    return AudioBackend::COREAUDIO;
#endif

    return AudioBackend::SDL;
}

void Audio::SetCurrentAudioBackend(AudioBackend backend) {
    mAudioBackend = backend;

    switch (backend) {
        case AudioBackend::WASAPI:
            mConfig->SetString("Window.AudioBackend", "wasapi");
            break;
        case AudioBackend::COREAUDIO:
            mConfig->SetString("Window.AudioBackend", "coreaudio");
            break;
        case AudioBackend::SDL:
            mConfig->SetString("Window.AudioBackend", "sdl");
            break;
        case AudioBackend::NUL:
            mConfig->SetString("Window.AudioBackend", "null");
            break;
        default:
            mConfig->SetString("Window.AudioBackend", "");
    }
    mConfig->Save();

    // The new player inherits the float-pipeline mode from mAudioSettings (kept
    // authoritative by SetUseFloatPipeline), so it comes up in the correct mode
    // by construction. InitAudioPlayer's hook then re-attaches any instance-bound
    // state (e.g. the FluidSynth mix source).
    InitAudioPlayer();
}

std::shared_ptr<std::vector<AudioBackend>> Audio::GetAvailableAudioBackends() {
    return mAvailableAudioBackends;
}

void Audio::SetAudioChannels(AudioChannelsSetting channels) {
    if (mAudioSettings.ChannelSetting != channels) {
        mAudioSettings.ChannelSetting = channels;
        // Reinitialize the existing audio player with the new channel configuration
        if (mAudioPlayer) {
            mAudioPlayer->SetAudioChannels(channels);
        }
    }
}

AudioChannelsSetting Audio::GetAudioChannels() const {
    return mAudioSettings.ChannelSetting;
}

AudioChannelsSetting Audio::GetSavedAudioChannelsSetting() {
    int32_t channelsSetting =
        mConfig->GetInt("CVars." CVAR_AUDIO_CHANNELS_SETTING, static_cast<int32_t>(AudioChannelsSetting::audioMax));
    switch (channelsSetting) {
        case AudioChannelsSetting::audioMatrix51:
            return AudioChannelsSetting::audioMatrix51;
        case AudioChannelsSetting::audioRaw51:
            return AudioChannelsSetting::audioRaw51;
        case AudioChannelsSetting::audioStereo:
        case AudioChannelsSetting::audioMax:
        default:
            return AudioChannelsSetting::audioStereo;
    }
}

} // namespace Ship
