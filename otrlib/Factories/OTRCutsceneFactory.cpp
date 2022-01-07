#include "OTRCutsceneFactory.h"

namespace OtrLib
{
	OTRCutscene* OTRCutsceneFactory::ReadCutscene(BinaryReader* reader)
	{
        OTRCutscene* cs = new OTRCutscene();
        OTRVersion version = (OTRVersion)reader->ReadUInt32();

        switch (version)
        {
        case OTRVersion::Deckard:
        {
            OTRCutsceneV0 otrCS = OTRCutsceneV0();
            otrCS.ParseFileBinary(reader, cs);
        }
        break;
        default:
            // VERSION NOT SUPPORTED
            break;
        }

        return cs;
	}
}