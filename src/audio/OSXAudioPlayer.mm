//
//  OSXAudioPlayer.mm
//  libultraship
//
//  Created by David Chavez on 3.1.23.
//

#if defined(__APPLE__)

#include "OSXAudioPlayer.h"
#include "OEGameAudio.h"

OEGameAudio* mAudio = NULL;

namespace Ship {
OSXAudioPlayer::OSXAudioPlayer() {
}

bool OSXAudioPlayer::DoInit(void) {
    PortAudioDescription mAudioDescription = {
        .mSampleRate = (Float64)this->GetSampleRate(),
        .mChannelCount = 2,
        .mBitDepth = 16,
        .mFrameInterval = 60
    };

    mAudio = [[OEGameAudio alloc] initWithAudioDescription:&mAudioDescription];
    [mAudio startAudio];
    return true;
}

int OSXAudioPlayer::Buffered(void) {
    return [mAudio bytesBuffered] / 4;
}

int OSXAudioPlayer::GetDesiredBuffered(void) {
    return 2480;
}

void OSXAudioPlayer::Play(const uint8_t* buff, size_t len) {
    [mAudio write:buff maxLength:len];
}
} // namespace Ship

#endif
