#include "OTRPlayerAnimation.h"
#include "PR/ultra64/gbi.h"

namespace OtrLib
{
	void OTRPlayerAnimationV0::ParseFileBinary(BinaryReader* reader, OTRResource* res)
	{
		OTRPlayerAnimation* anim = (OTRPlayerAnimation*)res;

		OTRResourceFile::ParseFileBinary(reader, res);

		int numEntries = reader->ReadInt32();

		for (int i = 0; i < numEntries; i++)
		{
			anim->limbRotData.push_back(reader->ReadUInt16());
		}
	}
}