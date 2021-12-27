#include "OTRPlayerAnimationFactory.h"

namespace OtrLib
{
    OTRPlayerAnimation* OTRPlayerAnimationFactory::ReadPlayerAnimation(BinaryReader* reader)
    {
        OTRPlayerAnimation* anim = new OTRPlayerAnimation();

        uint32_t version = reader->ReadUInt32();

        switch (version)
        {
        case 0:
        {
            OTRPlayerAnimationV0 otrAnim = OTRPlayerAnimationV0();
            otrAnim.ParseFileBinary(reader, anim);
        }
        break;
        default:
            // VERSION NOT SUPPORTED
            break;
        }

        return anim;

    }
}