#include "MemoryStream.h"
#include <cstring>

#ifndef _MSC_VER
#define memcpy_s(dest, destSize, source, sourceSize) memcpy(dest, source, destSize)
#endif

Ship::MemoryStream::MemoryStream() {
    mBuffer = std::make_shared<std::vector<char>>();
    // mBuffer.reserve(1024 * 16);
    mBufferSize = 0;
    mBaseAddress = 0;
}

Ship::MemoryStream::MemoryStream(char* nBuffer, size_t nBufferSize) : MemoryStream() {
    mBuffer = std::make_shared<std::vector<char>>(nBuffer, nBuffer + nBufferSize);
    mBufferSize = nBufferSize;
    mBaseAddress = 0;
}

Ship::MemoryStream::MemoryStream(std::shared_ptr<std::vector<char>> buffer) : MemoryStream() {
    mBuffer = buffer;
    mBufferSize = buffer->size();
    mBaseAddress = 0;
}

Ship::MemoryStream::~MemoryStream() {
}

uint64_t Ship::MemoryStream::GetLength() {
    return mBuffer->size();
}

void Ship::MemoryStream::Seek(int32_t offset, SeekOffsetType seekType) {
    if (seekType == SeekOffsetType::Start) {
        mBaseAddress = offset;
    } else if (seekType == SeekOffsetType::Current) {
        mBaseAddress += offset;
    } else if (seekType == SeekOffsetType::End) {
        mBaseAddress = mBufferSize - 1 - offset;
    }
}

std::unique_ptr<char[]> Ship::MemoryStream::Read(size_t length) {
    std::unique_ptr<char[]> result = std::make_unique<char[]>(length);

    memcpy_s(result.get(), length, &mBuffer->at(mBaseAddress), length);
    mBaseAddress += length;

    return result;
}

void Ship::MemoryStream::Read(const char* dest, size_t length) {
    memcpy_s((void*)dest, length, &mBuffer->at(mBaseAddress), length);
    mBaseAddress += length;
}

int8_t Ship::MemoryStream::ReadByte() {
    return mBuffer->at(mBaseAddress++);
}

void Ship::MemoryStream::Write(char* srcBuffer, size_t length) {
    if (mBaseAddress + length >= mBuffer->size()) {
        mBuffer->resize(mBaseAddress + length);
        mBufferSize += length;
    }

    memcpy_s(&((*mBuffer)[mBaseAddress]), length, srcBuffer, length);
    mBaseAddress += length;
}

void Ship::MemoryStream::WriteByte(int8_t value) {
    if (mBaseAddress >= mBuffer->size()) {
        mBuffer->resize(mBaseAddress + 1);
        mBufferSize = mBaseAddress;
    }

    mBuffer->at(mBaseAddress++) = value;
}

std::vector<char> Ship::MemoryStream::ToVector() {
    return *mBuffer;
}

void Ship::MemoryStream::Flush() {
}

void Ship::MemoryStream::Close() {
}
