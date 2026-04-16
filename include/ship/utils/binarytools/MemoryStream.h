#pragma once

#include <memory>
#include <vector>
#include "Stream.h"

namespace Ship {
/**
 * @brief A Stream backed by an in-memory byte buffer.
 *
 * MemoryStream can be constructed from an existing raw buffer, from a shared
 * vector (with an optional initial seek offset), or as an empty growable buffer.
 * It is the concrete stream type used by BinaryReader and BinaryWriter throughout
 * the archive and resource serialisation pipeline.
 *
 * All operations are performed in-process; no I/O is involved. The internal buffer
 * is managed via a shared_ptr<vector<char>> so that multiple owners can safely share
 * the same underlying storage.
 */
class MemoryStream final : public Stream {
  public:
    /** @brief Constructs an empty, growable memory stream. */
    MemoryStream();

    /**
     * @brief Constructs a memory stream by copying a raw byte buffer.
     *
     * The contents of the buffer are copied into an internal vector, so the
     * caller is free to release the original buffer after construction.
     *
     * @param nBuffer     Pointer to the raw byte buffer to copy.
     * @param nBufferSize Size of the buffer in bytes.
     */
    MemoryStream(char* nBuffer, size_t nBufferSize);

    /**
     * @brief Constructs a memory stream over a shared vector, positioned at the start.
     * @param buffer Shared vector to use as the backing store.
     */
    MemoryStream(std::shared_ptr<std::vector<char>> buffer);

    /**
     * @brief Constructs a memory stream over a shared vector at a given byte offset.
     * @param buffer Shared vector to use as the backing store.
     * @param offset Initial seek position (byte offset from the beginning of @p buffer).
     */
    MemoryStream(std::shared_ptr<std::vector<char>> buffer, size_t offset);
    ~MemoryStream();

    /**
     * @brief Returns the total number of bytes in the backing buffer.
     */
    uint64_t GetLength() override;

    /**
     * @brief Moves the current position within the backing buffer.
     * @param offset   Byte offset (can be negative for SeekOffsetType::Current and End).
     * @param seekType Reference point (Start, Current, or End).
     */
    void Seek(int32_t offset, SeekOffsetType seekType) override;

    /**
     * @brief Reads @p length bytes starting at the current position.
     * @param length Number of bytes to read.
     * @return Heap-allocated buffer containing the copied bytes.
     */
    std::unique_ptr<char[]> Read(size_t length) override;

    /**
     * @brief Reads @p length bytes into @p dest from the current position.
     * @param dest   Writable destination buffer (must be at least @p length bytes).
     * @param length Number of bytes to read.
     */
    void Read(char* dest, size_t length) override;

    /**
     * @brief Reads a single signed byte and advances the position by one.
     *
     * Throws std::out_of_range if the position is past the end of the buffer.
     *
     * @return The byte value at the current position.
     */
    int8_t ReadByte() override;

    /**
     * @brief Writes @p length bytes from @p srcBuffer at the current position.
     *
     * If the stream was constructed as growable (no fixed size), the backing vector
     * is extended as needed.
     *
     * @param srcBuffer Source buffer.
     * @param length    Number of bytes to write.
     */
    void Write(char* srcBuffer, size_t length) override;

    /**
     * @brief Writes a single signed byte at the current position.
     * @param value Byte to write.
     */
    void WriteByte(int8_t value) override;

    /**
     * @brief Returns a copy of the entire backing buffer as a vector.
     */
    std::vector<char> ToVector() override;

    /** @brief No-op for in-memory streams; included for interface compliance. */
    void Flush() override;

    /** @brief No-op for in-memory streams; the buffer remains valid after closing. */
    void Close() override;

  protected:
    std::shared_ptr<std::vector<char>> mBuffer; ///< Shared backing store.
    std::size_t mBufferSize;                    ///< Fixed size (0 if growable).
};
} // namespace Ship
