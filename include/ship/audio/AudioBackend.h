#pragma once

namespace Ship {
/** @brief Identifies the audio backend implementation in use. */
enum class AudioBackend { WASAPI, SDL, COREAUDIO, NUL };
} // namespace Ship
