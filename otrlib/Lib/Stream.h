#pragma once

#include <stdint.h>
#include <memory>

namespace OtrLib
{
	enum class SeekOffsetType
	{
		Start,
		Current,
		End
	};

	class Stream
	{
	public:
		int64_t BaseAddress;

		Stream();

		virtual void Seek(int32_t offset, SeekOffsetType seekType) = 0;

		virtual void Read(char* buffer, int32_t length) = 0;
		virtual int8_t ReadByte() = 0;

		virtual void Write(char* destBuffer, int32_t length) = 0;
		virtual void WriteByte(int8_t value) = 0;

		virtual void Flush() = 0;
		virtual void Close() = 0;
	};
}