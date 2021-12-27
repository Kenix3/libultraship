#include "OTRSkeletonFactory.h"

namespace OtrLib
{
    OTRSkeleton* OTRSkeletonFactory::ReadSkeleton(BinaryReader* reader)
    {
        OTRSkeleton* skel = new OTRSkeleton();

        uint32_t version = reader->ReadUInt32();

        switch (version)
        {
        case 0:
        {
            OTRSkeletonV0 otrSkel = OTRSkeletonV0();
            otrSkel.ParseFileBinary(reader, skel);
        }
        break;
        default:
            // VERSION NOT SUPPORTED
            break;
        }

        return skel;
    }
}