//
//  DarwinSpeechSynthesizer.hpp
//  libultraship
//
//  Created by David Chavez on 22.11.22.
//

#ifndef DarwinSpeechSynthesizer_hpp
#define DarwinSpeechSynthesizer_hpp

#include "SpeechSynthesizer.hpp"
#include <stdio.h>

namespace Ship {
class DarwinSpeechSynthesizer : public SpeechSynthesizer {
public:
    DarwinSpeechSynthesizer();

    void Speak(const char* text);

protected:
    bool DoInit(void);

private:
    void *mSynthesizer;
};
}

#endif /* DarwinSpeechSynthesizer_hpp */
