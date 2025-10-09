#include "ship/audio/NullAudioPlayer.h"
#include <spdlog/spdlog.h>

namespace Ship {

NullAudioPlayer::~NullAudioPlayer() {
    SPDLOG_TRACE("destruct Null audio player");
}

bool NullAudioPlayer::DoInit() {
    return true;
}

int NullAudioPlayer::Buffered() {
    return 0;
}

void NullAudioPlayer::Play(const uint8_t* buf, size_t len) {
}
} // namespace Ship
