#include "OTRCutscene.h"

void OtrLib::OTRCutsceneV0::ParseFileBinary(BinaryReader* reader, OTRResource* res)
{
	OTRCutscene* cs = (OTRCutscene*)res;

	OTRResourceFile::ParseFileBinary(reader, res);

	bool cont = true;

	while (cont)
	{

	}
}
