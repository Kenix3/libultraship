#pragma once

#include <string>
#include <memory>
#include "audio/AudioPlayer.h"

namespace LUS {
enum class AudioBackend { WASAPI, PULSE, SDL };


class Audio {
public:
    void Init();
    std::shared_ptr<AudioPlayer> GetAudioPlayer();
    AudioBackend GetAudioBackend();
    void SetAudioBackend(AudioBackend backend);
protected:
    static AudioBackend DetermineAudioBackendFromConfig();
    static std::string DetermineAudioBackendNameFromBackend(AudioBackend backend);
private:
    std::shared_ptr<AudioPlayer> mAudioPlayer;
    AudioBackend mAudioBackend;
};
} // namespace LUS
