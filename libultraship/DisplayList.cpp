#include "DisplayList.h"
#include "PR/ultra64/gbi.h"

namespace Ship
{
	void DisplayListV0::ParseFileBinary(BinaryReader* reader, Resource* res)
	{
		DisplayList* dl = (DisplayList*)res;

		ResourceFile::ParseFileBinary(reader, res);

		while (reader->GetBaseAddress() % 8 != 0)
			reader->ReadByte();

		while (true)
		{
			uint64_t data = reader->ReadUInt64();

			dl->instructions.push_back(data);
				
			uint8_t opcode = data >> 24;

			// These are 128-bit commands, so read an extra 64 bits...
			if (opcode == G_SETTIMG_LUS || opcode == G_DL_LUS || opcode == G_VTX_LUS || opcode == G_MARKER)
				dl->instructions.push_back(reader->ReadUInt64());

			if (opcode == G_ENDDL)
				break;
		}
	}
}