#include "Path.h"

namespace Ship
{
	void PathV0::ParseFileBinary(BinaryReader* reader, Resource* res)
	{
		Path* path = (Path*)res;

		ResourceFile::ParseFileBinary(reader, res);

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
