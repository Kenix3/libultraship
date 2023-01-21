//
//  OSXAudioPlayer.h
//  libultraship
//
//  Created by David Chavez on 3.1.23.
//

#if defined(__APPLE__)

#ifndef CA_AUDIO_PLAYER_H
#define CA_AUDIO_PLAYER_H

#include "AudioPlayer.h"

namespace Ship {
class OSXAudioPlayer : public AudioPlayer {
  public:
    OSXAudioPlayer();

    int Buffered(void);
    int GetDesiredBuffered(void);
    void Play(const uint8_t* buff, size_t len);

  protected:
    bool DoInit(void);
};
} // namespace Ship

#endif // CA_AUDIO_PLAYER_H

#endif // __APPLE__
