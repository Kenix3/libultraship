#pragma once

#include <string>
#include <memory>
#include <vector>
#include "endianness.h"
#include "Stream.h"

class BinaryReader;

namespace Ship {

/**
 * @brief Sequential binary reader with configurable byte-order support.
 *
 * BinaryReader wraps a Stream and provides typed read methods for all primitive
 * integer and floating-point types, as well as length-prefixed and null-terminated
 * strings. The byte order (endianness) used for multi-byte reads is configurable at
 * runtime via SetEndianness().
 *
 * Typical usage:
 * @code
 * auto stream = std::make_shared<Ship::MemoryStream>(rawPtr, size);
 * Ship::BinaryReader reader(stream);
 * reader.SetEndianness(Ship::Endianness::Big);
 * uint32_t magic = reader.ReadUInt32();
 * std::string name = reader.ReadString();
 * @endcode
 */
class BinaryReader {
  public:
    /**
     * @brief Constructs a BinaryReader over a raw byte buffer.
     *
     * Internally wraps the buffer in a MemoryStream.
     *
     * @param nBuffer     Pointer to the raw byte buffer.
     * @param nBufferSize Size of the buffer in bytes.
     */
    BinaryReader(char* nBuffer, size_t nBufferSize);

    /**
     * @brief Constructs a BinaryReader over a raw (non-owning) Stream pointer.
     * @param nStream Pointer to an already-constructed stream (ownership not transferred).
     */
    BinaryReader(Stream* nStream);

    /**
     * @brief Constructs a BinaryReader over a shared Stream.
     * @param nStream Shared stream to read from.
     */
    BinaryReader(std::shared_ptr<Stream> nStream);

    /**
     * @brief Closes the underlying stream and releases resources.
     */
    void Close();

    /**
     * @brief Sets the byte order used for all subsequent multi-byte reads.
     * @param endianness Endianness::Little, Big, or Native.
     */
    void SetEndianness(Endianness endianness);

    /**
     * @brief Returns the current byte order.
     */
    Endianness GetEndianness() const;

    /**
     * @brief Moves the stream position.
     * @param offset   Byte offset.
     * @param seekType Reference point (Start, Current, or End).
     */
    void Seek(int32_t offset, SeekOffsetType seekType);

    /**
     * @brief Returns the current absolute stream position.
     */
    uint32_t GetBaseAddress();

    /**
     * @brief Discards @p length bytes from the current position (advances without returning data).
     * @param length Number of bytes to skip.
     */
    void Read(int32_t length);

    /**
     * @brief Reads @p length raw bytes into @p buffer.
     * @param buffer Destination buffer (must be at least @p length bytes).
     * @param length Number of bytes to read.
     */
    void Read(char* buffer, int32_t length);

    /**
     * @brief Reads a single ASCII character.
     * @return Character at the current position.
     */
    char ReadChar();

    /** @brief Reads a signed 8-bit integer. */
    int8_t ReadInt8();

    /** @brief Reads a signed 16-bit integer, applying the configured byte order. */
    int16_t ReadInt16();

    /** @brief Reads a signed 32-bit integer, applying the configured byte order. */
    int32_t ReadInt32();

    /** @brief Reads a signed 64-bit integer, applying the configured byte order. */
    int64_t ReadInt64();

    /** @brief Reads an unsigned 8-bit integer. */
    uint8_t ReadUByte();

    /** @brief Reads an unsigned 16-bit integer, applying the configured byte order. */
    uint16_t ReadUInt16();

    /** @brief Reads an unsigned 32-bit integer, applying the configured byte order. */
    uint32_t ReadUInt32();

    /** @brief Reads an unsigned 64-bit integer, applying the configured byte order. */
    uint64_t ReadUInt64();

    /** @brief Reads a 32-bit IEEE 754 float, applying the configured byte order. */
    float ReadFloat();

    /** @brief Reads a 64-bit IEEE 754 double, applying the configured byte order. */
    double ReadDouble();

    /**
     * @brief Reads a length-prefixed UTF-8 string.
     *
     * The length prefix is a 32-bit unsigned integer in the configured byte order
     * followed by that many bytes of string data (no null terminator).
     */
    std::string ReadString();

    /**
     * @brief Reads a null-terminated C-style string.
     *
     * Reads bytes until a '\0' is encountered. The null terminator is consumed but
     * not included in the returned string.
     */
    std::string ReadCString();

    /**
     * @brief Returns the entire stream contents as a vector of chars.
     */
    std::vector<char> ToVector();

  protected:
    std::shared_ptr<Stream> mStream;              ///< Underlying byte stream.
    Endianness mEndianness = Endianness::Native;   ///< Active byte order for multi-byte reads.
};
} // namespace Ship
