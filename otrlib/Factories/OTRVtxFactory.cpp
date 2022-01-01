#include "OTRVtxFactory.h"

namespace OtrLib
{
	OTRVtx* OTRVtxFactory::ReadVtx(BinaryReader* reader) {
		OTRVtx* vtx = new OTRVtx();
		uint32_t version = reader->ReadUInt32();
		switch (version)
		{
		case 0:
		{
			OTRVtxV0 otrVtx = OTRVtxV0();
			otrVtx.ParseFileBinary(reader, vtx);
		}
		break;
		default:
			//VERSION NOT SUPPORTED
			break;
		}
		return vtx;
	}
}