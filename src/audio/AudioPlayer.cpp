#include "AudioPlayer.h"

namespace LUS {
AudioPlayer::AudioPlayer()
    : mInitialized(false){

      };

bool AudioPlayer::Init(void) {
    mInitialized = DoInit();
    return IsInitialized();
}

bool AudioPlayer::IsInitialized(void) {
    return mInitialized;
}
} // namespace LUS
