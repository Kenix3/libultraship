#pragma once
#if ENABLE_FLUIDSYNTH

#include "IMidiSynth.h"
#include <fluidsynth.h>
#include <mutex>

namespace Ship {

class FluidSynth final : public IMidiSynth {
public:
    // sampleRate     : must match the audio output rate (typically 44100 or 48000).
    // linearVelocity : when true, install the Graham-Smith volume curve (per
    //                  ANMP, github.com/derselbst/ANMP). Replaces the SF2
    //                  default vel / CC7 / CC11 → initial-attenuation modulators
    //                  with versions that keep the perceptual concave NEGATIVE
    //                  shape but halve the amount (960 → 480 cB). Maximum
    //                  attenuation drops from −96 dB to −48 dB, lifting quiet
    //                  voices while preserving dynamics shape. Default false
    //                  preserves the standard SF2 behavior.
    //                  (The "linear velocity" name is historical — an earlier
    //                  prototype actually switched CC11 to linear, but that
    //                  over-compressed the mid-range.)
    explicit FluidSynth(double sampleRate, bool linearVelocity = false);
    ~FluidSynth() override;

    void LoadSoundFont(const std::string& path) override;
    void NoteOn(uint8_t channel, uint8_t note, uint8_t velocity) override;
    void NoteOff(uint8_t channel, uint8_t note) override;
    void ProgramChange(uint8_t channel, uint16_t preset) override;
    void PitchBend(uint8_t channel, float semitones) override;
    void ControlChange(uint8_t channel, uint8_t cc, uint16_t value) override;
    void Render(float* out, uint32_t frameCount) override;

    // Configure the synth-wide reverb. Safe to call any time after construction;
    // takes the synth mutex. Useful for per-mode presets — callers swap reverb
    // settings without having to rebuild the synth. Parameters mirror the
    // FluidSynth fluid_synth_set_reverb_* calls:
    //   roomsize : [0..1] perceived reverb tail length.
    //   damping  : [0..1] high-frequency damping.
    //   width    : [0..100] stereo spread.
    //   level    : [0..1] reverb wet level.
    void SetReverbParams(double roomsize, double damping, double width, double level);

    // Pitch bend range in semitones sent to FluidSynth on channel init.
    // Must match what the MidiTranslator uses. Default: 12 semitones.
    static constexpr float kPitchBendRangeSemitones = 12.0f;

private:
    void InitChannel(uint8_t channel);

    // Installs the Graham-Smith volume curve on the freshly-created
    // fluid_synth_t (per ANMP). Replaces the SF2 default vel/CC7/CC11 →
    // attenuation modulators with versions at halved amount (480 cB).
    // Must be called after new_fluid_synth() but before any LoadSoundFont()
    // so that SF2 instrument-level modulators layer correctly on top of
    // the modified defaults. Name retained for historical reasons; see
    // the implementation in FluidSynth.cpp for the design rationale.
    void InstallLinearVelocityModulators();

    fluid_settings_t*  mSettings        = nullptr;
    fluid_synth_t*     mSynth           = nullptr;
    int                mSfontId         = FLUID_FAILED;
    double             mSampleRate;
    bool               mLinearVelocity  = false;

    // Protects fluid_synth_* calls from concurrent access.
    // The audio thread calls Render(); the game thread calls NoteOn/Off/etc.
    std::mutex         mSynthMutex;

    // Which channels have had InitChannel() called.
    bool               mChannelInited[16] = {};
};

} // namespace Ship

#endif // ENABLE_FLUIDSYNTH
