#pragma once

#include "OTRResource.h"

namespace OtrLib
{
	enum class LimbType
	{
		Invalid,
		Standard,
		LOD,
		Skin,
		Curve,
		Legacy,
	};

	enum class ZLimbSkinType
	{
		SkinType_0,           // Segment = 0
		SkinType_4 = 4,       // Segment = segmented address // Struct_800A5E28
		SkinType_5 = 5,       // Segment = 0
		SkinType_DList = 11,  // Segment = DList address
	};

	class OTRSkeletonLimbV0 : public OTRResourceFile
	{
	public:
		void ParseFileBinary(BinaryReader* reader, OTRResource* res) override;
	};

	class OTRSkeletonLimb : public OTRResource
	{
	public:
		LimbType limbType;
		ZLimbSkinType skinSegmentType;
		float legTransX, legTransY, legTransZ;  // Vec3f
		uint16_t rotX, rotY, rotZ;              // Vec3s

		std::string childPtr, siblingPtr, dListPtr, dList2Ptr;

		int16_t transX, transY, transZ;
		uint8_t childIndex, siblingIndex;

	};
}