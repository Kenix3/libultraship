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

			if (opcode == G_ENDDL)
				break;
		}
	}
}