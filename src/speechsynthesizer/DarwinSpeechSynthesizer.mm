//
//  DarwinSpeechSynthesizer.mm
//  libultraship
//
//  Created by David Chavez on 22.11.22.
//

#include "DarwinSpeechSynthesizer.h"
#import <AVFoundation/AVFoundation.h>

namespace Ship {
DarwinSpeechSynthesizer::DarwinSpeechSynthesizer() {}

bool DarwinSpeechSynthesizer::DoInit() {
    mSynthesizer = [[AVSpeechSynthesizer alloc] init];
    return true;
}

void DarwinSpeechSynthesizer::Speak(const char* text) {
    AVSpeechUtterance *utterance = [AVSpeechUtterance speechUtteranceWithString:@(text)];
    [utterance setVoice:[AVSpeechSynthesisVoice voiceWithIdentifier:@"com.apple.voice.compact.en-US.Samantha"]];

    if (@available(macOS 11.0, *)) {
        [utterance setPrefersAssistiveTechnologySettings:YES];
    }

    [(AVSpeechSynthesizer *)mSynthesizer stopSpeakingAtBoundary:AVSpeechBoundaryImmediate];
    [(AVSpeechSynthesizer *)mSynthesizer speakUtterance:utterance];
}
}
