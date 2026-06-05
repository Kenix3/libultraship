#pragma once
#if ENABLE_FLUIDSYNTH

#include "IMidiSynth.h"
#include <fluidsynth.h>
#include <mutex>
#include <vector>
#include <cstdint>

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

    // Single-shot replace: unloads every previously-loaded SF2 then loads
    // this one. Convenience wrapper over ClearSoundFonts + AddSoundFont*.
    void LoadSoundFont(const std::string& path) override;

    // Same shape as LoadSoundFont but takes an in-memory SF2 (e.g. one read
    // from a mounted .o2r archive). The buffer is copied into the synth's
    // internal storage so the caller may free their copy immediately.
    void LoadSoundFontFromMemory(const uint8_t* data, size_t size);

    // Add an SF2 alongside any already-loaded ones. FluidSynth's preset
    // lookup walks loaded soundfonts in REVERSE load order, so the most
    // recently added SF2 wins on (bank, program) collisions — matches the
    // "last loaded wins" semantics our mod stack uses elsewhere.
    //
    // Returns the FluidSynth sfont id on success, or FLUID_FAILED. The
    // memory variant copies the buffer into instance-owned storage and
    // routes through the mem:// sentinel; the path variant uses the
    // default filesystem loader.
    int AddSoundFont(const std::string& path);
    int AddSoundFontFromMemory(const uint8_t* data, size_t size);

    // Unload every loaded SF2. Safe to call when none are loaded.
    void ClearSoundFonts();

    // Loaded SF2 ids in load order. Use to map a sfont id back to its
    // pack name on the caller side (the caller knows what it loaded;
    // FluidSynth only knows the opaque ids).
    std::vector<int> GetLoadedSfontIds();

    // One row per preset across every loaded SF2 (every sfont's full
    // preset list, in iteration order — which is generally the SF2's
    // phdr order, grouped by sfont). Re-enumerated on demand; callers
    // typically cache the result and refresh when packs change.
    struct LoadedPreset {
        int         sfontId;
        int         bank;
        int         program;
        std::string name;
    };
    std::vector<LoadedPreset> EnumerateLoadedPresets();
    void NoteOn(uint8_t channel, uint8_t note, uint8_t velocity) override;
    void NoteOff(uint8_t channel, uint8_t note) override;
    void ProgramChange(uint8_t channel, uint16_t preset) override;
    bool ProgramSelect(uint8_t channel, int sfontId,
                       uint16_t bank, uint16_t program) override;
    void PitchBend(uint8_t channel, float semitones) override;
    void ControlChange(uint8_t channel, uint8_t cc, uint16_t value) override;
    void Render(float* out, uint32_t frameCount) override;
    uint32_t GetActiveVoiceCount() const override;
    uint32_t GetPolyphonyLimit() const override;

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
    double             mSampleRate;
    bool               mLinearVelocity  = false;

    // One entry per loaded SF2, in load order. FluidSynth itself walks
    // loaded sfonts in reverse load order during preset lookup, so the
    // tail of this vector wins on collisions.
    std::vector<int> mSfontIds;

    // Backing storage for memory-loaded SF2s, paired one-to-one with
    // mSfontIds entries. Filesystem-loaded SF2s use the default loader
    // and the corresponding slot here stays empty. Buffers must outlive
    // the sfload call so the mem-sfloader's callbacks have stable data
    // for the duration of the load.
    std::vector<std::vector<uint8_t>> mLoadedBuffers;

    // Protects fluid_synth_* calls from concurrent access.
    // The audio thread calls Render(); the game thread calls NoteOn/Off/etc.
    mutable std::mutex mSynthMutex;

    // Which channels have had InitChannel() called. Sized to kNumChannels
    // so the translator's per-pair channel allocation can address all of
    // them; the synth setting is matched to this in the constructor.
    static constexpr int kNumChannels = 64;
    bool               mChannelInited[kNumChannels] = {};
};

} // namespace Ship

#endif // ENABLE_FLUIDSYNTH
