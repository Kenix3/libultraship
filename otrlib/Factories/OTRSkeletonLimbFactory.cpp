#include "OTRSkeletonLimbFactory.h"

namespace OtrLib
{
    OTRSkeletonLimb* OTRSkeletonLimbFactory::ReadSkeletonLimb(BinaryReader* reader)
    {
        OTRSkeletonLimb* limb = new OTRSkeletonLimb();

        uint32_t version = reader->ReadUInt32();

        switch (version)
        {
        case 0:
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