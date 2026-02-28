#include "ship/audio/NullAudioPlayer.h"
#include <spdlog/spdlog.h>

namespace Ship {

NullAudioPlayer::~NullAudioPlayer() {
    SPDLOG_TRACE("destruct Null audio player");
}

bool NullAudioPlayer::DoInit() {
    return true;
}

void NullAudioPlayer::DoClose() {
    // Nothing to close for null player
}

int NullAudioPlayer::Buffered() {
    return 0;
}

void NullAudioPlayer::DoPlay(const uint8_t* buf, size_t len) {
}

void NullAudioPlayer::SetMuted(bool muted) {
}

bool NullAudioPlayer::IsMuted() const {
    return false;
}

} // namespace Ship
