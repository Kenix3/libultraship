#pragma once
#include <cstdint>
#include <string>

namespace Ship {

// MIDI-shaped soft synth interface. An implementation that is installed
// on the MidiSynthManager replaces the engine's native audio synthesis
// path: the audio thread fills its output buffer by calling Render()
// instead of running the native synth.
//
// When no implementation is installed, the manager returns nullptr and
// the audio thread falls back to native synthesis.
class IMidiSynth {
public:
    virtual ~IMidiSynth() = default;

    // Load an SF2 soundfont from disk. Implementations that do not use
    // SF2 may treat this as a no-op.
    virtual void LoadSoundFont(const std::string& path) = 0;

    // MIDI-like note events. channel index is implementation-defined; the
    // current FluidSynth backend exposes 64 channels. Standard MIDI drum
    // semantics are NOT pinned to channel 9 — drum vs melodic is decided
    // by the bank passed to ProgramChange (bank 128 = drum kit), and the
    // implementation flips the channel type per call.
    virtual void NoteOn(uint8_t channel, uint8_t note, uint8_t velocity) = 0;
    virtual void NoteOff(uint8_t channel, uint8_t note) = 0;

    // preset encodes both bank (high byte) and program (low byte).
    // The synth's preset lookup resolves this against the union of every
    // loaded soundfont — typically with last-loaded-wins precedence.
    virtual void ProgramChange(uint8_t channel, uint16_t preset) = 0;

    // Like ProgramChange but pins the channel to a SPECIFIC loaded
    // soundfont via its `sfontId`, bypassing the cross-soundfont
    // preset lookup. Use this when the caller knows exactly which
    // SF2 the preset must come from — for example, when the user
    // picked "[Xadra] Bank 10 prog 5" from the UI and we want to
    // play *that* even if another loaded SF2 also has (10, 5).
    //
    // Returns true when the pin succeeded (the sfontId is valid and
    // contains the (bank, program) tuple), false otherwise. Failure
    // is the caller's signal to fall back to native synthesis for
    // this entry — see the pack-bound resolution model in
    // docs/FluidSynthBackend.md.
    virtual bool ProgramSelect(uint8_t channel, int sfontId,
                               uint16_t bank, uint16_t program) = 0;

    // semitones is a signed float: +1.0 = one semitone up.
    // Range needed: approximately -12.0 to +12.0.
    virtual void PitchBend(uint8_t channel, float semitones) = 0;

    // Standard MIDI CC. value is 0-16383 (14-bit).
    virtual void ControlChange(uint8_t channel, uint8_t cc, uint16_t value) = 0;

    // Fill `out` with `frameCount` stereo interleaved float32 samples.
    // Called from the audio thread; must be real-time safe.
    virtual void Render(float* out, uint32_t frameCount) = 0;

    // Current number of audible voices held by the synth. Used by host UIs
    // as a real-time diagnostic — when this approaches GetPolyphonyLimit(),
    // new NoteOns will steal existing voices and the host can correlate
    // user-reported "cuts" with voice exhaustion. Implementations without
    // a voice pool may return 0.
    virtual uint32_t GetActiveVoiceCount() const = 0;
    virtual uint32_t GetPolyphonyLimit() const = 0;
};

} // namespace Ship
