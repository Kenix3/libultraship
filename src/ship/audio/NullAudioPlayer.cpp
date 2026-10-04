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
    // Report the device as always full; it discards all audio, so returning the
    // true count (0) makes the producer spin feeding it forever and hang.
    return GetDesiredBuffered();
}

void NullAudioPlayer::DoPlay(const uint8_t* buf, size_t len) {
}
} // namespace Ship
