#pragma once

#include "OTRResource.h"
#include "OTRSkeletonLimb.h"

namespace OtrLib
{
	enum class SkeletonType
	{
		Normal,
		Flex,
		Curve,
	};

	class OTRSkeletonV0 : public OTRResourceFile
	{
	public:
		void ParseFileBinary(BinaryReader* reader, OTRResource* res) override;
	};

	class OTRSkeleton : public OTRResource
	{
	public:
		SkeletonType type;
		LimbType limbType;
		int limbCount;
		int dListCount;
		LimbType limbTableType;
		std::vector<std::string> limbTable;
	};
}