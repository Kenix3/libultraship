#include "MemoryStream.h"
#include <cstring>

#ifndef _MSC_VER
#define memcpy_s(dest, destSize, source, sourceSize) memcpy(dest, source, destSize)
#endif

ShipDK::MemoryStream::MemoryStream() {
    mBuffer = std::make_shared<std::vector<char>>();
    // mBuffer.reserve(1024 * 16);
    mBufferSize = 0;
    mBaseAddress = 0;
}

ShipDK::MemoryStream::MemoryStream(char* nBuffer, size_t nBufferSize) : MemoryStream() {
    mBuffer = std::make_shared<std::vector<char>>(nBuffer, nBuffer + nBufferSize);
    mBufferSize = nBufferSize;
    mBaseAddress = 0;
}

ShipDK::MemoryStream::MemoryStream(std::shared_ptr<std::vector<char>> buffer) : MemoryStream() {
    mBuffer = buffer;
    mBufferSize = buffer->size();
    mBaseAddress = 0;
}

ShipDK::MemoryStream::~MemoryStream() {
}

uint64_t ShipDK::MemoryStream::GetLength() {
    return mBuffer->size();
}

void ShipDK::MemoryStream::Seek(int32_t offset, SeekOffsetType seekType) {
    if (seekType == SeekOffsetType::Start) {
        mBaseAddress = offset;
    } else if (seekType == SeekOffsetType::Current) {
        mBaseAddress += offset;
    } else if (seekType == SeekOffsetType::End) {
        mBaseAddress = mBufferSize - 1 - offset;
    }
}

std::unique_ptr<char[]> ShipDK::MemoryStream::Read(size_t length) {
    std::unique_ptr<char[]> result = std::make_unique<char[]>(length);

    memcpy_s(result.get(), length, &mBuffer->at(mBaseAddress), length);
    mBaseAddress += length;

    return result;
}

void ShipDK::MemoryStream::Read(const char* dest, size_t length) {
    memcpy_s((void*)dest, length, &mBuffer->at(mBaseAddress), length);
    mBaseAddress += length;
}

int8_t ShipDK::MemoryStream::ReadByte() {
    return mBuffer->at(mBaseAddress++);
}

void ShipDK::MemoryStream::Write(char* srcBuffer, size_t length) {
    if (mBaseAddress + length >= mBuffer->size()) {
        mBuffer->resize(mBaseAddress + length);
        mBufferSize += length;
    }

    memcpy_s(&((*mBuffer)[mBaseAddress]), length, srcBuffer, length);
    mBaseAddress += length;
}

void ShipDK::MemoryStream::WriteByte(int8_t value) {
    if (mBaseAddress >= mBuffer->size()) {
        mBuffer->resize(mBaseAddress + 1);
        mBufferSize = mBaseAddress;
    }

    mBuffer->at(mBaseAddress++) = value;
}

std::vector<char> ShipDK::MemoryStream::ToVector() {
    return *mBuffer;
}

void ShipDK::MemoryStream::Flush() {
}

void ShipDK::MemoryStream::Close() {
}
