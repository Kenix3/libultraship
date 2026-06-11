#pragma once

#if ENABLE_SDL3

#include "ship/audio/AudioPlayer.h"

#include <memory>

namespace Ship {

/*
 * SDL3AudioPlayer — cross-platform audio backend built on SDL3's audio-stream
 * API, independent of the SDL2 the rest of the engine uses for window / input /
 * ImGui.
 *
 * Pull model:
 *   - SDL3 owns the audio thread and invokes our callback when the device needs
 *     more samples; we never create a thread ourselves.
 *   - A ring buffer bridges the producer (DoPlay, game thread) and that callback
 *     (SDL3 audio thread).
 *   - On underrun the last good chunk is replayed with a linear fade-out instead
 *     of inserting silence, which masks load spikes perceptually.
 */
class SDL3AudioPlayer : public AudioPlayer {
  public:
    explicit SDL3AudioPlayer(AudioSettings settings);
    ~SDL3AudioPlayer() override;

    bool DoInit() override;
    void DoClose() override;
    int Buffered() override;
    void DoPlay(const uint8_t* buf, size_t len) override;

  private:
    // Using an opaque std::unique_ptr<Impl> (aka. pimpl) here to allow
    // SDL3 to live side-by-side with the rest of libultraship (window,
    // input, ImGui), built against SDL2. SDL2 and SDL3 cannot coexist in
    // one translation unit. Hiding the SDL3 types inside Impl is what
    // keeps this header includable from SDL2 code.
    // We can reconsider later if SDL3 is used everywhere.
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace Ship

#endif // #if ENABLE_SDL3
