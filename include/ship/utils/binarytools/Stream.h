#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace Ship {

/** @brief Identifies the reference point for a seek operation. */
enum class SeekOffsetType {
    Start,   ///< Seek relative to the beginning of the stream.
    Current, ///< Seek relative to the current position.
    End,     ///< Seek relative to the end of the stream.
};

/**
 * @brief Abstract base class for sequential byte streams (read/write).
 *
 * Stream provides a common interface used by BinaryReader and BinaryWriter so
 * that both can operate transparently over an in-memory buffer (MemoryStream) or
 * any other concrete stream implementation without code changes.
 *
 * All read and write operations advance an internal position cursor. Seeking is
 * supported via Seek() with relative or absolute offsets.
 */
class Stream {
  public:
    virtual ~Stream() = default;

    /**
     * @brief Returns the total length of the stream in bytes.
     */
    virtual uint64_t GetLength() = 0;

    /**
     * @brief Returns the current stream position as an absolute byte offset.
     */
    uint64_t GetBaseAddress();

    /**
     * @brief Moves the stream position.
     * @param offset   Number of bytes to seek (can be negative for SeekOffsetType::Current/End).
     * @param seekType Reference point for the seek (start, current, or end).
     */
    virtual void Seek(int32_t offset, SeekOffsetType seekType) = 0;

    /**
     * @brief Reads @p length bytes from the current position and advances the position.
     * @param length Number of bytes to read.
     * @return Heap-allocated buffer containing the read bytes.
     */
    virtual std::unique_ptr<char[]> Read(size_t length) = 0;

    /**
     * @brief Reads @p length bytes from the current position into a caller-supplied buffer.
     * @param dest   Writable destination buffer (must be at least @p length bytes).
     * @param length Number of bytes to read.
     */
    virtual void Read(char* dest, size_t length) = 0;

    /**
     * @brief Reads a single byte from the current position.
     *
     * Throws std::out_of_range if the position is past the end of the stream.
     *
     * @return The byte value at the current position.
     */
    virtual int8_t ReadByte() = 0;

    /**
     * @brief Writes @p length bytes from @p destBuffer at the current position.
     * @param destBuffer Source buffer.
     * @param length     Number of bytes to write.
     */
    virtual void Write(char* destBuffer, size_t length) = 0;

    /**
     * @brief Writes a single byte at the current position.
     * @param value Byte to write.
     */
    virtual void WriteByte(int8_t value) = 0;

    /**
     * @brief Returns the full stream contents as a std::vector<char>.
     */
    virtual std::vector<char> ToVector() = 0;

    /**
     * @brief Flushes any buffered writes to the underlying storage.
     */
    virtual void Flush() = 0;

    /**
     * @brief Closes the stream and releases any held resources.
     */
    virtual void Close() = 0;

  protected:
    uint64_t mBaseAddress; ///< Current read/write position (byte offset from the start).
};
} // namespace Ship
