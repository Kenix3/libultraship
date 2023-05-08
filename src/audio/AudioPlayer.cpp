#include "AudioPlayer.h"

namespace LUS {
AudioPlayer::AudioPlayer(std::string backend) : mInitialized(false), mBackend(backend){};

bool AudioPlayer::Init(void) {
    mInitialized = DoInit();
    return IsInitialized();
}

bool AudioPlayer::IsInitialized(void) {
    return mInitialized;
}

std::string AudioPlayer::GetBackend() {
    return mBackend;
}
} // namespace LUS
