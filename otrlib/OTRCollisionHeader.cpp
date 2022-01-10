#include "OTRCollisionHeader.h"

void OtrLib::OTRCollisionHeaderV0::ParseFileBinary(BinaryReader* reader, OTRResource* res)
{
	OTRCollisionHeader* col = (OTRCollisionHeader*)res;

	OTRResourceFile::ParseFileBinary(reader, res);

	col->absMinX = reader->ReadInt16();
	col->absMinY = reader->ReadInt16();
	col->absMinZ = reader->ReadInt16();

	col->absMaxX = reader->ReadInt16();
	col->absMaxY = reader->ReadInt16();
	col->absMaxZ = reader->ReadInt16();

	int vtxCnt = reader->ReadInt32();

	for (int i = 0; i < vtxCnt; i++)
	{
		float x = reader->ReadInt16();
		float y = reader->ReadInt16();
		float z = reader->ReadInt16();
		col->vertices.push_back(Vec3f(x, y, z));
	}

	int polyCnt = reader->ReadInt32();

	for (int i = 0; i < polyCnt; i++)
		col->polygons.push_back(OtrLib::PolygonEntry(reader));

	int polyTypesCnt = reader->ReadInt32();

	for (int i = 0; i < polyTypesCnt; i++)
		col->polygonTypes.push_back(reader->ReadUInt64());

	col->camData = new CameraDataList();

	int camEntriesCnt = reader->ReadInt32();

	for (int i = 0; i < camEntriesCnt; i++)
	{
		OtrLib::CameraDataEntry* entry = new OtrLib::CameraDataEntry();
		entry->cameraSType = reader->ReadUInt16();
		entry->numData = reader->ReadInt16();
		entry->cameraPosDataIdx = reader->ReadInt32();
		col->camData->entries.push_back(entry);
	}

	int camPosCnt = reader->ReadInt32();

	for (int i = 0; i < camPosCnt; i++)
	{
		OtrLib::CameraPositionData* entry = new OtrLib::CameraPositionData();
		entry->x = reader->ReadInt16();
		entry->y = reader->ReadInt16();
		entry->z = reader->ReadInt16();
		col->camData->cameraPositionData.push_back(entry);
	}

	int waterBoxCnt = reader->ReadInt32();

	for (int i = 0; i < waterBoxCnt; i++)
	{
		OtrLib::WaterBoxHeader waterBox;
		waterBox.xMin = reader->ReadInt16();
		waterBox.ySurface = reader->ReadInt16();
		waterBox.zMin = reader->ReadInt16();
		waterBox.xLength = reader->ReadInt16();
		waterBox.zLength = reader->ReadInt16();
		waterBox.properties = reader->ReadInt32();

		col->waterBoxes.push_back(waterBox);
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

OtrLib::WaterBoxHeader::WaterBoxHeader()
{
}