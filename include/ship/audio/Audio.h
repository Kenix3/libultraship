#pragma once

#include <string>
#include <memory>
#include <vector>
#include "ship/audio/AudioPlayer.h"
#include "ship/Component.h"

namespace Ship {
enum class AudioBackend { WASAPI, SDL, NUL };

class Audio : public Component {
  public:
    Audio(AudioSettings settings) : Component("Audio"), mAudioSettings(settings) {
    }
    ~Audio();

    void Init();
    std::shared_ptr<AudioPlayer> GetAudioPlayer();
    AudioBackend GetCurrentAudioBackend();
    std::shared_ptr<std::vector<AudioBackend>> GetAvailableAudioBackends();
    void SetCurrentAudioBackend(AudioBackend backend);

  protected:
    void InitAudioPlayer();

  private:
    std::shared_ptr<AudioPlayer> mAudioPlayer;
    AudioBackend mAudioBackend;
    AudioSettings mAudioSettings;
    std::shared_ptr<std::vector<AudioBackend>> mAvailableAudioBackends;
};
} // namespace Ship
