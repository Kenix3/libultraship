#include "AudioPlayer.h"

namespace Ship {
AudioPlayer::AudioPlayer() : mInitialized(false){};

bool AudioPlayer::Init(void) {
    mInitialized = doInit();
    return IsInitialized();
}

bool AudioPlayer::IsInitialized(void) {
    return mInitialized;
}

} // namespace Ship