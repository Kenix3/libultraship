#if ENABLE_FLUIDSYNTH
#include "ship/audio/FluidSynth.h"
#include <spdlog/spdlog.h>
#include <cstring>
#include <algorithm>

namespace Ship {

FluidSynth::FluidSynth(double sampleRate)
    : mSampleRate(sampleRate) {

    mSettings = new_fluid_settings();
    // Sample rate MUST be set before new_fluid_synth — the synth reads it
    // once at construction. fluid_synth_set_sample_rate() is deprecated and
    // silently ignored in FluidSynth 2.x, causing silence if used instead.
    fluid_settings_setnum(mSettings, "synth.sample-rate", sampleRate);
    fluid_settings_setint(mSettings, "synth.midi-channels", 16);
    // "none" = no internal audio driver; we pull samples via Render() ourselves.
    // "file" is an offline render-to-disk mode and must NOT be used here.
    fluid_settings_setstr(mSettings, "audio.driver", "none");

    mSynth = new_fluid_synth(mSettings);
    if (!mSynth) {
        SPDLOG_ERROR("[FluidSynth] Failed to create synth");
        return;
    }

    // Verify the sample rate FluidSynth actually locked in.
    double actualRate = 0.0;
    fluid_settings_getnum(mSettings, "synth.sample-rate", &actualRate);
    SPDLOG_INFO("[FluidSynth] Synth created. Requested sample rate={} actual={}", sampleRate, actualRate);
}

FluidSynth::~FluidSynth() {
    if (mSynth)    delete_fluid_synth(mSynth);
    if (mSettings) delete_fluid_settings(mSettings);
}

void FluidSynth::LoadSoundFont(const std::string& path) {
    std::lock_guard<std::mutex> lock(mSynthMutex);
    if (!mSynth) return;
    mSfontId = fluid_synth_sfload(mSynth, path.c_str(), /*reset_presets=*/1);
    if (mSfontId == FLUID_FAILED) {
        SPDLOG_ERROR("[FluidSynth] Failed to load SF2: {}", path);
    } else {
        SPDLOG_INFO("[FluidSynth] Loaded SF2: {} (id={})", path, mSfontId);
    }
    // Channels need their RPN-0 (pitch bend range) re-pushed on the next
    // NoteOn — reset_presets cleared channel state inside the synth.
    for (bool& inited : mChannelInited) inited = false;
}

void FluidSynth::InitChannel(uint8_t channel) {
    if (mChannelInited[channel]) return;
    mChannelInited[channel] = true;

    int ch = static_cast<int>(channel);

    // Set pitch bend range to kPitchBendRangeSemitones via RPN 0 (MIDI spec).
    // CC 101/100 = RPN MSB/LSB, CC 6 = Data Entry MSB (semitones),
    // CC 38 = Data Entry LSB (cents). Null the RPN afterwards so stray CC6
    // messages can't accidentally reset the range.
    fluid_synth_cc(mSynth, ch, 101, 0);
    fluid_synth_cc(mSynth, ch, 100, 0);
    fluid_synth_cc(mSynth, ch, 6, static_cast<int>(kPitchBendRangeSemitones));
    fluid_synth_cc(mSynth, ch, 38, 0);
    fluid_synth_cc(mSynth, ch, 101, 127); // null RPN
    fluid_synth_cc(mSynth, ch, 100, 127);
}

void FluidSynth::NoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    std::lock_guard<std::mutex> lock(mSynthMutex);
    if (!mSynth) return;
    InitChannel(channel);
    int result = fluid_synth_noteon(mSynth, channel, note, velocity);
    SPDLOG_TRACE("[FluidSynth] NoteOn ch={} note={} vel={} sfont={} result={}",
                 channel, note, velocity, mSfontId, result);
}

void FluidSynth::NoteOff(uint8_t channel, uint8_t note) {
    std::lock_guard<std::mutex> lock(mSynthMutex);
    if (!mSynth) return;
    fluid_synth_noteoff(mSynth, channel, note);
}

void FluidSynth::ProgramChange(uint8_t channel, uint16_t preset) {
    std::lock_guard<std::mutex> lock(mSynthMutex);
    if (!mSynth) return;
    InitChannel(channel);

    int bank    = (preset >> 8) & 0xFF;
    int program =  preset       & 0xFF;

    SPDLOG_TRACE("[FluidSynth] ProgramChange ch={} bank={} program={}", channel, bank, program);

    if (bank == 128) {
        fluid_synth_set_channel_type(mSynth, channel, CHANNEL_TYPE_DRUM);
        fluid_synth_bank_select(mSynth, channel, 128);
    } else {
        fluid_synth_set_channel_type(mSynth, channel, CHANNEL_TYPE_MELODIC);
        fluid_synth_bank_select(mSynth, channel, bank);
    }

    fluid_synth_program_change(mSynth, channel, program);
}

void FluidSynth::PitchBend(uint8_t channel, float semitones) {
    std::lock_guard<std::mutex> lock(mSynthMutex);
    if (!mSynth) return;
    float ratio = semitones / kPitchBendRangeSemitones;
    int val = static_cast<int>(ratio * 8192.0f) + 8192;
    val = std::clamp(val, 0, 16383);
    fluid_synth_pitch_bend(mSynth, channel, val);
}

void FluidSynth::ControlChange(uint8_t channel, uint8_t cc, uint16_t value) {
    std::lock_guard<std::mutex> lock(mSynthMutex);
    if (!mSynth) return;
    fluid_synth_cc(mSynth, channel, cc, (value >> 7) & 0x7F);
}

void FluidSynth::Render(float* out, uint32_t frameCount) {
    std::lock_guard<std::mutex> lock(mSynthMutex);
    if (!mSynth || mSfontId == FLUID_FAILED) {
        std::memset(out, 0, frameCount * 2 * sizeof(float));
        return;
    }

    fluid_synth_write_float(mSynth,
                            static_cast<int>(frameCount),
                            out, 0, 2,
                            out, 1, 2);
}

} // namespace Ship
#endif // ENABLE_FLUIDSYNTH