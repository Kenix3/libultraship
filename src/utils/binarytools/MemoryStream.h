#pragma once

#include <memory>
#include <vector>
#include "Stream.h"

namespace Ship {
class MemoryStream : public Stream {
  public:
    MemoryStream();
    MemoryStream(char* nBuffer, size_t nBufferSize);
    MemoryStream(std::shared_ptr<std::vector<char>> buffer);
    ~MemoryStream();

    uint64_t GetLength() override;

    void Seek(int32_t offset, SeekOffsetType seekType) override;

    std::unique_ptr<char[]> Read(size_t length) override;
    void Read(const char* dest, size_t length) override;
    int8_t ReadByte() override;

    void Write(char* srcBuffer, size_t length) override;
    void WriteByte(int8_t value) override;

    std::vector<char> ToVector() override;

    void Flush() override;
    void Close() override;

  protected:
    std::shared_ptr<std::vector<char>> mBuffer;
    std::size_t mBufferSize;
};
} // namespace Ship
