#include "BinaryWriter.h"

Ship::BinaryWriter::BinaryWriter(Stream* nStream) {
    stream.reset(nStream);
}

Ship::BinaryWriter::BinaryWriter(std::shared_ptr<Stream> nStream) {
    stream = nStream;
}

void Ship::BinaryWriter::SetEndianness(Endianness endianness) {
    this->endianness = endianness;
}

void Ship::BinaryWriter::Close() {
    stream->Close();
}

std::shared_ptr<Ship::Stream> Ship::BinaryWriter::GetStream() {
    return stream;
}

uint64_t Ship::BinaryWriter::GetBaseAddress() {
    return stream->GetBaseAddress();
}

uint64_t Ship::BinaryWriter::GetLength() {
    return stream->GetLength();
}

void Ship::BinaryWriter::Seek(int32_t offset, SeekOffsetType seekType) {
    stream->Seek(offset, seekType);
}

void Ship::BinaryWriter::Write(int8_t value) {
    stream->Write((char*)&value, sizeof(int8_t));
}

void Ship::BinaryWriter::Write(uint8_t value) {
    stream->Write((char*)&value, sizeof(uint8_t));
}

void Ship::BinaryWriter::Write(int16_t value) {
    if (endianness != Endianness::Native) {
        value = BSWAP16(value);
    }

    stream->Write((char*)&value, sizeof(int16_t));
}

void Ship::BinaryWriter::Write(uint16_t value) {
    if (endianness != Endianness::Native) {
        value = BSWAP16(value);
    }

    stream->Write((char*)&value, sizeof(uint16_t));
}

void Ship::BinaryWriter::Write(int32_t value) {
    if (endianness != Endianness::Native) {
        value = BSWAP32(value);
    }

    stream->Write((char*)&value, sizeof(int32_t));
}

void Ship::BinaryWriter::Write(int32_t valueA, int32_t valueB) {
    Write(valueA);
    Write(valueB);
}

void Ship::BinaryWriter::Write(uint32_t value) {
    if (endianness != Endianness::Native) {
        value = BSWAP32(value);
    }

    stream->Write((char*)&value, sizeof(uint32_t));
}

void Ship::BinaryWriter::Write(int64_t value) {
    if (endianness != Endianness::Native) {
        value = BSWAP64(value);
    }

    stream->Write((char*)&value, sizeof(int64_t));
}

void Ship::BinaryWriter::Write(uint64_t value) {
    if (endianness != Endianness::Native) {
        value = BSWAP64(value);
    }

    stream->Write((char*)&value, sizeof(uint64_t));
}

void Ship::BinaryWriter::Write(float value) {
    if (endianness != Endianness::Native) {
        float tmp;
        char* dst = (char*)&tmp;
        char* src = (char*)&value;
        dst[3] = src[0];
        dst[2] = src[1];
        dst[1] = src[2];
        dst[0] = src[3];
        value = tmp;
    }

    stream->Write((char*)&value, sizeof(float));
}

void Ship::BinaryWriter::Write(double value) {
    if (endianness != Endianness::Native) {
        double tmp;
        char* dst = (char*)&tmp;
        char* src = (char*)&value;
        dst[7] = src[0];
        dst[6] = src[1];
        dst[5] = src[2];
        dst[4] = src[3];
        dst[3] = src[4];
        dst[2] = src[5];
        dst[1] = src[6];
        dst[0] = src[7];
        value = tmp;
    }

    stream->Write((char*)&value, sizeof(double));
}

void Ship::BinaryWriter::Write(const std::string& str) {
    int strLen = str.size();
    Write(strLen);

    for (char c : str) {
        stream->WriteByte(c);
    }
}

void Ship::BinaryWriter::Write(char* srcBuffer, size_t length) {
    stream->Write(srcBuffer, length);
}
