#include "ship/audio/MidiSynthManager.h"

namespace Ship {

MidiSynthManager& MidiSynthManager::Instance() {
    static MidiSynthManager sInstance;
    return sInstance;
}

void MidiSynthManager::SetSynth(std::shared_ptr<IMidiSynth> synth) {
    std::lock_guard<std::mutex> lock(mMutex);
    mSynth = std::move(synth);
}

std::shared_ptr<IMidiSynth> MidiSynthManager::GetActiveSynth() {
    std::lock_guard<std::mutex> lock(mMutex);
    return mSynth;
}

} // namespace Ship
