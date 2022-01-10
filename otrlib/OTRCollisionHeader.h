#pragma once

#include <vector>
#include <string>
#include "OTRResource.h"
#include "Vec2f.h"
#include "Vec3f.h"
#include "Color3b.h"

namespace OtrLib
{
	class PolygonEntry
	{
	public:
		uint16_t type;
		uint16_t vtxA, vtxB, vtxC;
		uint16_t a, b, c, d;

		PolygonEntry(BinaryReader* reader);
	};

	class WaterBoxHeader
	{
	public:
		int16_t xMin;
		int16_t ySurface;
		int16_t zMin;
		int16_t xLength;
		int16_t zLength;
		int16_t pad;
		int32_t properties;

		WaterBoxHeader();
	};


	class CameraDataEntry
	{
	public:
		uint16_t cameraSType;
		int16_t numData;
		int32_t cameraPosDataIdx;
	};

	class CameraPositionData
	{
	public:
		int16_t x, y, z;
	};

	class CameraDataList
	{
	public:
		std::vector<CameraDataEntry*> entries;
		std::vector<CameraPositionData*> cameraPositionData;
	};

	class OTRCollisionHeaderV0 : public OTRResourceFile
	{
	public:
		int16_t absMinX, absMinY, absMinZ;
		int16_t absMaxX, absMaxY, absMaxZ;
		
		std::vector<Vec3f> vertices;
		std::vector<PolygonEntry> polygons;
		std::vector<uint64_t> polygonTypes;
		std::vector<WaterBoxHeader> waterBoxes;
		CameraDataList* camData = nullptr;

		void ParseFileBinary(BinaryReader* reader, OTRResource* res) override;
	};

    class OTRCollisionHeader : public OTRResource
    {
    public:
		int16_t absMinX, absMinY, absMinZ;
		int16_t absMaxX, absMaxY, absMaxZ;

		std::vector<Vec3f> vertices;
		std::vector<PolygonEntry> polygons;
		std::vector<uint64_t> polygonTypes;
		std::vector<WaterBoxHeader> waterBoxes;
		CameraDataList* camData = nullptr;
    };
}