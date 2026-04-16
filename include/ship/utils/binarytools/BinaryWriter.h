#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>
#include "endianness.h"
#include "Stream.h"

namespace Ship {
/**
 * @brief Sequential binary writer with configurable byte-order support.
 *
 * BinaryWriter wraps a Stream and provides typed write methods for all primitive
 * integer and floating-point types as well as strings. The byte order used for
 * multi-byte writes is configurable at runtime via SetEndianness().
 *
 * The default constructor creates an internal MemoryStream that grows automatically;
 * use GetStream() to retrieve the result or ToVector() for a raw copy.
 *
 * Typical usage:
 * @code
 * Ship::BinaryWriter writer;
 * writer.SetEndianness(Ship::Endianness::Big);
 * writer.Write(static_cast<uint32_t>(0xDEADBEEF));
 * writer.Write(std::string("hello"));
 * auto bytes = writer.ToVector();
 * @endcode
 */
class BinaryWriter {
  public:
    /** @brief Constructs a BinaryWriter backed by a new, empty, growable MemoryStream. */
    BinaryWriter();

    /**
     * @brief Constructs a BinaryWriter over a raw (non-owning) Stream pointer.
     * @param nStream Pointer to an already-constructed stream (ownership not transferred).
     */
    BinaryWriter(Stream* nStream);

    /**
     * @brief Constructs a BinaryWriter over a shared Stream.
     * @param nStream Shared stream to write into.
     */
    BinaryWriter(std::shared_ptr<Stream> nStream);

    /**
     * @brief Sets the byte order used for all subsequent multi-byte writes.
     * @param endianness Endianness::Little, Big, or Native.
     */
    void SetEndianness(Endianness endianness);

    /**
     * @brief Returns the underlying stream (e.g. to pass to a BinaryReader after writing).
     */
    std::shared_ptr<Stream> GetStream();

    /**
     * @brief Returns the current absolute write position.
     */
    uint64_t GetBaseAddress();

    /**
     * @brief Returns the total number of bytes written to the stream.
     */
    uint64_t GetLength();

    /**
     * @brief Moves the stream write position.
     * @param offset   Byte offset.
     * @param seekType Reference point (Start, Current, or End).
     */
    void Seek(int32_t offset, SeekOffsetType seekType);

    /**
     * @brief Closes the underlying stream and flushes any buffered data.
     */
    void Close();

    /** @brief Writes a signed 8-bit integer. */
    void Write(int8_t value);

    /** @brief Writes an unsigned 8-bit integer. */
    void Write(uint8_t value);

    /** @brief Writes a signed 16-bit integer, applying the configured byte order. */
    void Write(int16_t value);

    /** @brief Writes an unsigned 16-bit integer, applying the configured byte order. */
    void Write(uint16_t value);

    /** @brief Writes a signed 32-bit integer, applying the configured byte order. */
    void Write(int32_t value);

    /**
     * @brief Writes two signed 32-bit integers sequentially, applying the configured byte order.
     * @param valueA First integer.
     * @param valueB Second integer.
     */
    void Write(int32_t valueA, int32_t valueB);

    /** @brief Writes an unsigned 32-bit integer, applying the configured byte order. */
    void Write(uint32_t value);

    /** @brief Writes a signed 64-bit integer, applying the configured byte order. */
    void Write(int64_t value);

    /** @brief Writes an unsigned 64-bit integer, applying the configured byte order. */
    void Write(uint64_t value);

    /** @brief Writes a 32-bit IEEE 754 float, applying the configured byte order. */
    void Write(float value);

    /** @brief Writes a 64-bit IEEE 754 double, applying the configured byte order. */
    void Write(double value);

    /**
     * @brief Writes a length-prefixed UTF-8 string.
     *
     * Writes a 32-bit unsigned length (in the configured byte order) followed by the
     * string's bytes. No null terminator is written.
     *
     * @param str String to write.
     */
    void Write(const std::string& str);

    /**
     * @brief Writes @p length raw bytes from @p srcBuffer.
     * @param srcBuffer Source buffer.
     * @param length    Number of bytes to write.
     */
    void Write(char* srcBuffer, size_t length);

    /**
     * @brief Returns the entire stream contents as a vector of chars.
     */
    std::vector<char> ToVector();

  protected:
    std::shared_ptr<Stream> mStream;             ///< Underlying byte stream.
    Endianness mEndianness = Endianness::Native; ///< Active byte order for multi-byte writes.
};
} // namespace Ship
