#include "OTRVtx.h"

namespace OtrLib
{
	void OTRVtxV0::ParseFileBinary(BinaryReader* reader, OTRResource* res) {
		OTRVtx* vtx = (OTRVtx*)res;
		OTRResourceFile::ParseFileBinary(reader, res);

		uint32_t count = reader->ReadUInt32();

		for (uint32_t i = 0; i < count; i++) {
			Vtx data;
			data.x = reader->ReadInt16();
			data.y = reader->ReadInt16();
			data.z = reader->ReadInt16();
			data.flag = reader->ReadUInt16();
			data.s = reader->ReadInt16();
			data.t = reader->ReadInt16();
			data.r = reader->ReadUByte();
			data.g = reader->ReadUByte();
			data.b = reader->ReadUByte();
			data.a = reader->ReadUByte();
			vtx->vtxList.push_back(data);
		}
	}
}