//
//  SpeechSynthesizer.hpp
//  libultraship
//
//  Created by David Chavez on 22.11.22.
//

#ifndef SpeechSynthesizer_hpp
#define SpeechSynthesizer_hpp

#include <stdio.h>

namespace Ship {
class SpeechSynthesizer {
public:
    SpeechSynthesizer();

    bool Init(void);
    virtual void Speak(const char* text);

    bool IsInitialized(void);

protected:
    virtual bool DoInit(void) = 0;

private:
    bool mInitialized;
};
} // namespace Ship

#endif /* SpeechSynthesizer_hpp */


#ifdef _WIN32
#include "SAPISpeechSynthesizer.hpp"
#elif defined(__APPLE__)
#include "DarwinSpeechSynthesizer.hpp"
#endif
