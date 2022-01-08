#include "OTRCutscene.h"

void OtrLib::OTRCutsceneV0::ParseFileBinary(BinaryReader* reader, OTRResource* res)
{
	OTRCutscene* cs = (OTRCutscene*)res;

	OTRResourceFile::ParseFileBinary(reader, res);

	int numEntries = reader->ReadUInt32();
	
	for (int i = 0; i < numEntries; i++)
	{
		uint32_t data = reader->ReadUInt32();
		uint16_t opcode = data >> 16;

		cs->commands.push_back(data);
	}

	int bp = 0;
}
