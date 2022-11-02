#include "BinaryReader.h"
#include <cmath>
#include <stdexcept>

Ship::BinaryReader::BinaryReader(Stream* nStream) {
    stream.reset(nStream);
}

Ship::BinaryReader::BinaryReader(std::shared_ptr<Stream> nStream) {
    stream = nStream;
}

void Ship::BinaryReader::Close() {
    stream->Close();
}

void Ship::BinaryReader::SetEndianness(Endianness endianness) {
    this->endianness = endianness;
}

Ship::Endianness Ship::BinaryReader::GetEndianness() const {
    return endianness;
}

void Ship::BinaryReader::Seek(int32_t offset, SeekOffsetType seekType) {
    stream->Seek(offset, seekType);
}

uint32_t Ship::BinaryReader::GetBaseAddress() {
    return stream->GetBaseAddress();
}

void Ship::BinaryReader::Read(int32_t length) {
    stream->Read(length);
}

void Ship::BinaryReader::Read(char* buffer, int32_t length) {
    stream->Read(buffer, length);
}

char Ship::BinaryReader::ReadChar() {
    return (char)stream->ReadByte();
}

int8_t Ship::BinaryReader::ReadInt8() {
    return stream->ReadByte();
}

int16_t Ship::BinaryReader::ReadInt16() {
    int16_t result = 0;
    stream->Read((char*)&result, sizeof(int16_t));
    if (endianness != Endianness::Native) {
        result = BSWAP16(result);
    }

    return result;
}

int32_t Ship::BinaryReader::ReadInt32() {
    int32_t result = 0;

    stream->Read((char*)&result, sizeof(int32_t));

    if (endianness != Endianness::Native) {
        result = BSWAP32(result);
    }

    return result;
}

uint8_t Ship::BinaryReader::ReadUByte() {
    return (uint8_t)stream->ReadByte();
}

uint16_t Ship::BinaryReader::ReadUInt16() {
    uint16_t result = 0;

    stream->Read((char*)&result, sizeof(uint16_t));

    if (endianness != Endianness::Native) {
        result = BSWAP16(result);
    }

    return result;
}

uint32_t Ship::BinaryReader::ReadUInt32() {
    uint32_t result = 0;

    stream->Read((char*)&result, sizeof(uint32_t));

    if (endianness != Endianness::Native) {
        result = BSWAP32(result);
    }

    return result;
}

uint64_t Ship::BinaryReader::ReadUInt64() {
    uint64_t result = 0;

    stream->Read((char*)&result, sizeof(uint64_t));

    if (endianness != Endianness::Native) {
        result = BSWAP64(result);
    }

    return result;
}

float Ship::BinaryReader::ReadFloat() {
    float result = NAN;

    stream->Read((char*)&result, sizeof(float));

    if (endianness != Endianness::Native) {
        float tmp;
        char* dst = (char*)&tmp;
        char* src = (char*)&result;
        dst[3] = src[0];
        dst[2] = src[1];
        dst[1] = src[2];
        dst[0] = src[3];
        result = tmp;
    }

    if (std::isnan(result)) {
        throw std::runtime_error("BinaryReader::ReadFloat(): Error reading stream");
    }

    return result;
}

double Ship::BinaryReader::ReadDouble() {
    double result = NAN;

    stream->Read((char*)&result, sizeof(double));

    if (endianness != Endianness::Native) {
        double tmp;
        char* dst = (char*)&tmp;
        char* src = (char*)&result;
        dst[7] = src[0];
        dst[6] = src[1];
        dst[5] = src[2];
        dst[4] = src[3];
        dst[3] = src[4];
        dst[2] = src[5];
        dst[1] = src[6];
        dst[0] = src[7];
        result = tmp;
    }

    if (std::isnan(result)) {
        throw std::runtime_error("BinaryReader::ReadDouble(): Error reading stream");
    }

    return result;
}

Vec3f Ship::BinaryReader::ReadVec3f() {
    return Vec3f();
}

Vec3s Ship::BinaryReader::ReadVec3s() {
    return Vec3s(0, 0, 0);
}

Vec3s Ship::BinaryReader::ReadVec3b() {
    return Vec3s(0, 0, 0);
}

Vec2f Ship::BinaryReader::ReadVec2f() {
    return Vec2f();
}

Color3b Ship::BinaryReader::ReadColor3b() {
    return Color3b();
}

std::string Ship::BinaryReader::ReadString() {
    std::string res;
    int numChars = ReadInt32();
    for (int i = 0; i < numChars; i++) {
        res += ReadChar();
    }
    return res;
}
