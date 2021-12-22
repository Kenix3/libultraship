#include "OTRCollisionHeader.h"

void OtrLib::OTRCollisionHeaderV0::ParseFileBinary(BinaryReader* reader, OTRResource* res)
{
	OTRCollisionHeader* col = (OTRCollisionHeader*)res;

	OTRResourceFile::ParseFileBinary(reader, res);

	absMinX = reader->ReadInt16();
	absMinY = reader->ReadInt16();
	absMinZ = reader->ReadInt16();

	absMaxX = reader->ReadInt16();
	absMaxY = reader->ReadInt16();
	absMaxZ = reader->ReadInt16();

	int vtxCnt = reader->ReadInt32();

	for (int i = 0; i < vtxCnt; i++)
	{
		vertices.push_back(Vec3f(reader->ReadInt16(), reader->ReadInt16(), reader->ReadInt16()));
	}

	int polyCnt = reader->ReadInt32();

	for (int i = 0; i < polyCnt; i++)
	{
		polygons.push_back(OtrLib::PolygonEntry(reader));
	}

	int polyTypesCnt = reader->ReadInt32();

	for (int i = 0; i < polyTypesCnt; i++)
		polygonTypes.push_back(reader->ReadUInt64());

	int camEntriesCnt = reader->ReadInt32();

	// OTRTODO: FINISH THIS!!!
	for (int i = 0; i < camEntriesCnt; i++)
	{
		/*OtrLib::CameraDataEntry* entry = new OtrLib::CameraDataEntry();
		entry->cameraSType = reader->ReadInt16();
		entry->numData = reader->ReadInt16();
		entry->cameraPosDataSeg = reader->ReadInt32();
		camData->entries.push_back(entry);*/
	}
}

OtrLib::PolygonEntry::PolygonEntry(BinaryReader* reader)
{
	type = reader->ReadUInt16();
	
	vtxA = reader->ReadUInt16();
	vtxB = reader->ReadUInt16();
	vtxC = reader->ReadUInt16();
	
	a = reader->ReadUInt16();
	b = reader->ReadUInt16();
	c = reader->ReadUInt16();
	d = reader->ReadUInt16();
}
