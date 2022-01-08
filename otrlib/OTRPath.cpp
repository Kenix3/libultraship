#include "OTRPath.h"

namespace OtrLib
{
	void OTRPathV0::ParseFileBinary(BinaryReader* reader, OTRResource* res)
	{
		OTRPath* path = (OTRPath*)res;

		OTRResourceFile::ParseFileBinary(reader, res);

		uint32_t numNodes = reader->ReadUInt32();

		for (int i = 0; i < numNodes; i++)
		{
			int16_t x = reader->ReadInt16();
			int16_t y = reader->ReadInt16();
			int16_t z = reader->ReadInt16();

			path->nodes.push_back(Vec3s(x, y, z));
		}
	}
}
