#include "CutsceneFactory.h"

namespace Ship
{
	Cutscene* CutsceneFactory::ReadCutscene(BinaryReader* reader)
	{
        Cutscene* cs = new Cutscene();
        Version version = (Version)reader->ReadUInt32();

        switch (version)
        {
        case Version::Deckard:
        {
            CutsceneV0 otrCS = CutsceneV0();
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