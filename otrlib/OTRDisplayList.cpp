#include "OTRDisplayList.h"
#include "PR/ultra64/gbi.h"

namespace OtrLib
{
	void OTRDisplayListV0::ParseFileBinary(BinaryReader* reader, OTRResource* res)
	{
		OTRDisplayList* dl = (OTRDisplayList*)res;

		OTRResourceFile::ParseFileBinary(reader, res);

		while (reader->GetBaseAddress() % 8 != 0)
			reader->ReadByte();

		while (true)
		{
			uint64_t data = reader->ReadUInt64();

			dl->instructions.push_back(data);
				
			uint8_t opcode = data >> 24;

			// These are 128-bit commands, so read an extra 64 bits...
			if (opcode == G_SETTIMG_OTR || opcode == G_DL_OTR || opcode == G_VTX_OTR || opcode == G_MARKER)
				dl->instructions.push_back(reader->ReadUInt64());

			if (opcode == G_ENDDL)
				break;
		}
	}
}