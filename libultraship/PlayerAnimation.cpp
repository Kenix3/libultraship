#include "PlayerAnimation.h"
#include "PR/ultra64/gbi.h"

namespace Ship
{
	void PlayerAnimationV0::ParseFileBinary(BinaryReader* reader, Resource* res)
	{
		PlayerAnimation* anim = (PlayerAnimation*)res;

		ResourceFile::ParseFileBinary(reader, res);

		int numEntries = reader->ReadInt32();

		for (int i = 0; i < numEntries; i++)
		{
			anim->limbRotData.push_back(reader->ReadInt16());
		}
	}
}