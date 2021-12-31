#include "OTRSkeleton.h"

namespace OtrLib
{
    void OTRSkeletonV0::ParseFileBinary(BinaryReader* reader, OTRResource* res)
    {
        OTRSkeleton* skel = (OTRSkeleton*)res;

        OTRResourceFile::ParseFileBinary(reader, skel);

        skel->type = (SkeletonType)reader->ReadByte();
        skel->limbType = (LimbType)reader->ReadByte();

        skel->limbCount = reader->ReadUInt32();
        skel->dListCount = reader->ReadUInt32();

        skel->limbTableType = (LimbType)reader->ReadByte();

        int limbTblCnt = reader->ReadUInt32();

        for (int i = 0; i < limbTblCnt; i++)
        {
            std::string limbPath = reader->ReadString();

            skel->limbTable.push_back(limbPath);
        }
    }
}