#include "SkeletonLimbFactory.h"

namespace Ship
{
    SkeletonLimb* SkeletonLimbFactory::ReadSkeletonLimb(BinaryReader* reader)
    {
        SkeletonLimb* limb = new SkeletonLimb();

        Version version = (Version)reader->ReadUInt32();

        switch (version)
        {
        case Version::Deckard:
        {
            SkeletonLimbV0 otrLimb = SkeletonLimbV0();
            otrLimb.ParseFileBinary(reader, limb);
        }
        break;
        default:
            // VERSION NOT SUPPORTED
            break;
        }

        return limb;
    }
}