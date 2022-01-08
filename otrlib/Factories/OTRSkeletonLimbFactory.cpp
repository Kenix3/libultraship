#include "OTRSkeletonLimbFactory.h"

namespace OtrLib
{
    OTRSkeletonLimb* OTRSkeletonLimbFactory::ReadSkeletonLimb(BinaryReader* reader)
    {
        OTRSkeletonLimb* limb = new OTRSkeletonLimb();

        OTRVersion version = (OTRVersion)reader->ReadUInt32();

        switch (version)
        {
        case OTRVersion::Deckard:
        {
            OTRSkeletonLimbV0 otrLimb = OTRSkeletonLimbV0();
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