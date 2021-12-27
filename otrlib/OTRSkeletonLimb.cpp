#include "OTRSkeletonLimb.h"

namespace OtrLib
{
    void OTRSkeletonLimbV0::ParseFileBinary(BinaryReader* reader, OTRResource* res)
    {
        OTRSkeletonLimb* limb = (OTRSkeletonLimb*)res;

        OTRResourceFile::ParseFileBinary(reader, limb);

        limb->limbType = (LimbType)reader->ReadByte();
        limb->skinSegmentType = (ZLimbSkinType)reader->ReadByte();

        limb->legTransX = reader->ReadSingle();
        limb->legTransY = reader->ReadSingle();
        limb->legTransZ = reader->ReadSingle();
        
        limb->rotX = reader->ReadUInt16();
        limb->rotY = reader->ReadUInt16();
        limb->rotZ = reader->ReadUInt16();

        limb->childPtr = reader->ReadString();
        limb->siblingPtr = reader->ReadString();
        limb->dListPtr = reader->ReadString();
        limb->dList2Ptr = reader->ReadString();

        limb->transX = reader->ReadInt16();
        limb->transY = reader->ReadInt16();
        limb->transZ = reader->ReadInt16();

        limb->childIndex = reader->ReadUByte();
        limb->siblingIndex = reader->ReadUByte();
    }
}