#include "ship/audio/SoundMatrixDecoder.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <complex>
#include <vector>
#include "kissfft/kiss_fftr.h"

using cplx = std::complex<double>;

static constexpr double kPi = 3.14159265358979323846;
static constexpr double kEpsilon = 1e-10;
static constexpr unsigned kChannels = 6;

// Output channel indices in SMPTE/ITU 5.1 order
enum : unsigned { kFL = 0, kFR = 1, kFC = 2, kLFE = 3, kBL = 4, kBR = 5 };

// ---------------------------------------------------------------------------
// FFT-based stereo-to-5.1 surround upmixer using overlap-add.
//
// For each frequency bin the decoder measures two directional cues:
//   - Amplitude balance  (left/right panning)
//   - Phase correlation  (front/rear placement)
// and distributes energy to six channels through analytic gain functions.
//
// The overlap-add uses a sqrt-Hann window with 50 % overlap for
// perfect reconstruction, which is textbook DSP (e.g. Allen & Rabiner 1977).
// ---------------------------------------------------------------------------

struct Ship::SoundMatrixDecoder::DecoderState {
    unsigned N; // FFT block size (samples per channel)

    // Configuration
    float centerImage;
    float rearLevel;
    bool useLfe;
    float loCutBin;
    float hiCutBin;

    // FFT plans
    kiss_fftr_cfg forward;
    kiss_fftr_cfg inverse;

    // Analysis window (sqrt-Hann / N for perfect reconstruction)
    std::vector<double> wnd;

    // Overlap input buffer: holds 1.5 blocks of interleaved stereo
    // Layout after each decode(): [last N/2 frames | current N frames]
    std::vector<double> inbuf;

    // Overlap output buffer: (N + N/2) * C interleaved multichannel
    std::vector<double> outbuf;
    bool bufferEmpty;

    // Per-channel frequency-domain scratch
    std::vector<std::vector<cplx>> chFreq; // [channel][bin]

    // Temporary time-domain arrays (single-channel)
    std::vector<double> tmpL;
    std::vector<double> tmpR;
    std::vector<double> tmpOut;

    // Temporary frequency-domain arrays
    std::vector<cplx> freqL;
    std::vector<cplx> freqR;

    DecoderState(unsigned blockSize, int32_t sampleRate)
        : N(blockSize),
          centerImage(0.7f),
          rearLevel(0.7f),
          useLfe(true),
          wnd(N),
          inbuf(3 * N, 0.0),
          outbuf((N + N / 2) * kChannels, 0.0),
          bufferEmpty(true),
          chFreq(kChannels, std::vector<cplx>(N, cplx(0, 0))),
          tmpL(N),
          tmpR(N),
          tmpOut(N),
          freqL(N / 2 + 1),
          freqR(N / 2 + 1) {
        forward = kiss_fftr_alloc(N, 0, nullptr, nullptr);
        inverse = kiss_fftr_alloc(N, 1, nullptr, nullptr);

        float nyquist = sampleRate / 2.0f;
        loCutBin = 40.0f / nyquist * (N / 2);
        hiCutBin = 90.0f / nyquist * (N / 2);

        for (unsigned k = 0; k < N; k++)
            wnd[k] = std::sqrt(0.5 * (1.0 - std::cos(2.0 * kPi * k / N)) / N);
    }

    ~DecoderState() {
        free(forward);
        free(inverse);
    }

    // Process one full block of stereo input (N frames = 2*N doubles interleaved).
    // Returns pointer to internal output buffer containing N frames of 6-ch audio.
    double* decode(double* input) {
        // Append new block into the overlap region
        std::memcpy(&inbuf[N], &input[0], 2 * N * sizeof(double));

        // Two half-overlapped analysis windows
        processHalfBlock(&inbuf[0]);
        processHalfBlock(&inbuf[N]);

        // Shift tail for next overlap
        std::memcpy(&inbuf[0], &inbuf[2 * N], N * sizeof(double));
        bufferEmpty = false;
        return outbuf.data();
    }

    void flush() {
        std::fill(outbuf.begin(), outbuf.end(), 0.0);
        std::fill(inbuf.begin(), inbuf.end(), 0.0);
        bufferEmpty = true;
    }

  private:
    // Process one windowed FFT block (N frames of stereo starting at *input)
    void processHalfBlock(double* input) {
        // Demux stereo and apply analysis window
        for (unsigned k = 0; k < N; k++) {
            tmpL[k] = wnd[k] * input[k * 2];
            tmpR[k] = wnd[k] * input[k * 2 + 1];
        }

        // Forward FFT of left and right channels
        kiss_fftr(forward, tmpL.data(), reinterpret_cast<kiss_fft_cpx*>(freqL.data()));
        kiss_fftr(forward, tmpR.data(), reinterpret_cast<kiss_fft_cpx*>(freqR.data()));

        // Clear per-channel frequency bins
        for (unsigned c = 0; c < kChannels; c++)
            std::fill(chFreq[c].begin(), chFreq[c].end(), cplx(0, 0));

        // --- Per-bin directional analysis and channel distribution ---
        for (unsigned f = 1; f < N / 2; f++) {
            cplx L = freqL[f];
            cplx R = freqR[f];
            double ampL = std::abs(L);
            double ampR = std::abs(R);
            double ampSum = ampL + ampR;

            if (ampSum < kEpsilon)
                continue;

            double ampTotal = std::sqrt(ampL * ampL + ampR * ampR);

            // Left-right balance: -1 = full left, +1 = full right
            double balance = (ampR - ampL) / ampSum;

            // Phase difference between channels [0, pi]
            double phL = std::arg(L);
            double phR = std::arg(R);
            double dphi = std::abs(phL - phR);
            if (dphi > kPi)
                dphi = 2.0 * kPi - dphi;

            // Frontness: 1 when in-phase (frontal), 0 when anti-phase (ambient)
            // Hann-shaped mapping from phase difference
            double frontness = 0.5 * (1.0 + std::cos(dphi));

            // Panning weights
            double leftPan = 0.5 * (1.0 - balance);
            double rightPan = 0.5 * (1.0 + balance);
            double absBal = std::abs(balance);

            // --- Analytic channel gains ---

            // Center: strong when phase-correlated and amplitude-balanced
            double gC = centerImage * frontness * (1.0 - absBal * absBal);

            // Front L/R: frontal content, amplitude-panned
            double gFL = std::sqrt(frontness * leftPan) * (1.0 - 0.5 * gC);
            double gFR = std::sqrt(frontness * rightPan) * (1.0 - 0.5 * gC);

            // Rear L/R: decorrelated content, amplitude-panned
            double rearness = 1.0 - frontness;
            double gBL = std::sqrt(rearness * leftPan) * rearLevel;
            double gBR = std::sqrt(rearness * rightPan) * rearLevel;

            // Energy-preserving normalization
            double gainSq = gC * gC + gFL * gFL + gFR * gFR + gBL * gBL + gBR * gBR;
            double scale = 1.0 / std::sqrt(gainSq + kEpsilon);
            gC *= scale;
            gFL *= scale;
            gFR *= scale;
            gBL *= scale;
            gBR *= scale;

            // Phase references
            double phCenter = std::arg(L + R);
            double phDiff = std::arg(L - R);

            // Construct channel signals:
            // Front channels use original L/R phases (direct sound)
            // Rear channels use difference-signal phase (ambient content)
            chFreq[kFL][f] = std::polar(ampTotal * gFL, phL);
            chFreq[kFR][f] = std::polar(ampTotal * gFR, phR);
            chFreq[kFC][f] = std::polar(ampTotal * gC, phCenter);
            chFreq[kBL][f] = std::polar(ampTotal * gBL, phDiff);
            chFreq[kBR][f] = std::polar(ampTotal * gBR, phDiff + kPi);

            // LFE bass redirection: cosine crossover from loCut to hiCut
            if (useLfe && f < hiCutBin) {
                double lfeGain = (f < loCutBin)
                                     ? 1.0
                                     : 0.5 * (1.0 + std::cos(kPi * (f - loCutBin) / (hiCutBin - loCutBin)));
                chFreq[kLFE][f] = std::polar(ampTotal * lfeGain, phCenter);
                double reduction = 1.0 - lfeGain;
                chFreq[kFL][f] *= reduction;
                chFreq[kFR][f] *= reduction;
                chFreq[kFC][f] *= reduction;
                chFreq[kBL][f] *= reduction;
                chFreq[kBR][f] *= reduction;
            }
        }

        // Shift previous output for overlap-add
        std::memcpy(outbuf.data(), &outbuf[kChannels * N / 2],
                     N * kChannels * sizeof(double));
        std::memset(&outbuf[kChannels * N], 0,
                     kChannels * sizeof(double) * N / 2);

        // Inverse FFT each channel, window, and overlap-add
        for (unsigned c = 0; c < kChannels; c++) {
            kiss_fftri(inverse, reinterpret_cast<kiss_fft_cpx*>(chFreq[c].data()), tmpOut.data());
            for (unsigned k = 0; k < N; k++)
                outbuf[kChannels * (k + N / 2) + c] += wnd[k] * tmpOut[k];
        }
    }
};

// ===========================================================================
// SoundMatrixDecoder public interface
// ===========================================================================

namespace Ship {

SoundMatrixDecoder::SoundMatrixDecoder(int32_t sampleRate) : mSampleRate(sampleRate) {
    mDecoder = std::make_unique<DecoderState>(BLOCK_SIZE, sampleRate);
    mFloatInput.resize(BLOCK_SIZE * 2);
}

SoundMatrixDecoder::~SoundMatrixDecoder() = default;

void SoundMatrixDecoder::ResetState() {
    mDecoder->flush();
    mInputFrames = 0;
}

int16_t SoundMatrixDecoder::Saturate(float value) {
    float scaled = value * 32768.0f;
    if (scaled > 32767.0f)
        return 32767;
    if (scaled < -32768.0f)
        return -32768;
    return static_cast<int16_t>(scaled);
}

std::tuple<const uint8_t*, int> SoundMatrixDecoder::Process(const uint8_t* buf, size_t len) {
    const int16_t* stereoInput = reinterpret_cast<const int16_t*>(buf);
    int totalFrames = static_cast<int>(len / (2 * sizeof(int16_t)));

    mOutputBuffer.clear();

    int inputPos = 0;
    while (inputPos < totalFrames) {
        int framesToCopy =
            std::min(totalFrames - inputPos, static_cast<int>(BLOCK_SIZE - mInputFrames));

        for (int i = 0; i < framesToCopy; i++) {
            mFloatInput[(mInputFrames + i) * 2] = stereoInput[(inputPos + i) * 2] / 32768.0f;
            mFloatInput[(mInputFrames + i) * 2 + 1] = stereoInput[(inputPos + i) * 2 + 1] / 32768.0f;
        }

        mInputFrames += framesToCopy;
        inputPos += framesToCopy;

        if (mInputFrames == BLOCK_SIZE) {
            // Convert float input to double for FFT processing
            std::vector<double> doubleInput(BLOCK_SIZE * 2);
            for (unsigned i = 0; i < BLOCK_SIZE * 2; i++)
                doubleInput[i] = mFloatInput[i];

            double* decoded = mDecoder->decode(doubleInput.data());

            // Output is already in SMPTE order (FL, FR, C, LFE, BL, BR)
            size_t prevSize = mOutputBuffer.size();
            mOutputBuffer.resize(prevSize + BLOCK_SIZE * NUM_OUT_CHANNELS);

            for (unsigned i = 0; i < BLOCK_SIZE; i++) {
                for (unsigned c = 0; c < NUM_OUT_CHANNELS; c++) {
                    mOutputBuffer[prevSize + i * NUM_OUT_CHANNELS + c] =
                        Saturate(static_cast<float>(decoded[i * NUM_OUT_CHANNELS + c]));
                }
            }

            mInputFrames = 0;
        }
    }

    return { reinterpret_cast<const uint8_t*>(mOutputBuffer.data()),
             static_cast<int>(mOutputBuffer.size() * sizeof(int16_t)) };
}

} // namespace Ship
