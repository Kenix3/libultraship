#include "OTRSkeletonFactory.h"

namespace OtrLib
{
    OTRSkeleton* OTRSkeletonFactory::ReadSkeleton(BinaryReader* reader)
    {
        OTRSkeleton* skel = new OTRSkeleton();

        OTRVersion version = (OTRVersion)reader->ReadUInt32();

        switch (version)
        {
        case OTRVersion::Deckard:
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