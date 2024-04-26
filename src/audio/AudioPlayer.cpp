#include "AudioPlayer.h"
#include "spdlog/spdlog.h"

namespace ShipDK {
AudioPlayer::AudioPlayer() : mInitialized(false){};

AudioPlayer::~AudioPlayer() {
    SPDLOG_TRACE("destruct audio player");
}

bool AudioPlayer::Init(void) {
    mInitialized = DoInit();
    return IsInitialized();
}

bool AudioPlayer::IsInitialized(void) {
    return mInitialized;
}
} // namespace ShipDK
