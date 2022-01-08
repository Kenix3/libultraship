#include "OTRAnimation.h"

void OtrLib::OTRAnimationV0::ParseFileBinary(BinaryReader* reader, OTRResource* res)
{
	OTRAnimation* anim = (OTRAnimation*)res;

	OTRResourceFile::ParseFileBinary(reader, res);

	AnimationType animType = (AnimationType)reader->ReadUInt32();
	anim->type = animType;

	if (animType == AnimationType::Normal)
	{
		anim->frameCount = reader->ReadInt16();

		int rotValuesCnt = reader->ReadUInt32();
		
		for (int i = 0; i < rotValuesCnt; i++)
			anim->rotationValues.push_back(reader->ReadUInt16());

		int rotIndCnt = reader->ReadUInt32();

		for (int i = 0; i < rotIndCnt; i++)
		{
			float x = reader->ReadUInt16();
			float y = reader->ReadUInt16();
			float z = reader->ReadUInt16();
			anim->rotationIndices.push_back(RotationIndex(x, y, z));
		}
		anim->limit = reader->ReadInt16();
	}
	else if (animType == AnimationType::Curve)
	{
		anim->frameCount = reader->ReadInt16();

		int refArrCnt = reader->ReadUInt32();

		for (int i = 0; i < refArrCnt; i++)
			anim->refIndexArr.push_back(reader->ReadUByte());

		int transformDataCnt = reader->ReadUInt32();

		for (int i = 0; i < transformDataCnt; i++)
		{
			TransformData data;
			data.unk_00 = reader->ReadUInt16();
			data.unk_02 = reader->ReadInt16();
			data.unk_04 = reader->ReadInt16();
			data.unk_06 = reader->ReadInt16();
			data.unk_08 = reader->ReadSingle();

			anim->transformDataArr.push_back(data);
		}

		int copyValuesCnt = reader->ReadUInt32();

		for (int i = 0; i < copyValuesCnt; i++)
		{
			anim->copyValuesArr.push_back(reader->ReadInt16());
		}
	}
	else if (animType == AnimationType::Link)
	{
		anim->frameCount = reader->ReadInt16();
		anim->segPtr = reader->ReadUInt32();
	}
	else if (animType == AnimationType::Legacy)
	{
		printf("BEYTAH?!\n");
	}
}
