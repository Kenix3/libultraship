#pragma once

#include <cstdint>
#include <vector>
#include <tuple>
#include <memory>

namespace Ship {

/**
 * Stereo to 5.1 surround upmixer using FFT-based frequency-domain analysis.
 *
 * For each frequency bin, the decoder measures inter-channel amplitude balance
 * and phase correlation to determine source directionality, then distributes
 * energy to 6 output channels using analytic gain functions.
 *
 * Input: int16 stereo -> Output: int16 5.1 surround (FL, FR, C, LFE, BL, BR).
 */
class SoundMatrixDecoder {
  public:
    SoundMatrixDecoder(int32_t sampleRate);
    ~SoundMatrixDecoder();

    void ResetState();

    /**
     * Decode stereo to 5.1 surround.
     * Input: interleaved int16 stereo [L0, R0, L1, R1, ...]
     * Output: interleaved int16 5.1 [FL, FR, C, LFE, BL, BR, ...]
     *
     * Because the FFT decoder works in fixed-size blocks, output may be
     * empty if not enough input has accumulated yet.
     */
    std::tuple<const uint8_t*, int> Process(const uint8_t* buf, size_t len);

  private:
    static constexpr unsigned BLOCK_SIZE = 4096;
    static constexpr unsigned NUM_OUT_CHANNELS = 6;

    struct DecoderState;
    std::unique_ptr<DecoderState> mDecoder;

    std::vector<float> mFloatInput;
    unsigned mInputFrames = 0;

    std::vector<int16_t> mOutputBuffer;
    int32_t mSampleRate;

    static int16_t Saturate(float value);
};

} // namespace Ship
