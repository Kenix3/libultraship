#pragma once

/**
 * @brief Selects the number and arrangement of output audio channels.
 *
 * This enum controls both how the audio player configures the hardware device and
 * whether the SoundMatrixDecoder is engaged to downmix or upmix the game's signal.
 */
typedef enum AudioChannelsSetting {
    audioStereo,   ///< Standard 2-channel stereo output (L, R).
    audioMatrix51, ///< 5.1 surround produced by decoding a stereo signal through a sound matrix.
    audioRaw51,    ///< 5.1 surround using raw 6-channel game output without matrix decoding.
    audioMax       ///< Sentinel value; do not use as an audio mode.
} AudioChannelsSetting;

/**
 * @brief Returns a human-readable name for the given AudioChannelsSetting.
 * @param setting The channel mode to describe.
 * @return A null-terminated string such as "Stereo" or "5.1 Matrix".
 */
inline const char* AudioChannelsSettingName(AudioChannelsSetting setting) {
    switch (setting) {
        case audioStereo:
            return "Stereo";
        case audioMatrix51:
            return "5.1 Matrix";
        case audioRaw51:
            return "5.1 Raw";
        default:
            return "Unknown";
    }
}
