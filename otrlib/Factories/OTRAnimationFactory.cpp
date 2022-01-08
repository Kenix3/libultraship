#include "OTRAnimationFactory.h"

namespace OtrLib
{
    OTRAnimation* OTRAnimationFactory::ReadAnimation(BinaryReader* reader)
    {
        OTRAnimation* anim = new OTRAnimation();
        OTRVersion version = (OTRVersion)reader->ReadUInt32();

        switch (version)
        {
        case OTRVersion::Deckard:
        {
            OTRAnimationV0 otrAnim = OTRAnimationV0();
            otrAnim.ParseFileBinary(reader, anim);
        }
        break;
        default:
            // VERSION NOT SUPPORTED
            break;
        }

        return anim;
    }
};