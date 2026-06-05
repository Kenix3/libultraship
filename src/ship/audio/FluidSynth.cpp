#if ENABLE_FLUIDSYNTH
#include "ship/audio/FluidSynth.h"
#include <spdlog/spdlog.h>
#include <cstring>
#include <algorithm>
#include <cstdio>

namespace Ship {

namespace {
// ----------------------------------------------------------------------
// Memory-backed SF2 loader.
//
// FluidSynth's sound-font loaders are tried in order against the path
// passed to fluid_synth_sfload(). The default loader handles any
// filesystem path; we register an additional loader that responds to
// the fixed sentinel "mem://current" and ignores everything else, so
// path-based and memory-based loads coexist cleanly.
//
// The buffers live in mLoadedBuffers on the FluidSynth instance; the
// in-flight pointer below carries the address of the relevant buffer
// through the open callback (which has no user-data slot — only a
// filename string). AddSoundFontFromMemory runs from the GUI thread
// only and takes the synth mutex around the sfload call, so a single
// static slot is safe even with multiple SF2s loaded.
// ----------------------------------------------------------------------

struct MemoryInflight {
    const uint8_t* data = nullptr;
    size_t size = 0;
};
static MemoryInflight sMemoryInflight;

struct MemoryHandle {
    const uint8_t* data;
    size_t size;
    size_t pos;
};

constexpr const char* kMemorySentinel = "mem://current";

void* MemoryOpen(const char* filename) {
    if (filename == nullptr || std::strcmp(filename, kMemorySentinel) != 0) {
        return nullptr;
    }
    if (sMemoryInflight.data == nullptr || sMemoryInflight.size == 0) {
        return nullptr;
    }
    auto* h = new MemoryHandle{ sMemoryInflight.data, sMemoryInflight.size, 0 };
    // Single-shot: clear the slot so a stray repeat sfload can't replay.
    sMemoryInflight = {};
    return h;
}

int MemoryRead(void* buf, fluid_long_long_t count, void* handle) {
    auto* h = static_cast<MemoryHandle*>(handle);
    if (count < 0 || static_cast<size_t>(count) > h->size - h->pos) {
        return FLUID_FAILED;
    }
    std::memcpy(buf, h->data + h->pos, static_cast<size_t>(count));
    h->pos += static_cast<size_t>(count);
    return FLUID_OK;
}

int MemorySeek(void* handle, fluid_long_long_t offset, int origin) {
    auto* h = static_cast<MemoryHandle*>(handle);
    fluid_long_long_t newPos;
    switch (origin) {
        case SEEK_SET: newPos = offset; break;
        case SEEK_CUR: newPos = static_cast<fluid_long_long_t>(h->pos) + offset; break;
        case SEEK_END: newPos = static_cast<fluid_long_long_t>(h->size) + offset; break;
        default:       return FLUID_FAILED;
    }
    if (newPos < 0 || static_cast<size_t>(newPos) > h->size) {
        return FLUID_FAILED;
    }
    h->pos = static_cast<size_t>(newPos);
    return FLUID_OK;
}

fluid_long_long_t MemoryTell(void* handle) {
    return static_cast<fluid_long_long_t>(static_cast<MemoryHandle*>(handle)->pos);
}

int MemoryClose(void* handle) {
    delete static_cast<MemoryHandle*>(handle);
    return FLUID_OK;
}
} // namespace

FluidSynth::FluidSynth(double sampleRate, bool linearVelocity)
    : mSampleRate(sampleRate), mLinearVelocity(linearVelocity) {

    mSettings = new_fluid_settings();
    // Sample rate MUST be set before new_fluid_synth — the synth reads it
    // once at construction. fluid_synth_set_sample_rate() is deprecated and
    // silently ignored in FluidSynth 2.x, causing silence if used instead.
    fluid_settings_setnum(mSettings, "synth.sample-rate", sampleRate);
    // 64 channels = enough headroom for the per-pair channel allocator in
    // MidiTranslator to give each (fontId, instOrWave) pair its own MIDI
    // channel, so per-pair effect CCs (CC91/93/74/71) don't stomp each
    // other. Must be a multiple of 16; matches FluidSynth::kNumChannels.
    fluid_settings_setint(mSettings, "synth.midi-channels", kNumChannels);
    // "none" = no internal audio driver; we pull samples via Render() ourselves.
    // "file" is an offline render-to-disk mode and must NOT be used here.
    fluid_settings_setstr(mSettings, "audio.driver", "none");

    // FluidSynth's stock synth.gain is 0.2 — conservative to avoid clipping
    // when many SF2 voices play simultaneously. Native PCM coming out of the
    // engine peaks near 1.0, so at 0.2 FluidSynth voices are ~5× too quiet
    // against the native side of the additive Point B mix — independent of
    // which modulator set is active. Lift to 1.0 so the two sources arrive
    // balanced at the mix; the soft-clip in OTRAudio_Thread handles brief
    // over-budget sums.
    fluid_settings_setnum(mSettings, "synth.gain", 1.0);

    mSynth = new_fluid_synth(mSettings);
    if (!mSynth) {
        SPDLOG_ERROR("[FluidSynth] Failed to create synth");
        return;
    }

    // Verify the sample rate FluidSynth actually locked in.
    double actualRate = 0.0;
    fluid_settings_getnum(mSettings, "synth.sample-rate", &actualRate);
    SPDLOG_INFO("[FluidSynth] Synth created. Requested sample rate={} actual={} linearVelocity={}",
                sampleRate, actualRate, mLinearVelocity);

    if (mLinearVelocity) {
        InstallLinearVelocityModulators();
    }

    // Register the memory-backed sound-font loader alongside the default
    // filesystem loader. Loaders are tried in addition order: default
    // catches real filesystem paths, ours catches the mem:// sentinel.
    // FluidSynth takes ownership of the loader and frees it via
    // delete_fluid_synth.
    fluid_sfloader_t* memLoader = new_fluid_defsfloader(mSettings);
    if (memLoader) {
        fluid_sfloader_set_callbacks(memLoader, MemoryOpen, MemoryRead, MemorySeek, MemoryTell, MemoryClose);
        fluid_synth_add_sfloader(mSynth, memLoader);
    } else {
        SPDLOG_WARN("[FluidSynth] Memory sound-font loader unavailable; "
                    "LoadSoundFontFromMemory will fall back to default loader");
    }
}

void FluidSynth::InstallLinearVelocityModulators() {
    // Approach inspired by ANMP (GPL-2, github.com/derselbst/ANMP), specifically
    // src/InputLibraryWrapper/FluidsynthWrapper.cpp around L300-333. ANMP calls
    // this the "Graham-Smith volume curve": replace the SF2 spec's default
    // velocity / CC7 / CC11 → initial-attenuation modulators with versions that
    // keep the same perceptual concave NEGATIVE shape but halve the amount
    // (960 cB → 480 cB), pulling the maximum attenuation from −96 dB to −48 dB.
    // Lifts quiet voices without flattening overall dynamics — the curve still
    // tapers smoothly toward "no attenuation" near the top of the input range.
    //
    // We do NOT change the curve shape — an earlier version of this code
    // switched CC11 to LINEAR with the same amount, intending to "let the
    // translator's sqrt(velocity) curve dominate", but that compressed the
    // mid-range hard: linear NEGATIVE burns ~50% attenuation at CC11=64 while
    // concave NEGATIVE only burns ~13% there. Result was a uniform ~10 dB
    // drop on every voice, which is the opposite of the goal. Keep concave.
    //
    // ANMP's own CC11 handling is actually a *removal* of the modulator
    // (Dinosaur Planet uses CC11 for something else); we keep CC11 active
    // because the translator drives loudness dynamics through it.
    //
    // IMPORTANT: fluid_synth_add_default_mod(... FLUID_SYNTH_OVERWRITE) only
    // replaces an existing default if every source flag matches exactly.
    // fluid_synth_remove_default_mod followed by add_default_mod is safer and
    // documents intent; do it for all three for consistency.
    //
    // Must run after new_fluid_synth() but before any LoadSoundFont() — SF2
    // instrument-level modulators are layered on top of these defaults at load
    // time.

    fluid_mod_t* mod = new_fluid_mod();
    if (!mod) {
        SPDLOG_ERROR("[FluidSynth] new_fluid_mod() failed; Graham-Smith modulators disabled");
        return;
    }

    constexpr int kHalfAttenuationCentibels = 480; // = 960 / 2

    fluid_mod_set_source2(mod, FLUID_MOD_NONE, 0);
    fluid_mod_set_dest(mod, GEN_ATTENUATION);
    fluid_mod_set_amount(mod, kHalfAttenuationCentibels);

    // 1. NoteOn velocity → initial attenuation (concave, halved).
    fluid_mod_set_source1(mod, FLUID_MOD_VELOCITY,
                          FLUID_MOD_GC | FLUID_MOD_CONCAVE | FLUID_MOD_UNIPOLAR | FLUID_MOD_NEGATIVE);
    fluid_synth_remove_default_mod(mSynth, mod);
    fluid_synth_add_default_mod(mSynth, mod, FLUID_SYNTH_OVERWRITE);

    // 2. CC7 (channel volume) → initial attenuation (concave, halved).
    fluid_mod_set_source1(mod, 7,
                          FLUID_MOD_CC | FLUID_MOD_CONCAVE | FLUID_MOD_UNIPOLAR | FLUID_MOD_NEGATIVE);
    fluid_synth_remove_default_mod(mSynth, mod);
    fluid_synth_add_default_mod(mSynth, mod, FLUID_SYNTH_OVERWRITE);

    // 3. CC11 (expression) → initial attenuation (concave, halved).
    fluid_mod_set_source1(mod, 11,
                          FLUID_MOD_CC | FLUID_MOD_CONCAVE | FLUID_MOD_UNIPOLAR | FLUID_MOD_NEGATIVE);
    fluid_synth_remove_default_mod(mSynth, mod);
    fluid_synth_add_default_mod(mSynth, mod, FLUID_SYNTH_OVERWRITE);

    delete_fluid_mod(mod);

    SPDLOG_INFO("[FluidSynth] Graham-Smith modulators installed (vel/CC7/CC11 concave × 0.5)");
}

FluidSynth::~FluidSynth() {
    if (mSynth)    delete_fluid_synth(mSynth);
    if (mSettings) delete_fluid_settings(mSettings);
}

void FluidSynth::ClearSoundFonts() {
    std::lock_guard<std::mutex> lock(mSynthMutex);
    if (!mSynth) {
        mSfontIds.clear();
        mLoadedBuffers.clear();
        return;
    }
    for (int id : mSfontIds) {
        if (id != FLUID_FAILED) fluid_synth_sfunload(mSynth, id, /*reset_presets=*/1);
    }
    mSfontIds.clear();
    mLoadedBuffers.clear();
    mLoadedBuffers.shrink_to_fit();
    // reset_presets above cleared channel state inside the synth, so the
    // RPN-0 (pitch bend range) push needs to repeat on the next NoteOn.
    for (bool& inited : mChannelInited) inited = false;
}

int FluidSynth::AddSoundFont(const std::string& path) {
    std::lock_guard<std::mutex> lock(mSynthMutex);
    if (!mSynth) return FLUID_FAILED;
    // reset_presets only on the FIRST sfont — for subsequent loads we
    // want preset assignments on existing channels left alone so a
    // stacked pack doesn't blow away the prior pack's program selection.
    int resetPresets = mSfontIds.empty() ? 1 : 0;
    int id = fluid_synth_sfload(mSynth, path.c_str(), resetPresets);
    if (id == FLUID_FAILED) {
        SPDLOG_ERROR("[FluidSynth] Failed to load SF2: {}", path);
        return FLUID_FAILED;
    }
    SPDLOG_INFO("[FluidSynth] Loaded SF2: {} (id={})", path, id);
    mSfontIds.push_back(id);
    mLoadedBuffers.emplace_back(); // empty — filesystem load owns its own data
    if (resetPresets) {
        for (bool& inited : mChannelInited) inited = false;
    }
    return id;
}

int FluidSynth::AddSoundFontFromMemory(const uint8_t* data, size_t size) {
    std::lock_guard<std::mutex> lock(mSynthMutex);
    if (!mSynth || data == nullptr || size == 0) return FLUID_FAILED;

    // Pre-reserve the slot so we can hand its address through the static
    // sMemoryInflight pointer for the duration of sfload. Vector growth
    // is fine here because the SF2 buffer lives in the vector element,
    // which is itself a vector<uint8_t> (small, by-value relocations
    // don't invalidate the underlying heap-allocated data).
    mLoadedBuffers.emplace_back(data, data + size);
    auto& buf = mLoadedBuffers.back();
    sMemoryInflight = { buf.data(), buf.size() };
    int resetPresets = mSfontIds.empty() ? 1 : 0;
    int id = fluid_synth_sfload(mSynth, kMemorySentinel, resetPresets);
    sMemoryInflight = {};
    if (id == FLUID_FAILED) {
        SPDLOG_ERROR("[FluidSynth] Failed to load SF2 from memory ({} bytes)", size);
        mLoadedBuffers.pop_back();
        return FLUID_FAILED;
    }
    SPDLOG_INFO("[FluidSynth] Loaded SF2 from memory ({} bytes, id={})", size, id);
    mSfontIds.push_back(id);
    if (resetPresets) {
        for (bool& inited : mChannelInited) inited = false;
    }
    return id;
}

std::vector<int> FluidSynth::GetLoadedSfontIds() {
    std::lock_guard<std::mutex> lock(mSynthMutex);
    return mSfontIds;
}

std::vector<FluidSynth::LoadedPreset> FluidSynth::EnumerateLoadedPresets() {
    std::lock_guard<std::mutex> lock(mSynthMutex);
    std::vector<LoadedPreset> result;
    if (!mSynth) return result;
    for (int id : mSfontIds) {
        if (id == FLUID_FAILED) continue;
        fluid_sfont_t* sfont = fluid_synth_get_sfont_by_id(mSynth, id);
        if (!sfont) continue;
        fluid_sfont_iteration_start(sfont);
        while (fluid_preset_t* preset = fluid_sfont_iteration_next(sfont)) {
            LoadedPreset p;
            p.sfontId = id;
            p.bank    = fluid_preset_get_banknum(preset);
            p.program = fluid_preset_get_num(preset);
            const char* nm = fluid_preset_get_name(preset);
            p.name = nm ? nm : "";
            result.push_back(std::move(p));
        }
    }
    return result;
}

void FluidSynth::LoadSoundFont(const std::string& path) {
    ClearSoundFonts();
    AddSoundFont(path);
}

void FluidSynth::LoadSoundFontFromMemory(const uint8_t* data, size_t size) {
    ClearSoundFonts();
    AddSoundFontFromMemory(data, size);
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
    SPDLOG_TRACE("[FluidSynth] NoteOn ch={} note={} vel={} sfonts={} result={}",
                 channel, note, velocity, mSfontIds.size(), result);
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

bool FluidSynth::ProgramSelect(uint8_t channel, int sfontId,
                               uint16_t bank, uint16_t program) {
    std::lock_guard<std::mutex> lock(mSynthMutex);
    if (!mSynth) return false;
    InitChannel(channel);

    // Verify the sfontId is one we loaded — fluid_synth_program_select
    // would also reject an unknown id but its log goes through
    // FluidSynth's own logger rather than ours; pre-check so we can
    // emit our SPDLOG path uniformly.
    bool known = false;
    for (int id : mSfontIds) {
        if (id == sfontId) { known = true; break; }
    }
    if (!known) {
        SPDLOG_TRACE("[FluidSynth] ProgramSelect ch={} sfontId={} not loaded; rejecting pin",
                     channel, sfontId);
        return false;
    }

    // Set drum/melodic type before the select — bank 128 is the GM
    // percussion convention and FluidSynth's voice allocator branches
    // on channel type, not on the bank we're selecting into.
    if (bank == 128) {
        fluid_synth_set_channel_type(mSynth, channel, CHANNEL_TYPE_DRUM);
    } else {
        fluid_synth_set_channel_type(mSynth, channel, CHANNEL_TYPE_MELODIC);
    }

    int result = fluid_synth_program_select(mSynth, channel,
                                            static_cast<unsigned int>(sfontId),
                                            static_cast<unsigned int>(bank),
                                            static_cast<unsigned int>(program));
    if (result != FLUID_OK) {
        SPDLOG_TRACE("[FluidSynth] ProgramSelect ch={} sfontId={} bank={} prog={} -> FAILED",
                     channel, sfontId, bank, program);
        return false;
    }
    SPDLOG_TRACE("[FluidSynth] ProgramSelect ch={} sfontId={} bank={} prog={} -> OK",
                 channel, sfontId, bank, program);
    return true;
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

void FluidSynth::SetReverbParams(double roomsize, double damping, double width, double level) {
    std::lock_guard<std::mutex> lock(mSynthMutex);
    if (!mSynth) return;
    fluid_synth_set_reverb_roomsize(mSynth, roomsize);
    fluid_synth_set_reverb_damp(mSynth, damping);
    fluid_synth_set_reverb_width(mSynth, width);
    fluid_synth_set_reverb_level(mSynth, level);
    SPDLOG_INFO("[FluidSynth] Reverb set: roomsize={} damping={} width={} level={}",
                roomsize, damping, width, level);
}

void FluidSynth::Render(float* out, uint32_t frameCount) {
    std::lock_guard<std::mutex> lock(mSynthMutex);
    if (!mSynth || mSfontIds.empty()) {
        std::memset(out, 0, frameCount * 2 * sizeof(float));
        return;
    }

    fluid_synth_write_float(mSynth,
                            static_cast<int>(frameCount),
                            out, 0, 2,
                            out, 1, 2);
}

uint32_t FluidSynth::GetActiveVoiceCount() const {
    std::lock_guard<std::mutex> lock(mSynthMutex);
    if (!mSynth) return 0;
    int n = fluid_synth_get_active_voice_count(mSynth);
    return n < 0 ? 0u : static_cast<uint32_t>(n);
}

uint32_t FluidSynth::GetPolyphonyLimit() const {
    std::lock_guard<std::mutex> lock(mSynthMutex);
    if (!mSynth) return 0;
    int n = fluid_synth_get_polyphony(mSynth);
    return n < 0 ? 0u : static_cast<uint32_t>(n);
}

} // namespace Ship
#endif // ENABLE_FLUIDSYNTH